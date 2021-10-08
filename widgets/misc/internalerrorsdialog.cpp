#include "./internalerrorsdialog.h"

#include <c++utilities/io/ansiescapecodes.h>

#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QStringBuilder>
#include <QTextBrowser>
#include <QVBoxLayout>

#include <algorithm>
#include <iostream>
#include <limits>

using namespace std;
using namespace CppUtilities;
using namespace Data;

namespace QtGui {

InternalErrorsDialog *InternalErrorsDialog::s_instance = nullptr;
std::vector<InternalError> InternalErrorsDialog::s_internalErrors;

InternalErrorsDialog::InternalErrorsDialog()
    : TextViewDialog(tr("Internal errors"))
    , m_request(tr("Request URL:"))
    , m_response(tr("Response:"))
    , m_statusLabel(new QLabel(this))
{
    if (!s_instance) {
        s_instance = this;
    }

    // add layout to show status and additional buttons
    auto *const buttonLayout = new QHBoxLayout;
    buttonLayout->setContentsMargins(0, 0, 0, 0);

    // add label for overall status
    QFont boldFont(m_statusLabel->font());
    boldFont.setBold(true);
    m_statusLabel->setFont(boldFont);
    buttonLayout->addWidget(m_statusLabel);
    updateStatusLabel();

    // add errors to text view
    for (const InternalError &error : s_internalErrors) {
        internalAddError(error);
    }

    // add a button for clearing errors
    if (!s_internalErrors.empty()) {
        auto *const clearButton = new QPushButton(this);
        clearButton->setText(tr("Clear errors"));
        clearButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear")));
        buttonLayout->setContentsMargins(0, 0, 0, 0);
        buttonLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
        buttonLayout->addWidget(clearButton);
        connect(clearButton, &QPushButton::clicked, &InternalErrorsDialog::clearErrors);
        connect(clearButton, &QPushButton::clicked, this, &InternalErrorsDialog::errorsCleared);
    }

    layout()->addItem(buttonLayout);
}

InternalErrorsDialog::~InternalErrorsDialog()
{
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

void InternalErrorsDialog::addError(InternalError &&newError)
{
    s_internalErrors.emplace_back(newError);
    if (s_instance) {
        s_instance->internalAddError(s_internalErrors.back());
        s_instance->updateStatusLabel();
    }
}

void InternalErrorsDialog::addError(const QString &message, const QUrl &url, const QByteArray &response)
{
    s_internalErrors.emplace_back(message, url, response);
    if (s_instance) {
        s_instance->internalAddError(s_internalErrors.back());
        s_instance->updateStatusLabel();
    }
}

void InternalErrorsDialog::internalAddError(const InternalError &error)
{
    const QString url = error.url.toString();
    auto *const b = browser();
    b->append(QChar('[') % QString::fromUtf8(error.when.toString(DateTimeOutputFormat::Iso, true).data()) % QChar(']') % QChar(' ') % error.message);
    if (!url.isEmpty()) {
        b->append(m_request % QChar(' ') % url);
    }
    if (!error.response.isEmpty()) {
        b->append(m_response % QChar('\n') % QString::fromLocal8Bit(error.response));
    }

    // also log errors to console
    using namespace EscapeCodes;
    cerr << Phrases::Error << error.message.toLocal8Bit().data() << Phrases::End;
    if (!error.url.isEmpty()) {
        cerr << "request URL: " << url.toLocal8Bit().data() << '\n';
    }
    if (!error.response.isEmpty()) {
        cerr << "response: " << error.response.data() << '\n';
    }
}

void InternalErrorsDialog::updateStatusLabel()
{
    m_statusLabel->setText(tr("%1 error(s) occurred", nullptr, static_cast<int>(min<size_t>(s_internalErrors.size(), numeric_limits<int>::max())))
                               .arg(s_internalErrors.size()));
}

void InternalErrorsDialog::clearErrors()
{
    s_internalErrors.clear();
    if (s_instance) {
        s_instance->updateStatusLabel();
        s_instance->browser()->clear();
    }
}
} // namespace QtGui
