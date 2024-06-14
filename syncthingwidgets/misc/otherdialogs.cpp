#include "./otherdialogs.h"
#include "./textviewdialog.h"

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingdir.h>

#include <syncthingmodel/colors.h>
#include <syncthingmodel/syncthingfilemodel.h>

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

#include <QClipboard>
#include <QDialog>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkReply>
#include <QPixmap>
#include <QPushButton>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTreeView>
#include <QVBoxLayout>

using namespace std;
using namespace Data;

namespace QtGui {

DiffHighlighter::DiffHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
    , m_enabled(true)
{
    auto font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_baseFormat.setFont(font);

    font.setBold(true);
    m_addedFormat.setFont(font);
    m_addedFormat.setForeground(Colors::green(true));
    m_deletedFormat.setFont(font);
    m_deletedFormat.setForeground(Colors::red(true));
}

void DiffHighlighter::highlightBlock(const QString &text)
{
    if (text.startsWith(QChar('-'))) {
        setFormat(0, static_cast<int>(text.size()), QColor(Qt::red));
    } else if (text.startsWith(QChar('+'))) {
        setFormat(0, static_cast<int>(text.size()), QColor(Qt::green));
    }
}

static void setupOwnDeviceIdDialog(Data::SyncthingConnection &connection, int size, QWidget *dlg)
{
    dlg->setWindowTitle(QCoreApplication::translate("QtGui::OtherDialogs", "Own device ID") + QStringLiteral(" - " APP_NAME));
    dlg->setWindowIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setBackgroundRole(QPalette::Window);
    auto *layout = new QVBoxLayout(dlg);
    layout->setAlignment(Qt::AlignCenter);
    auto *pixmapLabel = new QLabel(dlg);
    pixmapLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(pixmapLabel);
    auto *textLabel = new QLabel(dlg);
    textLabel->setText(connection.myId().isEmpty() ? QCoreApplication::translate("QtGui::OtherDialogs", "device ID is unknown") : connection.myId());
    QFont defaultFont = textLabel->font();
    defaultFont.setBold(true);
    defaultFont.setPointSize(defaultFont.pointSize() + 2);
    textLabel->setFont(defaultFont);
    textLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(textLabel);
    auto *copyPushButton = new QPushButton(dlg);
    copyPushButton->setText(QCoreApplication::translate("QtGui::OtherDialogs", "Copy to clipboard"));
    QObject::connect(
        copyPushButton, &QPushButton::clicked, bind(&QClipboard::setText, QGuiApplication::clipboard(), connection.myId(), QClipboard::Clipboard));
    layout->addWidget(copyPushButton);
    connection.requestQrCode(connection.myId());
    QObject::connect(&connection, &SyncthingConnection::qrCodeAvailable, pixmapLabel,
        [pixmapLabel, devId = connection.myId(), size](const QString &text, const QByteArray &data) {
            if (text != devId) {
                return;
            }
            auto pixmap = QPixmap();
            pixmap.loadFromData(data);
            if (size) {
                pixmap = pixmap.scaledToHeight(size, Qt::SmoothTransformation);
            }
            pixmapLabel->setPixmap(pixmap);
        });
    dlg->setLayout(layout);
}

QDialog *ownDeviceIdDialog(Data::SyncthingConnection &connection)
{
    auto *dlg = new QDialog(nullptr, Qt::Window);
    setupOwnDeviceIdDialog(connection, 0, dlg);
    return dlg;
}

QWidget *ownDeviceIdWidget(Data::SyncthingConnection &connection, int size, QWidget *parent)
{
    auto *widget = new QWidget(parent);
    setupOwnDeviceIdDialog(connection, size, widget);
    return widget;
}

QDialog *browseRemoteFilesDialog(Data::SyncthingConnection &connection, const Data::SyncthingDir &dir, QWidget *parent)
{
    auto dlg = new QDialog(parent);
    dlg->setWindowTitle(QCoreApplication::translate("QtGui::OtherDialogs", "Remote/global tree of folder \"%1\"").arg(dir.displayName())
        + QStringLiteral(" - " APP_NAME));
    dlg->setWindowIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    // setup model/view
    auto model = new Data::SyncthingFileModel(connection, dir, &connection);
    auto view = new QTreeView(dlg);
    view->setModel(model);

    // setup context menu
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(view, &QTreeView::customContextMenuRequested, view, [view, model](const QPoint &pos) {
        const auto index = view->indexAt(pos);
        if (!index.isValid()) {
            return;
        }
        const auto actions = model->data(index, SyncthingFileModel::Actions).toStringList();
        if (actions.isEmpty()) {
            return;
        }
        const auto actionNames = model->data(index, SyncthingFileModel::ActionNames).toStringList();
        const auto actionIcons = model->data(index, SyncthingFileModel::ActionIcons).toList();
        auto menu = QMenu(view);
        auto actionIndex = QStringList::size_type();
        for (const auto &action : actions) {
            QObject::connect(menu.addAction(actionIndex < actionIcons.size() ? actionIcons.at(actionIndex).value<QIcon>() : QIcon(),
                                 actionIndex < actionNames.size() ? actionNames.at(actionIndex) : action),
                &QAction::triggered, model, [model, action, index]() { model->triggerAction(action, index); });
            ++actionIndex;
        }
        if (const auto selectionActions = model->selectionActions(); !selectionActions.isEmpty()) {
            menu.addSeparator();
            auto *const selectionMenu = menu.addMenu(QCoreApplication::translate("QtGui::OtherDialogs", "Selection"));
            selectionMenu->addActions(selectionActions);
            for (auto *const selectionAction : selectionActions) {
                selectionAction->setParent(&menu);
            }
        }
        menu.exec(view->viewport()->mapToGlobal(pos));
    });

    // setup handlers for notifications/confirmation
    QObject::connect(model, &Data::SyncthingFileModel::notification, dlg, [dlg](const QString &type, const QString &message, const QString &details) {
        auto messageBox = QMessageBox(QMessageBox::Information, dlg->windowTitle(), message, QMessageBox::Ok);
        if (type == QLatin1String("error")) {
            messageBox.setIcon(QMessageBox::Critical);
        } else if (type == QLatin1String("warning")) {
            messageBox.setIcon(QMessageBox::Warning);
        }
        messageBox.setDetailedText(details);
        messageBox.exec();
    });
    QObject::connect(
        model, &Data::SyncthingFileModel::actionNeedsConfirmation, dlg, [model](QAction *action, const QString &message, const QString &details) {
            auto messageBox = TextViewDialog(QStringLiteral("Confirm action - " APP_NAME));
            auto *const browser = messageBox.browser();
            auto *const highlighter = new DiffHighlighter(browser->document());
            auto *const buttonLayout = new QHBoxLayout(&messageBox);
            auto *const editBtn = new QPushButton(
                QIcon::fromTheme(QStringLiteral("document-edit")), QCoreApplication::translate("QtGui::OtherDialogs", "Edit manually"), &messageBox);
            auto *const yesBtn = new QPushButton(
                QIcon::fromTheme(QStringLiteral("dialog-ok")), QCoreApplication::translate("QtGui::OtherDialogs", "Apply"), &messageBox);
            auto *const noBtn = new QPushButton(
                QIcon::fromTheme(QStringLiteral("dialog-cancel")), QCoreApplication::translate("QtGui::OtherDialogs", "No"), &messageBox);
            QObject::connect(yesBtn, &QAbstractButton::clicked, &messageBox, [&messageBox] { messageBox.accept(); });
            QObject::connect(noBtn, &QAbstractButton::clicked, &messageBox, [&messageBox] { messageBox.reject(); });
            QObject::connect(editBtn, &QAbstractButton::clicked, &messageBox, [&messageBox, model, editBtn, highlighter] {
                auto *const b = messageBox.browser();
                editBtn->hide();
                b->clear();
                highlighter->setEnabled(false);
                b->setText(model->computeNewIgnorePatterns().ignore.join(QChar('\n')));
                b->setReadOnly(false);
                b->setUndoRedoEnabled(true);
            });
            buttonLayout->addWidget(editBtn);
            buttonLayout->addStretch();
            buttonLayout->addWidget(yesBtn);
            buttonLayout->addWidget(noBtn);
            browser->setText(details);
            messageBox.layout()->insertWidget(0, new QLabel(message, &messageBox));
            messageBox.layout()->addLayout(buttonLayout);
            messageBox.setAttribute(Qt::WA_DeleteOnClose, false);
            action->setParent(&messageBox);
            if (messageBox.exec() != QDialog::Accepted) {
                return;
            }
            if (!browser->isReadOnly()) {
                model->editIgnorePatternsManually(browser->toPlainText());
            }
            action->trigger();
        });

    // setup layout
    auto layout = new QVBoxLayout;
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(0);
    layout->setContentsMargins(QMargins());
    layout->addWidget(view);
    dlg->setLayout(layout);

    return dlg;
}

TextViewDialog *ignorePatternsDialog(Data::SyncthingConnection &connection, const Data::SyncthingDir &dir, QWidget *parent)
{
    auto *const dlg
        = new TextViewDialog(QCoreApplication::translate("QtGui::OtherDialogs", "Ignore patterns of folder \"%1\"").arg(dir.displayName()), parent);
    dlg->browser()->setText(QStringLiteral("Loading…"));
    QObject::connect(dlg, &TextViewDialog::reload, dlg, [&connection, dirId = dir.id, dlg] {
        dlg->browser()->setReadOnly(true);
        auto res = connection.ignores(dirId, [dlg](Data::SyncthingIgnores &&ignores, QString &&errorMessage) {
            auto *const browser = dlg->browser();
            browser->setUndoRedoEnabled(false);
            browser->clear();
            browser->document()->clearUndoRedoStacks();
            if (!errorMessage.isEmpty()) {
                browser->setText(errorMessage);
                return;
            }
            for (const auto &ignore : ignores.ignore) {
                browser->append(ignore);
            }
            browser->setUndoRedoEnabled(true);
            browser->setReadOnly(false);
            dlg->setProperty("savedRevision", browser->document()->revision());
        });
        QObject::connect(dlg, &QObject::destroyed, res.reply, &QNetworkReply::deleteLater);
    });
    emit dlg->reload();
    QObject::connect(dlg, &TextViewDialog::save, dlg, [&connection, dirId = dir.id, dlg] {
        dlg->setProperty("isSaving", true);
        auto *const browser = dlg->browser();
        auto newIgnores = SyncthingIgnores{ .ignore = browser->toPlainText().split(QChar('\n')), .expanded = QStringList() };
        auto setRes = connection.setIgnores(dirId, newIgnores, [dlg](const QString &error) {
            if (error.isEmpty()) {
                QMessageBox::information(
                    nullptr, dlg->windowTitle(), QCoreApplication::translate("QtGui::OtherDialogs", "Ignore patterns have been changed."));
                if (dlg->property("isClosing").toBool()) {
                    dlg->setCloseHandler(std::function<bool(TextViewDialog *)>());
                    dlg->close();
                }
            } else {
                QMessageBox::critical(
                    nullptr, dlg->windowTitle(), QCoreApplication::translate("QtGui::OtherDialogs", "Unable to save ignore patterns: %1").arg(error));
            }
            dlg->setProperty("isSaving", false);
            dlg->setProperty("savedRevision", dlg->browser()->document()->revision());
        });
        QObject::connect(dlg, &QObject::destroyed, setRes.reply, &QNetworkReply::deleteLater);
    });
    dlg->setCloseHandler([](TextViewDialog *textViewDlg) {
        textViewDlg->setProperty("isClosing", true);
        if (textViewDlg->property("isSaving").toBool()) {
            return true;
        }
        if (textViewDlg->browser()->document()->revision() <= textViewDlg->property("savedRevision").toInt()) {
            return false;
        }
        const auto question = QMessageBox::question(
            textViewDlg, textViewDlg->windowTitle(), QCoreApplication::translate("QtGui::OtherDialogs", "Do you want to save the changes?"));
        if (question == QMessageBox::Yes) {
            emit textViewDlg->save();
        }
        return question != QMessageBox::No;
    });
    return dlg;
}

} // namespace QtGui
