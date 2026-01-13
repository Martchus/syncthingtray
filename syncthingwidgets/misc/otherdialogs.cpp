#include "./otherdialogs.h"

#include "./diffhighlighter.h"
#include "./textviewdialog.h"

#include <qtutilities/models/checklistmodel.h>

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingdir.h>

#include <syncthingmodel/colors.h>
#include <syncthingmodel/syncthingerrormodel.h>
#include <syncthingmodel/syncthingfilemodel.h>

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

#include <QClipboard>
#include <QDialog>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QIcon>
#include <QItemSelectionModel>
#include <QLabel>
#include <QListView>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkReply>
#include <QPixmap>
#include <QPushButton>
#include <QSplitter>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTextEdit>
#include <QToolBar>
#include <QTreeView>
#include <QVBoxLayout>

#include <utility>

using namespace std;
using namespace Data;

namespace QtGui {

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

/// \cond
static QPushButton *initToolButton(const QString &text, QAction *action, QWidget *parent, bool single = false)
{
    auto btn = new QPushButton(parent);
    btn->setText(text);
    btn->setIcon(action->icon());
    btn->setFlat(true);
    if (single) {
        QObject::connect(btn, &QAbstractButton::clicked, action, &QAction::trigger);
    } else {
        auto *menu = new QMenu(btn);
        menu->addAction(action);
        btn->setMenu(menu);
    }
    return btn;
}
/// \endcond

QDialog *browseRemoteFilesDialog(Data::SyncthingConnection &connection, const Data::SyncthingDir &dir, QWidget *parent)
{
    auto dlg = new QDialog(parent);
    dlg->setWindowTitle(QCoreApplication::translate("QtGui::OtherDialogs", "Remote/global tree of folder \"%1\"").arg(dir.displayName())
        + QStringLiteral(" - " APP_NAME));
    dlg->setWindowIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    // setup model/view
    auto view = new QTreeView(dlg);
    auto model = new Data::SyncthingFileModel(connection, dir, view);
    view->setModel(model);

    // setup toolbar
    auto toolBar = new QToolBar(dlg);
    toolBar->setFloatable(false);
    toolBar->setMovable(false);
    auto updateToolBarActions = [toolBar, model, actions = QList<QAction *>()]() mutable {
        toolBar->clear();
        qDeleteAll(actions);
        actions = model->selectionActions();
        QPushButton *primaryBtn = nullptr, *ignoreBtn = nullptr, *includeBtn = nullptr, *otherBtn = nullptr;
        QStringList primaryTexts;
        QAction *firstPrimaryAction = nullptr;
        primaryTexts.reserve(4);
        for (auto *const action : std::as_const(actions)) {
            action->setParent(toolBar);
            const auto category = action->data().toString();
            if (category.startsWith(QStringLiteral("primary:"))) {
                primaryTexts << category.mid(8);
                if (!firstPrimaryAction) {
                    firstPrimaryAction = action;
                    continue;
                } else if (!primaryBtn) {
                    primaryBtn = initToolButton(QString(), firstPrimaryAction, toolBar);
                }
                primaryBtn->menu()->addAction(action);
            } else if (category == QStringLiteral("ignore")) {
                if (ignoreBtn) {
                    ignoreBtn->menu()->addAction(action);
                } else {
                    ignoreBtn = initToolButton(QCoreApplication::translate("QtGui::OtherDialogs", "Ignore"), action, toolBar);
                }
            } else if (category == QStringLiteral("include")) {
                if (includeBtn) {
                    includeBtn->menu()->addAction(action);
                } else {
                    includeBtn = initToolButton(QCoreApplication::translate("QtGui::OtherDialogs", "Include"), action, toolBar);
                }
            } else {
                if (otherBtn) {
                    otherBtn->menu()->addAction(action);
                } else {
                    otherBtn = initToolButton(QCoreApplication::translate("QtGui::OtherDialogs", "Other"), action, toolBar);
                }
            }
        }
        if (firstPrimaryAction) {
            if (primaryBtn) {
                primaryBtn->setText(primaryTexts.join(QChar('/')));
                toolBar->addWidget(primaryBtn);
            } else {
                toolBar->addWidget(initToolButton(firstPrimaryAction->text(), firstPrimaryAction, toolBar, true));
            }
        }
        if (ignoreBtn) {
            toolBar->addWidget(ignoreBtn);
        }
        if (includeBtn) {
            toolBar->addWidget(includeBtn);
        }
        if (otherBtn) {
            toolBar->addWidget(otherBtn);
        }
    };
    updateToolBarActions();
    QObject::connect(model, &Data::SyncthingFileModel::selectionActionsChanged, toolBar, std::move(updateToolBarActions));

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
            auto *const selectionMenu = menu.addMenu(QCoreApplication::translate("QtGui::OtherDialogs", "Manage ignore patterns (experimental)"));
            selectionMenu->setIcon(QIcon::fromTheme(QStringLiteral("document-edit")));
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
    QObject::connect(model, &Data::SyncthingFileModel::actionNeedsConfirmation, toolBar,
        [model, toolBar](QAction *action, const QString &message, const QString &details, const QSet<QString> &localDeletions) {
            auto *const rejectableAction = qobject_cast<RejectableAction *>(action);
            auto messageBox = TextViewDialog(QStringLiteral("Confirm action - " APP_NAME));
            auto deletionList = QString();
            if (!localDeletions.isEmpty()) {
                deletionList = QCoreApplication::translate(
                    "QtGui::OtherDialogs", "Deletion of the following local files (will affect other devices unless ignored below!):");
                auto requiredSize = deletionList.size() + localDeletions.size();
                for (const auto &path : localDeletions) {
                    requiredSize += path.size();
                }
                deletionList.reserve(requiredSize);
                for (const auto &path : localDeletions) {
                    deletionList += QChar('\n');
                    deletionList += path;
                }
            }
            auto *const browser = messageBox.browser();
            auto *const highlighter = new DiffHighlighter(browser->document());
            auto *const buttonLayout = new QHBoxLayout(&messageBox);
            auto *const editBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("document-edit")),
                QCoreApplication::translate("QtGui::OtherDialogs", "Edit patterns manually"), &messageBox);
            auto *const yesBtn = new QPushButton(
                QIcon::fromTheme(QStringLiteral("dialog-ok")), QCoreApplication::translate("QtGui::OtherDialogs", "Apply"), &messageBox);
            auto *const noBtn = new QPushButton(
                QIcon::fromTheme(QStringLiteral("dialog-cancel")), QCoreApplication::translate("QtGui::OtherDialogs", "No"), &messageBox);
            auto widgetIndex = 0;
            QObject::connect(yesBtn, &QAbstractButton::clicked, &messageBox, [&messageBox] { messageBox.accept(); });
            QObject::connect(noBtn, &QAbstractButton::clicked, &messageBox, [&messageBox] { messageBox.reject(); });
            QObject::connect(editBtn, &QAbstractButton::clicked, &messageBox, [&messageBox, model, editBtn, highlighter] {
                auto *const b = messageBox.browser();
                editBtn->hide();
                b->clear();
                highlighter->setEnabled(false);
                b->setText(model->computeNewIgnorePatternsAsString());
                b->setReadOnly(false);
                b->setUndoRedoEnabled(true);
            });
            buttonLayout->addWidget(editBtn);
            buttonLayout->addStretch();
            buttonLayout->addWidget(yesBtn);
            buttonLayout->addWidget(noBtn);
            browser->setText(details);
            messageBox.layout()->insertWidget(widgetIndex++, new QLabel(message, &messageBox));
            auto *deletionModel = localDeletions.isEmpty() ? nullptr : new QtUtilities::ChecklistModel(&messageBox);
            if (deletionModel) {
                auto *deletionView = new QListView(&messageBox);
                auto deletionItems = QList<QtUtilities::ChecklistItem>();
                deletionItems.reserve(localDeletions.size());
                for (const auto &path : localDeletions) {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
                    deletionItems.emplace_back(path, path, Qt::Checked);
#else
                    deletionItems.append(QtUtilities::ChecklistItem(path, path, Qt::Checked));
#endif
                }
                deletionModel->setItems(deletionItems);
                deletionView->setModel(deletionModel);
                messageBox.layout()->insertWidget(widgetIndex++,
                    new QLabel(QCoreApplication::translate("QtGui::OtherDialogs", "Deletion of the following local files:"), &messageBox));
                messageBox.layout()->insertWidget(widgetIndex++, deletionView);
                messageBox.layout()->insertWidget(
                    widgetIndex++, new QLabel(QCoreApplication::translate("QtGui::OtherDialogs", "Changes to ignore patterns:"), &messageBox));
            }
            messageBox.layout()->addLayout(buttonLayout);
            messageBox.setAttribute(Qt::WA_DeleteOnClose, false);
            // ensure action lives at least as long as message box (but don't remove it from the tool bar)
            if (action->parent() != toolBar) {
                action->setParent(&messageBox);
            }
            if (messageBox.exec() != QDialog::Accepted) {
                if (rejectableAction) {
                    rejectableAction->dismiss();
                }
                return;
            }
            if (!browser->isReadOnly()) {
                model->editIgnorePatternsManually(browser->toPlainText());
            }
            if (deletionModel) {
                auto editedLocalDeletions = QSet<QString>();
                editedLocalDeletions.reserve(deletionModel->items().size());
                for (const auto &item : deletionModel->items()) {
                    if (item.isChecked()) {
                        editedLocalDeletions.insert(item.id().toString());
                    }
                }
                model->editLocalDeletions(editedLocalDeletions);
            }
            action->trigger();
            if (rejectableAction) {
                rejectableAction->dismiss();
            }
        });

    // setup layout
    auto layout = new QVBoxLayout;
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(0);
    layout->setContentsMargins(QMargins());
    layout->addWidget(toolBar);
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

QDialog *errorNotificationsDialog(Data::SyncthingConnection &connection, QWidget *parent)
{
    auto dlg = new QDialog(parent);
    dlg->setWindowTitle(QCoreApplication::translate("QtGui::OtherDialogs", "Notifications/errors") + QStringLiteral(" - " APP_NAME));
    dlg->setWindowIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    // setup model/view
    auto *const splitter = new QSplitter(dlg);
    auto view = new QTreeView(dlg);
    auto model = new Data::SyncthingErrorModel(connection, view);
    splitter->setOrientation(Qt::Horizontal);
    splitter->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    splitter->addWidget(view);
    view->setFrameShape(QFrame::StyledPanel);
    view->setItemsExpandable(false);
    view->setRootIsDecorated(false);
    view->setModel(model);

    // setup context menu
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(view, &QTreeView::customContextMenuRequested, view, [view](const QPoint &pos) {
        const auto index = view->indexAt(pos);
        if (!index.isValid()) {
            return;
        }
        auto text = index.data().toString();
        if (text.isEmpty()) {
            return;
        }
        auto menu = QMenu(view);
        QObject::connect(menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy")), QCoreApplication::translate("QtGui::OtherDialogs", "Copy")),
            &QAction::triggered, &menu, [text = std::move(text)] {
                if (auto *const clipboard = QGuiApplication::clipboard()) {
                    clipboard->setText(text);
                }
            });
        menu.exec(view->viewport()->mapToGlobal(pos));
    });

    // add text edit showing current message
    auto *const rightWidget = new QWidget(dlg);
    auto *const rightLayout = new QVBoxLayout;
    auto *const rightLabel = new QLabel(QCoreApplication::translate("QtGui::OtherDialogs", "Selected notification:"), dlg);
    auto *const textEdit = new QTextEdit(dlg);
    auto font = rightLabel->font();
    font.setBold(true);
    rightLabel->setFont(font);
    textEdit->setReadOnly(true);
    textEdit->setContentsMargins(QMargins());
    rightLayout->setSpacing(7);
    rightLayout->setContentsMargins(7, 7, 0, 0);
    rightLayout->addWidget(rightLabel);
    rightLayout->addWidget(textEdit);
    rightWidget->setLayout(rightLayout);
    splitter->addWidget(rightWidget);
    const auto updateTextEdit = [textEdit](const QModelIndex &selected = QModelIndex()) {
        const auto valid = selected.isValid();
        textEdit->setEnabled(valid);
        textEdit->setPlainText(valid ? selected.data(SyncthingErrorModel::Message).toString() : QString());
    };
    QObject::connect(view->selectionModel(), &QItemSelectionModel::currentRowChanged, textEdit, updateTextEdit);
    QObject::connect(model, &QAbstractItemModel::modelReset, textEdit, updateTextEdit);

    // add a button for clearing errors
    auto *const bottomLayout = new QHBoxLayout;
    auto *const clearButton = new QPushButton(dlg);
    clearButton->setText(QCoreApplication::translate("QtGui::OtherDialogs", "Clear all notifications"));
    clearButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear")));
    clearButton->setVisible(connection.hasErrors());
    bottomLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
    bottomLayout->setContentsMargins(0, 7, 0, 0);
    bottomLayout->addWidget(clearButton);
    QObject::connect(
        &connection, &SyncthingConnection::newErrors, clearButton, [clearButton, &connection] { clearButton->setVisible(connection.hasErrors()); });
    QObject::connect(clearButton, &QPushButton::clicked, &connection, &SyncthingConnection::requestClearingErrors);

    // setup layout
    auto layout = new QVBoxLayout;
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(0);
    layout->setContentsMargins(7, 7, 7, 7);
    layout->addWidget(splitter);
    layout->addLayout(bottomLayout);
    dlg->setLayout(layout);

    return dlg;
}

} // namespace QtGui
