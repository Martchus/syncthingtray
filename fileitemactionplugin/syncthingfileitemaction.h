#ifndef SYNCTHINGFILEITEMACTION_H
#define SYNCTHINGFILEITEMACTION_H

#include "../connector/syncthingconnection.h"

#include <KAbstractFileItemActionPlugin>
#include <KFileItemListProperties>

#include <QWidgetAction>

QT_FORWARD_DECLARE_CLASS(QWidget)

class KFileItemListProperties;

class SyncthingMenuAction : public QAction {
    Q_OBJECT

public:
    SyncthingMenuAction(const KFileItemListProperties &properties = KFileItemListProperties(), const QList<QAction *> &actions = QList<QAction *>(),
        QWidget *parentWidget = nullptr);

public Q_SLOTS:
    void updateStatus(Data::SyncthingStatus status);

private:
    KFileItemListProperties m_properties;
};

class SyncthingInfoAction : public QWidgetAction {
    Q_OBJECT

public:
    SyncthingInfoAction(QObject *parent = nullptr);

protected:
    QWidget *createWidget(QWidget *parent) override;
};

class SyncthingDirActions : public QObject {
    Q_OBJECT
    friend QList<QAction *> &operator<<(QList<QAction *> &, SyncthingDirActions &);

public:
    SyncthingDirActions(const Data::SyncthingDir &dir, QObject *parent = nullptr);

public Q_SLOTS:
    void updateStatus(const std::vector<Data::SyncthingDir> &dirs);
    bool updateStatus(const Data::SyncthingDir &dir);

private:
    QString m_dirId;
    QAction m_infoAction;
    SyncthingInfoAction m_statusAction;
    SyncthingInfoAction m_globalStatusAction;
    SyncthingInfoAction m_localStatusAction;
    SyncthingInfoAction m_lastScanAction;
    SyncthingInfoAction m_rescanIntervalAction;
    SyncthingInfoAction m_errorsAction;
};

QList<QAction *> &operator<<(QList<QAction *> &actions, SyncthingDirActions &dirActions);

class SyncthingFileItemAction : public KAbstractFileItemActionPlugin {
    Q_OBJECT

public:
    SyncthingFileItemAction(QObject *parent, const QVariantList &args);
    QList<QAction *> actions(const KFileItemListProperties &fileItemInfo, QWidget *parentWidget) override;
    static Data::SyncthingConnection &connection();
    static QList<QAction *> createActions(const KFileItemListProperties &fileItemInfo, QWidget *parentWidget);

private Q_SLOTS:
    static void logConnectionStatus();
    static void logConnectionError(const QString &errorMessage, Data::SyncthingErrorCategory errorCategory);
    static void rescanDir(const QString &dirId, const QString &relpath = QString());
    static void showAboutDialog();

private:
    static Data::SyncthingConnection s_connection;
};

#endif // SYNCTHINGFILEITEMACTION_H
