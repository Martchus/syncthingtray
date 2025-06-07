#include <android/log.h>
#include <jni.h>

#include <csignal>

#define ST_LIB_LOG_TAG "SyncthingLibrary"
#define ST_LIB_LOG_INF(...) __android_log_print(ANDROID_LOG_INFO, ST_LIB_LOG_TAG, __VA_ARGS__)

static void handleSigsys(int signum, siginfo_t *info, void *context)
{
    // ignore the signal to prevent the app from crashing
    ST_LIB_LOG_INF("SIGSYS signal received and ignored");
}

static void initSigsysHandler()
{
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handleSigsys;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGSYS, &sa, nullptr) == -1) {
        ST_LIB_LOG_INF("Failed to set up SIGSYS handler");
    } else {
        ST_LIB_LOG_INF("SIGSYS handler set up successfully");
    }
}

extern "C" {

JNIEXPORT void JNICALL
Java_io_github_martchus_syncthingtray_Util_initSigsysHandler(JNIEnv *env, jobject thiz)
{
    initSigsysHandler();
}

}
