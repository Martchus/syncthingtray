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

using namespace QtUtilities;

namespace QtGui {

TrayMenu::TrayMenu(TrayIcon *trayIcon, QWidget *parent)
    : QMenu(parent)
    , m_trayIcon(trayIcon)
    , m_windowType(WindowType::Popup)
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
void moveInside(QPoint &point, const QSize &innerRect, const QRect &outerRect)
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
    if (windowType >= 0 && windowType <= 2) {
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
    }
    setWindowFlags(flags);
}

void TrayMenu::mousePressEvent(QMouseEvent *event)
{
    if (m_windowType != TrayMenu::WindowType::NormalWindow) {
        QMenu::mousePressEvent(event);
    }
}

void TrayMenu::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_windowType != TrayMenu::WindowType::NormalWindow) {
        QMenu::mouseReleaseEvent(event);
    }
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
