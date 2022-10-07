#include "./traymenu.h"
#include "./trayicon.h"
#include "./traywidget.h"

#include <syncthingwidgets/settings/settings.h>

#include <qtutilities/misc/dialogutils.h>

#include <QApplication>
#include <QHBoxLayout>

using namespace QtUtilities;

namespace QtGui {

TrayMenu::TrayMenu(TrayIcon *trayIcon, QWidget *parent)
    : QMenu(parent)
    , m_trayIcon(trayIcon)
    , m_pinned(false)
{
    setObjectName(QStringLiteral("QtGui::TrayMenu"));
    auto *const menuLayout = new QHBoxLayout;
    menuLayout->setContentsMargins(0, 0, 0, 0);
    menuLayout->setSpacing(0);
    menuLayout->addWidget(m_trayWidget = new TrayWidget(this));
    setLayout(menuLayout);
    setPlatformMenu(nullptr);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);
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
    moveInside(pos, size(), availableScreenGeometryAtPoint(pos));
    popup(pos);
    activateWindow();
}

void TrayMenu::setPinned(bool pinned)
{
    setWindowFlags(Qt::FramelessWindowHint | ((m_pinned = pinned) ? Qt::Window : Qt::Popup));
    show();
    activateWindow();
}

void TrayMenu::mousePressEvent(QMouseEvent *event)
{
    if (!m_pinned) {
        QMenu::mousePressEvent(event);
    }
}

void TrayMenu::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_pinned) {
        QMenu::mouseReleaseEvent(event);
    }
}

} // namespace QtGui
