/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * Adblock Plus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Adblock Plus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Adblock Plus.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "components/adblock/adblock_element_hider_impl.h"

#include "base/callback.h"
#include "base/json/string_escape.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "base/trace_event/trace_event.h"
#include "components/adblock/buildflags.h"
#include "components/grit/components_resources.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/isolated_world_ids.h"
#include "third_party/libadblockplus/src/include/AdblockPlus/IFilterEngine.h"
#include "ui/base/resource/resource_bundle.h"

namespace adblock {
namespace {

std::string GenerateBlockedElemhideJavaScript(
    const std::string& url,
    const std::string& filename_with_query) {
  TRACE_EVENT1("adblockplus", "GenerateBlockedElemhideJavaScript", "url", url);
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // basically copy-pasted JS and saved to resources from
  // https://github.com/adblockplus/adblockpluschrome/blob/master/include.preload.js#L546
  std::string result =
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
          IDR_ADBLOCK_ELEMHIDE_JS);

  result.append("\n");

  // the file is template with tokens to be replaced with actual variable values
  std::string query_jst =
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
          IDR_ADBLOCK_ELEMHIDE_FOR_SELECTOR_JS);

  std::string build_string;
  base::EscapeJSONString(filename_with_query, false, &build_string);

  base::ReplaceSubstringsAfterOffset(&query_jst, 0, "{{url}}", url);
  base::ReplaceSubstringsAfterOffset(&query_jst, 0, "{{filename_with_query}}",
                                     build_string);
  result.append(query_jst);

  return result;
}

std::string GenerateStylesheet(AdblockPlus::IFilterEngine* filter_engine,
                               const GURL& url,
                               const std::vector<GURL>& referrers) {
  TRACE_EVENT1("adblockplus", "GenerateStylesheet", "url", url.spec());
  DCHECK(filter_engine);

  std::vector<std::string> referrers_chain;
  for (auto& referer : referrers)
    referrers_chain.push_back(referer.spec());

  const bool specific_only = filter_engine->IsContentAllowlisted(
      url.spec(), AdblockPlus::IFilterEngine::CONTENT_TYPE_GENERICHIDE,
      referrers_chain);

  if (specific_only) {
    DVLOG(3) << "[ABP] Element hiding - specificOnly selectors";
  }

  return filter_engine->GetElementHidingStyleSheet(url.spec(), specific_only);
}

std::string GenerateElemHidingEmuJavaScript(
    AdblockPlus::IFilterEngine* filter_engine,
    const GURL& url) {
  TRACE_EVENT1("adblockplus", "GenerateElemHidingEmuJavaScript", "url",
               url.spec());

  DCHECK(filter_engine);

  std::vector<AdblockPlus::IFilterEngine::EmulationSelector> selectors =
      filter_engine->GetElementHidingEmulationSelectors(url.spec());

  DLOG(INFO) << "[ABP] Got " << selectors.size() << " selectors for url "
             << url;

  if (selectors.empty()) {
    return std::string();
  }

  // build the string with selectors
  std::string build_string;

  for (const auto& selector : selectors) {
    build_string.append("{selector:");
    base::EscapeJSONString(selector.selector, true, &build_string);
    build_string.append(", text:");
    base::EscapeJSONString(selector.text, true, &build_string);
    build_string.append("}, \n");
  }

  std::string elem_hide_emu_js =
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
          IDR_ADBLOCK_ELEMHIDE_EMU_JS);

  base::ReplaceSubstringsAfterOffset(
      &elem_hide_emu_js, 0, "{{elemHidingEmulatedPatternsDef}}", build_string);
  return elem_hide_emu_js;
}

std::string GenerateSnippetScript(AdblockPlus::IFilterEngine* filter_engine,
                                  const GURL& url) {
  TRACE_EVENT1("adblockplus", "GenerateSnippetScript", "url", url.spec());
  DCHECK(filter_engine);

  static std::string source =
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
          IDR_ADBLOCK_SNIPPETS_JS);

  return filter_engine->GetSnippetScript(url.spec(), source);
}

AdblockElementHider::ElemhideEmulationInjectionData
PrepareElemhideEmulationData(
    scoped_refptr<adblock::AdblockPlatformEmbedder> embedder,
    GURL gurl,
    std::vector<GURL> referrers,
    std::string sitekey) {
  TRACE_EVENT1("adblockplus", "PrepareElemhideEmulationData", "url",
               gurl.spec());
  DCHECK(embedder->GetAbpTaskRunner()->BelongsToCurrentThread());

  auto* filter_engine = embedder->GetFilterEngine();
  if (!filter_engine) {
    return {};
  }
  std::vector<std::string> referrers_chain;
  for (auto& referer : referrers)
    referrers_chain.push_back(referer.spec());

  // user domains allowing is implemented as adding exception filter for
  // domain so we can just use filter engine to do the work.
  if (gurl.SchemeIsHTTPOrHTTPS()) {
    const std::string& url = gurl.spec();
    if (embedder->GetFilterEngine()->IsContentAllowlisted(
            url, AdblockPlus::IFilterEngine::CONTENT_TYPE_DOCUMENT,
            referrers_chain, sitekey) ||
        embedder->GetFilterEngine()->IsContentAllowlisted(
            url, AdblockPlus::IFilterEngine::CONTENT_TYPE_ELEMHIDE,
            referrers_chain, sitekey)) {
      LOG(INFO) << "[ABP] Element hiding - skipped for " << url;
      return {};
    } else {
      // generate elemhide emu JS on background thread
      DLOG(INFO) << "[ABP] Generating ElemHidingEmu and UserStylesheet for "
                 << gurl;

      return {GenerateStylesheet(filter_engine, gurl, referrers),
              GenerateElemHidingEmuJavaScript(filter_engine, gurl),
              GenerateSnippetScript(filter_engine, gurl)};
    }
  }
  return {};
}

void InsertUserCSSAndApplyElemHidingEmuJS(
    content::GlobalRenderFrameHostId frame_host_id,
    base::OnceCallback<
        void(const AdblockElementHider::ElemhideEmulationInjectionData&)>
        on_finished,
    AdblockElementHider::ElemhideEmulationInjectionData input) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto* frame_host = content::RenderFrameHost::FromID(frame_host_id);
  if (!frame_host) {
    // Render frame host was destroyed before element hiding could be applied.
    // This is not a bug, just legitimate a race condition.
    std::move(on_finished).Run({});
    return;
  }
  if (!input.stylesheet.empty()) {
    frame_host->InsertUserStylesheet(input.stylesheet);
    DLOG(INFO) << "[ABP] Element hiding - inserted stylesheet in frame"
               << " '" << frame_host->GetFrameName() << "'";
  }

  if (!input.elemhide_js.empty()) {
    frame_host->ExecuteJavaScriptInIsolatedWorld(
        base::UTF8ToUTF16(input.elemhide_js),
        content::RenderFrameHost::JavaScriptResultCallback(),
        content::ISOLATED_WORLD_ID_ADBLOCK);

    DVLOG(1) << "[ABP] Element hiding emulation - called JS in frame"
             << " '" << frame_host->GetFrameName() << "'";
  }

  if (!input.snippet_js.empty()) {
    // PK: Extension API ends up generating isolated world for injected script
    // execution. See GetIsolatedWorldIdForInstance in
    // extensions/renderer/script_injection.cc. Why not to reuse adblock space?
    frame_host->ExecuteJavaScriptInIsolatedWorld(
        base::UTF8ToUTF16(input.snippet_js),
        content::RenderFrameHost::JavaScriptResultCallback(),
        content::ISOLATED_WORLD_ID_ADBLOCK);

    DVLOG(1) << "[ABP] Snippet - called JS in frame"
             << " '" << frame_host->GetFrameName() << "'";
  }

  std::move(on_finished).Run(input);
}

}  // namespace

AdblockElementHiderImpl::AdblockElementHiderImpl(
    scoped_refptr<AdblockPlatformEmbedder> embedder)
    : embedder_(std::move(embedder)) {}

AdblockElementHiderImpl::~AdblockElementHiderImpl() = default;

void AdblockElementHiderImpl::ApplyElementHidingEmulationOnPage(
    GURL url,
    std::vector<GURL> frame_hierarchy,
    content::RenderFrameHost* render_frame_host,
    std::string sitekey,
    base::OnceCallback<void(const ElemhideEmulationInjectionData&)>
        on_finished) {
  embedder_->GetAbpTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&PrepareElemhideEmulationData, embedder_, std::move(url),
                     std::move(frame_hierarchy), std::move(sitekey)),
      base::BindOnce(&InsertUserCSSAndApplyElemHidingEmuJS,
                     render_frame_host->GetGlobalId(), std::move(on_finished)));
}

bool AdblockElementHiderImpl::IsElementTypeHideable(
    ContentType adblock_resource_type) const {
  switch (adblock_resource_type) {
    case ContentType::CONTENT_TYPE_IMAGE:
    case ContentType::CONTENT_TYPE_OBJECT:
    case ContentType::CONTENT_TYPE_MEDIA:
      return true;

    default:
      break;
  }
  return false;
}

void AdblockElementHiderImpl::HideBlockedElement(
    const GURL& url,
    content::RenderFrameHost* render_frame_host) {
  TRACE_EVENT1("adblockplus", "AdblockElementHiderImpl::HideBlockedElemenet",
               "url", url.spec());
  // we can't get relative URL from URLRequest
  // so the hack is to select in JS with filename_with_query selector and then
  // check every found element's full absolute URL
  std::string filename_with_query = url.ExtractFileName();
  if (url.has_query()) {
    filename_with_query.append("?");
    filename_with_query.append(url.query());
  }

  const std::string js =
      GenerateBlockedElemhideJavaScript(url.spec(), filename_with_query);

  // elemhide resource by element hide rules
  render_frame_host->ExecuteJavaScriptInIsolatedWorld(
      base::UTF8ToUTF16(js),
      content::RenderFrameHost::JavaScriptResultCallback(),
      content::ISOLATED_WORLD_ID_ADBLOCK);

  DLOG(INFO) << "[ABP] Element hiding - called JS: " << js;
}

}  // namespace adblock
