#ifndef TRAY_MENU_H
#define TRAY_MENU_H

#include <QMenu>

namespace QtGui {

class TrayIcon;
class TrayWidget;

class TrayMenu : public QMenu {
    Q_OBJECT

public:
    TrayMenu(TrayIcon *trayIcon = nullptr, QWidget *parent = nullptr);

    QSize sizeHint() const override;
    TrayWidget &widget();
    const TrayWidget &widget() const;
    TrayIcon *icon();

public Q_SLOTS:
    void showUsingPositioningSettings();

private:
    TrayWidget *m_trayWidget;
    TrayIcon *m_trayIcon;
};

inline TrayWidget &TrayMenu::widget()
{
    return *m_trayWidget;
}

inline const TrayWidget &TrayMenu::widget() const
{
    return *m_trayWidget;
}

inline TrayIcon *TrayMenu::icon()
{
    return m_trayIcon;
}
} // namespace QtGui

#endif // TRAY_MENU_H
