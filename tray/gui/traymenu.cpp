#include "./traymenu.h"
#include "./trayicon.h"
#include "./traywidget.h"

#include <syncthingwidgets/settings/settings.h>

#include <qtutilities/misc/dialogutils.h>

#include <QApplication>
#include <QHBoxLayout>
#include <QPaintEvent>
#include <QPainter>
#include <QWindow>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
#define QT_SUPPORTS_SYSTEM_WINDOW_COMMANDS
#endif

using namespace QtUtilities;

namespace QtGui {

static constexpr auto border = 10;

TrayMenu::TrayMenu(TrayIcon *trayIcon, QWidget *parent)
    : QMenu(parent)
    , m_trayIcon(trayIcon)
    , m_windowType(WindowType::Popup)
    , m_startedSystemWindowCommand(false)
{
    setObjectName(QStringLiteral("QtGui::TrayMenu"));
    auto *const menuLayout = new QHBoxLayout;
    menuLayout->setContentsMargins(0, 0, 0, 0);
    menuLayout->setSpacing(0);
    menuLayout->addWidget(m_trayWidget = new TrayWidget(this));
    setLayout(menuLayout);
    setPlatformMenu(nullptr);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);
    setWindowIcon(m_trayWidget->windowIcon());
}

QSize TrayMenu::sizeHint() const
{
    return Settings::values().appearance.trayMenuSize;
}

/*!
 * \brief Moves the specified \a innerRect at the specified \a point into the specified \a outerRect
 *        by altering \a point.
 */
static void moveInside(QPoint &point, const QSize &innerRect, const QRect &outerRect)
{
    if (point.y() < outerRect.top()) {
        point.setY(outerRect.top());
    } else if (point.y() + innerRect.height() > outerRect.bottom()) {
        point.setY(outerRect.bottom() - innerRect.height());
    }
    if (point.x() < outerRect.left()) {
        point.setX(outerRect.left());
    } else if (point.x() + innerRect.width() > outerRect.right()) {
        point.setX(outerRect.right() - innerRect.width());
    }
}

void TrayMenu::showUsingPositioningSettings()
{
    if (m_windowType == WindowType::None) {
        widget().showWebUI();
        return;
    }
    resize(sizeHint());
    auto pos = Settings::values().appearance.positioning.positionToUse();
    if (pos.has_value()) {
        moveInside(pos.value(), size(), availableScreenGeometryAtPoint(pos.value()));
        popup(pos.value());
    } else {
        show();
    }
    activateWindow();
}

void TrayMenu::setWindowType(int windowType)
{
    if (windowType >= 0 && windowType <= 3) {
        setWindowType(static_cast<WindowType>(windowType));
    }
}

void TrayMenu::setWindowType(WindowType windowType)
{
    if (m_windowType == windowType) {
        return;
    }
    auto flags = Qt::WindowFlags();
    switch (m_windowType = windowType) {
    case WindowType::Popup:
        flags = Qt::FramelessWindowHint | Qt::Popup;
        break;
    case WindowType::NormalWindow:
        flags = Qt::Window;
        break;
    case WindowType::CustomWindow:
        flags = Qt::Dialog | Qt::CustomizeWindowHint;
        break;
    case WindowType::None:
        break;
    }
    setWindowFlags(flags);
}

void TrayMenu::mousePressEvent(QMouseEvent *event)
{
    // skip any special behavior if the tray menu is shown as a regular window
    if (m_windowType == TrayMenu::WindowType::NormalWindow) {
        return;
    }

    // try starting a system window resize/move to allow resizing/moving the borderless window
#ifdef QT_SUPPORTS_SYSTEM_WINDOW_COMMANDS
    if (auto *const window = this->windowHandle()) {
        // keep default behavior on X11 for popups as the start functions don't work there (even though they return true)
#if defined(Q_OS_UNIX) && !(defined(Q_OS_ANDROID) || defined(Q_OS_DARWIN))
        if (m_windowType == TrayMenu::WindowType::Popup) {
            static const auto platform = QGuiApplication::platformName();
            if (!platform.compare(QLatin1String("xcb"), Qt::CaseInsensitive)) {
                QMenu::mousePressEvent(event);
                return;
            }
        }
#endif
        // check relevant edges and start the appropriate system resize/move
        const auto pos = event->pos();
        auto edges = Qt::Edges();
        if (pos.x() < border)
            edges |= Qt::LeftEdge;
        if (pos.x() >= width() - border)
            edges |= Qt::RightEdge;
        if (pos.y() < border)
            edges |= Qt::TopEdge;
        if (pos.y() >= height() - border)
            edges |= Qt::BottomEdge;
        m_startedSystemWindowCommand = edges ? window->startSystemResize(edges) : window->startSystemMove();
    }
#endif

    // fallback to the default behavior for the current window type if system window resize/move is not possible
    if (!m_startedSystemWindowCommand) {
        QMenu::mousePressEvent(event);
    }
}

void TrayMenu::mouseReleaseEvent(QMouseEvent *event)
{
    // cover cases analogous to TrayMenu::mousePressEvent()
    if (m_windowType == TrayMenu::WindowType::NormalWindow) {
        return;
    }
    if (m_startedSystemWindowCommand) {
        m_startedSystemWindowCommand = false;
    } else {
        QMenu::mouseReleaseEvent(event);
    }
}

void TrayMenu::moveEvent(QMoveEvent *event)
{
    auto &settings = Settings::values().appearance.positioning;
    if (settings.useAssumedIconPosition) {
        Settings::values().appearance.positioning.assumedIconPosition = event->pos();
        emit positioningSettingsChanged();
    }
}

void TrayMenu::resizeEvent(QResizeEvent *event)
{
    Settings::values().appearance.trayMenuSize = event->size();
    emit positioningSettingsChanged();
}

void TrayMenu::paintEvent(QPaintEvent *event)
{
    if (m_windowType == WindowType::Popup) {
        QMenu::paintEvent(event);
    } else {
        QPainter(this).fillRect(event->rect(), palette().window());
        QWidget::paintEvent(event);
    }
}

void TrayMenu::focusOutEvent(QFocusEvent *)
{
    if (m_windowType == WindowType::CustomWindow) {
        if (const auto *fw = focusWidget(); fw->hasFocus()) {
            return;
        }
        close();
    }
}

} // namespace QtGui
