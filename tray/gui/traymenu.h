#ifndef TRAY_MENU_H
#define TRAY_MENU_H

#include <QMenu>

namespace QtGui {

class TrayIcon;
class TrayWidget;

class TrayMenu : public QMenu {
    Q_OBJECT

public:
    TrayMenu(TrayIcon *trayIcon, const QString &connectionConfig = QString(), QWidget *parent = nullptr);
    TrayMenu(const QString &connectionConfig = QString(), QWidget *parent = nullptr);

    QSize sizeHint() const;
    TrayWidget *widget();
    TrayIcon *icon();

public slots:
    void showAtCursor();

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
