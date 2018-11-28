#ifndef SYNCTHINGINFOACTION_H
#define SYNCTHINGINFOACTION_H

#include <QWidget>
#include <QWidgetAction>

QT_FORWARD_DECLARE_CLASS(QLabel)

class SyncthingInfoAction;

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

class SyncthingInfoAction : public QWidgetAction {
    Q_OBJECT

public:
    explicit SyncthingInfoAction(QObject *parent = nullptr);

protected:
    QWidget *createWidget(QWidget *parent) override;
};

#endif // SYNCTHINGINFOACTION_H
