#include "./runtimecondition.h"
#include "./utils.h"

#ifdef SYNCTHINGCONNECTION_SUPPORT_METERED
#include <QNetworkInformation>
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

} // namespace Data
