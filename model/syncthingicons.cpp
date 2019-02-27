#include "./syncthingicons.h"

#include <QFile>
#include <QPainter>
#include <QStringBuilder>
#include <QSvgRenderer>

namespace Data {

/// \cond
namespace Detail {
template <typename SourceType> QPixmap renderSvgImage(const SourceType &source, const QSize &size, int margin)
{
    QSvgRenderer renderer(source);
    QSize renderSize(renderer.defaultSize());
    renderSize.scale(size.width() - margin, size.height() - margin, Qt::KeepAspectRatio);
    QRect renderBounds(QPoint(), size);
    if (renderSize.width() < renderBounds.width()) {
        const auto diff = (renderBounds.width() - renderSize.width()) / 2;
        renderBounds.setX(diff);
        renderBounds.setWidth(renderSize.width());
    }
    if (renderSize.height() < renderBounds.height()) {
        const auto diff = (renderBounds.height() - renderSize.height()) / 2;
        renderBounds.setY(diff);
        renderBounds.setHeight(renderSize.height());
    }
    QPixmap pm(size);
    pm.fill(QColor(Qt::transparent));
    QPainter painter(&pm);
    renderer.render(&painter, renderBounds);
    return pm;
}
} // namespace Detail
/// \endcond

/*!
 * \brief Renders an SVG image to a QPixmap.
 * \remarks If instantiating QIcon directly from SVG image the icon is not displayed in the tray under Plasma 5. It works
 *          with Tint2, however.
 */
QPixmap renderSvgImage(const QString &path, const QSize &size, int margin)
{
    return Detail::renderSvgImage(path, size, margin);
}

/*!
 * \brief Renders an SVG image to a QPixmap.
 */
QPixmap renderSvgImage(const QByteArray &contents, const QSize &size, int margin)
{
    return Detail::renderSvgImage(contents, size, margin);
}

/*!
 * \brief Returns the font awesome icon with the specified \a iconName and \a color.
 */
QByteArray loadFontAwesomeIcon(const QString &iconName, const QColor &color, bool solid)
{
    QByteArray result;
    QFile icon((solid ? QStringLiteral(":/icons/hicolor/scalable/fa/") : QStringLiteral(":/icons/hicolor/scalable/fa-non-solid/")) % iconName
        % QStringLiteral(".svg"));
    if (!icon.open(QFile::ReadOnly)) {
        return result;
    }
    result = icon.readAll();
    const auto pathBegin = result.indexOf("<path ");
    if (pathBegin > 0) {
        result.insert(pathBegin + 6, QStringLiteral("fill=\"") % color.name(QColor::HexRgb) % QStringLiteral("\" "));
    }
    return result;
}

StatusIcons::StatusIcons()
    : disconnected(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-disconnected.svg"))))
    , idling(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-ok.svg"))))
    , scanninig(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-default.svg"))))
    , notify(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-notify.svg"))))
    , pause(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-pause.svg"))))
    , sync(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-sync.svg"))))
    , syncComplete(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-sync-complete.svg"))))
    , error(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-error.svg"))))
    , errorSync(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-error-sync.svg"))))
    , newItem(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-new.svg"))))
{
}

FontAwesomeIcons::FontAwesomeIcons(const QColor &color, const QSize &size, int margin)
    : hashtag(renderSvgImage(loadFontAwesomeIcon(QStringLiteral("hashtag"), color), size, margin))
    , folderOpen(renderSvgImage(loadFontAwesomeIcon(QStringLiteral("folder-open"), color), size, margin))
    , globe(renderSvgImage(loadFontAwesomeIcon(QStringLiteral("globe"), color), size, margin))
    , home(renderSvgImage(loadFontAwesomeIcon(QStringLiteral("home"), color), size, margin))
    , shareAlt(renderSvgImage(loadFontAwesomeIcon(QStringLiteral("share-alt"), color), size, margin))
    , refresh(renderSvgImage(loadFontAwesomeIcon(QStringLiteral("redo"), color), size, margin))
    , clock(renderSvgImage(loadFontAwesomeIcon(QStringLiteral("clock"), color), size, margin))
    , exchangeAlt(renderSvgImage(loadFontAwesomeIcon(QStringLiteral("exchange-alt"), color), size, margin))
    , exclamationTriangle(renderSvgImage(loadFontAwesomeIcon(QStringLiteral("exclamation-triangle"), color), size, margin))
    , cogs(renderSvgImage(loadFontAwesomeIcon(QStringLiteral("cogs"), color), size, margin))
    , link(renderSvgImage(loadFontAwesomeIcon(QStringLiteral("link"), color), size, margin))
    , eye(renderSvgImage(loadFontAwesomeIcon(QStringLiteral("eye"), color), size, margin))
    , fileArchive(renderSvgImage(loadFontAwesomeIcon(QStringLiteral("file-archive"), color), size, margin))
    , folder(renderSvgImage(loadFontAwesomeIcon(QStringLiteral("folder"), color), size, margin))
    , certificate(renderSvgImage(loadFontAwesomeIcon(QStringLiteral("certificate"), color), size, margin))
    , networkWired(renderSvgImage(loadFontAwesomeIcon(QStringLiteral("network-wired"), color), size, margin))
    , cloudDownloadAlt(renderSvgImage(loadFontAwesomeIcon(QStringLiteral("cloud-download-alt"), color), size, margin))
    , cloudUploadAlt(renderSvgImage(loadFontAwesomeIcon(QStringLiteral("cloud-upload-alt"), color), size, margin))
    , tag(renderSvgImage(loadFontAwesomeIcon(QStringLiteral("tag"), color), size, margin))
{
}

const StatusIcons &statusIcons()
{
    static const StatusIcons icons;
    return icons;
}

const FontAwesomeIcons &fontAwesomeIconsForLightTheme()
{
    static const FontAwesomeIcons icons(QColor(10, 10, 10), QSize(64, 64), 8);
    return icons;
}

const FontAwesomeIcons &fontAwesomeIconsForDarkTheme()
{
    static const FontAwesomeIcons icons(Qt::white, QSize(64, 64), 8);
    return icons;
}

} // namespace Data
