/*
 *  SPDX-FileCopyrightText: 2011 Marco Martin <mart@kde.org>
 *  SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *  SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>
 *  SPDX-FileCopyrightText: 2024 Martchus <martchus@gmx.net>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QIcon>
#include <QQuickItem>
#include <QVariant>

namespace QtGui {

/**
 * \brief The Icon class renders an icon (a QBitmap, QPixmap, QImage, QIcon or a QColor) in a Qt Quick UI.
 * \remarks
 * This class has been copied from Kirigami and stripped down from a generic icon loader with animations to
 * a class that simply renders the mentioned Qt image types in a Qt Quick scene. This is useful because the
 * Image type of Qt itself does not accept those image types directly.
 */
class Icon : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    /**
     * The source of this icon. It can be a QBitmap, QPixmap, QImage, QIcon or a QColor.
     */
    Q_PROPERTY(QVariant source READ source WRITE setSource NOTIFY sourceChanged FINAL)

    /**
     * Whether this icon will use the QIcon::Active mode when drawing the icon,
     * resulting in a graphical effect being applied to the icon to indicate that
     * it is currently active.
     *
     * This is typically used to indicate when an item is being hovered or pressed.
     */
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged FINAL)

    /**
     * Whether this icon will use the QIcon::Selected mode when drawing the icon,
     * resulting in a graphical effect being applied to the icon to indicate that
     * it is currently selected.
     *
     * This is typically used to indicate when a list item is currently selected.
     */
    Q_PROPERTY(bool selected READ selected WRITE setSelected NOTIFY selectedChanged FINAL)

    /**
     * The width of the painted area measured in pixels. This will be smaller than or
     * equal to the width of the area taken up by the Item itself. This can be 0.
     */
    Q_PROPERTY(qreal paintedWidth READ paintedWidth NOTIFY paintedAreaChanged FINAL)

    /**
     * The height of the painted area measured in pixels. This will be smaller than or
     * equal to the height of the area taken up by the Item itself. This can be 0.
     */
    Q_PROPERTY(qreal paintedHeight READ paintedHeight NOTIFY paintedAreaChanged FINAL)

public:
    explicit Icon(QQuickItem *parent = nullptr);
    ~Icon() override;

    void componentComplete() override;

    void setSource(const QVariant &source);
    QVariant source() const;

    void setActive(bool active = true);
    bool active() const;

    void setSelected(bool selected = true);
    bool selected() const;

    qreal paintedWidth() const;
    qreal paintedHeight() const;

    QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *data) override;

Q_SIGNALS:
    void sourceChanged();
    void activeChanged();
    void selectedChanged();
    void paintedAreaChanged();

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    QIcon::Mode iconMode() const;
    bool guessMonochrome(const QImage &img);
    void updatePolish() override;
    void updatePaintedGeometry();
    void updateIsMaskHeuristic(const QString &iconSource);
    void itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value) override;

private:
    QSGNode *createSubtree(qreal initialOpacity);
    void updateSubtree(QSGNode *node, qreal opacity);
    QSize iconSizeHint() const;
    inline QImage iconPixmap(const QIcon &icon) const;

    QVariant m_source;
    qreal m_devicePixelRatio = 1.0;
    bool m_textureChanged = false;
    bool m_sizeChanged = false;
    bool m_active;
    bool m_selected;
    QSizeF m_paintedSize;
    QImage m_icon;
};

}
