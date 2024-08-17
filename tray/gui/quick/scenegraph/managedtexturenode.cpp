/*
 *  SPDX-FileCopyrightText: 2011 Marco Martin <mart@kde.org>
 *  SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *  SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>
 *  SPDX-FileCopyrightText: 2024 Martchus <martchus@gmx.net>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "./managedtexturenode.h"

namespace QtGui {

ManagedTextureNode::ManagedTextureNode()
{
}

void ManagedTextureNode::setTexture(std::shared_ptr<QSGTexture> texture)
{
    m_texture = texture;
    QSGSimpleTextureNode::setTexture(texture.get());
}

ImageTexturesCache::ImageTexturesCache()
    : d(new ImageTexturesCachePrivate)
{
}

ImageTexturesCache::~ImageTexturesCache()
{
}

std::shared_ptr<QSGTexture> ImageTexturesCache::loadTexture(QQuickWindow *window, const QImage &image, QQuickWindow::CreateTextureOptions options)
{
    qint64 id = image.cacheKey();
    std::shared_ptr<QSGTexture> texture = d->cache.value(id).value(window).lock();

    if (!texture) {
        auto cleanAndDelete = [this, window, id](QSGTexture *textureToDelete) {
            QHash<QWindow *, std::weak_ptr<QSGTexture>> &textures = (d->cache)[id];
            textures.remove(window);
            if (textures.isEmpty()) {
                d->cache.remove(id);
            }
            delete textureToDelete;
        };
        texture = std::shared_ptr<QSGTexture>(window->createTextureFromImage(image, options), cleanAndDelete);
        (d->cache)[id][window] = texture;
    }

    // if we have a cache in an atlas but our request cannot use an atlassed texture
    // create a new texture and use that
    // don't use removedFromAtlas() as that requires keeping a reference to the non atlased version
    if (!(options & QQuickWindow::TextureCanUseAtlas) && texture->isAtlasTexture()) {
        texture = std::shared_ptr<QSGTexture>(window->createTextureFromImage(image, options));
    }

    return texture;
}

std::shared_ptr<QSGTexture> ImageTexturesCache::loadTexture(QQuickWindow *window, const QImage &image)
{
    return loadTexture(window, image, {});
}

} // namespace QtGui
