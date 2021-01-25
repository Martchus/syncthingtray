#ifndef SYNCTHINGDIRACTIONS_H
#define SYNCTHINGDIRACTIONS_H

#include <syncthingconnector/syncthingdir.h>

#include "./syncthinginfoaction.h"

class SyncthingFileItemActionStaticData;

/*!
 * \brief The SyncthingDirActions class provides the read-only directory info actions.
 */
class SyncthingDirActions : public QObject {
    Q_OBJECT
    friend QList<QAction *> &operator<<(QList<QAction *> &, SyncthingDirActions &);

public:
    explicit SyncthingDirActions(const Data::SyncthingDir &dir, const SyncthingFileItemActionStaticData *data = nullptr, QObject *parent = nullptr);

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

#endif // SYNCTHINGDIRACTIONS_H
