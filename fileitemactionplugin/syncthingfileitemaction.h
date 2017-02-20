#ifndef SYNCTHINGFILEITEMACTION_H
#define SYNCTHINGFILEITEMACTION_H

#include "../connector/syncthingconnection.h"

#include <KAbstractFileItemActionPlugin>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QWidget)

class KFileItemListProperties;

class SyncthingFileItemAction : public KAbstractFileItemActionPlugin
{
    Q_OBJECT

public:
    SyncthingFileItemAction(QObject* parent, const QVariantList &args);
    QList<QAction *> actions(const KFileItemListProperties &fileItemInfo, QWidget *parentWidget) override;

private Q_SLOTS:
    static void logConnectionStatus();
    static void logConnectionError(const QString &errorMessage);
    static void rescanDir(const QString &dirId, const QString &relpath = QString());
    static void showAboutDialog();

private:
    static Data::SyncthingConnection s_connection;
};

#endif // SYNCTHINGFILEITEMACTION_H
