#include "./textviewdialog.h"

#include "../../connector/syncthingconnection.h"
#include "../../connector/syncthingdir.h"

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

#include <QClipboard>
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

QWidget *ownDeviceIdDialog(Data::SyncthingConnection &connection)
{
    auto *dlg = new QWidget(nullptr, Qt::Window);
    dlg->setWindowTitle(QCoreApplication::translate("QtGui::OtherDialogs", "Own device ID") + QStringLiteral(" - " APP_NAME));
    dlg->setWindowIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setBackgroundRole(QPalette::Background);
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
    QObject::connect(dlg, &QWidget::destroyed, bind(static_cast<bool (*)(const QMetaObject::Connection &)>(&QObject::disconnect),
                                                   connection.requestQrCode(connection.myId(), [pixmapLabel](const QByteArray &data) {
                                                       QPixmap pixmap;
                                                       pixmap.loadFromData(data);
                                                       pixmapLabel->setPixmap(pixmap);
                                                   })));
    dlg->setLayout(layout);
    return dlg;
}
}
