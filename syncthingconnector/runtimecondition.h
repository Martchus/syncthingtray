#ifndef DATA_RUNTIMECONDITION_H
#define DATA_RUNTIMECONDITION_H

#include "./global.h"

#include <QObject>

#include <c++utilities/misc/flagenumclass.h>

#include <optional>

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
    };

private:
    Q_OBJECT
    Q_PROPERTY(bool supposedToRun READ isSupposedToRun NOTIFY supposedToRunChanged)
    Q_PROPERTY(std::optional<bool> networkConnectionMetered READ isNetworkConnectionMetered NOTIFY networkConnectionMeteredChanged)
    Q_PROPERTY(QString meteredStatus READ meteredStatus NOTIFY networkConnectionMeteredChanged)
    Q_PROPERTY(std::optional<bool> batterySaving READ isBatterySaving NOTIFY batterySavingChanged)
    Q_PROPERTY(QString batterySavingStatus READ batterySavingStatus NOTIFY batterySavingChanged)
    Q_PROPERTY(Conditions enabledConditions READ enabledConditions WRITE setEnabledConditions NOTIFY enabledConditionsChanged)

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
     * \brief Returns a short translated status message about the metered state of the connection.
     */
    QString meteredStatus() const;

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
     * \brief Emitted when the enabled conditions have changed.
     */
    void enabledConditionsChanged(Data::RuntimeCondition::Conditions enabledConditions);

private:
    void updateSupposedToRun();

    Conditions m_enabledConditions;
    std::optional<bool> m_metered;
    std::optional<bool> m_batterySaving;
    mutable std::optional<bool> m_supposedToRun;
};

} // namespace Data

CPP_UTILITIES_MARK_FLAG_ENUM_CLASS(Data, Data::RuntimeCondition::Conditions)

namespace Data {

inline RuntimeCondition::Conditions RuntimeCondition::enabledConditions() const
{
    return m_enabledConditions;
}

inline void RuntimeCondition::modEnabledConditions(Conditions conditionsToModify, bool enable)
{
    auto conds = m_enabledConditions;
    setEnabledConditions(CppUtilities::modFlagEnum(conds, conditionsToModify, enable));
}

} // namespace Data

#endif // DATA_RUNTIMECONDITION_H
