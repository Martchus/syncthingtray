#ifndef TRAY_MENU_H
#define TRAY_MENU_H

#include <QMenu>

namespace QtGui {

class TrayIcon;
class TrayWidget;

class TrayMenu : public QMenu {
    Q_OBJECT
    Q_PROPERTY(bool windowed READ isWindowed WRITE setWindowed)

public:
    explicit TrayMenu(TrayIcon *trayIcon = nullptr, QWidget *parent = nullptr);

    QSize sizeHint() const override;
    TrayWidget &widget();
    const TrayWidget &widget() const;
    TrayIcon *icon();
    bool isWindowed() const;

public Q_SLOTS:
    void showUsingPositioningSettings();
    void setWindowed(bool windowed);

protected:
    void mouseReleaseEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void paintEvent(QPaintEvent *) override;

private:
    TrayWidget *m_trayWidget;
    TrayIcon *m_trayIcon;
    bool m_windowed = false;
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

inline bool TrayMenu::isWindowed() const
{
    return m_windowed;
}

} // namespace QtGui

#endif // TRAY_MENU_H
