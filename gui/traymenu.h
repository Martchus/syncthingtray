#ifndef TRAY_MENU_H
#define TRAY_MENU_H

#include <QMenu>

namespace QtGui {

class TrayWidget;

class TrayMenu : public QMenu
{
    Q_OBJECT

public:
    TrayMenu(QWidget *parent = nullptr);

    QSize sizeHint() const;
    TrayWidget *widget();

private:
    TrayWidget *m_trayWidget;
};

inline TrayWidget *TrayMenu::widget()
{
    return m_trayWidget;
}

}

#endif // TRAY_MENU_H
