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

static std::vector<RuntimeCondition *> s_instances;

#if defined(Q_OS_ANDROID)
class AndroidBatteryMonitor {
public:
    explicit AndroidBatteryMonitor()
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

    void queryState(RuntimeCondition *instance) const
    {
        instance->setBatteryInfo(m_onBattery, m_batteryLevel);
        instance->setBatterySaving(m_batterySaving);
    }

    void handlePowerSaveModeChanged(bool powerSaveMode)
    {
        m_batterySaving = powerSaveMode;
        updateInstances();
    }

    void handleBatteryStatusChanged(bool onBattery, int batteryLevel)
    {
        m_onBattery = onBattery;
        m_batteryLevel = batteryLevel;
        updateInstances();
    }

private:
    void updateInstances()
    {
        for (auto *instance : s_instances) {
            instance->setBatteryInfo(m_onBattery, m_batteryLevel);
            instance->setBatterySaving(m_batterySaving);
        }
    }

    std::optional<bool> m_onBattery;
    std::optional<int> m_batteryLevel;
    std::optional<bool> m_batterySaving;
};

static std::unique_ptr<AndroidBatteryMonitor> s_androidMonitor;

#elif defined(PLATFORM_WINDOWS)
class WindowsBatteryMonitor {
public:
    explicit WindowsBatteryMonitor()
    {
        auto status = SYSTEM_POWER_STATUS();
        if (GetSystemPowerStatus(&status)) {
            m_onBattery
                = (status.ACLineStatus == 0 ? std::make_optional(true) : (status.ACLineStatus == 1 ? std::make_optional(false) : std::nullopt));
            m_batteryLevel = (status.BatteryLifePercent != 255 ? std::make_optional(static_cast<int>(status.BatteryLifePercent)) : std::nullopt);
            m_batterySaving = (status.SystemStatusFlag == 1 ? std::make_optional(true)
                                                            : (status.SystemStatusFlag == 0 ? std::make_optional(false) : std::nullopt));
        }

        auto wc = WNDCLASSEXW{ sizeof(WNDCLASSEXW) };
        wc.lpfnWndProc = wndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"SyncthingBatteryMonitorClass";
        RegisterClassExW(&wc);

        m_hwnd = CreateWindowExW(0, L"SyncthingBatteryMonitorClass", nullptr, 0, 0, 0, 0, 0, nullptr, nullptr, GetModuleHandleW(nullptr), this);
        if (m_hwnd) {
            m_hPowerNotifyACDC = RegisterPowerSettingNotification(m_hwnd, &GUID_ACDC_POWER_SOURCE, DEVICE_NOTIFY_WINDOW_HANDLE);
            m_hPowerNotifyPercent = RegisterPowerSettingNotification(m_hwnd, &GUID_BATTERY_PERCENTAGE_REMAINING, DEVICE_NOTIFY_WINDOW_HANDLE);
            m_hPowerNotifySaver = RegisterPowerSettingNotification(m_hwnd, &GUID_POWER_SAVING_STATUS, DEVICE_NOTIFY_WINDOW_HANDLE);
        }
    }

    void queryState(RuntimeCondition *instance) const
    {
        instance->setBatteryInfo(m_onBattery, m_batteryLevel);
        instance->setBatterySaving(m_batterySaving);
    }

    ~WindowsBatteryMonitor()
    {
        if (m_hPowerNotifyACDC)
            UnregisterPowerSettingNotification(m_hPowerNotifyACDC);
        if (m_hPowerNotifyPercent)
            UnregisterPowerSettingNotification(m_hPowerNotifyPercent);
        if (m_hPowerNotifySaver)
            UnregisterPowerSettingNotification(m_hPowerNotifySaver);
        if (m_hwnd)
            DestroyWindow(m_hwnd);
    }

private:
    void updateInstances()
    {
        for (auto *instance : s_instances) {
            instance->setBatteryInfo(m_onBattery, m_batteryLevel);
            instance->setBatterySaving(m_batterySaving);
        }
    }

    static LRESULT CALLBACK wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (uMsg == WM_CREATE) {
            auto *createStruct = reinterpret_cast<CREATESTRUCTW *>(lParam);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
            return 0;
        }
        auto *self = reinterpret_cast<WindowsBatteryMonitor *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (self && uMsg == WM_POWERBROADCAST && wParam == PBT_POWERSETTINGCHANGE) {
            const auto *setting = reinterpret_cast<const POWERBROADCAST_SETTING *>(lParam);
            self->handlePowerSetting(setting);
        }
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    void handlePowerSetting(const POWERBROADCAST_SETTING *setting)
    {
        if (setting->PowerSetting == GUID_ACDC_POWER_SOURCE) {
            const auto value = *reinterpret_cast<const DWORD *>(setting->Data);
            m_onBattery = (value == 1);
            updateInstances();
        } else if (setting->PowerSetting == GUID_BATTERY_PERCENTAGE_REMAINING) {
            const auto value = *reinterpret_cast<const DWORD *>(setting->Data);
            m_batteryLevel = static_cast<int>(value);
            updateInstances();
        } else if (setting->PowerSetting == GUID_POWER_SAVING_STATUS) {
            const auto value = *reinterpret_cast<const DWORD *>(setting->Data);
            m_batterySaving = (value == 1);
            updateInstances();
        }
    }

    HWND m_hwnd = nullptr;
    HPOWERNOTIFY m_hPowerNotifyACDC = nullptr;
    HPOWERNOTIFY m_hPowerNotifyPercent = nullptr;
    HPOWERNOTIFY m_hPowerNotifySaver = nullptr;
    std::optional<bool> m_onBattery;
    std::optional<int> m_batteryLevel;
    std::optional<bool> m_batterySaving;
};

static std::unique_ptr<WindowsBatteryMonitor> s_windowsMonitor;

#elif defined(PLATFORM_LINUX)

class LinuxBatteryMonitor : public QObject {
    Q_OBJECT
public:
    explicit LinuxBatteryMonitor()
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

    void queryState(RuntimeCondition *instance) const
    {
        instance->setBatteryInfo(m_onBattery, m_batteryLevel);
        instance->setBatterySaving(m_batterySaving);
    }

private:
    void updateInstances()
    {
        for (auto *instance : s_instances) {
            instance->setBatteryInfo(m_onBattery, m_batteryLevel);
            instance->setBatterySaving(m_batterySaving);
        }
    }

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_DBUS_BASED_POWER_MONITORING
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
        connect(timer, &QTimer::timeout, this, &LinuxBatteryMonitor::pollSysFs);
        timer->start(pollInterval);
    }

    mutable std::optional<bool> m_onBattery;
    mutable std::optional<int> m_batteryLevel;
    mutable std::optional<bool> m_batterySaving;
    bool m_logging;
};

static std::unique_ptr<LinuxBatteryMonitor> s_linuxMonitor;

#endif

#ifdef Q_OS_ANDROID
static void handlePowerSaveModeChanged(JNIEnv *, jobject, jboolean powerSaveMode)
{
    if (s_androidMonitor) {
        s_androidMonitor->handlePowerSaveModeChanged(powerSaveMode == JNI_TRUE);
    }
}

static void handleBatteryStatusChanged(JNIEnv *, jobject, jboolean onBattery, jint batteryLevel)
{
    if (s_androidMonitor) {
        s_androidMonitor->handleBatteryStatusChanged(onBattery == JNI_TRUE, static_cast<int>(batteryLevel));
    }
}

void RuntimeCondition::registerServiceJniMethods()
{
    auto env = QJniEnvironment();
    static const JNINativeMethod serviceMethods[] = {
        { "handlePowerSaveModeChanged", "(Z)V", reinterpret_cast<void *>(handlePowerSaveModeChanged) },
        { "handleBatteryStatusChanged", "(ZI)V", reinterpret_cast<void *>(handleBatteryStatusChanged) },
    };
    env.registerNativeMethods("io/github/martchus/syncthingtray/SyncthingService", serviceMethods, 2);
}

void RuntimeCondition::unregisterServiceJniMethods()
{
}
#endif

void RuntimeCondition::registerInstance(RuntimeCondition *instance)
{
    s_instances.push_back(instance);
}

void RuntimeCondition::unregisterInstance(RuntimeCondition *instance)
{
    if (auto it = std::find(s_instances.begin(), s_instances.end(), instance); it != s_instances.end()) {
        s_instances.erase(it);
    }
    if (s_instances.empty()) {
#if defined(PLATFORM_WINDOWS)
        s_windowsMonitor.reset();
#elif defined(PLATFORM_LINUX) && !defined(PLATFORM_ANDROID)
        s_linuxMonitor.reset();
#elif defined(Q_OS_ANDROID)
        s_androidMonitor.reset();
#endif
    }
}

RuntimeCondition::RuntimeCondition(Conditions conditions, QObject *parent)
    : QObject(parent)
    , m_enabledConditions(conditions)
    , m_initializedConditions(Conditions::ForceSuspend)
    , m_batteryPercentage(100)
    , m_initializing(false)
{
    registerInstance(this);
}

RuntimeCondition::~RuntimeCondition()
{
    unregisterInstance(this);
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

std::optional<bool> RuntimeCondition::isNetworkConnectionMetered() const
{
#ifdef SYNCTHINGCONNECTION_SUPPORT_METERED
    if (!(m_initializedConditions && Conditions::Metered)) {
        if (const auto [networkInformation, isInitiallyMetered] = loadNetworkInformationBackendForMetered(); networkInformation) {
            connect(networkInformation, &QNetworkInformation::isMeteredChanged, this,
                static_cast<void (RuntimeCondition::*)(bool)>(&RuntimeCondition::setNetworkConnectionMetered));
            m_metered = isInitiallyMetered;
        }
        m_initializedConditions += Conditions::Metered;
    }
#endif
    return m_metered;
}

void RuntimeCondition::setNetworkConnectionMetered(std::optional<bool> metered)
{
    if (metered != m_metered) {
        emit networkConnectionMeteredChanged(m_metered = metered);
        updateSupposedToRun();
    }
}

QString RuntimeCondition::meteredStatus() const
{
    if (const auto metered = isNetworkConnectionMetered(); metered.has_value()) {
        return metered.value() ? tr("Network connection is metered") : tr("Network connection is not metered");
    } else {
        return tr("State of network connection cannot be determined");
    }
}

std::optional<bool> RuntimeCondition::isBatterySaving() const
{
    initializeBatteryMonitoring();
    return m_batterySaving;
}

void RuntimeCondition::setBatterySaving(std::optional<bool> batterySaving)
{
    if (batterySaving != m_batterySaving) {
        emit batterySavingChanged(m_batterySaving = batterySaving);
        updateSupposedToRun();
    }
}

QString RuntimeCondition::batterySavingStatus() const
{
    if (const auto batterySaving = isBatterySaving(); batterySaving.has_value()) {
        return batterySaving.value() ? tr("Battery saving mode is enabled") : tr("Battery saving mode is disabled");
    } else {
        return tr("State of battery saving mode cannot be determined");
    }
}

std::optional<bool> RuntimeCondition::isOnBattery() const
{
    initializeBatteryMonitoring();
    return m_onBattery;
}

void RuntimeCondition::setOnBattery(std::optional<bool> onBattery)
{
    if (onBattery != m_onBattery) {
        emit onBatteryChanged(m_onBattery = onBattery);
        updateSupposedToRun();
    }
}

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

void RuntimeCondition::setBatteryLevel(std::optional<int> batteryLevel)
{
    if (batteryLevel != m_batteryLevel) {
        emit batteryLevelChanged(m_batteryLevel = batteryLevel);
        emit onBatteryChanged(m_onBattery);
        updateSupposedToRun();
    }
}

void RuntimeCondition::setBatteryInfo(std::optional<bool> onBattery, std::optional<int> batteryLevel)
{
    if (onBattery != m_onBattery || batteryLevel != m_batteryLevel) {
        m_onBattery = onBattery;
        m_batteryLevel = batteryLevel;
        emit batteryLevelChanged(m_batteryLevel);
        emit onBatteryChanged(m_onBattery);
        updateSupposedToRun();
    }
}

int RuntimeCondition::batteryPercentage() const
{
    return m_batteryPercentage;
}

void RuntimeCondition::setBatteryPercentage(int percentage)
{
    if (percentage != m_batteryPercentage) {
        emit batteryPercentageChanged(m_batteryPercentage = percentage);
        updateSupposedToRun();
    }
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

void RuntimeCondition::setEnabledConditions(Conditions enabledConditions)
{
    if (enabledConditions != m_enabledConditions) {
        emit enabledConditionsChanged(m_enabledConditions = enabledConditions);
        updateSupposedToRun();
    }
}

void RuntimeCondition::setInitializing(bool initializing)
{
    if (!(m_initializing = initializing)) {
        updateSupposedToRun();
    }
}

void RuntimeCondition::updateSupposedToRun()
{
    if (m_initializing) {
        return;
    }
    const bool oldSupposedToRun = m_supposedToRun.value_or(true);
    m_supposedToRun.reset();
    const bool newSupposedToRun = isSupposedToRun();
    if (newSupposedToRun != oldSupposedToRun) {
        emit supposedToRunChanged(newSupposedToRun);
    }
}

void RuntimeCondition::initializeBatteryMonitoring() const
{
    if (!(m_initializedConditions && (Conditions::BatterySaving | Conditions::OnBattery))) {
#if defined(PLATFORM_WINDOWS)
        if (!s_windowsMonitor) {
            s_windowsMonitor = std::make_unique<WindowsBatteryMonitor>();
        }
        s_windowsMonitor->queryState(const_cast<RuntimeCondition *>(this));
#elif defined(PLATFORM_LINUX) && !defined(PLATFORM_ANDROID)
        if (!s_linuxMonitor) {
            s_linuxMonitor = std::make_unique<LinuxBatteryMonitor>();
        }
        s_linuxMonitor->queryState(const_cast<RuntimeCondition *>(this));
#elif defined(Q_OS_ANDROID)
        if (!s_androidMonitor) {
            s_androidMonitor = std::make_unique<AndroidBatteryMonitor>();
        }
        s_androidMonitor->queryState(const_cast<RuntimeCondition *>(this));
#endif
        m_initializedConditions += Conditions::BatterySaving | Conditions::OnBattery;
    }
}

} // namespace Data

#if defined(PLATFORM_LINUX) && !defined(PLATFORM_ANDROID)
#include "runtimecondition.moc"
#endif
