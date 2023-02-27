#ifndef DATA_SYNCTHINGSTATUSCOMPUTIONMODELL_H
#define DATA_SYNCTHINGSTATUSCOMPUTIONMODELL_H

#include "./global.h"

#include <qtutilities/models/checklistmodel.h>

#define SYNCTHING_MODEL_ENUM_CLASS enum class
namespace Data {
SYNCTHING_MODEL_ENUM_CLASS SyncthingStatusComputionFlags : quint64;
}
#undef SYNCTHING_MODEL_ENUM_CLASS

namespace Data {

class LIB_SYNCTHING_MODEL_EXPORT SyncthingStatusComputionModel : public QtUtilities::ChecklistModel {
    Q_OBJECT

public:
    explicit SyncthingStatusComputionModel(QObject *parent = nullptr);
    QString labelForId(const QVariant &id) const override;
    SyncthingStatusComputionFlags statusComputionFlags() const;
    void setStatusComputionFlags(SyncthingStatusComputionFlags flags);
};

} // namespace Data

#endif // DATA_SYNCTHINGSTATUSCOMPUTIONMODELL_H
