/*
 * This file is part of eyeo Chromium SDK,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * eyeo Chromium SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * eyeo Chromium SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "chrome/browser/android/adblock/adblock_jni.h"

#include <algorithm>
#include <iterator>
#include <vector>

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/android/jni_weak_ref.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/version.h"
#include "chrome/browser/adblock/adblock_controller_factory.h"
#include "chrome/browser/android/adblock/adblock_jni_factory.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/adblock/android/jni_headers/AdblockComposeFilterSuggestionsCallback_jni.h"
#include "components/adblock/android/jni_headers/AdblockController_jni.h"
#include "components/adblock/android/jni_headers/AdblockElement_jni.h"
#include "components/adblock/core/adblock_switches.h"
#include "components/adblock/core/common/adblock_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"

using base::android::AttachCurrentThread;
using base::android::ConvertJavaStringToUTF8;
using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;
using base::android::JavaRef;
using base::android::ScopedJavaGlobalRef;
using base::android::ScopedJavaLocalRef;
using base::android::ToJavaArrayOfStrings;

namespace adblock {

namespace {

AdblockController* GetController() {
  if (!g_browser_process || !g_browser_process->profile_manager())
    return nullptr;
  return AdblockControllerFactory::GetForBrowserContext(
      g_browser_process->profile_manager()->GetLastUsedProfile());
}

AdblockJNI* GetJNI() {
  if (!g_browser_process || !g_browser_process->profile_manager())
    return nullptr;
  return AdblockJNIFactory::GetForBrowserContext(
      g_browser_process->profile_manager()->GetLastUsedProfile());
}

absl::optional<KnownSubscriptionInfo> FindRecommendedSubscription(
    const base::android::JavaParamRef<jstring>& subscription) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(adblock::GetController());

  GURL url(ConvertJavaStringToUTF8(subscription));
  auto recommended = adblock::GetController()->GetKnownSubscriptions();
  auto item = std::find_if(recommended.begin(), recommended.end(),
                           [&url](const KnownSubscriptionInfo& subscription) {
                             return subscription.url == url;
                           });

  if (item == recommended.end()) {
    return {};
  }

  return *item;
}

// TODO(mpawlowski): this is currently useless, see DPD-699
class JavaElement {
 public:
  JavaElement(JNIEnv* environment, const ScopedJavaGlobalRef<jobject>& object)
      : obj_(object) {}

  ~JavaElement() {}

  std::string GetLocalName() const {
    JNIEnv* env = AttachCurrentThread();
    auto name = Java_AdblockElement_getLocalName(env, obj_);
    return name.is_null() ? "" : ConvertJavaStringToUTF8(name);
  }

  std::string GetDocumentLocation() const {
    JNIEnv* env = AttachCurrentThread();
    auto location = Java_AdblockElement_getDocumentLocation(env, obj_);
    return location.is_null() ? "" : ConvertJavaStringToUTF8(location);
  }

  std::string GetAttribute(const std::string& name) const {
    JNIEnv* env = AttachCurrentThread();
    auto attr = Java_AdblockElement_getAttribute(
        env, obj_, ConvertUTF8ToJavaString(env, name));

    return attr.is_null() ? "" : ConvertJavaStringToUTF8(attr);
  }

  std::vector<const JavaElement*> GetChildren() const {
    std::vector<const JavaElement*> res;

    if (children_.empty()) {
      JNIEnv* env = AttachCurrentThread();
      auto array = Java_AdblockElement_getChildren(env, obj_);

      if (!array.is_null()) {
        jsize array_length = env->GetArrayLength(array.obj());
        children_.reserve(array_length);

        for (jsize n = 0; n != array_length; ++n) {
          children_.push_back(JavaElement(
              env, ScopedJavaGlobalRef<jobject>(
                       env, env->GetObjectArrayElement(array.obj(), n))));
        }
      }
    }

    res.reserve(children_.size());
    std::transform(children_.begin(), children_.end(), std::back_inserter(res),
                   [](const auto& cur) { return &cur; });

    return res;
  }

 private:
  ScopedJavaGlobalRef<jobject> obj_;
  mutable std::vector<JavaElement> children_;
};

void ComposeFilterSuggestionsResult(
    const base::android::ScopedJavaGlobalRef<jobject>& element,
    const base::android::ScopedJavaGlobalRef<jobject>& callback,
    const std::vector<std::string>& filters) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  auto j_filters = ToJavaArrayOfStrings(env, filters);
  Java_AdblockComposeFilterSuggestionsCallback_onDone(env, callback, element,
                                                      j_filters);
}

constexpr int kNoId = -1;

int GetTabId(content::RenderFrameHost* render_frame_host) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  if (!web_contents)
    return kNoId;

  auto* tab = TabAndroid::FromWebContents(web_contents);
  if (!tab)
    return kNoId;

  return tab->GetAndroidId();
}

std::vector<std::string> SubscriptionsToUrls(
    const std::vector<scoped_refptr<Subscription>>& subscriptions) {
  std::vector<std::string> urls;
  urls.reserve(subscriptions.size());
  base::ranges::transform(
      subscriptions, std::back_inserter(urls),
      [](const auto& sub) { return sub->GetSourceUrl().spec(); });
  return urls;
}

}  // namespace

AdblockJNI::AdblockJNI(ResourceClassificationRunner* classification_runner,
                       AdblockController* controller)
    : classification_runner_(classification_runner), controller_(controller) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (classification_runner_)  // Might not be there in unit tests.
    classification_runner_->AddObserver(this);
  if (controller_)
    controller_->AddObserver(this);
}

AdblockJNI::~AdblockJNI() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (controller_)
    controller_->RemoveObserver(this);
  if (classification_runner_)
    classification_runner_->RemoveObserver(this);
}

void AdblockJNI::Bind(JavaObjectWeakGlobalRef weak_java_controller) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  weak_java_controller_ = weak_java_controller;
}

void AdblockJNI::OnAdMatched(const GURL& url,
                             mojom::FilterMatchResult result,
                             const std::vector<GURL>& parent_frame_urls,
                             ContentType content_type,
                             content::RenderFrameHost* render_frame_host,
                             const GURL& subscription) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(render_frame_host);
  DCHECK(result == mojom::FilterMatchResult::kBlockRule ||
         result == mojom::FilterMatchResult::kAllowRule);
  if (!has_counters_observers_)
    return;
  const bool was_blocked = result == mojom::FilterMatchResult::kBlockRule;
  DVLOG(3) << "[eyeo] Ad matched " << url << "(type: " << content_type
           << (was_blocked ? ", blocked" : ", allowed") << ")";
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jobject> obj = weak_java_controller_.get(env);
  if (obj.is_null())
    return;

  ScopedJavaLocalRef<jstring> j_url = ConvertUTF8ToJavaString(env, url.spec());
  ScopedJavaLocalRef<jobjectArray> j_parents =
      ToJavaArrayOfStrings(env, adblock::utils::ConvertURLs(parent_frame_urls));
  ScopedJavaLocalRef<jstring> j_subscription =
      ConvertUTF8ToJavaString(env, subscription.spec());
  int tab_id = GetTabId(render_frame_host);
  Java_AdblockController_adMatchedCallback(
      env, obj, j_url, was_blocked, j_parents, j_subscription,
      static_cast<int>(content_type), tab_id);
}

void AdblockJNI::OnPageAllowed(const GURL& url,
                               content::RenderFrameHost* render_frame_host,
                               const GURL& subscription) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(render_frame_host);
  if (!has_counters_observers_)
    return;
  DVLOG(3) << "[eyeo] Page allowed " << url;
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jobject> obj = weak_java_controller_.get(env);
  if (obj.is_null())
    return;

  ScopedJavaLocalRef<jstring> j_url = ConvertUTF8ToJavaString(env, url.spec());
  ScopedJavaLocalRef<jstring> j_subscription =
      ConvertUTF8ToJavaString(env, subscription.spec());
  int tab_id = GetTabId(render_frame_host);
  Java_AdblockController_pageAllowedCallback(env, obj, j_url, j_subscription,
                                             tab_id);
}

void AdblockJNI::OnPopupMatched(const GURL& url,
                                mojom::FilterMatchResult result,
                                const GURL& opener_url,
                                content::RenderFrameHost* render_frame_host,
                                const GURL& subscription) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(render_frame_host);
  DCHECK(result == mojom::FilterMatchResult::kBlockRule ||
         result == mojom::FilterMatchResult::kAllowRule);
  if (!has_counters_observers_)
    return;
  const bool was_blocked = result == mojom::FilterMatchResult::kBlockRule;
  DVLOG(3) << "[eyeo] Popup matched " << url
           << (was_blocked ? ", blocked" : ", allowed");
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jobject> obj = weak_java_controller_.get(env);
  if (obj.is_null())
    return;

  ScopedJavaLocalRef<jstring> j_url = ConvertUTF8ToJavaString(env, url.spec());
  ScopedJavaLocalRef<jstring> j_opener =
      ConvertUTF8ToJavaString(env, opener_url.spec());
  ScopedJavaLocalRef<jstring> j_subscription =
      ConvertUTF8ToJavaString(env, subscription.spec());
  int tab_id = GetTabId(render_frame_host);
  Java_AdblockController_popupMatchedCallback(env, obj, j_url, was_blocked,
                                              j_opener, j_subscription, tab_id);
}

void AdblockJNI::SetHasAdblockCountersObservers(bool has_observers) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  has_counters_observers_ = has_observers;
}

void AdblockJNI::OnSubscriptionUpdated(const GURL& url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = weak_java_controller_.get(env);
  if (obj.is_null())
    return;

  ScopedJavaLocalRef<jstring> j_url = ConvertUTF8ToJavaString(env, url.spec());
  Java_AdblockController_subscriptionUpdatedCallback(env, obj, j_url);
}

}  // namespace adblock

static void JNI_AdblockController_SetHasAdblockCountersObservers(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    jboolean has_observers) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!adblock::GetController())
    return;
  adblock::GetJNI()->SetHasAdblockCountersObservers(has_observers == JNI_TRUE);
}

static void JNI_AdblockController_Bind(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& caller) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!adblock::GetController())
    return;
  JavaObjectWeakGlobalRef weak_controller_ref(env, caller);
  adblock::GetJNI()->Bind(weak_controller_ref);
}

static jboolean JNI_AdblockController_IsAdblockEnabled(JNIEnv* env) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(adblock::GetController());
  return adblock::GetController()->IsAdblockEnabled() ? JNI_TRUE : JNI_FALSE;
}

static jboolean JNI_AdblockController_IsAcceptableAdsEnabled(JNIEnv* env) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(adblock::GetController());
  return adblock::GetController()->IsAcceptableAdsEnabled() ? JNI_TRUE
                                                            : JNI_FALSE;
}

static void JNI_AdblockController_SetAdblockEnabled(JNIEnv* env,
                                                    jboolean j_aa_enabled) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(adblock::GetController());
  return adblock::GetController()->SetAdblockEnabled(j_aa_enabled == JNI_TRUE);
}

static void JNI_AdblockController_SetAcceptableAdsEnabled(
    JNIEnv* env,
    jboolean j_aa_enabled) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(adblock::GetController());
  return adblock::GetController()->SetAcceptableAdsEnabled(j_aa_enabled ==
                                                           JNI_TRUE);
}

static void JNI_AdblockController_SelectSubscription(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& url) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!adblock::GetController())
    return;
  adblock::GetController()->SelectBuiltInSubscription(
      GURL{ConvertJavaStringToUTF8(url)});
}

static base::android::ScopedJavaLocalRef<jobjectArray>
JNI_AdblockController_GetSelectedSubscriptions(JNIEnv* env) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!adblock::GetController())
    return ToJavaArrayOfStrings(env, std::vector<std::string>{});

  return ToJavaArrayOfStrings(
      env, adblock::SubscriptionsToUrls(
               adblock::GetController()->GetSelectedBuiltInSubscriptions()));
}

static void JNI_AdblockController_UnselectSubscription(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& url) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!adblock::GetController())
    return;

  adblock::GetController()->UnselectBuiltInSubscription(
      GURL{ConvertJavaStringToUTF8(url)});
}

static void JNI_AdblockController_AddCustomSubscription(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& url) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!adblock::GetController())
    return;
  adblock::GetController()->AddCustomSubscription(
      GURL{ConvertJavaStringToUTF8(url)});
}

static base::android::ScopedJavaLocalRef<jobjectArray>
JNI_AdblockController_GetCustomSubscriptions(JNIEnv* env) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!adblock::GetController())
    return ToJavaArrayOfStrings(env, std::vector<std::string>{});

  return ToJavaArrayOfStrings(
      env, adblock::SubscriptionsToUrls(
               adblock::GetController()->GetCustomSubscriptions()));
}

static void JNI_AdblockController_RemoveCustomSubscription(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& url) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!adblock::GetController())
    return;
  adblock::GetController()->RemoveCustomSubscription(
      GURL{ConvertJavaStringToUTF8(url)});
}

static void JNI_AdblockController_AddAllowedDomain(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& domain) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!adblock::GetController())
    return;
  adblock::GetController()->AddAllowedDomain(ConvertJavaStringToUTF8(domain));
}

static void JNI_AdblockController_RemoveAllowedDomain(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& domain) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!adblock::GetController())
    return;
  adblock::GetController()->RemoveAllowedDomain(
      ConvertJavaStringToUTF8(domain));
}

static base::android::ScopedJavaLocalRef<jobjectArray>
JNI_AdblockController_GetAllowedDomains(JNIEnv* env) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!adblock::GetController())
    return ToJavaArrayOfStrings(env, std::vector<std::string>{});

  return ToJavaArrayOfStrings(env,
                              adblock::GetController()->GetAllowedDomains());
}

static base::android::ScopedJavaLocalRef<jobjectArray>
JNI_AdblockController_GetRecommendedSubscriptions(JNIEnv* env) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!adblock::GetController())
    return ToJavaArrayOfStrings(env, std::vector<std::string>{});

  auto list = adblock::GetController()->GetKnownSubscriptions();
  std::vector<std::string> recommended;
  for (const auto& element : list) {
    if (element.ui_visibility == adblock::SubscriptionUiVisibility::Visible)
      recommended.emplace_back(element.url.spec());
  }
  return ToJavaArrayOfStrings(env, recommended);
}

static base::android::ScopedJavaLocalRef<jstring>
JNI_AdblockController_GetRecommendedSubscriptionTitle(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& subscription) {
  if (!adblock::GetController())
    return ConvertUTF8ToJavaString(env, {});
  auto item = adblock::FindRecommendedSubscription(subscription);
  std::string title;
  if (item) {
    title = item->title;
  }

  ScopedJavaLocalRef<jstring> j_title = ConvertUTF8ToJavaString(env, title);
  return j_title;
}

static base::android::ScopedJavaLocalRef<jstring>
JNI_AdblockController_GetRecommendedSubscriptionLanguages(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& subscription) {
  if (!adblock::GetController())
    return ConvertUTF8ToJavaString(env, {});
  auto item = adblock::FindRecommendedSubscription(subscription);
  std::string languages_str;
  if (item) {
    languages_str = adblock::utils::SerializeLanguages(item->languages);
  }
  ScopedJavaLocalRef<jstring> j_languages =
      ConvertUTF8ToJavaString(env, languages_str);
  return j_languages;
}

static base::android::ScopedJavaLocalRef<jstring>
JNI_AdblockController_GetSelectedSubscriptionVersion(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& subscription) {
  const GURL url(ConvertJavaStringToUTF8(subscription));
  const auto built_in_subscriptions =
      adblock::GetController()->GetSelectedBuiltInSubscriptions();
  auto it = base::ranges::find(built_in_subscriptions, url,
                               &adblock::Subscription::GetSourceUrl);
  if (it == built_in_subscriptions.end()) {
    const auto custom_subscriptions =
        adblock::GetController()->GetCustomSubscriptions();
    it = base::ranges::find(custom_subscriptions, url,
                            &adblock::Subscription::GetSourceUrl);
    if (it == custom_subscriptions.end())
      return ConvertUTF8ToJavaString(env, {});
  }
  ScopedJavaLocalRef<jstring> j_version =
      ConvertUTF8ToJavaString(env, (*it)->GetCurrentVersion());
  return j_version;
}

static void JNI_AdblockController_ComposeFilterSuggestions(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& element,
    const base::android::JavaParamRef<jobject>& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  // TODO implement ComposeFilterSuggestions in flatbuffer core: DPD-699
  // This is currently not doing the right thing.
  base::android::ScopedJavaGlobalRef<jobject> element_ref(element);
  base::android::ScopedJavaGlobalRef<jobject> callback_ref(callback);
  adblock::ComposeFilterSuggestionsResult(element_ref, callback_ref, {});
}

static void JNI_AdblockController_AddCustomFilter(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& filter) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto* controller = adblock::GetController();
  if (!controller)
    return;

  controller->AddCustomFilter(ConvertJavaStringToUTF8(filter));
}

static void JNI_AdblockController_RemoveCustomFilter(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& filter) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto* controller = adblock::GetController();
  if (!controller)
    return;

  controller->RemoveCustomFilter(ConvertJavaStringToUTF8(filter));
}

static base::android::ScopedJavaLocalRef<jobjectArray>
JNI_AdblockController_GetCustomFilters(JNIEnv* env) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto* controller = adblock::GetController();
  if (!controller)
    return ToJavaArrayOfStrings(env, std::vector<std::string>{});

  return ToJavaArrayOfStrings(env, controller->GetCustomFilters());
}
