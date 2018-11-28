#ifndef SYNCTHINGINFOACTION_H
#define SYNCTHINGINFOACTION_H

#include <QWidget>
#include <QWidgetAction>

QT_FORWARD_DECLARE_CLASS(QLabel)

class SyncthingInfoAction;

/*!
 * \brief The SyncthingInfoWidget class displays a SyncthingInfoAction.
 */
class SyncthingInfoWidget : public QWidget {
    Q_OBJECT

public:
    explicit SyncthingInfoWidget(const SyncthingInfoAction *action, QWidget *parent = nullptr);

private Q_SLOTS:
    void updateFromSender();
    void updateFromAction(const SyncthingInfoAction *action);

private:
    QLabel *const m_textLabel;
    QLabel *const m_iconLabel;
};

/*!
 * \brief The SyncthingInfoAction class provides a display-only QAction used to show eg. the directory info.
 * \remarks In contrast to a regular QAction, this class has no highlighting on mouseover event.
 */
class SyncthingInfoAction : public QWidgetAction {
    Q_OBJECT

public:
    explicit SyncthingInfoAction(QObject *parent = nullptr);

protected:
    QWidget *createWidget(QWidget *parent) override;
};

#endif // SYNCTHINGINFOACTION_H
