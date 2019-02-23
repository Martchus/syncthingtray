#include "./syncthingicons.h"

#include <QFile>
#include <QPainter>
#include <QStringBuilder>
#include <QSvgRenderer>

namespace Data {

/// \cond
namespace Detail {
template <typename SourceType> QPixmap renderSvgImage(const SourceType &source, const QSize &size)
{
    QSvgRenderer renderer(source);
    QPixmap pm(size);
    pm.fill(QColor(Qt::transparent));
    QPainter painter(&pm);
    renderer.render(&painter, pm.rect());
    return pm;
}
} // namespace Detail
/// \endcond

/*!
 * \brief Renders an SVG image to a QPixmap.
 * \remarks If instantiating QIcon directly from SVG image the icon is not displayed in the tray under Plasma 5. It works
 *          with Tint2, however.
 */
QPixmap renderSvgImage(const QString &path, const QSize &size)
{
    return Detail::renderSvgImage(path, size);
}

/*!
 * \brief Renders an SVG image to a QPixmap.
 */
QPixmap renderSvgImage(const QByteArray &contents, const QSize &size)
{
    return Detail::renderSvgImage(contents, size);
}

/*!
 * \brief Returns the font awesome icon with the specified \a iconName and \a color.
 */
QByteArray loadFontAwesomeIcon(const QString &iconName, const QColor &color)
{
    QByteArray result;
    QFile icon(QStringLiteral(":/icons/hicolor/scalable/fa/") % iconName % QStringLiteral(".svg"));
    if (!icon.open(QFile::ReadOnly)) {
        return result;
    }
    result = icon.readAll();
    result.replace("currentColor", color.name(QColor::HexRgb).toUtf8());
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

const StatusIcons LIB_SYNCTHING_MODEL_EXPORT &statusIcons()
{
    static const StatusIcons icons;
    return icons;
}

} // namespace Data
