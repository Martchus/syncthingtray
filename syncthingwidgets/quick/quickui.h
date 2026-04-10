#ifndef SYNCTHINGWIDGETS_QUICK_UI_H
#define SYNCTHINGWIDGETS_QUICK_UI_H

#include "../global.h"

#include <QJSValue>
#include <QObject>
#include <QPalette>
#include <QVariantMap>

#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK_MODE_DESKTOP
#include <QQmlApplicationEngine>
#endif

#include <QtQmlIntegration/qqmlintegration.h>

QT_FORWARD_DECLARE_CLASS(QGuiApplication)
QT_FORWARD_DECLARE_CLASS(QQmlEngine)
QT_FORWARD_DECLARE_CLASS(QQuickItem)
QT_FORWARD_DECLARE_CLASS(QJSEngine)

namespace QtForkAwesome {
class QuickImageProvider;
}

namespace QtUtilities {
class QtSettings;
}

namespace QtGui {

#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
#ifdef Q_OS_ANDROID
#define SYNCTHING_APP_DARK_MODE_FROM_COLOR_SCHEME
#endif
#ifdef SYNCTHING_APP_DARK_MODE_FROM_COLOR_SCHEME
#define SYNCTHING_APP_IS_PALETTE_DARK(palette) false
#else
#define SYNCTHING_APP_IS_PALETTE_DARK(palette) QtUtilities::isPaletteDark(palette)
#endif

class SYNCTHINGWIDGETS_EXPORT QuickUI : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString faUrlBase READ faUrlBase CONSTANT)
    Q_PROPERTY(QString mode READ mode CONSTANT)
    Q_PROPERTY(bool desktop READ isDesktop CONSTANT)
    Q_PROPERTY(bool darkmodeEnabled READ isDarkmodeEnabled NOTIFY darkmodeEnabledChanged)
    Q_PROPERTY(int iconSize READ iconSize CONSTANT)
    Q_PROPERTY(int iconWidthDelegate READ iconWidthDelegate CONSTANT)
    Q_PROPERTY(bool windowPopups READ windowPopups CONSTANT)
    Q_PROPERTY(bool extendedClientArea READ extendedClientArea CONSTANT)
    Q_PROPERTY(QString fontFamily READ fontFamily CONSTANT)
    Q_PROPERTY(qreal fontScale READ fontScale CONSTANT)
    Q_PROPERTY(int fontWeightAdjustment READ fontWeightAdjustment CONSTANT)
    Q_PROPERTY(QFont font READ font CONSTANT)
    Q_PROPERTY(QObject *currentDialog READ currentDialog)
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit QuickUI(QGuiApplication *app, QtUtilities::QtSettings &qtSettings, QQmlEngine *engine = nullptr, const QString &mode = QString(),
        QObject *parent = nullptr);
    ~QuickUI() override;
    static QuickUI *create(QQmlEngine *, QJSEngine *engine);
    static std::string_view modes();
    static std::string_view primaryMode();

    void setEngine(QQmlEngine *engine)
    {
        m_engine = engine;
    }

    const QString &faUrlBase() const
    {
        return m_faUrlBase;
    }
    const QString &mode() const
    {
        return m_mode;
    }
    bool isDesktop() const
    {
        return m_mode == QStringLiteral("desktop");
    }
    QtForkAwesome::QuickImageProvider *imageProvider()
    {
        return m_imageProvider;
    }

#if defined(Q_OS_ANDROID)
    static constexpr bool windowPopups()
    {
        return false;
    }
#else
    bool windowPopups() const;
#endif
    static constexpr bool extendedClientArea()
    {
        // disable extended client areas by default as further tweaking is needed
        // note: Safe areas are not working as expected with Qt 6.9.2 at all but this is fixed with Qt 6.9.3.
#ifdef SYNCTHINGWIDGETS_EXTENDED_CLIENT_AREA
        return true;
#else
        return false;
#endif
    }
    /*!
     * \brief Returns whether darkmode is enabled.
     * \remarks
     * The QML code could just use "Qt.styleHints.colorScheme === Qt.Dark" but this would not be consistent with the approach in
     * QtSettings that also takes the palette into account for platforms without darkmode flag.
     */
    bool isDarkmodeEnabled() const
    {
        return m_darkmodeEnabled;
    }

    int iconSize() const
    {
        return m_iconSize;
    }
    int iconWidthDelegate() const
    {
        return m_iconWidthDelegate;
    }
    QObject *currentDialog()
    {
        return m_dialogs.isEmpty() ? nullptr : m_dialogs.back();
    }

    QString fontFamily() const;
    qreal fontScale() const;
    int fontWeightAdjustment() const;
    QFont font() const;

    Q_INVOKABLE void addDialog(QObject *dialog);
    Q_INVOKABLE void removeDialog(QObject *dialog);
    Q_INVOKABLE void setPalette(const QColor &foreground, const QColor &background);
    Q_INVOKABLE void applyDarkmodeChange(const QPalette &palette);
    Q_INVOKABLE void applyDarkmodeChange(bool isDarkColorSchemeEnabled, bool isDarkPaletteEnabled);
    Q_INVOKABLE bool performHapticFeedback();
    Q_INVOKABLE bool showError(const QString &errorMessage);
    Q_INVOKABLE bool showToast(const QString &message);
    Q_INVOKABLE void requestOpeningUrl(const QUrl &url)
    {
        emit openingUrlRequested(url);
    }
    Q_INVOKABLE bool showMainWindow();
    Q_INVOKABLE bool showPage(
        QAnyStringView uri, QAnyStringView typeName, const QVariantMap &initialProperties = QVariantMap(), QQuickItem *stackView = nullptr);
    Q_INVOKABLE bool editDir(const QString &dirId, const QString &dirName, QQuickItem *stackView = nullptr);
    Q_INVOKABLE bool editDev(const QString &devId, const QString &devName, QQuickItem *stackView = nullptr);
    Q_INVOKABLE QObject *loadComponent(QAnyStringView uri, QAnyStringView typeName, const QVariantMap &initialProperties = QVariantMap());

Q_SIGNALS:
    void darkmodeEnabledChanged(bool darkmodeEnabled);
    void error(const QString &errorMessage, const QString &details = QString());
    void openingUrlRequested(const QUrl &url);

private:
    QGuiApplication *m_app;
    QQmlEngine *m_engine;
    QtUtilities::QtSettings &m_qtSettings;
    QString m_faUrlBase;
    QString m_mode;
    QtForkAwesome::QuickImageProvider *m_imageProvider;
    QObjectList m_dialogs;
    int m_iconSize;
    int m_iconWidthDelegate;
    bool m_darkmodeEnabled;
    bool m_darkColorScheme;
    bool m_darkPalette;
};

#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK_MODE_DESKTOP
struct QuickGuiEngine {
    explicit QuickGuiEngine(QGuiApplication *app, QtUtilities::QtSettings &qtSettings)
        : engine()
        , ui(app, qtSettings, &engine, QStringLiteral("desktop"))
    {
    }
    QQmlApplicationEngine engine;
    QuickUI ui;
};
#endif

#endif

} // namespace QtGui

#endif // SYNCTHINGWIDGETS_QUICK_UI_H
