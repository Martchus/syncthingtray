#ifndef DATA_RUNTIMECONDITION_H
#define DATA_RUNTIMECONDITION_H

#include "./global.h"

#include <QObject>

#include <c++utilities/misc/flagenumclass.h>

#include <memory>
#include <optional>

#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
#define SYNCTHINGCONNECTION_SUPPORT_METERED
#endif

namespace Data {

class LIB_SYNCTHING_CONNECTOR_EXPORT RuntimeCondition : public QObject {
public:
    /*!
     * \brief The Conditions enum specifies condition flags that can be enabled.
     */
    enum class Conditions : qulonglong {
        None = 0, /*!< No conditions enabled; always allowed to run. */
        Metered = (1 << 0), /*!< Checked to pause/suspend on metered network connections. */
        ForceSuspend = (1 << 1), /*!< Checked to force suspension of Syncthing. */
        BatterySaving = (1 << 2), /*!< Checked to pause/suspend when battery saving is enabled. */
        OnBattery = (1 << 3), /*!< Checked to pause/suspend when system is only on battery and under the configured percentage. */
    };

private:
    Q_OBJECT
    Q_PROPERTY(bool supposedToRun READ isSupposedToRun NOTIFY supposedToRunChanged)
    Q_PROPERTY(std::optional<bool> networkConnectionMetered READ isNetworkConnectionMetered NOTIFY networkConnectionMeteredChanged)
    Q_PROPERTY(QString meteredStatus READ meteredStatus NOTIFY networkConnectionMeteredChanged)
    Q_PROPERTY(std::optional<bool> batterySaving READ isBatterySaving NOTIFY batterySavingChanged)
    Q_PROPERTY(QString batterySavingStatus READ batterySavingStatus NOTIFY batterySavingChanged)
    Q_PROPERTY(std::optional<bool> onBattery READ isOnBattery NOTIFY onBatteryChanged)
    Q_PROPERTY(QString onBatteryStatus READ onBatteryStatus NOTIFY onBatteryChanged)
    Q_PROPERTY(std::optional<int> batteryLevel READ batteryLevel NOTIFY batteryLevelChanged)
    Q_PROPERTY(int batteryPercentage READ batteryPercentage WRITE setBatteryPercentage NOTIFY batteryPercentageChanged)
    Q_PROPERTY(Conditions enabledConditions READ enabledConditions WRITE setEnabledConditions NOTIFY enabledConditionsChanged)
    Q_PROPERTY(bool initializinbg READ isInitializing WRITE setInitializing)

public:
    /*!
     * \brief Constructs a new RuntimeCondition with the specified \a conditions.
     */
    explicit RuntimeCondition(Conditions conditions = Conditions::None, QObject *parent = nullptr);

    /*!
     * \brief Destructs the RuntimeCondition.
     */
    ~RuntimeCondition() override;

    /*!
     * \brief Returns whether Syncthing is supposed to run based on the currently enabled conditions.
     * \note This is a cached/lazy-initialized property.
     */
    bool isSupposedToRun() const;

    /*!
     * \brief Returns whether Syncthing is supposed to run based on the specified \a conditions.
     */
    bool isSupposedToRun(Conditions conditions) const;

    /*!
     * \brief Returns whether the current network connection is metered, or std::nullopt if unknown.
     */
    std::optional<bool> isNetworkConnectionMetered() const;

    /*!
     * \brief Sets whether the current network connection is metered.
     */
    void setNetworkConnectionMetered(std::optional<bool> metered);

    /*!
     * \brief Sets whether the current network connection is metered.
     */
    void setNetworkConnectionMetered(bool metered);

    /*!
     * \brief Returns a short translated status message about the metered state of the connection.
     */
    QString meteredStatus() const;

#ifdef Q_OS_ANDROID
    static void registerServiceJniMethods();
    static void unregisterServiceJniMethods();
#endif

    /*!
     * \brief Returns whether battery saving mode is enabled, or std::nullopt if unknown.
     */
    std::optional<bool> isBatterySaving() const;

    /*!
     * \brief Sets whether battery saving mode is enabled.
     */
    void setBatterySaving(std::optional<bool> batterySaving);

    /*!
     * \brief Returns a short translated status message about the battery saving mode.
     */
    QString batterySavingStatus() const;

    /*!
     * \brief Returns whether the system is running on battery, or std::nullopt if unknown.
     */
    std::optional<bool> isOnBattery() const;

    /*!
     * \brief Sets whether the system is running on battery.
     */
    void setOnBattery(std::optional<bool> onBattery);

    /*!
     * \brief Returns a short translated status message about the battery status.
     */
    QString onBatteryStatus() const;

    /*!
     * \brief Returns the current battery level (0-100), or std::nullopt if unknown.
     */
    std::optional<int> batteryLevel() const;

    /*!
     * \brief Sets the current battery level (0-100).
     */
    void setBatteryLevel(std::optional<int> batteryLevel);

    /*!
     * \brief Sets whether the system is running on battery and the current battery level (0-100).
     */
    void setBatteryInfo(std::optional<bool> onBattery, std::optional<int> batteryLevel);

    /*!
     * \brief Returns the configured battery percentage threshold under which to pause.
     */
    int batteryPercentage() const;

    /*!
     * \brief Sets the configured battery percentage threshold.
     */
    void setBatteryPercentage(int percentage);

    /*!
     * \brief Returns a short translated status message about why Syncthing is temporarily stopped, or empty string.
     */
    QString stopStatusMessage() const;

    /*!
     * \brief Returns a short translated status message about why Syncthing is temporarily stopped for the specified \a conditions, or empty string.
     */
    QString stopStatusMessage(Conditions conditions) const;

    /*!
     * \brief Returns the currently enabled conditions.
     */
    Conditions enabledConditions() const;

    /*!
     * \brief Sets the currently enabled conditions.
     */
    void setEnabledConditions(Conditions enabledConditions);

    /*!
     * \brief Modifies the specified \a conditionsToModify by enabling (if \a enable is true) or disabling them.
     */
    void modEnabledConditions(Conditions conditionsToModify, bool enable = true);

    /*!
     * \brief Returns whether the runtime condition is being initialized.
     * \remarks The supposedToRunChanged() signal is not emitted while the runtime condition is initialized.
     */
    bool isInitializing() const;

    /*!
     * \brief Set whether the runtime condition is being initialized.
     */
    void setInitializing(bool initializing);

Q_SIGNALS:
    /*!
     * \brief Emitted when whether Syncthing is supposed to run has changed.
     */
    void supposedToRunChanged(bool supposedToRun);

    /*!
     * \brief Emitted when the metered state of the connection has changed.
     */
    void networkConnectionMeteredChanged(std::optional<bool> isMetered);

    /*!
     * \brief Emitted when the battery saving mode has changed.
     */
    void batterySavingChanged(std::optional<bool> isBatterySaving);

    /*!
     * \brief Emitted when the battery status (on battery or not) has changed.
     */
    void onBatteryChanged(std::optional<bool> isOnBattery);

    /*!
     * \brief Emitted when the current battery level has changed.
     */
    void batteryLevelChanged(std::optional<int> batteryLevel);

    /*!
     * \brief Emitted when the configured battery percentage threshold has changed.
     */
    void batteryPercentageChanged(int percentage);

    /*!
     * \brief Emitted when the enabled conditions have changed.
     */
    void enabledConditionsChanged(Data::RuntimeCondition::Conditions enabledConditions);

private:
    void updateSupposedToRun();
    void initializeBatteryMonitoring() const;
    static void registerInstance(RuntimeCondition *instance);
    static void unregisterInstance(RuntimeCondition *instance);
    static std::optional<bool> queryBatterySaving();
    static std::optional<std::pair<bool, int>> queryBatteryInfo();

    Conditions m_enabledConditions;
    mutable Conditions m_initializedConditions;
    mutable std::optional<bool> m_metered;
    mutable std::optional<bool> m_batterySaving;
    mutable std::optional<bool> m_onBattery;
    mutable std::optional<int> m_batteryLevel;
    int m_batteryPercentage;
    mutable std::optional<bool> m_supposedToRun;
    bool m_initializing;
};

} // namespace Data

CPP_UTILITIES_MARK_FLAG_ENUM_CLASS(Data, Data::RuntimeCondition::Conditions)

namespace Data {

inline RuntimeCondition::Conditions RuntimeCondition::enabledConditions() const
{
    return m_enabledConditions;
}

inline void RuntimeCondition::setNetworkConnectionMetered(bool metered)
{
    setNetworkConnectionMetered(std::make_optional(metered));
}

inline void RuntimeCondition::modEnabledConditions(Conditions conditionsToModify, bool enable)
{
    auto conds = m_enabledConditions;
    setEnabledConditions(CppUtilities::modFlagEnum(conds, conditionsToModify, enable));
}

inline bool RuntimeCondition::isInitializing() const
{
    return m_initializing;
}

} // namespace Data

#endif // DATA_RUNTIMECONDITION_H
