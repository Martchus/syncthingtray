#include "./runtimecondition.h"
#include "./utils.h"

#include "resources/config.h"

#ifdef SYNCTHINGCONNECTION_SUPPORT_METERED
#include <QNetworkInformation>
#endif

#if defined(PLATFORM_LINUX) && !defined(PLATFORM_ANDROID)
#include <QDir>
#include <QFile>
#include <QTimer>

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_DBUS_BASED_POWER_MONITORING
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#endif
#endif

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#endif

#if defined(Q_OS_ANDROID) && defined(SYNCTHINGCONNECTION_SUPPORT_METERED)
#include <QDebug>
#include <QJniEnvironment>
#include <QtCore/private/qandroidextras_p.h>
#endif

#include <c++utilities/io/ansiescapecodes.h>

#include <algorithm>
#include <iostream>

namespace Data {

/*!
 * \class RuntimeCondition
 * \brief The RuntimeCondition class evaluates and handles conditions determining whether Syncthing should run or suspend.
 *
 * This class monitors platform-specific network parameters (e.g. metered connections) and tracks custom
 * runtime states (e.g. force suspend).
 *
 * \remarks
 * When extending the \ref Conditions flags in the future (e.g., adding battery status):
 * - Define a new flag within the enum.
 * - Handle the new condition's evaluation inside the \ref isSupposedToRun(Conditions) const method.
 * - Make sure to adjust `Settings::Launcher`, `SyncthingConnectionSettings` JSON serialization, and all related settings/UI code.
 */

#ifdef SYNCTHINGCONNECTION_SUPPORT_METERED
/*!
 * \brief Loads the QNetworkInformation backend for determining whether the connection is metered.
 * \remarks Returns the backend and whether the network connection is metered right now.
 */
static std::pair<const QNetworkInformation *, bool> loadNetworkInformationBackendForMetered()
{
    static const auto *const backend = []() -> const QNetworkInformation * {
#ifdef Q_OS_ANDROID
        // load the network information plugin under Android by its name because it doesn't advertise supporting detection of metered
        // connections even though it supports it (at least as of Qt 6.8.0)
        constexpr auto expectedFeatures = QNetworkInformation::Feature::TransportMedium;
        QNetworkInformation::loadBackendByName(QStringLiteral("android"));
#else
        constexpr auto expectedFeatures = QNetworkInformation::Feature::Metered;
        QNetworkInformation::loadBackendByFeatures(expectedFeatures);
#endif
        if (const auto *const networkInformation = QNetworkInformation::instance();
            networkInformation && networkInformation->supports(expectedFeatures)) {
            return networkInformation;
        }

#ifdef Q_OS_ANDROID
        qDebug() << "Unable to load network information backend, available backends: " << QNetworkInformation::availableBackends();
#else
        std::cerr << CppUtilities::EscapeCodes::Phrases::Error
                  << "Unable to load network information backend to monitor metered connections, available backends:"
                  << CppUtilities::EscapeCodes::Phrases::End;
        const auto availableBackends = QNetworkInformation::availableBackends();
        if (availableBackends.isEmpty()) {
            std::cerr << "none\n";
        } else {
            for (const auto &backendName : availableBackends) {
                std::cerr << " - " << backendName.toStdString() << '\n';
            }
        }
#endif
        return nullptr;
    }();

    auto isInitiallyMetered = backend && backend->isMetered();
#ifdef Q_OS_ANDROID
    // detect the initial status of whether the network connection is metered manually under Android because QNetworkInformation always
    // returns false on startup with no way to know when it has been initialized
    if (!isInitiallyMetered) {
        if (const auto context = QNativeInterface::QAndroidApplication::context(); context.isValid()) {
            auto env = QJniEnvironment();
            if (auto method = env.findMethod(context.objectClass(), "isNetworkConnectionMetered", "()Z")) {
                isInitiallyMetered = env->CallBooleanMethod(context.object(), method) == JNI_TRUE;
            }
        }
    }
#endif
    return std::make_pair(backend, isInitiallyMetered);
}
#endif

/*!
 * \brief The BatteryMonitorBase class is a base class for battery monitoring,
 * providing the capability to query and update battery states across all runtime condition instances.
 */
class BatteryMonitorBase {
public:
    /*!
     * \brief Queries and copies the current battery monitoring states into the specified \a instance.
     */
    void queryState(const RuntimeCondition *instance) const
    {
        instance->m_onBattery = m_onBattery;
        instance->m_batteryLevel = m_batteryLevel;
        instance->m_batterySaving = m_batterySaving;
    }

protected:
    /*!
     * \brief Updates the battery states and supposed-to-run status for all active RuntimeCondition instances.
     */
    void updateInstances()
    {
        for (auto *const instance : RuntimeCondition::s_instances) {
            instance->m_updating = true;
            const auto batteryInfoChanged = instance->setBatteryInfo(m_onBattery, m_batteryLevel);
            const auto batterySavingChanged = instance->setBatterySaving(m_batterySaving);
            instance->m_updating = false;
            if (batteryInfoChanged || batterySavingChanged) {
                instance->updateSupposedToRun();
            }
        }
    }

    std::optional<bool> m_onBattery; /*!< Whether the system is running on battery. */
    std::optional<int> m_batteryLevel; /*!< The current battery level percentage (0-100). */
    std::optional<bool> m_batterySaving; /*!< Whether battery saving mode is enabled. */
};

#if defined(Q_OS_ANDROID)
/*!
 * \brief The BatteryMonitor class monitors battery level, battery saving, and power source changes on Android.
 */
class BatteryMonitor : public QObject, public BatteryMonitorBase {
    Q_OBJECT
public:
    /*!
     * \brief Constructs a new BatteryMonitor and queries the initial battery and power-saving states.
     */
    explicit BatteryMonitor()
    {
        if (const auto context = QNativeInterface::QAndroidApplication::context(); context.isValid()) {
            auto env = QJniEnvironment();
            if (auto method = env.findMethod(context.objectClass(), "isPowerSaveMode", "()Z")) {
                m_batterySaving = std::make_optional(env->CallBooleanMethod(context.object(), method) == JNI_TRUE);
            }
            if (auto method = env.findMethod(context.objectClass(), "queryBatteryInfo", "()I")) {
                const int val = static_cast<int>(env->CallIntMethod(context.object(), method));
                if ((m_onBattery = val >= 0)) {
                    m_batteryLevel = val;
                } else {
                    m_batteryLevel = -1 - val;
                }
            }
        }
    }

    /*!
     * \brief JNI callback invoked when the Android power save mode changes.
     */
    static void jniHandlePowerSaveModeChanged(JNIEnv *, jobject, jboolean powerSaveMode)
    {
        if (RuntimeCondition::s_batteryMonitor) {
            QMetaObject::invokeMethod(
                RuntimeCondition::s_batteryMonitor.get(), "handlePowerSaveModeChanged", Qt::QueuedConnection, Q_ARG(bool, powerSaveMode == JNI_TRUE));
        }
    }

    /*!
     * \brief JNI callback invoked when the Android battery status changes.
     */
    static void jniHandleBatteryStatusChanged(JNIEnv *, jobject, jboolean onBattery, jint batteryLevel)
    {
        if (RuntimeCondition::s_batteryMonitor) {
            QMetaObject::invokeMethod(RuntimeCondition::s_batteryMonitor.get(), "handleBatteryStatusChanged", Qt::QueuedConnection,
                Q_ARG(bool, onBattery == JNI_TRUE), Q_ARG(int, static_cast<int>(batteryLevel)));
        }
    }

public Q_SLOTS:
    /*!
     * \brief Handles changes in power save mode from Android.
     */
    void handlePowerSaveModeChanged(bool powerSaveMode)
    {
        m_batterySaving = powerSaveMode;
        updateInstances();
    }

    /*!
     * \brief Handles changes in battery status from Android.
     */
    void handleBatteryStatusChanged(bool onBattery, int batteryLevel)
    {
        m_onBattery = onBattery;
        m_batteryLevel = batteryLevel;
        updateInstances();
    }
};

#elif defined(PLATFORM_WINDOWS)

#ifndef GUID_ACDC_POWER_SOURCE
static const GUID GUID_ACDC_POWER_SOURCE = { 0x5d3e9a59, 0xe9d5, 0x4b00, { 0xa6, 0xbd, 0xff, 0x34, 0xff, 0x51, 0x65, 0x48 } };
#endif
#ifndef GUID_BATTERY_PERCENTAGE_REMAINING
static const GUID GUID_BATTERY_PERCENTAGE_REMAINING = { 0xa7ad8041, 0xb45a, 0x4cae, { 0x87, 0xa3, 0xee, 0xcb, 0xb4, 0x68, 0xa9, 0xe1 } };
#endif
#ifndef GUID_POWER_SAVING_STATUS
static const GUID GUID_POWER_SAVING_STATUS = { 0xe00958c0, 0xc213, 0x4ace, { 0xac, 0x77, 0xfe, 0xcc, 0xed, 0x2e, 0xee, 0xa5 } };
#endif

/*!
 * \brief The BatteryMonitor class monitors battery level, battery saving, and power source changes on Windows.
 */
class BatteryMonitor : public BatteryMonitorBase {
public:
    /*!
     * \brief Constructs a new BatteryMonitor, queries the initial power status, and registers for power setting notifications.
     */
    explicit BatteryMonitor()
    {
        auto status = SYSTEM_POWER_STATUS();
        if (GetSystemPowerStatus(&status)) {
            m_onBattery
                = (status.ACLineStatus == 0 ? std::make_optional(true) : (status.ACLineStatus == 1 ? std::make_optional(false) : std::nullopt));
            m_batteryLevel = (status.BatteryLifePercent != 255 ? std::make_optional(static_cast<int>(status.BatteryLifePercent)) : std::nullopt);
            m_batterySaving = (status.SystemStatusFlag == 1 ? std::make_optional(true)
                                                            : (status.SystemStatusFlag == 0 ? std::make_optional(false) : std::nullopt));
        }

        auto wc = WNDCLASSEXW{};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = wndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"SyncthingBatteryMonitorClass";
        RegisterClassExW(&wc);

        m_hwnd
            = CreateWindowExW(0, L"SyncthingBatteryMonitorClass", nullptr, WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, GetModuleHandleW(nullptr), this);
        if (m_hwnd) {
            m_hPowerNotifyACDC = RegisterPowerSettingNotification(m_hwnd, &GUID_ACDC_POWER_SOURCE, DEVICE_NOTIFY_WINDOW_HANDLE);
            m_hPowerNotifyPercent = RegisterPowerSettingNotification(m_hwnd, &GUID_BATTERY_PERCENTAGE_REMAINING, DEVICE_NOTIFY_WINDOW_HANDLE);
            m_hPowerNotifySaver = RegisterPowerSettingNotification(m_hwnd, &GUID_POWER_SAVING_STATUS, DEVICE_NOTIFY_WINDOW_HANDLE);
        }
    }

    /*!
     * \brief Destructs the BatteryMonitor and unregisters all power setting notifications.
     */
    ~BatteryMonitor()
    {
        if (m_hPowerNotifyACDC)
            UnregisterPowerSettingNotification(m_hPowerNotifyACDC);
        if (m_hPowerNotifyPercent)
            UnregisterPowerSettingNotification(m_hPowerNotifyPercent);
        if (m_hPowerNotifySaver)
            UnregisterPowerSettingNotification(m_hPowerNotifySaver);
        if (m_hwnd)
            DestroyWindow(m_hwnd);
        UnregisterClassW(L"SyncthingBatteryMonitorClass", GetModuleHandleW(nullptr));
    }

private:
    /*!
     * \brief Window procedure to handle WM_POWERBROADCAST messages.
     */
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (uMsg == WM_CREATE) {
            auto *createStruct = reinterpret_cast<CREATESTRUCTW *>(lParam);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
            return 0;
        }
        auto *self = reinterpret_cast<BatteryMonitor *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (self && uMsg == WM_POWERBROADCAST && wParam == PBT_POWERSETTINGCHANGE) {
            const auto *setting = reinterpret_cast<const POWERBROADCAST_SETTING *>(lParam);
            self->handlePowerSetting(setting);
        }
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    /*!
     * \brief Handles changes in registered power settings.
     */
    void handlePowerSetting(const POWERBROADCAST_SETTING *setting)
    {
        if (IsEqualGUID(setting->PowerSetting, GUID_ACDC_POWER_SOURCE)) {
            const auto value = *reinterpret_cast<const DWORD *>(setting->Data);
            m_onBattery = (value == 1);
            updateInstances();
        } else if (IsEqualGUID(setting->PowerSetting, GUID_BATTERY_PERCENTAGE_REMAINING)) {
            const auto value = *reinterpret_cast<const DWORD *>(setting->Data);
            m_batteryLevel = static_cast<int>(value);
            updateInstances();
        } else if (IsEqualGUID(setting->PowerSetting, GUID_POWER_SAVING_STATUS)) {
            const auto value = *reinterpret_cast<const DWORD *>(setting->Data);
            m_batterySaving = (value == 1);
            updateInstances();
        }
    }

    HWND m_hwnd = nullptr; /*!< Window handle for the monitor window. */
    HPOWERNOTIFY m_hPowerNotifyACDC = nullptr; /*!< Power notification handle for AC/DC changes. */
    HPOWERNOTIFY m_hPowerNotifyPercent = nullptr; /*!< Power notification handle for battery percentage changes. */
    HPOWERNOTIFY m_hPowerNotifySaver = nullptr; /*!< Power notification handle for power saving state changes. */
};

#elif defined(PLATFORM_LINUX)

/*!
 * \brief The BatteryMonitor class monitors battery level, battery saving, and power source changes on Linux.
 * \remarks It uses DBus-based power monitoring when available, falling back to SysFs-based polling.
 */
class BatteryMonitor : public QObject, public BatteryMonitorBase {
    Q_OBJECT
public:
    /*!
     * \brief Constructs a new BatteryMonitor, setting up DBus power monitoring or SysFs polling fallback.
     */
    explicit BatteryMonitor()
        : QObject(nullptr)
        , m_logging(qEnvironmentVariableIntValue(PROJECT_VARNAME_UPPER "_LOG_POWER_MONITORING") != 0)
    {
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_DBUS_BASED_POWER_MONITORING
        const auto forceSysFs = qEnvironmentVariableIntValue(PROJECT_VARNAME_UPPER "_POWER_MONITORING_FORCE_SYS_FS") != 0;
        if (!forceSysFs && setupDBus()) {
            queryInitialDBusState();
        } else {
            setupSysFsFallback();
        }
#else
        setupSysFsFallback();
#endif
    }

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_DBUS_BASED_POWER_MONITORING
    /*!
     * \brief Sets up DBus connections to UPower and PowerProfiles.
     */
    bool setupDBus()
    {
        auto sysBus = QDBusConnection::systemBus();
        if (!sysBus.isConnected()) {
            return false;
        }
        // clang-format off
        auto connected = sysBus.connect(QStringLiteral("org.freedesktop.UPower"), QStringLiteral("/org/freedesktop/UPower/devices/DisplayDevice"),
            QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("PropertiesChanged"), this,
            SLOT(handleUPowerPropertiesChanged(QString,QVariantMap,QStringList)));
        connected &= sysBus.connect(QStringLiteral("org.freedesktop.UPower.PowerProfiles"), QStringLiteral("/org/freedesktop/UPower/PowerProfiles"),
            QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("PropertiesChanged"), this,
            SLOT(handlePowerProfilesPropertiesChanged(QString,QVariantMap,QStringList)));
        // clang-format on
        if (m_logging && !connected) {
            using namespace CppUtilities::EscapeCodes;
            std::cerr << Phrases::Warning
                      << "Power monitoring via D-Bus not available as connecting to the D-Bus services "
                         "\"org.freedesktop.UPower:/org/freedesktop/UPower/devices/DisplayDevice\" and "
                         "\"org.freedesktop.UPower.PowerProfile:/org/freedesktop/UPower/PowerProfiless\" is not possible."
                      << Phrases::End;
        }
        return connected;
    }

    /*!
     * \brief Queries initial battery and power saver state over DBus.
     */
    void queryInitialDBusState()
    {
        auto sysBus = QDBusConnection::systemBus();
        if (auto upowerDevice = QDBusInterface(QStringLiteral("org.freedesktop.UPower"),
                QStringLiteral("/org/freedesktop/UPower/devices/DisplayDevice"), QStringLiteral("org.freedesktop.UPower.Device"), sysBus);
            upowerDevice.isValid()) {
            const bool isPresent = upowerDevice.property("IsPresent").toBool();
            if (isPresent) {
                const auto state = upowerDevice.property("State").toUInt();
                const auto percentage = upowerDevice.property("Percentage").toDouble();
                m_onBattery = (state == 2);
                m_batteryLevel = static_cast<int>(percentage);
            } else {
                m_onBattery = false;
                m_batteryLevel = 100;
            }
        }
        if (auto powerProfiles = QDBusInterface(QStringLiteral("org.freedesktop.UPower.PowerProfiles"),
                QStringLiteral("/org/freedesktop/UPower/PowerProfiles"), QStringLiteral("org.freedesktop.UPower.PowerProfiles"), sysBus);
            powerProfiles.isValid()) {
            const auto activeProfile = powerProfiles.property("ActiveProfile").toString();
            m_batterySaving = (activeProfile == QLatin1String("power-saver"));
        } else {
            queryPortalPowerSaver();
        }
        updateInstances();
    }

    /*!
     * \brief Queries the Flatpak Portal settings for the power saver mode status.
     */
    void queryPortalPowerSaver()
    {
        auto sessionBus = QDBusConnection::sessionBus();
        if (!sessionBus.isConnected())
            return;

        auto portalSettings = QDBusInterface(QStringLiteral("org.freedesktop.portal.Desktop"), QStringLiteral("/org/freedesktop/portal/desktop"),
            QStringLiteral("org.freedesktop.portal.Settings"), sessionBus);
        if (portalSettings.isValid()) {
            auto reply
                = portalSettings.call(QStringLiteral("Read"), QStringLiteral("org.freedesktop.appearance"), QStringLiteral("power-saver-enabled"));
            if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
                const auto value = reply.arguments().at(0).value<QDBusVariant>().variant();
                auto isSaving = false;
                if (value.userType() == QMetaType::Bool) {
                    isSaving = value.toBool();
                } else if (value.userType() == QMetaType::UInt) {
                    isSaving = (value.toUInt() == 1);
                }
                m_batterySaving = isSaving;
            }

            // clang-format off
            sessionBus.connect(QStringLiteral("org.freedesktop.portal.Desktop"), QStringLiteral("/org/freedesktop/portal/desktop"),
                QStringLiteral("org.freedesktop.portal.Settings"), QStringLiteral("SettingChanged"), this,
                SLOT(handlePortalSettingChanged(QString,QString,QDBusVariant)));
            // clang-format on
        }
    }
#endif

private Q_SLOTS:
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_DBUS_BASED_POWER_MONITORING
    /*!
     * \brief Slot called when UPower device properties change.
     */
    void handleUPowerPropertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
    {
        Q_UNUSED(interface)
        Q_UNUSED(invalidatedProperties)
        if (changedProperties.contains(QLatin1String("IsPresent"))) {
            if (!changedProperties.value(QLatin1String("IsPresent")).toBool()) {
                m_onBattery = false;
                m_batteryLevel = 100;
                updateInstances();
                return;
            }
        }
        if (changedProperties.contains(QLatin1String("State")) || changedProperties.contains(QLatin1String("Percentage"))) {
            const auto state = changedProperties.contains(QLatin1String("State")) ? changedProperties.value(QLatin1String("State")).toUInt()
                : m_onBattery.value_or(false)                                     ? 2u
                                                                                  : 1u;
            const auto percentage = changedProperties.contains(QLatin1String("Percentage"))
                ? changedProperties.value(QLatin1String("Percentage")).toDouble()
                : m_batteryLevel.value_or(100);
            m_onBattery = (state == 2);
            m_batteryLevel = static_cast<int>(percentage);
            updateInstances();
        }
    }

    /*!
     * \brief Slot called when UPower PowerProfiles properties change.
     */
    void handlePowerProfilesPropertiesChanged(
        const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
    {
        Q_UNUSED(interface)
        Q_UNUSED(invalidatedProperties)
        if (changedProperties.contains(QLatin1String("ActiveProfile"))) {
            const auto activeProfile = changedProperties.value(QLatin1String("ActiveProfile")).toString();
            m_batterySaving = (activeProfile == QLatin1String("power-saver"));
            updateInstances();
        }
    }

    /*!
     * \brief Slot called when Flatpak Portal settings change.
     */
    void handlePortalSettingChanged(const QString &namespaceName, const QString &key, const QDBusVariant &value)
    {
        if (namespaceName == QLatin1String("org.freedesktop.appearance") && key == QLatin1String("power-saver-enabled")) {
            const auto variant = value.variant();
            auto isSaving = false;
            if (variant.userType() == QMetaType::Bool) {
                isSaving = variant.toBool();
            } else if (variant.userType() == QMetaType::UInt) {
                isSaving = (variant.toUInt() == 1);
            }
            m_batterySaving = isSaving;
            updateInstances();
        }
    }
#endif

    /*!
     * \brief Polls the Linux `/sys/class/power_supply` filesystem for battery status.
     */
    void pollSysFs()
    {
        auto dir = QDir(QStringLiteral("/sys/class/power_supply"));
        if (!dir.exists()) {
            if (m_logging) {
                using namespace CppUtilities::EscapeCodes;
                std::cerr << Phrases::Warning << "Power monitoring not available as \"/sys/class/power_supply\" does not exist." << Phrases::End;
            }
            m_onBattery = std::nullopt;
            m_batteryLevel = std::nullopt;
            updateInstances();
            return;
        }

        auto onBattery = false, hasBattery = false;
        auto minBatteryLevel = 100;

        const auto entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const auto &entry : entries) {
            auto deviceDir = QDir(dir.absoluteFilePath(entry));
            auto typeFile = QFile(deviceDir.absoluteFilePath(QStringLiteral("type")));
            if (!typeFile.open(QIODevice::ReadOnly) || typeFile.readAll().trimmed() != QByteArrayLiteral("Battery")) {
                continue;
            }
            if (auto scopeFile = QFile(deviceDir.absoluteFilePath(QStringLiteral("scope"))); scopeFile.open(QIODevice::ReadOnly)) {
                if (scopeFile.readAll().trimmed() == QByteArrayLiteral("Device")) {
                    continue;
                }
            }
            hasBattery = true;
            if (auto statusFile = QFile(deviceDir.absoluteFilePath(QStringLiteral("status"))); statusFile.open(QIODevice::ReadOnly)) {
                if (statusFile.readAll().trimmed() == QByteArrayLiteral("Discharging")) {
                    onBattery = true;
                }
            }
            if (auto capacityFile = QFile(deviceDir.absoluteFilePath(QStringLiteral("capacity"))); capacityFile.open(QIODevice::ReadOnly)) {
                auto ok = false;
                auto level = capacityFile.readAll().trimmed().toInt(&ok);
                if (ok && level < minBatteryLevel) {
                    minBatteryLevel = level;
                }
            }
        }

        if (hasBattery) {
            m_onBattery = onBattery;
            m_batteryLevel = minBatteryLevel;
        } else {
            m_onBattery = false;
            m_batteryLevel = 100;
        }
        updateInstances();
    }

private:
    /*!
     * \brief Sets up the SysFs fallback timer with the configured poll interval.
     */
    void setupSysFsFallback()
    {
        using namespace CppUtilities::EscapeCodes;

        auto ok = false;
        auto pollInterval = qEnvironmentVariableIntValue(PROJECT_VARNAME_UPPER "_POWER_MONITORING_POLL_INTERVAL", &ok);
        if (pollInterval < 1) {
            if (m_logging) {
                std::cerr << Phrases::Info
                          << "Polling \"/sys/class/power_supply\" is disabled. Set the environment variable " PROJECT_VARNAME_UPPER
                             "_POWER_MONITORING_POLL_INTERVAL to enable this fall back by specifying the poll interval in milliseconds."
                          << Phrases::End;
            }
            return;
        }
        if (m_logging) {
            std::cerr << Phrases::Warning << "Falling back to polling \"/sys/class/power_supply\" every " << pollInterval
                      << " ms to monitor power/battery status." << Phrases::End;
        }

        pollSysFs();
        auto *const timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &BatteryMonitor::pollSysFs);
        timer->start(pollInterval);
    }

    bool m_logging;
};
#endif

/*!
 * \brief Active instances of RuntimeCondition.
 */
std::vector<RuntimeCondition *> RuntimeCondition::s_instances;
#ifdef SYNCTHINGCONNECTION_SUPPORT_BATTERY_MONITORING
/*!
 * \brief Active platform-specific battery monitor instance.
 */
std::unique_ptr<BatteryMonitor> RuntimeCondition::s_batteryMonitor;
#endif

/*!
 * \brief Constructs a new RuntimeCondition with the specified \a conditions.
 */
RuntimeCondition::RuntimeCondition(Conditions conditions, QObject *parent)
    : QObject(parent)
    , m_enabledConditions(conditions)
    , m_initializedConditions(Conditions::ForceSuspend)
    , m_batteryPercentage(100)
    , m_updating(false)
{
    registerInstance(this);
}

/*!
 * \brief Destructs the RuntimeCondition.
 */
RuntimeCondition::~RuntimeCondition()
{
    unregisterInstance(this);
}

#ifdef Q_OS_ANDROID
/*!
 * \brief Registers native JNI methods for battery monitoring under Android.
 */
bool RuntimeCondition::registerJniMethods(const char *className)
{
    auto env = QJniEnvironment();
    static const JNINativeMethod methods[] = {
        { "handlePowerSaveModeChanged", "(Z)V", reinterpret_cast<void *>(BatteryMonitor::jniHandlePowerSaveModeChanged) },
        { "handleBatteryStatusChanged", "(ZI)V", reinterpret_cast<void *>(BatteryMonitor::jniHandleBatteryStatusChanged) },
    };
    return env.registerNativeMethods(className, methods, 2);
}

/*!
 * \brief Unregisters native JNI methods.
 */
void RuntimeCondition::unregisterJniMethods()
{
}
#endif

/*!
 * \brief Registers a RuntimeCondition \a instance for battery state updates.
 */
void RuntimeCondition::registerInstance(RuntimeCondition *instance)
{
    s_instances.push_back(instance);
}

/*!
 * \brief Unregisters a RuntimeCondition \a instance.
 */
void RuntimeCondition::unregisterInstance(RuntimeCondition *instance)
{
    if (auto it = std::find(s_instances.begin(), s_instances.end(), instance); it != s_instances.end()) {
        s_instances.erase(it);
    }
#ifdef SYNCTHINGCONNECTION_SUPPORT_BATTERY_MONITORING
    if (s_instances.empty()) {
        s_batteryMonitor.reset();
    }
#endif
}

/*!
 * \brief Returns whether the service is supposed to run based on the current runtime conditions.
 */
bool RuntimeCondition::isSupposedToRun() const
{
    if (m_supposedToRun.has_value()) {
        return m_supposedToRun.value();
    }
    return m_supposedToRun.emplace(isSupposedToRun(m_enabledConditions));
}

/*!
 * \brief Returns whether the service is supposed to run based on the specified runtime \a conditions.
 */
bool RuntimeCondition::isSupposedToRun(Conditions conditions) const
{
    return !((conditions && Conditions::ForceSuspend) || (conditions && Conditions::Metered && isNetworkConnectionMetered().value_or(false))
        || (conditions && Conditions::BatterySaving && isBatterySaving().value_or(false))
        || (conditions && Conditions::OnBattery && isOnBattery().value_or(false)
            && (batteryLevel().value_or(100) < m_batteryPercentage || m_batteryPercentage == 100)));
}

/*!
 * \brief Returns whether the current network connection is metered, or std::nullopt if unknown.
 */
std::optional<bool> RuntimeCondition::isNetworkConnectionMetered() const
{
#ifdef SYNCTHINGCONNECTION_SUPPORT_METERED
    if (!(m_initializedConditions && Conditions::Metered)) {
        if (const auto [networkInformation, isInitiallyMetered] = loadNetworkInformationBackendForMetered(); networkInformation) {
            connect(networkInformation, &QNetworkInformation::isMeteredChanged, this,
                static_cast<bool (RuntimeCondition::*)(bool)>(&RuntimeCondition::setNetworkConnectionMetered));
            m_metered = isInitiallyMetered;
        }
        m_initializedConditions += Conditions::Metered;
    }
#endif
    return m_metered;
}

/*!
 * \brief Sets whether the current network connection is metered.
 */
bool RuntimeCondition::setNetworkConnectionMetered(std::optional<bool> metered)
{
    if (metered != m_metered) {
        emit networkConnectionMeteredChanged(m_metered = metered);
        updateSupposedToRun();
        return true;
    }
    return false;
}

/*!
 * \brief Returns a short translated status message about the metered state of the connection.
 */
QString RuntimeCondition::meteredStatus() const
{
    if (const auto metered = isNetworkConnectionMetered(); metered.has_value()) {
        return metered.value() ? tr("Network connection is metered") : tr("Network connection is not metered");
    } else {
        return tr("State of network connection cannot be determined");
    }
}

/*!
 * \brief Returns whether battery saving mode is enabled, or std::nullopt if unknown.
 */
std::optional<bool> RuntimeCondition::isBatterySaving() const
{
    initializeBatteryMonitoring();
    return m_batterySaving;
}

/*!
 * \brief Sets whether battery saving mode is enabled.
 */
bool RuntimeCondition::setBatterySaving(std::optional<bool> batterySaving)
{
    if (batterySaving != m_batterySaving) {
        emit batterySavingChanged(m_batterySaving = batterySaving);
        updateSupposedToRun();
        return true;
    }
    return false;
}

/*!
 * \brief Returns a short translated status message about the battery saving mode.
 */
QString RuntimeCondition::batterySavingStatus() const
{
    if (const auto batterySaving = isBatterySaving(); batterySaving.has_value()) {
        return batterySaving.value() ? tr("Battery saving mode is enabled") : tr("Battery saving mode is disabled");
    } else {
        return tr("State of battery saving mode cannot be determined");
    }
}

/*!
 * \brief Returns whether the system is running on battery, or std::nullopt if unknown.
 */
std::optional<bool> RuntimeCondition::isOnBattery() const
{
    initializeBatteryMonitoring();
    return m_onBattery;
}

/*!
 * \brief Sets whether the system is running on battery.
 */
bool RuntimeCondition::setOnBattery(std::optional<bool> onBattery)
{
    if (onBattery != m_onBattery) {
        emit onBatteryChanged(m_onBattery = onBattery);
        updateSupposedToRun();
        return true;
    }
    return false;
}

/*!
 * \brief Returns a short translated status message about the battery status.
 */
QString RuntimeCondition::onBatteryStatus() const
{
    if (const auto onBattery = isOnBattery(); onBattery.has_value()) {
        if (onBattery.value()) {
            if (const auto level = batteryLevel(); level.has_value()) {
                return tr("Running on battery (%1%)").arg(level.value());
            } else {
                return tr("Running on battery");
            }
        } else {
            return tr("Power supply connected");
        }
    } else {
        return tr("Battery status cannot be determined");
    }
}

std::optional<int> RuntimeCondition::batteryLevel() const
{
    initializeBatteryMonitoring();
    return m_batteryLevel;
}

bool RuntimeCondition::setBatteryLevel(std::optional<int> batteryLevel)
{
    if (batteryLevel != m_batteryLevel) {
        emit batteryLevelChanged(m_batteryLevel = batteryLevel);
        emit onBatteryChanged(m_onBattery);
        updateSupposedToRun();
        return true;
    }
    return false;
}

bool RuntimeCondition::setBatteryInfo(std::optional<bool> onBattery, std::optional<int> batteryLevel)
{
    if (onBattery != m_onBattery || batteryLevel != m_batteryLevel) {
        m_onBattery = onBattery;
        m_batteryLevel = batteryLevel;
        emit batteryLevelChanged(m_batteryLevel);
        emit onBatteryChanged(m_onBattery);
        updateSupposedToRun();
        return true;
    }
    return false;
}

int RuntimeCondition::batteryPercentage() const
{
    return m_batteryPercentage;
}

bool RuntimeCondition::setBatteryPercentage(int percentage)
{
    if (percentage != m_batteryPercentage) {
        emit batteryPercentageChanged(m_batteryPercentage = percentage);
        updateSupposedToRun();
        return true;
    }
    return false;
}

QString RuntimeCondition::stopStatusMessage() const
{
    return stopStatusMessage(m_enabledConditions);
}

QString RuntimeCondition::stopStatusMessage(Conditions conditions) const
{
    if (conditions && Conditions::Metered && m_metered.value_or(false)) {
        return tr("Syncthing is temporarily stopped due to metered connection");
    } else if (conditions && Conditions::BatterySaving && m_batterySaving.value_or(false)) {
        return tr("Syncthing is temporarily stopped due to battery saving mode");
    } else if (conditions && Conditions::OnBattery && isOnBattery().value_or(false)
        && (batteryLevel().value_or(100) < m_batteryPercentage || m_batteryPercentage == 100)) {
        if (m_batteryPercentage == 100) {
            return tr("Syncthing is temporarily stopped due to running on battery");
        } else {
            return tr("Syncthing is temporarily stopped due to low battery");
        }
    } else if (conditions && Conditions::ForceSuspend) {
        return tr("Syncthing is temporarily stopped manually");
    }
    return QString();
}

bool RuntimeCondition::setEnabledConditions(Conditions enabledConditions)
{
    if (enabledConditions != m_enabledConditions) {
        emit enabledConditionsChanged(m_enabledConditions = enabledConditions);
        updateSupposedToRun();
        return true;
    }
    return false;
}

/*!
 * \brief Recalculates and updates the cached supposed-to-run status, emitting the changed signal if necessary.
 */
void RuntimeCondition::updateSupposedToRun()
{
    if (m_updating) {
        return;
    }
    m_updating = true;
    const bool oldSupposedToRun = m_supposedToRun.value_or(true);
    m_supposedToRun.reset();
    const bool newSupposedToRun = isSupposedToRun();
    if (newSupposedToRun != oldSupposedToRun) {
        emit supposedToRunChanged(newSupposedToRun);
    }
    m_updating = false;
}

/*!
 * \brief Initializes the battery monitor instance if needed.
 */
void RuntimeCondition::initializeBatteryMonitoring() const
{
    if (!(m_initializedConditions && (Conditions::BatterySaving | Conditions::OnBattery))) {
#ifdef SYNCTHINGCONNECTION_SUPPORT_BATTERY_MONITORING
        if (!s_batteryMonitor) {
            s_batteryMonitor = std::make_unique<BatteryMonitor>();
        }
        s_batteryMonitor->queryState(this);
#endif
        m_initializedConditions += Conditions::BatterySaving | Conditions::OnBattery;
    }
}

} // namespace Data

#if defined(PLATFORM_LINUX)
#include "runtimecondition.moc"
#endif
