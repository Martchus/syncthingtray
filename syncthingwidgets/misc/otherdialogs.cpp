#include "./otherdialogs.h"

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingdir.h>

#include <syncthingmodel/syncthingfilemodel.h>

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

#include <QClipboard>
#include <QDialog>
#include <QFont>
#include <QGuiApplication>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QPixmap>
#include <QPushButton>
#include <QTreeView>
#include <QVBoxLayout>

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
        auto actionIndex = qsizetype();
        for (const auto &action : actions) {
            QObject::connect(menu.addAction(actionIndex < actionIcons.size() ? actionIcons.at(actionIndex).value<QIcon>() : QIcon(),
                                 actionIndex < actionNames.size() ? actionNames.at(actionIndex) : action),
                &QAction::triggered, model, [model, action, index]() { model->triggerAction(action, index); });
            ++actionIndex;
        }
        menu.exec(view->viewport()->mapToGlobal(pos));
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

} // namespace QtGui
