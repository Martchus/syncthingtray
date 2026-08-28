#include "./runtimecondition.h"
#include "./utils.h"

#ifdef SYNCTHINGCONNECTION_SUPPORT_METERED
#include <QNetworkInformation>
#endif

#ifdef PLATFORM_WINDOWS
#include <windows.h>
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

RuntimeCondition::RuntimeCondition(Conditions conditions, QObject *parent)
    : QObject(parent)
    , m_enabledConditions(conditions)
    , m_initializedConditions(Conditions::ForceSuspend)
    , m_batteryPercentage(100)
    , m_initializing(false)
{
}

RuntimeCondition::~RuntimeCondition() = default;

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
        if (const auto [networkInformation, isInitiallyMetered] = loadNetworkInformationBackendForMetered(true); networkInformation) {
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
        m_windowsMonitor = std::make_unique<WindowsBatteryMonitor>(const_cast<RuntimeCondition *>(this));
#elif defined(PLATFORM_LINUX) && !defined(PLATFORM_ANDROID)
        m_linuxMonitor = std::make_unique<LinuxBatteryMonitor>(const_cast<RuntimeCondition *>(this));
#endif
        m_initializedConditions += Conditions::BatterySaving | Conditions::OnBattery;
    }
}

#ifdef PLATFORM_WINDOWS

#ifndef GUID_ACDC_POWER_SOURCE
static const GUID GUID_ACDC_POWER_SOURCE = { 0x5ce81283, 0x4e6e, 0x409c, { 0x93, 0x4e, 0x91, 0x35, 0x2d, 0x18, 0x5f, 0x65 } };
#endif
#ifndef GUID_BATTERY_PERCENTAGE_REMAINING
static const GUID GUID_BATTERY_PERCENTAGE_REMAINING = { 0xa7ad8041, 0x2c41, 0x4c14, { 0xb6, 0x91, 0x68, 0x26, 0x1a, 0xb4, 0x75, 0x4a } };
#endif
#ifndef GUID_POWER_SAVING_STATUS
static const GUID GUID_POWER_SAVING_STATUS = { 0xe5812c53, 0xbc8e, 0x4eff, { 0xba, 0x1a, 0x98, 0x2c, 0x7e, 0x71, 0x0b, 0x77 } };
#endif

class RuntimeCondition::WindowsBatteryMonitor {
public:
    explicit WindowsBatteryMonitor(RuntimeCondition *condition)
        : m_condition(condition)
    {
        auto status = SYSTEM_POWER_STATUS();
        if (GetSystemPowerStatus(&status)) {
            m_condition->setBatteryInfo(
                status.ACLineStatus == 0 ? std::make_optional(true) : (status.ACLineStatus == 1 ? std::make_optional(false) : std::nullopt),
                status.BatteryLifePercent != 255 ? std::make_optional(static_cast<int>(status.BatteryLifePercent)) : std::nullopt);
            m_condition->setBatterySaving(
                status.SystemStatusFlag == 1 ? std::make_optional(true) : (status.SystemStatusFlag == 0 ? std::make_optional(false) : std::nullopt));
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
            m_condition->setOnBattery(value == 1);
        } else if (setting->PowerSetting == GUID_BATTERY_PERCENTAGE_REMAINING) {
            const auto value = *reinterpret_cast<const DWORD *>(setting->Data);
            m_condition->setBatteryLevel(static_cast<int>(value));
        } else if (setting->PowerSetting == GUID_POWER_SAVING_STATUS) {
            const auto value = *reinterpret_cast<const DWORD *>(setting->Data);
            m_condition->setBatterySaving(value == 1);
        }
    }

    RuntimeCondition *m_condition;
    HWND m_hwnd = nullptr;
    HPOWERNOTIFY m_hPowerNotifyACDC = nullptr;
    HPOWERNOTIFY m_hPowerNotifyPercent = nullptr;
    HPOWERNOTIFY m_hPowerNotifySaver = nullptr;
};
#endif

#if defined(PLATFORM_LINUX) && !defined(PLATFORM_ANDROID)

class RuntimeCondition::LinuxBatteryMonitor : public QObject {
    Q_OBJECT
public:
    explicit LinuxBatteryMonitor(RuntimeCondition *condition)
        : QObject(condition)
        , m_condition(condition)
    {
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_DBUS_BASED_POWER_MONITORING
        if (setupDBus()) {
            queryInitialDBusState();
        } else {
            setupSysFsFallback();
        }
#else
        setupSysFsFallback();
#endif
    }

private:
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
                m_condition->setBatteryInfo(state == 2, static_cast<int>(percentage));
            } else {
                m_condition->setBatteryInfo(false, 100);
            }
        }
        if (auto powerProfiles = QDBusInterface(QStringLiteral("org.freedesktop.UPower.PowerProfiles"),
                QStringLiteral("/org/freedesktop/UPower/PowerProfiles"), QStringLiteral("org.freedesktop.UPower.PowerProfiles"), sysBus);
            powerProfiles.isValid()) {
            const auto activeProfile = powerProfiles.property("ActiveProfile").toString();
            m_condition->setBatterySaving(activeProfile == QLatin1String("power-saver"));
        } else {
            queryPortalPowerSaver();
        }
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
                m_condition->setBatterySaving(isSaving);
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
                m_condition->setBatteryInfo(false, 100);
                return;
            }
        }
        if (changedProperties.contains(QLatin1String("State")) || changedProperties.contains(QLatin1String("Percentage"))) {
            const auto state = changedProperties.contains(QLatin1String("State")) ? changedProperties.value(QLatin1String("State")).toUInt()
                : m_condition->isOnBattery().value_or(false)                      ? 2u
                                                                                  : 1u;
            const auto percentage = changedProperties.contains(QLatin1String("Percentage"))
                ? changedProperties.value(QLatin1String("Percentage")).toDouble()
                : m_condition->batteryLevel().value_or(100);
            m_condition->setBatteryInfo(state == 2, static_cast<int>(percentage));
        }
    }

    void handlePowerProfilesPropertiesChanged(
        const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
    {
        Q_UNUSED(interface)
        Q_UNUSED(invalidatedProperties)
        if (changedProperties.contains(QLatin1String("ActiveProfile"))) {
            const auto activeProfile = changedProperties.value(QLatin1String("ActiveProfile")).toString();
            m_condition->setBatterySaving(activeProfile == QLatin1String("power-saver"));
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
            m_condition->setBatterySaving(isSaving);
        }
    }
#endif

    void pollSysFs()
    {
        auto dir = QDir(QStringLiteral("/sys/class/power_supply"));
        if (!dir.exists()) {
            m_condition->setBatteryInfo(std::nullopt, std::nullopt);
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
            m_condition->setBatteryInfo(onBattery, minBatteryLevel);
        } else {
            m_condition->setBatteryInfo(false, 100);
        }
    }

private:
    void setupSysFsFallback()
    {
        pollSysFs();
        auto *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &LinuxBatteryMonitor::pollSysFs);
        timer->start(60000); // poll every minute
    }

    RuntimeCondition *m_condition;
};
#endif

} // namespace Data

#if defined(PLATFORM_LINUX) && !defined(PLATFORM_ANDROID)
#include "runtimecondition.moc"
#endif
