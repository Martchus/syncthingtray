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

#ifdef TRAY_MENU_HANDLE_WINDOWS11_STYLE
#include <QStyle>
#if (QT_VERSION < QT_VERSION_CHECK(6, 9, 0))
#include <QLibraryInfo>
#endif
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
#define QT_SUPPORTS_SYSTEM_WINDOW_COMMANDS
#endif

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#define QT_PLATFORM_MAY_NOT_SUPPORT_POPUP
#define QT_PLATFORM_POPUP_FLAGS_SPECIFIER
#else
#define QT_PLATFORM_POPUP_FLAGS_SPECIFIER constexpr
#endif

using namespace QtUtilities;

namespace QtGui {

static constexpr auto border = 10;

static QT_PLATFORM_POPUP_FLAGS_SPECIFIER auto popupFlags = Qt::FramelessWindowHint | Qt::Popup;

#ifdef TRAY_MENU_HANDLE_WINDOWS11_STYLE
static bool isWindows11Style(const QWidget *widget)
{
    const auto *const s = widget->style();
    return s && s->name().compare(QLatin1String("windows11"), Qt::CaseInsensitive) == 0;
}
#endif

TrayMenu::TrayMenu(TrayIcon *trayIcon, QWidget *parent)
    : QMenu(parent)
    , m_layout(new QHBoxLayout)
    , m_trayIcon(trayIcon)
    , m_windowType(WindowType::Popup)
    , m_startedSystemWindowCommand(false)
#ifdef TRAY_MENU_HANDLE_WINDOWS11_STYLE
    , m_isWindows11Style(isWindows11Style(this))
#endif
{
    // disable use of the popup type under platforms that don't support it and emulate closing the window by checking
    // the application state
    // note: The Wayland platform does not support popups without a parent that received input events. Trying to show
    //       the menu as popup would lead to "qt.qpa.wayland: Failed to create grabbing popup. Ensure popup … has a
    //       transientParent set and that parent window has received input." and the menu would not show up. This is
    //       not always/everywhere reproducible but it is nevertheless best to avoid using a popup completely.
#ifdef QT_PLATFORM_MAY_NOT_SUPPORT_POPUP
    if (QGuiApplication::platformName() == QStringLiteral("wayland")) {
        popupFlags = Qt::Dialog | Qt::FramelessWindowHint | Qt::CustomizeWindowHint;
        QObject::connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
            // close the menu if the application becomes inactive
            // note: This is not perfect as the menu will stay open if another window is active.
            switch (state) {
            case Qt::ApplicationInactive:
                close();
                break;
            default:;
            }
        });
    }
#endif

    setObjectName(QStringLiteral("QtGui::TrayMenu"));
    setLayout(m_layout);
    updateContentMargins();
    m_layout->setSpacing(0);
    m_layout->addWidget(m_trayWidget = new TrayWidget(this));
    setPlatformMenu(nullptr);
    setWindowFlags(popupFlags);
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

/*!
 * \brief Shows the menu using positioning settings.
 * \remarks
 * Not using `popup(pos.value())` because as of Qt 6.9.0 this function returns early if there is not visible action. This could be worked
 * around using `addAction(QString())` but in some styles (e.g. the classic "Windows" style) this leads to an empty action being drawn. So
 * this function just uses `show()` in any case now.
 */
void TrayMenu::showUsingPositioningSettings()
{
    if (m_windowType == WindowType::None) {
        widget().showWebUI();
        return;
    }
    resize(sizeHint());
    if (auto pos = Settings::values().appearance.positioning.positionToUse(); pos.has_value()) {
        const auto popupSize = size();
        moveInside(pos.value(), popupSize, availableScreenGeometryAtPoint(pos.value()));
        setGeometry(QRect(pos.value(), popupSize));
    }
    show();
    activateWindow();
}

bool TrayMenu::event(QEvent *event)
{
#ifdef TRAY_MENU_HANDLE_WINDOWS11_STYLE
    switch (event->type()) {
    case QEvent::StyleChange:
        m_isWindows11Style = isWindows11Style(this);
        updateContentMargins();
        break;
    case QEvent::PolishRequest:
    case QEvent::Polish:
        if (m_windowType != TrayMenu::WindowType::Popup && m_isWindows11Style) {
            // avoid polishing via the Windows 11 style as it would break behavior if we don't actually show this as popup
            event->accept();
            return true;
        }
        break;
    default:;
    }
#endif
    return QMenu::event(event);
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
        flags = popupFlags;
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

#ifdef TRAY_MENU_HANDLE_WINDOWS11_STYLE
    // ensure correct margins and polishing when using Windows 11 style
    if (m_isWindows11Style) {
        updateContentMargins();
        if (windowType == WindowType::Popup) {
            if (auto *const s = style()) {
                s->polish(this);
            }
        }
    }
#endif
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
#ifdef TRAY_MENU_HANDLE_WINDOWS11_STYLE
        const auto p = m_windowType != TrayMenu::WindowType::Popup && m_isWindows11Style ? QGuiApplication::palette() : QPalette(palette());
#else
        const auto &p = palette();
#endif
        QPainter(this).fillRect(event->rect(), p.color(backgroundRole()));
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

void TrayMenu::updateContentMargins()
{
#ifdef TRAY_MENU_HANDLE_WINDOWS11_STYLE
    // set higher margins to account for the shadow effects of the Windows 11 style
    // note: Not sure whether there's a way to determine the required margins dynamically.
    if (m_windowType == TrayMenu::WindowType::Popup && m_isWindows11Style) {
        static constexpr auto normalMargin = 2;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 9, 0))
        static constexpr auto extraMargin = normalMargin;
#else
        static const auto needsExtraMargin = QLibraryInfo::version() < QVersionNumber(6, 8, 1);
        static const auto extraMargin = needsExtraMargin ? 10 : normalMargin;
#endif
        m_layout->setContentsMargins(normalMargin, normalMargin, extraMargin, normalMargin);
        return;
    }
#endif
    m_layout->setContentsMargins(0, 0, 0, 0);
}

} // namespace QtGui
