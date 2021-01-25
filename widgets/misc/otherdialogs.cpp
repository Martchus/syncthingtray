#include "./otherdialogs.h"

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingdir.h>

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

#include <QClipboard>
#include <QDialog>
#include <QFont>
#include <QGuiApplication>
#include <QIcon>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

using namespace std;
using namespace Data;

namespace QtGui {

QDialog *ownDeviceIdDialog(Data::SyncthingConnection &connection)
{
    auto *dlg = new QDialog(nullptr, Qt::Window);
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
    QObject::connect(dlg, &QObject::destroyed,
        bind(static_cast<bool (*)(const QMetaObject::Connection &)>(&QObject::disconnect),
            QObject::connect(&connection, &SyncthingConnection::qrCodeAvailable,
                [pixmapLabel, devId = connection.myId()](const QString &text, const QByteArray &data) {
                    if (text != devId) {
                        return;
                    }
                    QPixmap pixmap;
                    pixmap.loadFromData(data);
                    pixmapLabel->setPixmap(pixmap);
                })));
    dlg->setLayout(layout);
    return dlg;
}
} // namespace QtGui
