/*
 *  SPDX-FileCopyrightText: 2011 Marco Martin <mart@kde.org>
 *  SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *  SPDX-FileCopyrightText: 2024 Martchus <martchus@gmx.net>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "./quickicon.h"
#include "./scenegraph/managedtexturenode.h"

#include <QBitmap>
#include <QGuiApplication>
#include <QIcon>
#include <QPainter>
#include <QQuickImageProvider>
#include <QQuickWindow>
#include <QSGSimpleTextureNode>
#include <QSGTexture>
#include <QScreen>

#include <cstdlib>

namespace QtGui {

Q_GLOBAL_STATIC(ImageTexturesCache, s_iconImageCache)

Icon::Icon(QQuickItem *parent)
    : QQuickItem(parent)
    , m_active(false)
    , m_selected(false)
{
    setFlag(ItemHasContents, true);
    // Using 32 because Icon used to redefine implicitWidth and implicitHeight and hardcode them to 32
    setImplicitSize(32, 32);

    connect(this, &QQuickItem::smoothChanged, this, &QQuickItem::polish);
    connect(this, &QQuickItem::enabledChanged, this, &QQuickItem::polish);
}

Icon::~Icon()
{
}

void Icon::componentComplete()
{
    QQuickItem::componentComplete();

    QQmlEngine *engine = qmlEngine(this);
    Q_ASSERT(engine);
    updatePaintedGeometry();
}

void Icon::setSource(const QVariant &icon)
{
    if (m_source == icon) {
        return;
    }
    m_source = icon;
    polish();
    Q_EMIT sourceChanged();
}

QVariant Icon::source() const
{
    return m_source;
}

void Icon::setActive(const bool active)
{
    if (active == m_active) {
        return;
    }
    m_active = active;
    polish();
    Q_EMIT activeChanged();
}

bool Icon::active() const
{
    return m_active;
}

void Icon::setSelected(const bool selected)
{
    if (selected == m_selected) {
        return;
    }
    m_selected = selected;
    polish();
    Q_EMIT selectedChanged();
}

bool Icon::selected() const
{
    return m_selected;
}

QSGNode *Icon::createSubtree(qreal initialOpacity)
{
    auto opacityNode = new QSGOpacityNode{};
    opacityNode->setFlag(QSGNode::OwnedByParent, true);
    opacityNode->setOpacity(initialOpacity);

    auto *mNode = new ManagedTextureNode;

    mNode->setTexture(s_iconImageCache->loadTexture(window(), m_icon, QQuickWindow::TextureCanUseAtlas));

    opacityNode->appendChildNode(mNode);

    return opacityNode;
}

void Icon::updateSubtree(QSGNode *node, qreal opacity)
{
    auto opacityNode = static_cast<QSGOpacityNode *>(node);
    opacityNode->setOpacity(opacity);

    auto textureNode = static_cast<ManagedTextureNode *>(opacityNode->firstChild());
    textureNode->setFiltering(smooth() ? QSGTexture::Linear : QSGTexture::Nearest);
}

QSGNode *Icon::updatePaintNode(QSGNode *node, QQuickItem::UpdatePaintNodeData * /*data*/)
{
    if (m_source.isNull() || qFuzzyIsNull(width()) || qFuzzyIsNull(height())) {
        delete node;
        return nullptr;
    }

    if (!node) {
        node = new QSGNode{};
    }

    if (node->childCount() == 0) {
        node->appendChildNode(createSubtree(1.0));
        m_textureChanged = true;
    }

    if (node->childCount() > 1) {
        auto toRemove = node->firstChild();
        node->removeChildNode(toRemove);
        delete toRemove;
    }

    updateSubtree(node->firstChild(), 1.0);

    if (m_textureChanged) {
        auto mNode = static_cast<ManagedTextureNode *>(node->lastChild()->firstChild());
        mNode->setTexture(s_iconImageCache->loadTexture(window(), m_icon, QQuickWindow::TextureCanUseAtlas));
        m_textureChanged = false;
        m_sizeChanged = true;
    }

    if (m_sizeChanged) {
        const QSizeF iconPixSize(m_icon.width() / m_devicePixelRatio, m_icon.height() / m_devicePixelRatio);
        const QSizeF itemPixSize = QSizeF((size() * m_devicePixelRatio).toSize()) / m_devicePixelRatio;
        QRectF nodeRect(QPoint(0, 0), itemPixSize);

        if (itemPixSize.width() != 0 && itemPixSize.height() != 0) {
            if (iconPixSize != itemPixSize) {
                // At this point, the image will already be scaled, but we need to output it in
                // the correct aspect ratio, painted centered in the viewport. So:
                QRectF destination(QPointF(0, 0), QSizeF(m_icon.size()).scaled(m_paintedSize, Qt::KeepAspectRatio));
                destination.moveCenter(nodeRect.center());
                destination.moveTopLeft(QPointF(destination.topLeft().toPoint() * m_devicePixelRatio) / m_devicePixelRatio);
                nodeRect = destination;
            }
        }

        // Adjust the final node on the pixel grid
        const auto globalPixelPos = mapToScene(nodeRect.topLeft()) * m_devicePixelRatio;
        const auto posAdjust = QPointF(globalPixelPos.x() - std::round(globalPixelPos.x()), globalPixelPos.y() - std::round(globalPixelPos.y()));
        nodeRect.moveTopLeft(nodeRect.topLeft() - posAdjust);

        for (int i = 0; i < node->childCount(); ++i) {
            auto mNode = static_cast<ManagedTextureNode *>(node->childAtIndex(i)->firstChild());
            mNode->setRect(nodeRect);
        }

        m_sizeChanged = false;
    }

    return node;
}

void Icon::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size()) {
        m_sizeChanged = true;
        updatePaintedGeometry();
        polish();
    }
}

void Icon::updatePolish()
{
    QQuickItem::updatePolish();

    if (const auto *const w = window()) {
        m_devicePixelRatio = w->effectiveDevicePixelRatio();
    }

    if (m_source.isNull()) {
        updatePaintedGeometry();
        update();
        return;
    }

    if (const auto itemSize = QSizeF(width(), height()); !itemSize.isNull()) {
        const auto size = itemSize.toSize();
        switch (m_source.userType()) {
        case QMetaType::QPixmap:
            m_icon = m_source.value<QPixmap>().toImage();
            break;
        case QMetaType::QImage:
            m_icon = m_source.value<QImage>();
            break;
        case QMetaType::QBitmap:
            m_icon = m_source.value<QBitmap>().toImage();
            break;
        case QMetaType::QIcon:
            m_icon = iconPixmap(m_source.value<QIcon>());
            break;
        case QMetaType::QColor:
            m_icon = QImage(size, QImage::Format_Alpha8);
            m_icon.fill(m_source.value<QColor>());
            break;
        default:
            break;
        }

        if (m_icon.isNull()) {
            m_icon = QImage(size, QImage::Format_RGB32);
            m_icon.fill(Qt::red);
        }
    }

    m_textureChanged = true;
    updatePaintedGeometry();
    update();
}

QIcon::Mode Icon::iconMode() const
{
    if (!isEnabled()) {
        return QIcon::Disabled;
    } else if (m_selected) {
        return QIcon::Selected;
    } else if (m_active) {
        return QIcon::Active;
    }
    return QIcon::Normal;
}

qreal Icon::paintedWidth() const
{
    return std::round(m_paintedSize.width());
}

qreal Icon::paintedHeight() const
{
    return std::round(m_paintedSize.height());
}

QSize Icon::iconSizeHint() const
{
    const auto s = static_cast<int>(std::min(width(), height()));
    return QSize(s, s);
}

QImage Icon::iconPixmap(const QIcon &icon) const
{
    const QSize actualSize = icon.actualSize(iconSizeHint());
    return icon.pixmap(actualSize, m_devicePixelRatio, iconMode(), QIcon::On).toImage();
}

void Icon::updatePaintedGeometry()
{
    auto newSize = QSizeF();
    if (!m_icon.width() || !m_icon.height()) {
        newSize = {0, 0};
    } else {
        const qreal roundedWidth = std::round(32 * m_devicePixelRatio) / m_devicePixelRatio;
        if (const auto roundedSize = QSizeF(roundedWidth, roundedWidth); size() == roundedSize) {
            m_paintedSize = roundedSize;
            m_textureChanged = true;
            update();
            Q_EMIT paintedAreaChanged();
            return;
        }
        const QSizeF iconPixSize(m_icon.width() / m_devicePixelRatio, m_icon.height() / m_devicePixelRatio);
        const qreal w = widthValid() ? width() : iconPixSize.width();
        const qreal widthScale = w / iconPixSize.width();
        const qreal h = heightValid() ? height() : iconPixSize.height();
        const qreal heightScale = h / iconPixSize.height();
        if (widthScale <= heightScale) {
            newSize = QSizeF(w, widthScale * iconPixSize.height());
        } else if (heightScale < widthScale) {
            newSize = QSizeF(heightScale * iconPixSize.width(), h);
        }
    }
    if (newSize != m_paintedSize) {
        m_paintedSize = newSize;
        m_textureChanged = true;
        update();
        Q_EMIT paintedAreaChanged();
    }
}

void Icon::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value)
{
    if (change == QQuickItem::ItemDevicePixelRatioHasChanged) {
        if (const auto *const w = window()) {
            m_devicePixelRatio = w->effectiveDevicePixelRatio();
        }
        polish();
    } else if (change == QQuickItem::ItemSceneChange && value.window) {
        m_devicePixelRatio = value.window->effectiveDevicePixelRatio();
    }
    QQuickItem::itemChange(change, value);
}

}

#include "moc_quickicon.cpp"
