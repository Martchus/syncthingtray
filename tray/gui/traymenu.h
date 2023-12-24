#ifndef TRAY_MENU_H
#define TRAY_MENU_H

#include <QMenu>

QT_FORWARD_DECLARE_CLASS(QHBoxLayout)

#if defined(Q_OS_WINDOWS) && (QT_VERSION >= QT_VERSION_CHECK(6, 1, 0))
#define TRAY_MENU_HANDLE_WINDOWS11_STYLE
#endif

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
        None,
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
    bool event(QEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void moveEvent(QMoveEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void paintEvent(QPaintEvent *) override;
    void focusOutEvent(QFocusEvent *) override;

private:
    void updateContentMargins();

    QHBoxLayout *m_layout;
    TrayWidget *m_trayWidget;
    TrayIcon *m_trayIcon;
    WindowType m_windowType;
    bool m_startedSystemWindowCommand;
#ifdef TRAY_MENU_HANDLE_WINDOWS11_STYLE
    bool m_isWindows11Style;
#endif
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
