#include "./quickui.h"

#include "./helpers.h"

#include <qtutilities/misc/desktoputils.h>

#include <QGuiApplication>

#ifdef SYNCTHING_APP_DYNAMIC_STYLE
#include <QQuickStyle>
#endif

#ifdef Q_OS_ANDROID
#include <QDebug>
#include <QFontDatabase>
#include <QJniEnvironment>
#include <QJniObject>

#include <android/font.h>
#include <android/font_matcher.h>

#include <dlfcn.h>
#endif

#include <syncthingmodel/syncthingicons.h>

#include <syncthingconnector/utils.h>

#include <qtquickforkawesome/imageprovider.h>

#include <qtutilities/misc/desktoputils.h>
#include <qtutilities/settingsdialog/qtsettings.h>

#include <c++utilities/conversion/stringconversion.h>

#include "resources/config.h"

namespace QtGui {

#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
/*!
 * \macro SYNCTHING_APP_DARK_MODE_FROM_COLOR_SCHEME
 * \brief Configures dark mode depending on the platform.
 * \remarks
 * 1. Some platforms just provide a "dark mode flag", e.g. Windows and Android. Qt can read this flag and
 *    provide a Qt::ColorScheme value. Qt will only populate an appropriate QPalette on some platforms, e.g.
 *    it does on Windows but not on Android. On platforms where Qt does not populate an appropriate palette
 *    we therefore need to go by the Qt::ColorScheme value and populate the QPalette ourselves from the colors
 *    used by the Qt Quick Controls 2 style. This behavior is enabled via SYNCTHING_APP_DARK_MODE_FROM_COLOR_SCHEME
 *    in subsequent code.
 *    Custom icons (Syncthing icons, ForkAwesome icons) are rendered using the text color from the application
 *    QPalette. This is the reason why we still populate the QPalette in this case and don't just ignore it.
 * 2. Some platforms allow the user to configure a custom palette but do *not* provide a "dark mode flag", e.g.
 *    KDE. In this case reading the Qt::ColorScheme value from Qt is useless but QPalette will be populated. We
 *    therefore need to determine whether the current color scheme is dark from the QPalette and set the Qt
 *    Quick Controls 2 style based on that.
 */

/*!
 * \class QuickUI
 * \brief The QuickUI class contains helper functions for the Qt Quick GUI.
 * \remarks This class is available as singleton in Qml code.
 */

QuickUI::QuickUI(QGuiApplication *app, QtUtilities::QtSettings &qtSettings, QQmlEngine *engine, QObject *parent)
    : QObject(parent)
    , m_app(app)
    , m_engine(engine)
    , m_qtSettings(qtSettings)
    , m_faUrlBase(QStringLiteral("image://fa/"))
    , m_imageProvider(nullptr)
    , m_iconSize(SYNCTHING_APP_ICON_SIZE)
    , m_iconWidthDelegate(SYNCTHING_APP_ICON_WIDTH_DELEGATE)
    , m_darkmodeEnabled(false)
    , m_darkColorScheme(false)
    , m_darkPalette(app ? SYNCTHING_APP_IS_PALETTE_DARK(app->palette()) : false)
{
    if (app) {
        app->setWindowIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));

#ifdef SYNCTHING_APP_DARK_MODE_FROM_COLOR_SCHEME
        QtUtilities::onDarkModeChanged([this](bool darkColorScheme) { applyDarkmodeChange(darkColorScheme, m_darkPalette); }, this);
#else
        applyDarkmodeChange(m_darkColorScheme, m_darkPalette);
#endif
    }
}

QuickUI::~QuickUI()
{
}

QuickUI *QuickUI::create(QQmlEngine *qmlEngine, QJSEngine *engine)
{
    auto *const quickUI = dataObjectFromProperty<QuickUI>(qmlEngine, engine);
    if (qmlEngine) {
        auto *const imageProvider = new QtForkAwesome::QuickImageProvider(Data::IconManager::instance().forkAwesomeRenderer());
        connect(quickUI->m_imageProvider = imageProvider, &QObject::destroyed, quickUI, [quickUI]() { quickUI->m_imageProvider = nullptr; });
        qmlEngine->addImageProvider(QStringLiteral("fa"), imageProvider);
    }
    return quickUI;
}

/*!
 * \brief Returns a space-separated list of available Qt Quick GUI modes.
 * \remarks The underlying string is null-terminated.
 */
std::string_view QuickUI::modes()
{
    return GUI_QTQUICK_MODES;
}

/*!
 * \brief Returns the primary/default mode of the Qt Quick GUI.
 * \remarks The underlying string is null-terminated.
 */
std::string_view QuickUI::primaryMode()
{
    return GUI_QTQUICK_PRIMARY_MODE;
}

#if !(defined(Q_OS_ANDROID))
bool QuickUI::windowPopups() const
{
    static const auto enablewindowPopups = [] {
        auto ok = false;
        return qEnvironmentVariableIntValue(PROJECT_VARNAME_UPPER "_WINDOW_POPUPS", &ok) > 0 || !ok;
    }();
    return enablewindowPopups;
}
#endif

void QuickUI::addDialog(QObject *dialog)
{
    m_dialogs.append(dialog);
    connect(dialog, &QObject::destroyed, this, &QuickUI::removeDialog);
}

void QuickUI::removeDialog(QObject *dialog)
{
    disconnect(dialog, &QObject::destroyed, this, &QuickUI::removeDialog);
    m_dialogs.removeAll(dialog);
}

#ifdef Q_OS_ANDROID
#define REQUIRES_ANDROID_API(x) __attribute__((__availability__(android, introduced = x)))
#define ANDROID_API_AT_LEAST(x) __builtin_available(android x, *)

struct FontFunctions {
    explicit FontFunctions(void *libandroid) REQUIRES_ANDROID_API(29)
        : FontMatcher_create(reinterpret_cast<decltype(&AFontMatcher_create)>(dlsym(libandroid, "AFontMatcher_create")))
        , FontMatcher_destroy(reinterpret_cast<decltype(&AFontMatcher_destroy)>(dlsym(libandroid, "AFontMatcher_destroy")))
        , FontMatcher_match(reinterpret_cast<decltype(&AFontMatcher_match)>(dlsym(libandroid, "AFontMatcher_match")))
        , FontMatcher_setStyle(reinterpret_cast<decltype(&AFontMatcher_setStyle)>(dlsym(libandroid, "AFontMatcher_setStyle")))
        , Font_getFontFilePath(reinterpret_cast<decltype(&AFont_getFontFilePath)>(dlsym(libandroid, "AFont_getFontFilePath")))
        , Font_close(reinterpret_cast<decltype(&AFont_close)>(dlsym(libandroid, "AFont_close")))
    {
    }
    operator bool() const REQUIRES_ANDROID_API(29)
    {
        return FontMatcher_create && FontMatcher_destroy && FontMatcher_match && FontMatcher_setStyle && Font_getFontFilePath && Font_close;
    }
    decltype(&AFontMatcher_create) FontMatcher_create REQUIRES_ANDROID_API(29);
    decltype(&AFontMatcher_destroy) FontMatcher_destroy REQUIRES_ANDROID_API(29);
    decltype(&AFontMatcher_match) FontMatcher_match REQUIRES_ANDROID_API(29);
    decltype(&AFontMatcher_setStyle) FontMatcher_setStyle REQUIRES_ANDROID_API(29);
    decltype(&AFont_getFontFilePath) Font_getFontFilePath REQUIRES_ANDROID_API(29);
    decltype(&AFont_close) Font_close REQUIRES_ANDROID_API(29);
};

static void loadSystemFont(std::string_view fontPath, QString &fontFamily)
{
    auto fontFile = QFile(QString::fromUtf8(fontPath.data(), static_cast<qsizetype>(fontPath.size())));
    if (!fontFile.open(QFile::ReadOnly)) {
        qDebug() << "Unable to open font file: " << fontPath;
        return;
    }
    if (const auto fontID = QFontDatabase::addApplicationFontFromData(fontFile.readAll()); fontID != -1) {
        if (const auto families = QFontDatabase::applicationFontFamilies(fontID); !families.isEmpty() && fontFamily.isEmpty()) {
            qDebug() << "Loaded font file: " << fontPath;
            fontFamily = families.front();
        }
    } else {
        qDebug() << "Unable to load font file: " << fontPath;
    }
}

static void matchAndLoadDefaultFont(const FontFunctions &fontFn, AFontMatcher *matcher, std::uint16_t weight, QString &fontFamily)
    REQUIRES_ANDROID_API(34)
{
    fontFn.FontMatcher_setStyle(matcher, weight, false);
    if (auto *const font = fontFn.FontMatcher_match(matcher, "FontFamily.Default", reinterpret_cast<const uint16_t *>(u"foobar"), 3, nullptr)) {
        auto path = std::string_view(fontFn.Font_getFontFilePath(font));
        qDebug() << "Found system font: " << path;
        loadSystemFont(path, fontFamily);
        fontFn.Font_close(font);
    }
}

/*!
 * \brief Loads the default system font on Android 14 and newer.
 * \remarks The API would be available as of Android 10 (API 29) but using it leads to crashes. Not sure about
 *          Android 11, 12 and 13 so only enabling this as of Android 14.
 */
static QString loadDefaultSystemFonts()
{
    auto defaultSystemFontFamily = QString();
    if (ANDROID_API_AT_LEAST(34)) {
        qDebug() << "Loading default system fonts";
        auto *const libandroid = dlopen("libandroid.so", RTLD_LOCAL);
        if (!libandroid) {
            qDebug() << "Failed to open libandroid.so: " << dlerror();
            return defaultSystemFontFamily;
        }
        const auto fontFn = FontFunctions(libandroid);
        if (!fontFn) {
            qDebug() << "Failed loading font functions in libandroid.so.";
            dlclose(libandroid);
            return defaultSystemFontFamily;
        }
        if (auto *const m = fontFn.FontMatcher_create()) {
            qDebug() << "Instantiated matcher";
            matchAndLoadDefaultFont(fontFn, m, AFONT_WEIGHT_NORMAL, defaultSystemFontFamily);
            matchAndLoadDefaultFont(fontFn, m, AFONT_WEIGHT_THIN, defaultSystemFontFamily);
            matchAndLoadDefaultFont(fontFn, m, AFONT_WEIGHT_LIGHT, defaultSystemFontFamily);
            matchAndLoadDefaultFont(fontFn, m, AFONT_WEIGHT_MEDIUM, defaultSystemFontFamily);
            matchAndLoadDefaultFont(fontFn, m, AFONT_WEIGHT_BOLD, defaultSystemFontFamily);
            fontFn.FontMatcher_destroy(m);
        }
        dlclose(libandroid);
    }
    return defaultSystemFontFamily;
}
#endif

QString QuickUI::fontFamily() const
{
#ifdef Q_OS_ANDROID
    static const auto defaultSystemFontFamily = loadDefaultSystemFonts();
    if (defaultSystemFontFamily.isEmpty()) {
        qDebug() << "Unable to determine/load system font family.";
    } else {
        qDebug() << "System font family: " << defaultSystemFontFamily;
        return defaultSystemFontFamily;
    }
#endif
    return QString();
}

qreal QuickUI::fontScale() const
{
#ifdef Q_OS_ANDROID
    return qreal(QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<jfloat>("fontScale", "()F"));
#else
    return qreal(1.0);
#endif
}

int QuickUI::fontWeightAdjustment() const
{
#ifdef Q_OS_ANDROID
    return QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<jint>("fontWeightAdjustment", "()I");
#else
    return 0;
#endif
}

/*!
 * \brief Return the font applying extra settings where necessary.
 * \remarks On some platforms like on Android this is required because Qt does not read these settings.
 */
QFont QuickUI::font() const
{
    auto f = QFont();
    if (const auto family = fontFamily(); !family.isEmpty()) {
        f.setFamily(family);
    }
    if (const auto scale = fontScale(); scale != qreal(1.0)) {
        f.setPixelSize(static_cast<int>(static_cast<qreal>(f.pixelSize()) * scale));
        f.setPointSizeF(f.pointSizeF() * scale);
    }
    if (const auto weightAdjustment = fontWeightAdjustment(); weightAdjustment != 0) {
        f.setWeight(static_cast<QFont::Weight>(f.weight() + weightAdjustment));
    }
    return f;
}

void QuickUI::setPalette(const QColor &foreground, const QColor &background)
{
#ifdef SYNCTHING_APP_DARK_MODE_FROM_COLOR_SCHEME
    if (m_app) {
        auto palette = m_app->palette();
        palette.setColor(QPalette::Active, QPalette::Text, foreground);
        palette.setColor(QPalette::Active, QPalette::Base, background);
        palette.setColor(QPalette::Active, QPalette::WindowText, foreground);
        palette.setColor(QPalette::Active, QPalette::Window, background);
        m_app->setPalette(palette);
    }
#else
    Q_UNUSED(foreground)
    Q_UNUSED(background)
#endif
}

void QuickUI::applyDarkmodeChange(const QPalette &palette)
{
    applyDarkmodeChange(m_darkColorScheme, SYNCTHING_APP_IS_PALETTE_DARK(palette));
}

void QuickUI::applyDarkmodeChange(bool isDarkColorSchemeEnabled, bool isDarkPaletteEnabled)
{
    m_darkColorScheme = isDarkColorSchemeEnabled;
    m_darkPalette = isDarkPaletteEnabled;
    const auto isDarkmodeEnabled = m_darkColorScheme || m_darkPalette;
    m_qtSettings.reapplyDefaultIconTheme(isDarkmodeEnabled);
    if (isDarkmodeEnabled == m_darkmodeEnabled) {
        return;
    }
    qDebug() << "Darkmode has changed: " << isDarkmodeEnabled;
    m_darkmodeEnabled = isDarkmodeEnabled;
    emit darkmodeEnabledChanged(isDarkmodeEnabled);
}

bool QuickUI::performHapticFeedback()
{
#ifdef Q_OS_ANDROID
    return QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<jboolean>("performHapticFeedback");
#else
    return false;
#endif
}

bool QuickUI::showError(const QString &errorMessage)
{
    emit error(errorMessage);
    return true;
}

bool QuickUI::showToast(const QString &message)
{
#ifdef Q_OS_ANDROID
    return QJniObject(QNativeInterface::QAndroidApplication::context())
        .callMethod<jboolean>("showToast", "(Ljava/lang/String;)Z", QJniObject::fromString(message));
#else
    Q_UNUSED(message)
    return false;
#endif
}
#endif

} // namespace QtGui
