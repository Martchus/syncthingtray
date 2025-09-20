/*
 *  SPDX-FileCopyrightText: 2011 Marco Martin <mart@kde.org>
 *  SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *  SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>
 *  SPDX-FileCopyrightText: 2024 Martchus <martchus@gmx.net>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once
#include <QImage>
#include <QQuickWindow>
#include <QSGSimpleTextureNode>
#include <QSGTexture>
#include <memory>

namespace QtGui {

class ManagedTextureNode : public QSGSimpleTextureNode {
    Q_DISABLE_COPY(ManagedTextureNode)
public:
    ManagedTextureNode();

    void setTexture(std::shared_ptr<QSGTexture> texture);

private:
    std::shared_ptr<QSGTexture> m_texture;
};

typedef QHash<qint64, QHash<QWindow *, std::weak_ptr<QSGTexture>>> TexturesCache;

struct ImageTexturesCachePrivate {
    TexturesCache cache;
};

class ImageTexturesCache {
public:
    ImageTexturesCache();
    ~ImageTexturesCache();

    /**
     * @returns the texture for a given @p window and @p image.
     *
     * If an @p image id is the same as one already provided before, we won't create
     * a new texture and return a shared pointer to the existing texture.
     */
    std::shared_ptr<QSGTexture> loadTexture(QQuickWindow *window, const QImage &image, QQuickWindow::CreateTextureOptions options);

    std::shared_ptr<QSGTexture> loadTexture(QQuickWindow *window, const QImage &image);

private:
    std::unique_ptr<ImageTexturesCachePrivate> d;
};

} // namespace QtGui
