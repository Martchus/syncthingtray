#ifndef TRAY_MENU_H
#define TRAY_MENU_H

#include <QMenu>

namespace QtGui {

class TrayIcon;
class TrayWidget;

class TrayMenu : public QMenu {
    Q_OBJECT

public:
    enum class WindowType {
        Popup,
        NormalWindow,
        CustomWindow,
    };

    explicit TrayMenu(TrayIcon *trayIcon = nullptr, QWidget *parent = nullptr);

    QSize sizeHint() const override;
    TrayWidget &widget();
    const TrayWidget &widget() const;
    TrayIcon *icon();
    WindowType windowType() const;
    void setWindowType(int windowType);
    void setWindowType(WindowType windowType);

Q_SIGNALS:
    void positioningSettingsChanged();

public Q_SLOTS:
    void showUsingPositioningSettings();

protected:
    void mouseReleaseEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void moveEvent(QMoveEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void paintEvent(QPaintEvent *) override;
    void focusOutEvent(QFocusEvent *) override;

private:
    TrayWidget *m_trayWidget;
    TrayIcon *m_trayIcon;
    WindowType m_windowType;
    bool m_startedSystemWindowCommand;
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

inline TrayMenu::WindowType TrayMenu::windowType() const
{
    return m_windowType;
}

} // namespace QtGui

#endif // TRAY_MENU_H
