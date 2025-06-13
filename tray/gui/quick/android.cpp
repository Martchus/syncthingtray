#include "./android.h"

#include "./app.h"
#include "./appservice.h"

#ifdef Q_OS_ANDROID
#include <QJniEnvironment>
#endif

namespace QtGui {

/// \brief The JniFn namespace defines functions called from Java.
namespace JniFn {
static AppService *appServiceObjectForJava = nullptr;
static App *appObjectForJava = nullptr;

static void loadQtQuickGui(JNIEnv *, jobject)
{
    QMetaObject::invokeMethod(appObjectForJava, "initEngine", Qt::QueuedConnection);
}

static void unloadQtQuickGui(JNIEnv *, jobject)
{
    QMetaObject::invokeMethod(appObjectForJava, "destroyEngine", Qt::QueuedConnection);
}

static void stopLibSyncthing(JNIEnv *, jobject)
{
    QMetaObject::invokeMethod(appServiceObjectForJava, "stopLibSyncthing", Qt::QueuedConnection);
}

static void handleMessageFromService(JNIEnv *, jobject, jint what, jint arg1, jint arg2, jstring str)
{
    appObjectForJava->handleMessageFromService(static_cast<ActivityAction>(what), arg1, arg2, QJniObject::fromLocalRef(str).toString());
}

static void broadcastLauncherStatus(JNIEnv *, jobject)
{
    QMetaObject::invokeMethod(appServiceObjectForJava, "broadcastLauncherStatus", Qt::QueuedConnection);
}

static void handleLauncherStatusBroadcast(JNIEnv *, jobject, jobject intent)
{
    const auto status = QAndroidIntent(QJniObject::fromLocalRef(intent)).extraVariant(QStringLiteral("status"));
    QMetaObject::invokeMethod(appObjectForJava, "handleLauncherStatusBroadcast", Qt::QueuedConnection, Q_ARG(QVariant, status));
}

static void handleMessageFromActivity(JNIEnv *, jobject, jint what, jint arg1, jint arg2, jstring str)
{
    appServiceObjectForJava->handleMessageFromActivity(static_cast<ServiceAction>(what), arg1, arg2, QJniObject::fromLocalRef(str).toString());
}

static void handleAndroidIntent(JNIEnv *, jobject, jstring page, jboolean fromNotification)
{
    QMetaObject::invokeMethod(appObjectForJava, "handleAndroidIntent", Qt::QueuedConnection,
        Q_ARG(QString, QJniObject::fromLocalRef(page).toString()), Q_ARG(bool, fromNotification));
}

static void handleStoragePermissionChanged(JNIEnv *, jobject, jboolean storagePermissionGranted)
{
    QMetaObject::invokeMethod(appObjectForJava, "handleStoragePermissionChanged", Qt::QueuedConnection, Q_ARG(bool, storagePermissionGranted));
}

static void handleNotificationPermissionChanged(JNIEnv *, jobject, jboolean notificationPermissionGranted)
{
    QMetaObject::invokeMethod(
        appObjectForJava, "handleNotificationPermissionChanged", Qt::QueuedConnection, Q_ARG(bool, notificationPermissionGranted));
}

void registerServiceJniMethods(AppService *appService)
{
    if (JniFn::appServiceObjectForJava) {
        return;
    }
    qDebug() << "Registering service JNI methods";
    JniFn::appServiceObjectForJava = appService;
    auto env = QJniEnvironment();
    auto registeredMethods = true;
    static const JNINativeMethod serviceMethods[] = {
        { "stopLibSyncthing", "()V", reinterpret_cast<void *>(JniFn::stopLibSyncthing) },
        { "handleMessageFromActivity", "(IIILjava/lang/String;)V", reinterpret_cast<void *>(JniFn::handleMessageFromActivity) },
        { "broadcastLauncherStatus", "()V", reinterpret_cast<void *>(JniFn::broadcastLauncherStatus) },
    };
    registeredMethods = env.registerNativeMethods("io/github/martchus/syncthingtray/SyncthingService", serviceMethods, 3) && registeredMethods;
    if (!registeredMethods) {
        qWarning() << "Unable to register all native service methods in JNI environment.";
    }
}

void unregisterServiceJniMethods(AppService *appService)
{
    if (JniFn::appServiceObjectForJava == appService) {
        JniFn::appServiceObjectForJava = nullptr;
    }
}

void registerActivityJniMethods(App *app)
{
    if (JniFn::appObjectForJava) {
        return;
    }
    qDebug() << "Registering activity JNI methods";
    JniFn::appObjectForJava = app;
    auto env = QJniEnvironment();
    auto registeredMethods = true;
    static const JNINativeMethod activityMethods[] = {
        { "handleLauncherStatusBroadcast", "(Landroid/content/Intent;)V", reinterpret_cast<void *>(JniFn::handleLauncherStatusBroadcast) },
        { "handleMessageFromService", "(IIILjava/lang/String;)V", reinterpret_cast<void *>(JniFn::handleMessageFromService) },
        { "handleAndroidIntent", "(Ljava/lang/String;Z)V", reinterpret_cast<void *>(JniFn::handleAndroidIntent) },
        { "handleStoragePermissionChanged", "(Z)V", reinterpret_cast<void *>(JniFn::handleStoragePermissionChanged) },
        { "handleNotificationPermissionChanged", "(Z)V", reinterpret_cast<void *>(JniFn::handleNotificationPermissionChanged) },
        { "loadQtQuickGui", "()V", reinterpret_cast<void *>(JniFn::loadQtQuickGui) },
        { "unloadQtQuickGui", "()V", reinterpret_cast<void *>(JniFn::unloadQtQuickGui) },
    };
    registeredMethods = env.registerNativeMethods("io/github/martchus/syncthingtray/Activity", activityMethods, 7) && registeredMethods;
    if (!registeredMethods) {
        qWarning() << "Unable to register all native activity methods in JNI environment.";
    }
}

void unregisterActivityJniMethods(App *app)
{
    if (JniFn::appObjectForJava == app) {
        JniFn::appObjectForJava = nullptr;
    }
}

} // namespace JniFn

} // namespace QtGui
