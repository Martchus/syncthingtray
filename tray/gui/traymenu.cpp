#include "./traymenu.h"
#include "./traywidget.h"
#include "./trayicon.h"

#include "../application/settings.h"

#include <QHBoxLayout>

namespace QtGui {

TrayMenu::TrayMenu(TrayIcon *trayIcon, QWidget *parent) :
    TrayMenu(parent)
{
    m_trayIcon = trayIcon;
}

TrayMenu::TrayMenu(QWidget *parent) :
    QMenu(parent),
    m_trayIcon(nullptr)
{
    auto *menuLayout = new QHBoxLayout;
    menuLayout->setMargin(0), menuLayout->setSpacing(0);
    menuLayout->addWidget(m_trayWidget = new TrayWidget(this));
    setLayout(menuLayout);
    setPlatformMenu(nullptr);
}

QSize TrayMenu::sizeHint() const
{
    return Settings::trayMenuSize();
}

}
