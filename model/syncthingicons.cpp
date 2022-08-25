#include "./syncthingicons.h"

#include "resources/qtconfig.h"

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

#include <qtutilities/misc/compat.h>

#include <qtforkawesome/icon.h>

#include <QFile>
#include <QGuiApplication>
#include <QPainter>
#include <QPalette>
#include <QStringBuilder>
#include <QSvgRenderer>

#ifndef LIB_SYNCTHING_MODEL_STATIC
ENABLE_QT_RESOURCES_OF_STATIC_DEPENDENCIES
#endif

namespace Data {

/*!
 * \brief Generates the SVG code for the Syncthing icon with the specified \a colors and status emblem.
 */
QByteArray makeSyncthingIcon(const StatusIconColorSet &colors, StatusEmblem statusEmblem)
{
    // serialize colors
    auto gradientStartColor = colors.backgroundStart.name(QColor::HexRgb);
    auto gradientEndColor = colors.backgroundEnd.name(QColor::HexRgb);
    constexpr auto opaque = static_cast<decltype(colors.backgroundStart.alphaF())>(1.0);
    if (colors.backgroundStart.alphaF() < opaque) {
        gradientStartColor += QStringLiteral(";stop-opacity:") + QString::number(static_cast<double>(colors.backgroundStart.alphaF()));
    }
    if (colors.backgroundEnd.alphaF() < opaque) {
        gradientEndColor += QStringLiteral(";stop-opacity:") + QString::number(static_cast<double>(colors.backgroundEnd.alphaF()));
    }
    auto fillColor = colors.foreground.name(QColor::HexRgb);
    auto strokeColor = fillColor;
    if (colors.foreground.alphaF() < opaque) {
        const auto alpha = QString::number(static_cast<double>(colors.foreground.alphaF()));
        fillColor += QStringLiteral(";fill-opacity:") + alpha;
        strokeColor += QStringLiteral(";stroke-opacity:") + alpha;
    }

    // make SVG document
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
    return (QStringLiteral(
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<svg xmlns:svg=\"http://www.w3.org/2000/svg\" xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 16 16\">"
          "<defs>"
              "<linearGradient id=\"grad\" gradientUnits=\"userSpaceOnUse\" x1=\"8\" y1=\"0\" x2=\"8\" y2=\"16\">"
                  "<stop offset=\"0\" style=\"stop-color:") % gradientStartColor % QStringLiteral("\"/>"
                  "<stop offset=\"1\" style=\"stop-color:") % gradientEndColor % QStringLiteral("\"/>"
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
              "<circle id=\"inner\" cx=\"8\" cy=\"7.9727402\" r=\"5.9557071\" style=\"fill:none;stroke:") % strokeColor % QStringLiteral(";stroke-width:0.81771719\"/>"
              "<line id=\"arm-l\" x1=\"9.1993189\" y1=\"8.776825\" x2=\"2.262351\" y2=\"9.4173737\" style=\"stroke:") % strokeColor % QStringLiteral(";stroke-width:0.81771719\"/>"
              "<line id=\"arm-tr\" x1=\"9.1993189\" y1=\"8.776825\" x2=\"13.301533\" y2=\"5.3696747\" style=\"stroke:") % strokeColor % QStringLiteral(";stroke-width:0.81771719\"/>"
              "<line id=\"arm-br\" x1=\"9.1993189\" y1=\"8.776825\" x2=\"11.788756\" y2=\"12.51107\" style=\"stroke:") % strokeColor % QStringLiteral(";stroke-width:0.81771719\"/>"
              "<circle id=\"node-c\" cx=\"9.1993189\" cy=\"8.776825\" r=\"1.22\" style=\"fill:") % fillColor % QStringLiteral("\"/>"
              "<circle id=\"node-l\" cx=\"2.262351\" cy=\"9.4173737\" r=\"1.22\" style=\"fill:") % fillColor % QStringLiteral("\"/>"
              "<circle id=\"node-tr\" cx=\"13.301533\" cy=\"5.3696747\" r=\"1.22\" style=\"fill:") % fillColor % QStringLiteral("\"/>"
              "<circle id=\"node-br\" cx=\"11.788756\" cy=\"12.51107\" r=\"1.22\" style=\"fill:") % fillColor % QStringLiteral("\"/>"
          "</g>") %
          (emblemData.isEmpty() ? QString() : emblemData) % QStringLiteral(
      "</svg>"
    )).toUtf8();
    // clang-format on
}

/// \cond
namespace Detail {
template <typename SourceType> QPixmap renderSvgImage(const SourceType &source, const QSize &givenSize, int margin)
{
    const auto scaleFactor =
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        !QCoreApplication::testAttribute(Qt::AA_UseHighDpiPixmaps) ? 1.0 :
#endif
                                                                   qGuiApp->devicePixelRatio();
    const auto scaledSize = QSize(givenSize * scaleFactor);
    auto renderer = QSvgRenderer(source);
    auto renderSize = QSize(renderer.defaultSize());
    renderSize.scale(scaledSize.width() - margin, scaledSize.height() - margin, Qt::KeepAspectRatio);
    auto renderBounds = QRect(QPoint(), scaledSize);
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
    auto pm = QPixmap(scaledSize);
    pm.fill(QColor(Qt::transparent));
    auto painter = QPainter(&pm);
    renderer.render(&painter, renderBounds);
    pm.setDevicePixelRatio(scaleFactor);
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

StatusIconSettings::StatusIconSettings()
    : defaultColor({ QStringLiteral("#26B6DB"), QStringLiteral("#0882C8"), QStringLiteral("#FFFFFF") })
    , errorColor({ QStringLiteral("#DB3C26"), QStringLiteral("#C80828"), QStringLiteral("#FFFFFF") })
    , warningColor({ QStringLiteral("#c9ce3b"), QStringLiteral("#ebb83b"), QStringLiteral("#FFFFFF") })
    , idleColor({ QStringLiteral("#2D9D69"), QStringLiteral("#2D9D69"), QStringLiteral("#FFFFFF") })
    , scanningColor({ QStringLiteral("#26B6DB"), QStringLiteral("#0882C8"), QStringLiteral("#FFFFFF") })
    , synchronizingColor({ QStringLiteral("#26B6DB"), QStringLiteral("#0882C8"), QStringLiteral("#FFFFFF") })
    , pausedColor({ QStringLiteral("#A9A9A9"), QStringLiteral("#58656C"), QStringLiteral("#FFFFFF") })
    , disconnectedColor({ QStringLiteral("#A9A9A9"), QStringLiteral("#58656C"), QStringLiteral("#FFFFFF") })
{
}

StatusIconSettings::StatusIconSettings(StatusIconSettings::DarkTheme)
    : defaultColor({ QStringLiteral("#00000000"), QStringLiteral("#00000000"), QStringLiteral("#FFFFFFFF") })
    , errorColor({ QStringLiteral("#00000000"), QStringLiteral("#00000000"), QStringLiteral("#FFFFAEA5") })
    , warningColor({ QStringLiteral("#00FFFFFF"), QStringLiteral("#00FFFFFF"), QStringLiteral("#FFFFF6A5") })
    , idleColor({ QStringLiteral("#00FFFFFF"), QStringLiteral("#00FFFFFF"), QStringLiteral("#FFFFFFFF") })
    , scanningColor({ QStringLiteral("#00FFFFFF"), QStringLiteral("#00FFFFFF"), QStringLiteral("#FFA5EFFF") })
    , synchronizingColor({ QStringLiteral("#00FFFFFF"), QStringLiteral("#00FFFFFF"), QStringLiteral("#FFA5EFFF") })
    , pausedColor({ QStringLiteral("#00000000"), QStringLiteral("#00FFFFFF"), QStringLiteral("#FFA7A7A7") })
    , disconnectedColor({ QStringLiteral("#00000000"), QStringLiteral("#00000000"), QStringLiteral("#FFA7A7A7") })
{
}

StatusIconSettings::StatusIconSettings(StatusIconSettings::BrightTheme)
    : defaultColor({ QStringLiteral("#00FFFFFF"), QStringLiteral("#00FFFFFF"), QStringLiteral("#FF000000") })
    , errorColor({ QStringLiteral("#00FFFFFF"), QStringLiteral("#00FFFFFF"), QStringLiteral("#FFDB3C26") })
    , warningColor({ QStringLiteral("#00000000"), QStringLiteral("#00000000"), QStringLiteral("#FFC9CE3B") })
    , idleColor({ QStringLiteral("#00000000"), QStringLiteral("#00000000"), QStringLiteral("#FF000000") })
    , scanningColor({ QStringLiteral("#00000000"), QStringLiteral("#00000000"), QStringLiteral("#FF26B6DB") })
    , synchronizingColor({ QStringLiteral("#00000000"), QStringLiteral("#00000000"), QStringLiteral("#FF26B6DB") })
    , pausedColor({ QStringLiteral("#00FFFFFF"), QStringLiteral("#00FFFFFF"), QStringLiteral("#FFA7A7A7") })
    , disconnectedColor({ QStringLiteral("#00FFFFFF"), QStringLiteral("#00FFFFFF"), QStringLiteral("#FFA7A7A7") })
{
}

std::vector<StatusIconSettings::ColorMapping> StatusIconSettings::colorMapping()
{
    return std::vector<ColorMapping>({
        { QCoreApplication::translate("Data::StatusIconSettings", "Misc. notifications"), StatusEmblem::Complete, defaultColor },
        { QCoreApplication::translate("Data::StatusIconSettings", "Error"), StatusEmblem::Alert, errorColor },
        { QCoreApplication::translate("Data::StatusIconSettings", "Warning"), StatusEmblem::Alert, warningColor },
        { QCoreApplication::translate("Data::StatusIconSettings", "Idle"), StatusEmblem::None, idleColor },
        { QCoreApplication::translate("Data::StatusIconSettings", "Scanning"), StatusEmblem::Scanning, scanningColor },
        { QCoreApplication::translate("Data::StatusIconSettings", "Synchronizing"), StatusEmblem::Synchronizing, synchronizingColor },
        { QCoreApplication::translate("Data::StatusIconSettings", "Paused"), StatusEmblem::Paused, pausedColor },
        { QCoreApplication::translate("Data::StatusIconSettings", "Disconnected"), StatusEmblem::None, disconnectedColor },
    });
}

StatusIconSettings::StatusIconSettings(const QString &str)
    : StatusIconSettings()
{
    const auto parts = QtUtilities::splitRef(str, QChar(';'));
    int index = 0;
    for (auto *field :
        { &defaultColor, &errorColor, &warningColor, &idleColor, &scanningColor, &synchronizingColor, &pausedColor, &disconnectedColor }) {
        if (index >= parts.size()) {
            break;
        }
        const auto colors = parts[index].split(QChar(','));
        if (colors.size() >= 2) {
            field->backgroundStart = colors[0].toString();
            field->backgroundEnd = colors[1].toString();
        }
        if (colors.size() >= 3) {
            field->foreground = colors[2].toString();
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
        res += field->backgroundStart.name(QColor::HexArgb) % QChar(',') % field->backgroundEnd.name(QColor::HexArgb) % QChar(',')
            % field->foreground.name(QColor::HexArgb);
    }
    return res;
}

StatusIcons::StatusIcons(const StatusIconSettings &settings)
    : disconnected(QIcon(renderSvgImage(makeSyncthingIcon(settings.disconnectedColor, StatusEmblem::None), settings.renderSize)))
    , idling(QIcon(renderSvgImage(makeSyncthingIcon(settings.idleColor, StatusEmblem::None), settings.renderSize)))
    , scanninig(QIcon(renderSvgImage(makeSyncthingIcon(settings.scanningColor, StatusEmblem::Scanning), settings.renderSize)))
    , notify(QIcon(renderSvgImage(makeSyncthingIcon(settings.warningColor, StatusEmblem::Alert), settings.renderSize)))
    , pause(QIcon(renderSvgImage(makeSyncthingIcon(settings.pausedColor, StatusEmblem::Paused), settings.renderSize)))
    , sync(QIcon(renderSvgImage(makeSyncthingIcon(settings.synchronizingColor, StatusEmblem::Synchronizing), settings.renderSize)))
    , syncComplete(QIcon(renderSvgImage(makeSyncthingIcon(settings.defaultColor, StatusEmblem::Complete), settings.renderSize)))
    , error(QIcon(renderSvgImage(makeSyncthingIcon(settings.errorColor, StatusEmblem::Alert), settings.renderSize)))
    , errorSync(QIcon(renderSvgImage(makeSyncthingIcon(settings.errorColor, StatusEmblem::Synchronizing), settings.renderSize)))
    , newItem(QIcon(renderSvgImage(makeSyncthingIcon(settings.defaultColor, StatusEmblem::Add), settings.renderSize)))
    , isValid(true)
{
}

ForkAwesomeIcons::ForkAwesomeIcons(QtForkAwesome::Renderer &renderer, const QColor &color, const QSize &size)
    : hashtag(renderer.pixmap(QtForkAwesome::Icon::Hashtag, size, color))
    , folderOpen(renderer.pixmap(QtForkAwesome::Icon::FolderOpen, size, color))
    , globe(renderer.pixmap(QtForkAwesome::Icon::Globe, size, color))
    , home(renderer.pixmap(QtForkAwesome::Icon::Home, size, color))
    , shareAlt(renderer.pixmap(QtForkAwesome::Icon::ShareAlt, size, color))
    , refresh(renderer.pixmap(QtForkAwesome::Icon::Refresh, size, color))
    , clock(renderer.pixmap(QtForkAwesome::Icon::ClockO, size, color))
    , exchangeAlt(renderer.pixmap(QtForkAwesome::Icon::Exchange, size, color))
    , exclamationTriangle(renderer.pixmap(QtForkAwesome::Icon::ExclamationTriangle, size, color))
    , cogs(renderer.pixmap(QtForkAwesome::Icon::Cogs, size, color))
    , link(renderer.pixmap(QtForkAwesome::Icon::Link, size, color))
    , eye(renderer.pixmap(QtForkAwesome::Icon::Eye, size, color))
    , fileArchive(renderer.pixmap(QtForkAwesome::Icon::FileArchiveO, size, color))
    , folder(renderer.pixmap(QtForkAwesome::Icon::Folder, size, color))
    , certificate(renderer.pixmap(QtForkAwesome::Icon::Certificate, size, color))
    , networkWired(renderer.pixmap(QtForkAwesome::Icon::Sitemap, size, color))
    , cloudDownloadAlt(renderer.pixmap(QtForkAwesome::Icon::CloudDownload, size, color))
    , cloudUploadAlt(renderer.pixmap(QtForkAwesome::Icon::CloudUpload, size, color))
    , tag(renderer.pixmap(QtForkAwesome::Icon::Tag, size, color))
{
}

IconManager::IconManager(const QPalette *palette)
    : m_statusIcons()
    , m_trayIcons(m_statusIcons)
    , m_commonForkAwesomeIcons(
          m_forkAwesomeRenderer, (palette ? *palette : QGuiApplication::palette()).color(QPalette::Normal, QPalette::Text), QSize(64, 64))
{
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    if (!palette) {
        QObject::connect(qGuiApp, &QGuiApplication::paletteChanged, this, &IconManager::setPalette);
    }
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
}

void IconManager::setPalette(const QPalette &palette)
{
    emit forkAwesomeIconsChanged(
        m_commonForkAwesomeIcons = ForkAwesomeIcons(m_forkAwesomeRenderer, palette.color(QPalette::Normal, QPalette::Text), QSize(64, 64)));
}

IconManager &IconManager::instance(const QPalette *palette)
{
    static auto iconManager = IconManager(palette);
    return iconManager;
}

QString aboutDialogAttribution()
{
    return QStringLiteral(
        "<p>Developed by " APP_AUTHOR "<br>Fallback icons from <a href=\"https://invent.kde.org/frameworks/breeze-icons\">KDE/Breeze</a> "
        "project (copyright © 2014 Uri Herrera <uri_herrera@nitrux.in> and others, see the according "
        "<a href=\"" APP_URL "/blob/master/LICENSE.LESSER\">LGPL-3.0 license</a>)"
        "<br>Syncthing icons from <a href=\"https://syncthing.net\">Syncthing project</a> "
        "(copyright © 2014-2016 The Syncthing Authors, see the according "
        "<a href=\"" APP_URL "/blob/master/LICENSE.MPL-2.0\">MPL-2.0 license</a>)"
        "<br>Using icons from <a href=\"https://forkaweso.me\">Fork "
        "Awesome</a> (see <a href=\"https://forkaweso.me/Fork-Awesome/license\">their license</a>)</p>");
}

QImage aboutDialogImage()
{
    return renderSvgImage(makeSyncthingIcon(), QSize(128, 128)).toImage();
}

} // namespace Data
