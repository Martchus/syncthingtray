#ifndef SYNCTHINGFILEITEMACTION_H
#define SYNCTHINGFILEITEMACTION_H

#include "./syncthingfileitemactionstaticdata.h"

#include <KAbstractFileItemActionPlugin>

class KFileItemListProperties;

/*!
 * \brief The SyncthingFileItemAction class implements the plugin interface.
 * \remarks This is the only class directly used by Dolphin.
 */
class SyncthingFileItemAction : public KAbstractFileItemActionPlugin {
    Q_OBJECT

public:
    SyncthingFileItemAction(QObject *parent, const QVariantList &args);
    QList<QAction *> actions(const KFileItemListProperties &fileItemInfo, QWidget *parentWidget) override;
    static QList<QAction *> createActions(const KFileItemListProperties &fileItemInfo, QWidget *parentWidget);
    static SyncthingFileItemActionStaticData &staticData();

private:
    static SyncthingFileItemActionStaticData s_data;
};

inline SyncthingFileItemActionStaticData &SyncthingFileItemAction::staticData()
{
    return s_data;
}

#endif // SYNCTHINGFILEITEMACTION_H
