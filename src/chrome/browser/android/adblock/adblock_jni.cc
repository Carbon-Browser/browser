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
#include "chrome/browser/android/adblock/adblock_jni.h"

#include <iterator>

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/android/jni_weak_ref.h"
#include "base/logging.h"
#include "base/version.h"
#include "chrome/android/chrome_jni_headers/AdblockComposeFilterSuggestionsCallback_jni.h"
#include "chrome/android/chrome_jni_headers/AdblockController_jni.h"
#include "chrome/android/chrome_jni_headers/AdblockElement_jni.h"
#include "chrome/android/chrome_jni_headers/AdblockMatchingFilterCheckCallback_jni.h"
#include "chrome/browser/adblock/adblock_controller_factory.h"
#include "chrome/browser/adblock/adblock_platform_embedder_factory.h"
#include "chrome/browser/adblock/adblock_request_classifier_factory.h"
#include "chrome/browser/android/adblock/adblock_jni_factory.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/adblock/adblock_platform_embedder.h"
#include "components/adblock/adblock_request_classifier.h"
#include "components/adblock/adblock_utils.h"
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

AdblockRequestClassifier* GetRequestClassifier() {
  if (!g_browser_process || !g_browser_process->profile_manager())
    return nullptr;
  return adblock::AdblockRequestClassifierFactory::GetForBrowserContext(
      g_browser_process->profile_manager()->GetLastUsedProfile());
}

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

absl::optional<Subscription> FindRecommendedSubscription(
    const base::android::JavaParamRef<jstring>& subscription) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(adblock::GetController());

  GURL url(ConvertJavaStringToUTF8(subscription));
  auto recommended = adblock::GetController()->GetRecommendedSubscriptions();
  auto item = std::find_if(recommended.begin(), recommended.end(),
                           [&url](const Subscription& subscription) {
                             return subscription.url == url;
                           });

  if (item == recommended.end()) {
    return {};
  }

  return *item;
}

class JavaElement : public AdblockPlus::IElement {
 public:
  JavaElement(JNIEnv* environment, const ScopedJavaGlobalRef<jobject>& object)
      : obj_(object) {}

  ~JavaElement() override {}

  std::string GetLocalName() const override {
    JNIEnv* env = AttachCurrentThread();
    auto name = Java_AdblockElement_getLocalName(env, obj_);
    return name.is_null() ? "" : ConvertJavaStringToUTF8(name);
  }

  std::string GetDocumentLocation() const override {
    JNIEnv* env = AttachCurrentThread();
    auto location = Java_AdblockElement_getDocumentLocation(env, obj_);
    return location.is_null() ? "" : ConvertJavaStringToUTF8(location);
  }

  std::string GetAttribute(const std::string& name) const override {
    JNIEnv* env = AttachCurrentThread();
    auto attr = Java_AdblockElement_getAttribute(
        env, obj_, ConvertUTF8ToJavaString(env, name));

    return attr.is_null() ? "" : ConvertJavaStringToUTF8(attr);
  }

  std::vector<const IElement*> GetChildren() const override {
    std::vector<const IElement*> res;

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

void CheckFilterMatchResult(
    const base::android::ScopedJavaGlobalRef<jobject>& callback,
    bool result) {
  JNIEnv* env = AttachCurrentThread();
  Java_AdblockMatchingFilterCheckCallback_onMatchingFilterCheckResult(
      env, callback, result);
}

constexpr int kNoId = -1;

int GetTabId(int process_id, int render_frame_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto* render_frame_host = content::RenderFrameHost::FromID(
      process_id, render_frame_id);  // required to be done on UI thread
  if (!render_frame_host)
    return kNoId;

  auto* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  if (!web_contents)
    return kNoId;

  auto* tab = TabAndroid::FromWebContents(web_contents);
  if (!tab)
    return kNoId;

  return tab->GetAndroidId();
}

}  // namespace

AdblockJNI::AdblockJNI(AdblockRequestClassifier* classifier,
                       AdblockController* controller)
    : classifier_(classifier), controller_(controller) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (classifier_)  // Might not be there in unit tests.
    classifier_->AddObserver(this);
  if (controller_)
    controller_->AddObserver(this);
}

AdblockJNI::~AdblockJNI() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (controller_)
    controller_->RemoveObserver(this);
  if (classifier_)
    classifier_->RemoveObserver(this);
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
                             const std::vector<GURL>& subscriptions) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(result == mojom::FilterMatchResult::kBlockRule ||
         result == mojom::FilterMatchResult::kAllowRule);
  // if (!has_counters_observers_)
  //   return;
  const bool was_blocked = result == mojom::FilterMatchResult::kBlockRule;
  DVLOG(3) << "[ABP] Ad matched " << url << "(type: " << content_type
           << (was_blocked ? ", blocked" : ", allowed") << ")";
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jobject> obj = weak_java_controller_.get(env);
  if (obj.is_null())
    return;

  ScopedJavaLocalRef<jstring> j_url = ConvertUTF8ToJavaString(env, url.spec());
  ScopedJavaLocalRef<jobjectArray> j_parents =
      ToJavaArrayOfStrings(env, adblock::utils::ConvertURLs(parent_frame_urls));
  ScopedJavaLocalRef<jobjectArray> j_subscriptions =
      ToJavaArrayOfStrings(env, adblock::utils::ConvertURLs(subscriptions));
  // |render_frame_host| is null when the tab was closed after an ad-check was
  // performed but before this notification was sent.
  int tab_id = render_frame_host
                   ? GetTabId(render_frame_host->GetProcess()->GetID(),
                              render_frame_host->GetRoutingID())
                   : -1;
  Java_AdblockController_adMatchedCallback(
      env, obj, j_url, was_blocked, j_parents, j_subscriptions,
      static_cast<int>(content_type), tab_id);
}

void AdblockJNI::SetHasAdblockCountersObservers(bool has_observers) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  has_counters_observers_ = has_observers;
}

void AdblockJNI::OnRecommendedSubscriptionsAvailable(
    const std::vector<Subscription>& recommended) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = weak_java_controller_.get(env);
  if (obj.is_null())
    return;
  Java_AdblockController_recommendedSubscriptionsAvailableCallback(env, obj);
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
      env, adblock::utils::ConvertURLs(
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
      env, adblock::utils::ConvertURLs(
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

  auto list = adblock::GetController()->GetRecommendedSubscriptions();
  std::vector<std::string> recommended;
  for (const auto& element : list) {
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

static void JNI_AdblockController_ComposeFilterSuggestions(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& element,
    const base::android::JavaParamRef<jobject>& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto embedder = adblock::AdblockPlatformEmbedderFactory::GetForBrowserContext(
      g_browser_process->profile_manager()->GetLastUsedProfile());

  base::android::ScopedJavaGlobalRef<jobject> element_ref(element);
  base::android::ScopedJavaGlobalRef<jobject> callback_ref(callback);
  std::unique_ptr<adblock::JavaElement> j_element =
      std::make_unique<adblock::JavaElement>(env, element_ref);
  embedder->ComposeFilterSuggestions(
      std::move(j_element),
      base::BindOnce(&adblock::ComposeFilterSuggestionsResult, element_ref,
                     callback_ref));
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

static void JNI_AdblockController_HasMatchingFilter(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& j_url,
    jint content_type,
    const base::android::JavaParamRef<jstring>& j_parent_url,
    const base::android::JavaParamRef<jstring>& j_sitekey,
    jboolean j_specific_only,
    const base::android::JavaParamRef<jobject>& j_callback) {
  auto* classifier = adblock::GetRequestClassifier();
  if (!classifier)
    return;
  bool specific_only = j_specific_only == JNI_TRUE;
  auto url = ConvertJavaStringToUTF8(j_url);
  auto sitekey = j_sitekey.is_null() ? std::string{""}
                                     : ConvertJavaStringToUTF8(j_sitekey);
  auto parent_url = j_parent_url.is_null()
                        ? std::string{""}
                        : ConvertJavaStringToUTF8(j_parent_url);
  base::android::ScopedJavaGlobalRef<jobject> callback(j_callback);
  classifier->HasMatchingFilter(
      GURL{url}, static_cast<adblock::ContentType>(content_type),
      GURL{parent_url}, adblock::SiteKey{sitekey}, specific_only,
      base::BindOnce(&adblock::CheckFilterMatchResult, callback));
}
