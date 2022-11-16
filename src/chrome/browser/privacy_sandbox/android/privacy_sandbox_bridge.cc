// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/callback_android.h"
#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/command_line.h"
#include "base/no_destructor.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "chrome/browser/privacy_sandbox/android/jni_headers/PrivacySandboxBridge_jni.h"
#include "chrome/browser/privacy_sandbox/privacy_sandbox_service.h"
#include "chrome/browser/privacy_sandbox/privacy_sandbox_service_factory.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/privacy_sandbox/canonical_topic.h"
#include "components/privacy_sandbox/privacy_sandbox_settings.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/browser_thread.h"
#include "ui/base/l10n/l10n_util.h"

using base::android::ConvertUTF16ToJavaString;
using base::android::ScopedJavaLocalRef;

namespace {
const char TOPICS_JAVA_CLASS[] =
    "org/chromium/chrome/browser/privacy_sandbox/Topic";

PrivacySandboxService* GetPrivacySandboxService() {
  return PrivacySandboxServiceFactory::GetForProfile(
      ProfileManager::GetActiveUserProfile());
}

ScopedJavaLocalRef<jobjectArray> ToJavaTopicsArray(
    JNIEnv* env,
    const std::vector<privacy_sandbox::CanonicalTopic>& topics) {
  std::vector<ScopedJavaLocalRef<jobject>> j_topics;
  for (const auto& topic : topics) {
    j_topics.push_back(Java_PrivacySandboxBridge_createTopic(
        env, topic.topic_id().value(), topic.taxonomy_version(),
        ConvertUTF16ToJavaString(env, topic.GetLocalizedRepresentation())));
  }
  return base::android::ToJavaArrayOfObjects(
      env, base::android::GetClass(env, TOPICS_JAVA_CLASS), j_topics);
}
}  // namespace

static jboolean JNI_PrivacySandboxBridge_IsPrivacySandboxEnabled(JNIEnv* env) {
  return GetPrivacySandboxService()->IsPrivacySandboxEnabled();
}

static jboolean JNI_PrivacySandboxBridge_IsPrivacySandboxManaged(JNIEnv* env) {
  return GetPrivacySandboxService()->IsPrivacySandboxManaged();
}

static jboolean JNI_PrivacySandboxBridge_IsPrivacySandboxRestricted(
    JNIEnv* env) {
  return GetPrivacySandboxService()->IsPrivacySandboxRestricted();
}

static void JNI_PrivacySandboxBridge_SetPrivacySandboxEnabled(
    JNIEnv* env,
    jboolean enabled) {
  GetPrivacySandboxService()->SetPrivacySandboxEnabled(enabled);
}

static ScopedJavaLocalRef<jstring> JNI_PrivacySandboxBridge_GetFlocStatusString(
    JNIEnv* env) {
  // FLoC always disabled while OT not active.
  // TODO(crbug.com/1299720): Perform cleanup / adjustment as required.
  return ConvertUTF16ToJavaString(
      env,
      l10n_util::GetStringUTF16(IDS_PRIVACY_SANDBOX_FLOC_STATUS_NOT_ACTIVE));
}

static ScopedJavaLocalRef<jstring> JNI_PrivacySandboxBridge_GetFlocGroupString(
    JNIEnv* env) {
  // TODO(crbug.com/1299720): Remove this and all the UI code which uses it.
  return ConvertUTF16ToJavaString(
      env, l10n_util::GetStringUTF16(IDS_PRIVACY_SANDBOX_FLOC_INVALID));
}

static ScopedJavaLocalRef<jstring> JNI_PrivacySandboxBridge_GetFlocUpdateString(
    JNIEnv* env) {
  // TODO(crbug.com/1299720): Remove this and all the UI code which uses it.
  return ConvertUTF16ToJavaString(
      env, l10n_util::GetStringUTF16(
               IDS_PRIVACY_SANDBOX_FLOC_TIME_TO_NEXT_COMPUTE_INVALID));
}

static ScopedJavaLocalRef<jstring>
JNI_PrivacySandboxBridge_GetFlocDescriptionString(JNIEnv* env) {
  // TODO(crbug.com/1299720): Remove this and all the UI code which uses it.
  return ConvertUTF16ToJavaString(env,
                                  l10n_util::GetPluralStringFUTF16(
                                      IDS_PRIVACY_SANDBOX_FLOC_DESCRIPTION, 7));
}

static ScopedJavaLocalRef<jstring>
JNI_PrivacySandboxBridge_GetFlocResetExplanationString(JNIEnv* env) {
  // TODO(crbug.com/1299720): Remove this and all the UI code which uses it.
  return ConvertUTF16ToJavaString(
      env, l10n_util::GetPluralStringFUTF16(
               IDS_PRIVACY_SANDBOX_FLOC_RESET_EXPLANATION, 7));
}

static ScopedJavaLocalRef<jobjectArray>
JNI_PrivacySandboxBridge_GetCurrentTopTopics(JNIEnv* env) {
  return ToJavaTopicsArray(env,
                           GetPrivacySandboxService()->GetCurrentTopTopics());
}

static ScopedJavaLocalRef<jobjectArray>
JNI_PrivacySandboxBridge_GetBlockedTopics(JNIEnv* env) {
  return ToJavaTopicsArray(env, GetPrivacySandboxService()->GetBlockedTopics());
}

static void JNI_PrivacySandboxBridge_SetTopicAllowed(JNIEnv* env,
                                                     jint topic_id,
                                                     jint taxonomy_version,
                                                     jboolean allowed) {
  GetPrivacySandboxService()->SetTopicAllowed(
      privacy_sandbox::CanonicalTopic(browsing_topics::Topic(topic_id),
                                      taxonomy_version),
      allowed);
}

static void JNI_PrivacySandboxBridge_GetFledgeJoiningEtldPlusOneForDisplay(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_callback) {
  GetPrivacySandboxService()->GetFledgeJoiningEtldPlusOneForDisplay(
      base::BindOnce(
          [](const base::android::JavaRef<jobject>& j_callback,
             std::vector<std::string> strings) {
            DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
            JNIEnv* env = base::android::AttachCurrentThread();
            base::android::RunObjectCallbackAndroid(
                j_callback, base::android::ToJavaArrayOfStrings(env, strings));
          },
          base::android::ScopedJavaGlobalRef(j_callback)));
}

static base::android::ScopedJavaLocalRef<jobjectArray>
JNI_PrivacySandboxBridge_GetBlockedFledgeJoiningTopFramesForDisplay(
    JNIEnv* env) {
  return base::android::ToJavaArrayOfStrings(
      env,
      GetPrivacySandboxService()->GetBlockedFledgeJoiningTopFramesForDisplay());
}

static void JNI_PrivacySandboxBridge_SetFledgeJoiningAllowed(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& top_frame_etld_plus1,
    jboolean allowed) {
  GetPrivacySandboxService()->SetFledgeJoiningAllowed(
      base::android::ConvertJavaStringToUTF8(top_frame_etld_plus1), allowed);
}

static jint JNI_PrivacySandboxBridge_GetRequiredPromptType(JNIEnv* env) {
  // If the FRE is disabled, as it is in tests which must not be interrupted
  // with dialogs, do not attempt to show a dialog.
  const auto& command_line = *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch("disable-fre"))
    return static_cast<int>(PrivacySandboxService::PromptType::kNone);

  return static_cast<int>(PrivacySandboxServiceFactory::GetForProfile(
                              ProfileManager::GetActiveUserProfile())
                              ->GetRequiredPromptType());
}

static void JNI_PrivacySandboxBridge_PromptActionOccurred(JNIEnv* env,
                                                          jint action) {
  PrivacySandboxServiceFactory::GetForProfile(
      ProfileManager::GetActiveUserProfile())
      ->PromptActionOccurred(
          static_cast<PrivacySandboxService::PromptAction>(action));
}
