#ifndef TRAY_MENU_H
#define TRAY_MENU_H

#include <QMenu>

namespace QtGui {

class TrayIcon;
class TrayWidget;

class TrayMenu : public QMenu {
    Q_OBJECT
    Q_PROPERTY(bool pinned READ isPinned WRITE setPinned)

public:
    explicit TrayMenu(TrayIcon *trayIcon = nullptr, QWidget *parent = nullptr);

    QSize sizeHint() const override;
    TrayWidget &widget();
    const TrayWidget &widget() const;
    TrayIcon *icon();
    bool isPinned() const;

public Q_SLOTS:
    void showUsingPositioningSettings();
    void setPinned(bool pinned);

protected:
    void mouseReleaseEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;

private:
    TrayWidget *m_trayWidget;
    TrayIcon *m_trayIcon;
    bool m_pinned = false;
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

inline bool TrayMenu::isPinned() const
{
    return m_pinned;
}

} // namespace QtGui

#endif // TRAY_MENU_H
