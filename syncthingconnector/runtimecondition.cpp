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
{
#ifdef SYNCTHINGCONNECTION_SUPPORT_METERED
    if (const auto [networkInformation, isInitiallyMetered] = loadNetworkInformationBackendForMetered(true); networkInformation) {
        connect(networkInformation, &QNetworkInformation::isMeteredChanged, this, [this](bool isMetered) { setNetworkConnectionMetered(isMetered); });
        m_metered = isInitiallyMetered;
    }
#endif

    // force initial determination
    isSupposedToRun();
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
    if (conditions && Conditions::Metered && m_metered.value_or(false)) {
        return false;
    } else if (conditions && Conditions::ForceSuspend) {
        return false;
    }
    return true;
}

std::optional<bool> RuntimeCondition::isNetworkConnectionMetered() const
{
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
    if (m_metered.has_value()) {
        return m_metered.value() ? tr("Network connection is metered") : tr("Network connection is not metered");
    } else {
        return tr("State of network connection cannot be determined");
    }
}

void RuntimeCondition::setEnabledConditions(Conditions enabledConditions)
{
    if (enabledConditions != m_enabledConditions) {
        emit enabledConditionsChanged(m_enabledConditions = enabledConditions);
        updateSupposedToRun();
    }
}

void RuntimeCondition::updateSupposedToRun()
{
    const bool oldSupposedToRun = m_supposedToRun.value_or(true);
    m_supposedToRun.reset();
    const bool newSupposedToRun = isSupposedToRun();
    if (newSupposedToRun != oldSupposedToRun) {
        emit supposedToRunChanged(newSupposedToRun);
    }
}

} // namespace Data
