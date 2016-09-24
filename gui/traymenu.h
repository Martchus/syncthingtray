#ifndef TRAY_MENU_H
#define TRAY_MENU_H

#include <QMenu>

namespace QtGui {

class TrayIcon;
class TrayWidget;

class TrayMenu : public QMenu
{
    Q_OBJECT

public:
    TrayMenu(TrayIcon *trayIcon, QWidget *parent = nullptr);
    TrayMenu(QWidget *parent = nullptr);
    ~TrayMenu();

    QSize sizeHint() const;
    TrayWidget *widget();
    TrayIcon *icon();

private:
    TrayWidget *m_trayWidget;
    TrayIcon *m_trayIcon;
};

inline TrayWidget *TrayMenu::widget()
{
    return m_trayWidget;
}

inline TrayIcon *TrayMenu::icon()
{
    return m_trayIcon;
}

}

#endif // TRAY_MENU_H
