#include "./syncthingicons.h"

#include <QFile>
#include <QPainter>
#include <QStringBuilder>
#include <QSvgRenderer>

namespace Data {

/*!
 * \brief Generates the SVG code for the Syncthing icon with the specified colors and status emblem.
 */
QByteArray makeSyncthingIcon(const GradientColor &gradientColor, StatusEmblem statusEmblem)
{
    // clang-format off
    static const QString emblems[] = {
        QString(),
        QStringLiteral(
            "<circle id=\"bubble\" cx=\"11.5\" cy=\"11.5\" r=\"4.5\" style=\"fill:#000000\"/>"
            "<g id=\"search\">"
                "<path style=\"fill:#ffffff;fill-opacity:1;stroke:none;stroke-width:0.48022598\" d=\"m 10.669491,8.1035085 c -1.1972025,0 -2.2607791,1.065213 -2.2607791,2.2609745 0,1.197203 1.0650175,2.261025 2.2607791,2.261025 0.510789,0 1.007001,-0.113577 1.376093,-0.407927 l 1.936914,2.2081 0.763264,-0.763264 -2.106406,-1.919965 C 12.935006,11.372784 12.931,10.876087 12.931,10.364483 12.931,9.1672809 11.865254,8.1035085 10.669491,8.1035085 Z M 10.669712,8.884 c 0.931159,0 1.481288,0.5488435 1.481288,1.48 0,0.931159 -0.55035,1.482 -1.481509,1.482 C 9.7383328,11.846 9.189,11.295642 9.189,10.364483 9.189,9.4333265 9.7385538,8.884 10.669712,8.884 Z\"/>"
            "</g>"
        ),
        QStringLiteral(
            "<circle id=\"bubble\" cx=\"11.5\" cy=\"11.5\" r=\"4.5\" style=\"fill:#000000\"/>"
            "<g id=\"arrows\" transform=\"rotate(0 11.5 11.5)\">"
                "<path id=\"arrow-left\" d=\"m 11.5,14 0,-1 c -1.5,0 -1.5,0 -1.5,-2 l 1,0 -1.5,-2 -1.5,2 1,0 c 0,3 0,3 2.5,3 z\" style=\"fill:#ffffff\"/>"
                "<path id=\"arrow-right\" d=\"m 11.5,9 0,1 c 1.5,0 1.5,0 1.5,2 l -1,0 1.5,2 1.5,-2 -1,0 C 14,9 14,9 11.5,9 Z\" style=\"fill:#ffffff\"/>"
            "</g>"
        ),
        QStringLiteral(
            "<circle id=\"bubble\" cx=\"11.5\" cy=\"11.5\" r=\"4.5\" style=\"fill:#000000\"/>"
            "<g id=\"exclaim\">"
                "<rect id=\"exclaim-top\" x=\"11\" y=\"9\" width=\"1\" height=\"3\" style=\"fill:#ffffff\"/>"
                "<rect id=\"exclaim-bottom\" x=\"11\" y=\"13\" width=\"1\" height=\"1\" style=\"fill:#ffffff\"/>"
            "</g>"
        ),
        QStringLiteral(
            "<circle id=\"bubble\" cx=\"11.5\" cy=\"11.5\" r=\"4.5\" style=\"fill:#000000\"/>"
            "<g id=\"pause\">"
                "<rect id=\"pause-leftbar\" x=\"10\" y=\"9\" width=\"1\" height=\"5\" style=\"fill:#ffffff\"/>"
                "<rect id=\"pause-rightbar\" x=\"12\" y=\"9\" width=\"1\" height=\"5\" style=\"fill:#ffffff\"/>"
            "</g>"
        ),
        QStringLiteral(
            "<circle id=\"bubble\" cx=\"11.5\" cy=\"11.5\" r=\"4.5\" style=\"fill:#27ae60\"/>"
            "<path style=\"fill:#ffffff;fill-opacity:1;stroke:none\" d=\"m 13.661017,9.2966105 -3,2.9999995 -1,-1 -1,1 1,1 1,1 4,-4 z\"/>"
        ),
        QStringLiteral(
            "<circle id=\"bubble\" cx=\"11.5\" cy=\"11.5\" r=\"4.5\" style=\"fill:#27ae60\"/>"
            "<g id=\"tick\">"
                "<path style=\"opacity:1;fill:#ffffff;fill-opacity:0.98581561;fill-rule:nonzero;stroke:none;stroke-width:1.14997458;stroke-linecap:butt;stroke-linejoin:miter;stroke-miterlimit:4;stroke-dasharray:none;stroke-dashoffset:1.99954295;stroke-opacity:1;paint-order:normal;\" d=\"m 11.008051,9.1016944 h 1.25 v 5.0000006 h -1.25 z M 9.1330507,10.976695 h 5.0000003 v 1.25 H 9.1330507 Z\"/>"
            "</g>"
        ),
        QStringLiteral(
            "<circle id=\"bubble\" cx=\"11.5\" cy=\"11.5\" r=\"4.5\" style=\"fill:#000000\"/>"
            "<g id=\"plus\">"
                "<path style=\"fill:#fffff6;fill-opacity:1;stroke:none;stroke-width:0.48022598\" d=\"m 10.771186,8.4407554 c -1.1972026,0 -2.1610164,0.9652542 -2.1610164,2.1610166 0,1.197203 0.9652547,2.161017 2.1610164,2.161017 0.510789,0 0.973102,-0.183061 1.342194,-0.477411 l 1.919966,1.919965 0.339535,-0.339535 -1.919965,-1.919966 c 0.29565,-0.369668 0.479287,-0.832466 0.479287,-1.34407 0,-1.197203 -0.965254,-2.1610166 -2.161017,-2.1610166 z m 0,0.480226 c 0.931159,0 1.680791,0.7496332 1.680791,1.6807906 0,0.931159 -0.749632,1.680791 -1.680791,1.680791 -0.9311583,0 -1.6807905,-0.749632 -1.6807905,-1.680791 0,-0.9311574 0.7496322,-1.6807906 1.6807905,-1.6807906 z\"/>"
            "</g>"
        ),
    };
    const auto &emblemData = emblems[static_cast<int>(statusEmblem)];
    auto gradientStart = gradientColor.start.name(QColor::HexRgb);
    auto gradientEnd = gradientColor.end.name(QColor::HexRgb);
    if (gradientColor.start.alphaF() < 1.0) {
        gradientStart += QStringLiteral(";stop-opacity:") + QString::number(gradientColor.start.alphaF());
    }
    if (gradientColor.end.alphaF() < 1.0) {
        gradientEnd += QStringLiteral(";stop-opacity:") + QString::number(gradientColor.end.alphaF());
    }
    return (QStringLiteral(
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<svg xmlns:svg=\"http://www.w3.org/2000/svg\" xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 16 16\">"
          "<defs>"
              "<linearGradient id=\"grad\" gradientUnits=\"userSpaceOnUse\" x1=\"8\" y1=\"0\" x2=\"8\" y2=\"16\">"
                  "<stop offset=\"0\" style=\"stop-color:") % gradientStart % QStringLiteral("\"/>"
                  "<stop offset=\"1\" style=\"stop-color:") % gradientEnd % QStringLiteral("\"/>"
              "</linearGradient>"
              "<mask id=\"bitemask\" maskUnits=\"userSpaceOnUse\">"
                  "<g>"
                      "<rect id=\"mask-bg\" x=\"0\" y=\"0\" width=\"16\" height=\"16\" style=\"fill:#ffffff\"/>"
                      "<circle id=\"mask-subtract\" cx=\"11.5\" cy=\"11.5\" r=\"5.5\" style=\"fill:#000000\"/>"
                  "</g>"
              "</mask>"
          "</defs>"
          "<g id=\"syncthing-logo\" mask=\"url(#bitemask)\">"
              "<circle id=\"outer\" cx=\"8\" cy=\"8\" r=\"8\" style=\"fill:url(#grad)\"/>"
              "<circle id=\"inner\" cx=\"8\" cy=\"7.9727402\" r=\"5.9557071\" style=\"fill:none;stroke:#ffffff;stroke-width:0.81771719\"/>"
              "<line id=\"arm-l\" x1=\"9.1993189\" y1=\"8.776825\" x2=\"2.262351\" y2=\"9.4173737\" style=\"stroke:#ffffff;stroke-width:0.81771719\"/>"
              "<line id=\"arm-tr\" x1=\"9.1993189\" y1=\"8.776825\" x2=\"13.301533\" y2=\"5.3696747\" style=\"stroke:#ffffff;stroke-width:0.81771719\"/>"
              "<line id=\"arm-br\" x1=\"9.1993189\" y1=\"8.776825\" x2=\"11.788756\" y2=\"12.51107\" style=\"stroke:#ffffff;stroke-width:0.81771719\"/>"
              "<circle id=\"node-c\" cx=\"9.1993189\" cy=\"8.776825\" r=\"1.22\" style=\"fill:#ffffff\"/>"
              "<circle id=\"node-l\" cx=\"2.262351\" cy=\"9.4173737\" r=\"1.22\" style=\"fill:#ffffff\"/>"
              "<circle id=\"node-tr\" cx=\"13.301533\" cy=\"5.3696747\" r=\"1.22\" style=\"fill:#ffffff\"/>"
              "<circle id=\"node-br\" cx=\"11.788756\" cy=\"12.51107\" r=\"1.22\" style=\"fill:#ffffff\"/>"
          "</g>") %
          (emblemData.isEmpty() ? QString() : emblemData) % QStringLiteral(
      "</svg>"
    )).toUtf8();
    // clang-format on
}

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

StatusIconSettings::StatusIconSettings()
    : defaultColor({ QStringLiteral("#26B6DB"), QStringLiteral("#0882C8") })
    , errorColor({ QStringLiteral("#DB3C26"), QStringLiteral("#C80828") })
    , warningColor({ QStringLiteral("#c9ce3b"), QStringLiteral("#ebb83b") })
    , idleColor({ QStringLiteral("#2D9D69"), QStringLiteral("#2D9D69") })
    , scanningColor({ QStringLiteral("#26B6DB"), QStringLiteral("#0882C8") })
    , synchronizingColor({ QStringLiteral("#26B6DB"), QStringLiteral("#0882C8") })
    , pausedColor({ QStringLiteral("#A9A9A9"), QStringLiteral("#58656C") })
    , disconnectedColor({ QStringLiteral("#A9A9A9"), QStringLiteral("#58656C") })
{
}

std::vector<StatusIconSettings::ColorMapping> StatusIconSettings::colorMapping()
{
    return std::vector<ColorMapping>({
        { QStringLiteral("Default"), StatusEmblem::None, defaultColor },
        { QStringLiteral("Error"), StatusEmblem::Alert, errorColor },
        { QStringLiteral("Warning"), StatusEmblem::Alert, warningColor },
        { QStringLiteral("Idle"), StatusEmblem::None, idleColor },
        { QStringLiteral("Scanning"), StatusEmblem::Scanning, scanningColor },
        { QStringLiteral("Synchronizing"), StatusEmblem::Synchronizing, synchronizingColor },
        { QStringLiteral("Paused"), StatusEmblem::Paused, pausedColor },
        { QStringLiteral("Disconnected"), StatusEmblem::None, disconnectedColor },
    });
}

StatusIconSettings::StatusIconSettings(const QString &str)
    : StatusIconSettings()
{
    const auto parts = str.splitRef(QChar(';'));
    int index = 0;
    for (auto *field :
        { &defaultColor, &errorColor, &warningColor, &idleColor, &scanningColor, &synchronizingColor, &pausedColor, &disconnectedColor }) {
        if (index >= parts.size()) {
            break;
        }
        const auto colors = parts[index].split(QChar(','));
        if (colors.size() >= 2) {
            field->start = colors[0].toString();
            field->end = colors[1].toString();
        }
        ++index;
    }
}

QString StatusIconSettings::toString() const
{
    QString res;
    res.reserve(128);
    for (auto *field :
        { &defaultColor, &errorColor, &warningColor, &idleColor, &scanningColor, &synchronizingColor, &pausedColor, &disconnectedColor }) {
        if (!res.isEmpty()) {
            res += QChar(';');
        }
        res += field->start.name(QColor::HexArgb) % QChar(',') % field->end.name(QColor::HexArgb);
    }
    return res;
}

StatusIcons::StatusIcons(const StatusIconSettings &settings)
    : disconnected(QIcon(renderSvgImage(makeSyncthingIcon(settings.disconnectedColor, StatusEmblem::None))))
    , idling(QIcon(renderSvgImage(makeSyncthingIcon(settings.idleColor, StatusEmblem::None))))
    , scanninig(QIcon(renderSvgImage(makeSyncthingIcon(settings.scanningColor, StatusEmblem::Scanning))))
    , notify(QIcon(renderSvgImage(makeSyncthingIcon(settings.warningColor, StatusEmblem::Alert))))
    , pause(QIcon(renderSvgImage(makeSyncthingIcon(settings.pausedColor, StatusEmblem::Paused))))
    , sync(QIcon(renderSvgImage(makeSyncthingIcon(settings.synchronizingColor, StatusEmblem::Synchronizing))))
    , syncComplete(QIcon(renderSvgImage(makeSyncthingIcon(settings.defaultColor, StatusEmblem::Complete))))
    , error(QIcon(renderSvgImage(makeSyncthingIcon(settings.errorColor, StatusEmblem::Alert))))
    , errorSync(QIcon(renderSvgImage(makeSyncthingIcon(settings.errorColor, StatusEmblem::Synchronizing))))
    , newItem(QIcon(renderSvgImage(makeSyncthingIcon(settings.defaultColor, StatusEmblem::Add))))
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

IconManager::IconManager()
    : m_statusIcons()
    , m_fontAwesomeIconsForLightTheme(QColor(10, 10, 10), QSize(64, 64), 8)
    , m_fontAwesomeIconsForDarkTheme(Qt::white, QSize(64, 64), 8)
{
}

IconManager &IconManager::instance()
{
    static IconManager iconManager;
    return iconManager;
}

} // namespace Data
