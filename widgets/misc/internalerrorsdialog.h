#ifndef SYNCTHINGWIDGETS_INTERNAL_ERRORS_DIALOG_H
#define SYNCTHINGWIDGETS_INTERNAL_ERRORS_DIALOG_H

#include "./internalerror.h"
#include "./textviewdialog.h"

#include <vector>

QT_FORWARD_DECLARE_CLASS(QLabel)

namespace QtGui {

class SYNCTHINGWIDGETS_EXPORT InternalErrorsDialog : public TextViewDialog {
    Q_OBJECT
public:
    ~InternalErrorsDialog() override;
    static InternalErrorsDialog *instance();
    static bool hasInstance();
    static void addError(InternalError &&newError);
    static void addError(const QString &message = QString(), const QUrl &url = QUrl(), const QByteArray &response = QByteArray());

Q_SIGNALS:
    void errorsCleared();

public Q_SLOTS:
    static void showInstance();
    static void clearErrors();

private Q_SLOTS:
    void internalAddError(const InternalError &error);
    void updateStatusLabel();

private:
    InternalErrorsDialog();

    const QString m_request;
    const QString m_response;
    QLabel *const m_statusLabel;
    static InternalErrorsDialog *s_instance;
    static std::vector<InternalError> s_internalErrors;
};

inline InternalErrorsDialog *InternalErrorsDialog::instance()
{
    return s_instance ? s_instance : (s_instance = new InternalErrorsDialog);
}

inline bool InternalErrorsDialog::hasInstance()
{
    return s_instance != nullptr;
}

inline void InternalErrorsDialog::showInstance()
{
    instance()->show();
}
} // namespace QtGui

#endif // SYNCTHINGWIDGETS_INTERNAL_ERRORS_DIALOG_H
