#include "chrome/android/chrome_jni_headers/LaunchHelper_jni.h"

#include "chrome/browser/lifetime/application_lifetime.h"

namespace chrome {
namespace android {

void JNI_LaunchHelper_Restart(JNIEnv* env) {
  chrome::AttemptRestart();
}

}  // namespace android
}  // namespace chrome
