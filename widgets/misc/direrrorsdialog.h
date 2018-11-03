#ifndef SYNCTHINGWIDGETS_DIRECTORY_ERRORS_DIALOG_H
#define SYNCTHINGWIDGETS_DIRECTORY_ERRORS_DIALOG_H

#include "./textviewdialog.h"

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace Data {
class SyncthingConnection;
struct SyncthingDir;
} // namespace Data

namespace QtGui {

class SYNCTHINGWIDGETS_EXPORT DirectoryErrorsDialog : public TextViewDialog {
    Q_OBJECT
public:
    explicit DirectoryErrorsDialog(const Data::SyncthingConnection &connection, const Data::SyncthingDir &dir, QWidget *parent = nullptr);
    ~DirectoryErrorsDialog() override;

private Q_SLOTS:
    void handleDirStatusChanged(const Data::SyncthingDir &dir);
    void handleNewDirs();
    void updateErrors(const Data::SyncthingDir &dir);
    void removeNonEmptyDirs();

private:
    const Data::SyncthingConnection &m_connection;
    QString m_dirId;
    QStringList m_nonEmptyDirs;
    QLabel *m_statusLabel;
    QPushButton *m_rmNonEmptyDirsButton;
};

} // namespace QtGui

#endif // SYNCTHINGWIDGETS_DIRECTORY_ERRORS_DIALOG_H
