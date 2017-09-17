#ifndef SYNCTHINGWIDGETS_ERRORVIEWDIALOG_H
#define SYNCTHINGWIDGETS_ERRORVIEWDIALOG_H

#include "./internalerror.h"
#include "./textviewdialog.h"

#include <vector>

QT_FORWARD_DECLARE_CLASS(QLabel)

namespace QtGui {

class SYNCTHINGWIDGETS_EXPORT ErrorViewDialog : public TextViewDialog {
    Q_OBJECT
public:
    ~ErrorViewDialog();
    static ErrorViewDialog *instance();
    static void addError(InternalError &&newError);

Q_SIGNALS:
    void errorsCleared();

public Q_SLOTS:
    static void showInstance();
    static void clearErrors();

private Q_SLOTS:
    void internalAddError(const InternalError &error);
    void updateStatusLabel();

private:
    ErrorViewDialog();

    const QString m_request;
    const QString m_response;
    QLabel *const m_statusLabel;
    static ErrorViewDialog *s_instance;
    static std::vector<InternalError> s_internalErrors;
};

inline ErrorViewDialog *ErrorViewDialog::instance()
{
    return s_instance ? s_instance : (s_instance = new ErrorViewDialog);
}

inline void ErrorViewDialog::showInstance()
{
    instance()->show();
}
} // namespace QtGui

#endif // SYNCTHINGWIDGETS_ERRORVIEWDIALOG_H
