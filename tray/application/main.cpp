#ifdef GUI_QTWIDGETS
#include "./singleinstance.h"

#include "../gui/trayicon.h"
#include "../gui/traywidget.h"
#ifdef SYNCTHINGTRAY_SETUP_TOOLS_ENABLED
#include "../gui/helper.h"
#endif
#endif

#include <syncthingwidgets/misc/syncthinglauncher.h>
#ifdef GUI_QTWIDGETS
#include <syncthingwidgets/settings/settings.h>
#include <syncthingwidgets/settings/settingsdialog.h>
#include <syncthingwidgets/webview/webviewdialog.h>
#endif
#ifdef GUI_QTQUICK
#include <syncthingwidgets/quick/app.h>
#include <syncthingwidgets/quick/appservice.h>
#endif

#include <syncthingmodel/syncthingicons.h>

#include <syncthingconnector/syncthingprocess.h>
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
#include <syncthingconnector/syncthingservice.h>
#endif

#ifdef SYNCTHINGTRAY_USE_LIBSYNCTHING
#include <syncthing/interface.h>
#endif

#include "resources/config.h"
#include "resources/qtconfig.h"

#include <c++utilities/application/argumentparser.h>
#include <c++utilities/application/commandlineutils.h>
#include <c++utilities/io/ansiescapecodes.h>
#include <c++utilities/misc/parseerror.h>

#include <qtutilities/misc/dialogutils.h>
#include <qtutilities/resources/importplugin.h>
#include <qtutilities/resources/qtconfigarguments.h>
#include <qtutilities/resources/resources.h>
#include <qtutilities/settingsdialog/qtsettings.h>

#ifdef SYNCTHINGTRAY_SETUP_TOOLS_ENABLED
#include <c++utilities/misc/signingkeys.h>
#include <c++utilities/misc/verification.h>
#include <qtutilities/setup/updater.h>
#endif

#include <QNetworkAccessManager>
#include <QSettings>
#include <QStringBuilder>

#ifdef GUI_QTWIDGETS
#include <QApplication>
#include <QMessageBox>
using QtApp = QApplication;
#else
#include <QGuiApplication>
using QtApp = QGuiApplication;
#endif

#ifdef GUI_QTQUICK
#ifdef SYNCTHINGTRAY_HAS_WEBVIEW
#include <QtWebView/QtWebView>
#endif

#ifdef SYNCTHINGTRAY_HAS_WEBVIEW_PAGE
#include <QQmlEngineExtensionPlugin>
Q_IMPORT_QML_PLUGIN(WebViewItemPlugin)
#endif
#endif

#ifdef SYNCTHINGWIDGETS_HAS_SCHEME_HANDLER
#include <QWebEngineUrlScheme>
#endif

#include <iostream>

#ifdef Q_OS_ANDROID
#include <QDebug>
#include <QLocale>
#include <QSslSocket>
#include <QtCore/private/qandroidextras_p.h>
#endif

using namespace std;
using namespace CppUtilities;
using namespace QtGui;
using namespace Data;

// import static plugins
#include <QtPlugin>
#if defined(QT_FORK_AWESOME_ICON_ENGINE_STATIC)
Q_IMPORT_PLUGIN(ForkAwesomeIconEnginePlugin)
#endif
#if defined(SYNCTHINGWIDGETS_STATIC) && defined(GUI_QTQUICK)
Q_IMPORT_PLUGIN(MainPlugin)
#endif
ENABLE_QT_RESOURCES_OF_STATIC_DEPENDENCIES

#ifdef GUI_QTWIDGETS

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
static void handleSystemdServiceError(const QString &context, const QString &name, const QString &message)
{
    auto *const msgBox = new QMessageBox;
    msgBox->setAttribute(Qt::WA_DeleteOnClose);
    msgBox->setIcon(QMessageBox::Critical);
    msgBox->setText(QCoreApplication::translate("main", "Unable to ") + context);
    msgBox->setInformativeText(name % QStringLiteral(":\n") % message);
    msgBox->show();
}
#endif

// define public key and signature extension depending on verification backend
#ifdef SYNCTHINGTRAY_SETUP_TOOLS_ENABLED
#ifdef SYNCTHINGTRAY_USE_LIBSYNCTHING
#define SYNCTHINGTRAY_SIGNATURE_EXTENSION ".stsigtool.sig"
#define SYNCTHINGTRAY_SIGNING_KEYS SigningKeys::stsigtool
#else
#define SYNCTHINGTRAY_SIGNATURE_EXTENSION ".openssl.sig"
#define SYNCTHINGTRAY_SIGNING_KEYS SigningKeys::openssl
#endif
#endif

QObject *parentObject = nullptr;

static int initSyncthingTray(bool windowed, bool waitForTray, const Argument &connectionConfigArg)
{
    // get settings
    auto &settings = Settings::values();
    static const auto defaultConnection = std::vector<const char *>({ "" });
    const auto &connectionConfigurations
        = connectionConfigArg.isPresent() && !connectionConfigArg.values().empty() ? connectionConfigArg.values() : defaultConnection;

    // handle "windowed" case
    if (windowed) {
        // launch Syncthing if configured
        settings.launcher.autostart();

        // show a window for each connection
        for (const auto *const connectionConfig : connectionConfigurations) {
            auto *const trayWidget = new TrayWidget();
            trayWidget->setAttribute(Qt::WA_DeleteOnClose);
            trayWidget->show();
            trayWidget->applySettings(QString::fromLocal8Bit(connectionConfig));
        }
        return 0;
    }

#ifndef QT_NO_SYSTEMTRAYICON
    // check whether system tray is available
    if (!QSystemTrayIcon::isSystemTrayAvailable() && !waitForTray) {
        QMessageBox::critical(nullptr, QApplication::applicationName(),
            QApplication::translate("main",
                "The system tray is (currently) not available. You could open the tray menu as a regular window using the --windowed flag, though."
                "It is also possible to start Syncthing Tray with --wait to wait until the system tray becomes available instead of showing this "
                "message."));
        return -1;
    }

    // launch Syncthing if configured
    settings.launcher.autostart();

    // show a tray icon for each connection
    TrayWidget *widget;
    for (const auto *const connectionConfig : connectionConfigurations) {
        auto *const trayIcon = new TrayIcon(QString::fromLocal8Bit(connectionConfig), parentObject);
        trayIcon->show();
        widget = &trayIcon->trayMenu().widget();
    }

    // show wizard on first launch
    if (settings.firstLaunch || settings.fakeFirstLaunch) {
        widget->showWizard();
    }
    return 0;

#else
    // show error if system tray is not supported by Qt
    QMessageBox::critical(nullptr, QApplication::applicationName(),
        QApplication::translate("main",
            "The Qt libraries have not been built with tray icon support. You could open the tray menu as a regular "
            "window using the -w flag, though."));
    return -2;
#endif
}

static void trigger(bool tray, bool webUi, bool wizard)
{
    if (TrayWidget::instances().empty() || !(tray || webUi || wizard)) {
        return;
    }
    auto *const trayWidget = TrayWidget::instances().front();
    if (webUi) {
        trayWidget->showWebUI();
    }
    if (tray) {
        trayWidget->showUsingPositioningSettings();
    }
    if (wizard) {
        trayWidget->showWizard();
    }
}

static void shutdownSyncthingTray()
{
    Settings::save();
    if (const auto &error = Settings::values().error; !error.isEmpty()) {
        QMessageBox::critical(nullptr, QCoreApplication::applicationName(), error);
    }
    Settings::Launcher::terminate();
}

#endif

static int runApplication(int argc, const char *const *argv)
{
    // setup argument parser
    auto parser = ArgumentParser();
    auto qtConfigArgs = QT_CONFIG_ARGUMENTS();
    auto insecureArg = ConfigValueArgument("insecure", '\0', "allow any self-signed certificate");
    insecureArg.setFlags(Argument::Flags::Deprecated, true); // hide as it is only for development
#ifdef GUI_QTWIDGETS
    auto windowedArg = ConfigValueArgument("windowed", 'w', "opens the tray menu as a regular window");
    auto showWebUiArg = ConfigValueArgument("webui", '\0', "instantly shows the web UI - meant for creating shortcut to web UI");
    auto triggerArg = ConfigValueArgument("trigger", '\0', "instantly shows the left-click tray menu - meant for creating a shortcut");
    auto showWizardArg = ConfigValueArgument("show-wizard", '\0', "instantly shows the setup wizard");
    auto assumeFirstLaunchArg = ConfigValueArgument("assume-first-launch", '\0', "assumes first launch");
    assumeFirstLaunchArg.setFlags(Argument::Flags::Deprecated, true); // hide as it is debug-only
    auto wipArg = ConfigValueArgument("wip", '\0', "enables WIP features");
    wipArg.setFlags(Argument::Flags::Deprecated, true); // hide as it is debug-only
    auto waitForTrayArg = ConfigValueArgument("wait", '\0',
        "wait until the system tray becomes available instead of showing an error message if the system tray is not available on start-up");
    auto connectionArg = ConfigValueArgument("connection", '\0', "specifies one or more connection configurations to be used", { "config name" });
    connectionArg.setRequiredValueCount(Argument::varValueCount);
    auto configPathArg = ConfigValueArgument("config-dir-path", '\0', "specifies the path to the configuration directory", { "path" });
    configPathArg.setEnvironmentVariable(PROJECT_VARNAME_UPPER "_CONFIG_DIR");
    auto singleInstanceArg = Argument("single-instance", '\0', "does nothing if a tray icon is already shown");
    auto newInstanceArg = Argument("new-instance", '\0', "disable the usual single-process behavior");
    auto replaceArg = Argument("replace", '\0', "replaces a currently running instance");
    auto quitArg = OperationArgument("quit", '\0', "quits the currently running instance");
    quitArg.setFlags(Argument::Flags::Deprecated, true); // hide as only used internally for --replace
    auto &widgetsGuiArg = qtConfigArgs.qtWidgetsGuiArg();
    widgetsGuiArg.addSubArguments({ &windowedArg, &showWebUiArg, &triggerArg, &waitForTrayArg, &connectionArg, &configPathArg, &singleInstanceArg,
        &newInstanceArg, &replaceArg, &showWizardArg, &assumeFirstLaunchArg, &wipArg, &insecureArg });
#endif
#ifdef GUI_QTQUICK
    auto &quickGuiArg = qtConfigArgs.qtQuickGuiArg();
    quickGuiArg.addSubArgument(&insecureArg);
#endif
#ifdef SYNCTHINGTRAY_USE_LIBSYNCTHING
    auto cliArg = OperationArgument("cli", 'c', "runs Syncthing's CLI");
    auto cliHelp = ConfigValueArgument("help", 'h', "shows help for Syncthing's CLI");
    cliArg.setRequiredValueCount(Argument::varValueCount);
    cliArg.setFlags(Argument::Flags::Greedy, true);
    cliArg.setValueCompletionBehavior(ValueCompletionBehavior::PreDefinedValues | ValueCompletionBehavior::InvokeCallback);
    cliArg.setCallback([](const ArgumentOccurrence &occurrence) {
        CMD_UTILS_START_CONSOLE;
        std::exit(static_cast<int>(LibSyncthing::runCli(occurrence.values)));
    });
    cliArg.setSubArguments({ &cliHelp });
    auto syncthingArg = OperationArgument("syncthing", '\0', "runs Syncthing");
    auto syncthingHelp = ConfigValueArgument("help", 'h', "lists Syncthing's top-level commands");
    syncthingArg.setRequiredValueCount(Argument::varValueCount);
    syncthingArg.setFlags(Argument::Flags::Greedy, true);
    syncthingArg.setValueCompletionBehavior(ValueCompletionBehavior::PreDefinedValues | ValueCompletionBehavior::InvokeCallback);
    syncthingArg.setCallback([](const ArgumentOccurrence &occurrence) {
        CMD_UTILS_START_CONSOLE;
        std::exit(static_cast<int>(LibSyncthing::runCommand(occurrence.values)));
    });
    syncthingArg.setSubArguments({ &syncthingHelp });
#endif
#ifdef Q_OS_ANDROID
    auto serviceArg = OperationArgument("service", '\0', "runs service");
#endif

    parser.setMainArguments({
#ifdef GUI_QTWIDGETS
        &widgetsGuiArg,
#endif
#ifdef GUI_QTQUICK
        &quickGuiArg,
#endif
#ifdef SYNCTHINGTRAY_USE_LIBSYNCTHING
        &cliArg, &syncthingArg,
#endif
#ifdef Q_OS_ANDROID
        &serviceArg,
#endif
        &parser.noColorArg(), &parser.helpArg(),
#ifdef GUI_QTWIDGETS
        &quitArg
#endif
    });

    // parse arguments
#if defined(Q_OS_ANDROID)
    qDebug() << "Parsing CLI arguments";
    parser.setExitFunction([](int status) {
        qWarning() << "Unable to parse CLI arguments, exiting early";
        std::exit(status);
    });
#endif
    parser.parseArgs(argc, argv);

#ifdef GUI_QTWIDGETS
    // quit already running application if quit is present
    static auto firstRun = true;
    if (quitArg.isPresent() && !firstRun) {
        std::cerr << EscapeCodes::Phrases::Info << "Quitting as told by another instance" << EscapeCodes::Phrases::EndFlush;
        QCoreApplication::quit();
        return EXIT_SUCCESS;
    }
#endif

#ifdef Q_OS_ANDROID
    if (serviceArg.isPresent()) {
        qDebug() << "Initializing service";
        SET_QT_APPLICATION_INFO;
        auto androidService = QAndroidService(argc, const_cast<char **>(argv));
#ifdef SYNCTHINGTRAY_GUI_CODE_IN_SERVICE
        qputenv("QT_QPA_PLATFORM", "minimal"); // cannot use android platform as it would get stuck without activity
        auto guiApp = QGuiApplication(argc, const_cast<char **>(argv)); // need GUI app for using QIcon and such
#endif

        // initialize default locale as Qt does not seem to do this for the QAndroidService process
        const auto localeName = QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<jstring>("getLocale").toString();
        const auto locale = QLocale(localeName);
        QLocale::setDefault(locale);
        qDebug() << "Qt locale (service): " << locale;
        LOAD_QT_TRANSLATIONS;

        auto serviceApp = AppService(insecureArg.isPresent());
        networkAccessManager().setParent(&androidService);
        qDebug() << "Executing service";
        const auto res = androidService.exec();
        qDebug() << "Qt service event loop exited with return code " << res;
        return res;
    }
#endif

#ifdef GUI_QTQUICK
    if (quickGuiArg.isPresent()) {
        qDebug() << "Initializing Qt Quick GUI";
#ifdef SYNCTHINGTRAY_FORCE_VULKAN
        // force Vulkan RHI backend to test it on Android or other platforms where setting an env variable is not so easy
        qputenv("QSG_RHI_BACKEND", "vulkan");
        qputenv("QSG_INFO", "1");
#endif
#ifdef SYNCTHINGTRAY_HAS_WEBVIEW
        QtWebView::initialize();
#endif
        SET_QT_APPLICATION_INFO;
        auto app = QtApp(argc, const_cast<char **>(argv));
#if defined(Q_OS_ANDROID)
        qDebug() << "Qt locale: " << QLocale();
#endif
        LOAD_QT_TRANSLATIONS;
#if defined(Q_OS_ANDROID) && !defined(QT_NO_SSL)
        qDebug() << "TLS support available: " << QSslSocket::supportsSsl();
#endif
        qtConfigArgs.applySettings(true);
        networkAccessManager().setParent(&app);
#if !defined(Q_OS_ANDROID)
        auto appService = AppService(insecureArg.isPresent());
#endif
        auto quickApp = App(insecureArg.isPresent());
#if !defined(Q_OS_ANDROID)
        QObject::connect(&quickApp, &App::syncthingTerminationRequested, &appService, &AppService::terminateSyncthing);
        QObject::connect(&quickApp, &App::syncthingRestartRequested, &appService, &AppService::restartSyncthing);
        QObject::connect(&quickApp, &App::syncthingShutdownRequested, &appService, &AppService::shutdownSyncthing);
        QObject::connect(&quickApp, &App::syncthingConnectRequested, appService.connection(),
            static_cast<void (SyncthingConnection::*)()>(&SyncthingConnection::connect));
        QObject::connect(&quickApp, &App::settingsReloadRequested, &appService, &AppService::reloadSettings);
        QObject::connect(&quickApp, &App::launcherStatusRequested, &appService, &AppService::broadcastLauncherStatus);
        QObject::connect(&quickApp, &App::clearLogRequested, &appService, &AppService::clearLog);
        QObject::connect(&quickApp, &App::replayLogRequested, &appService, &AppService::replayLog);
        QObject::connect(&appService, &AppService::launcherStatusChanged, &quickApp, &App::handleLauncherStatusBroadcast);
        QObject::connect(&appService, &AppService::logsAvailable, &quickApp, &App::logsAvailable);
        QObject::connect(&appService, &AppService::error, &quickApp, &App::error);
        appService.broadcastLauncherStatus();
#endif
        const auto res = app.exec();
#if defined(Q_OS_ANDROID)
        qDebug() << "Qt UI event loop exited with return code " << res;
#endif
        return res;
    }
#endif

#ifdef GUI_QTWIDGETS
    // quit unless Qt Widgets GUI should be shown
    if (!widgetsGuiArg.isPresent()) {
        return EXIT_SUCCESS;
    }

    // handle override for config dir
    if (const char *const configPathDir = configPathArg.firstValue()) {
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, QString::fromLocal8Bit(configPathDir));
    }

    // do first-time initializations
    if (firstRun) {
        firstRun = false;
        SET_QT_APPLICATION_INFO;

#ifdef SYNCTHINGWIDGETS_HAS_SCHEME_HANDLER
        auto scheme = QWebEngineUrlScheme(QByteArrayLiteral("st"));
        scheme.setSyntax(QWebEngineUrlScheme::Syntax::Path);
        scheme.setFlags(QWebEngineUrlScheme::SecureScheme | QWebEngineUrlScheme::LocalScheme | QWebEngineUrlScheme::LocalAccessAllowed
            | QWebEngineUrlScheme::ViewSourceAllowed | QWebEngineUrlScheme::FetchApiAllowed);
        QWebEngineUrlScheme::registerScheme(scheme);
#endif

        auto application = QApplication(argc, const_cast<char **>(argv));
        QGuiApplication::setQuitOnLastWindowClosed(false);
#if defined(Q_OS_ANDROID)
        qDebug() << "Running Qt Widgets GUI";
#if !defined(QT_NO_SSL)
        qDebug() << "TLS support available: " << QSslSocket::supportsSsl();
#endif
#endif
        // stop possibly running instance if --replace is present
        if (replaceArg.isPresent()) {
            const char *const replaceArgs[] = { parser.executable(), quitArg.name() };
            SingleInstance::passArgsToRunningInstance(2, replaceArgs, SingleInstance::applicationId(), true);
        }
        auto singleInstance = SingleInstance(argc, argv, newInstanceArg.isPresent(), replaceArg.isPresent());
        networkAccessManager().setParent(&singleInstance);
        QObject::connect(&singleInstance, &SingleInstance::newInstance, &runApplication);
        Settings::restore();
        auto &settings = Settings::values();
        settings.qt.disableNotices();
        settings.qt.apply();
        qtConfigArgs.applySettings(true);
        if (assumeFirstLaunchArg.isPresent()) {
            settings.fakeFirstLaunch = true;
        }
        if (wipArg.isPresent()) {
            settings.enableWipFeatures = true;
        }
        if (insecureArg.isPresent()) {
            settings.connection.insecure = true;
        }
        if (newInstanceArg.isPresent()) {
            settings.isIndependentInstance = true;
        }
        LOAD_QT_TRANSLATIONS;
        if (!settings.error.isEmpty()) {
            QMessageBox::critical(nullptr, QCoreApplication::applicationName(), settings.error);
        }
        auto launcher = SyncthingLauncher();
        SyncthingLauncher::setMainInstance(&launcher);
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
        auto service = SyncthingService();
        SyncthingService::setMainInstance(&service);
        settings.systemd.setupService(service);
        QObject::connect(&service, &SyncthingService::errorOccurred, &handleSystemdServiceError);
#endif
        if (settings.icons.preferIconsFromTheme) {
            Data::setForkAwesomeThemeOverrides();
        }
#ifdef SYNCTHINGTRAY_SETUP_TOOLS_ENABLED
        auto verificationErrorMsgBox = QtUtilities::VerificationErrorMessageBox();
        auto updateHandler = QtUtilities::UpdateHandler(
            QString(), QStringLiteral(SYNCTHINGTRAY_SIGNATURE_EXTENSION), &Settings::settings(), &networkAccessManager());
        updateHandler.updater()->setVerifier([&verificationErrorMsgBox](const QtUtilities::Updater::Update &update) {
            auto error = QString();
            if (update.signature.empty()) {
                error = QStringLiteral("empty/non-existent signature");
            } else {
#ifdef SYNCTHINGTRAY_USE_LIBSYNCTHING
                const auto res = CppUtilities::verifySignature(SYNCTHINGTRAY_SIGNING_KEYS, update.signature, update.data, &LibSyncthing::verify);
#else
                const auto res = CppUtilities::verifySignature(SYNCTHINGTRAY_SIGNING_KEYS, update.signature, update.data);
#endif
                error = QString::fromUtf8(res.data(), static_cast<QString::size_type>(res.size()));
            }
            if (!error.isEmpty()) {
                auto explanation = QCoreApplication::translate("main",
                    "<p>This can have different causes:</p>"
                    "<ul>"
                    "<li>Data corruption occurred during the download/extraction. In this case cancelling and retrying the update will "
                    "help.</li>"
                    "<li>The signing key or updating mechanism in general has changed. In this case an according release note will be present "
                    "on <a href=\"https://martchus.github.io/syncthingtray/#downloads-section\">the website</a> and <a "
                    "href=\"https://github.com/Martchus/syncthingtray/releases\">GitHub</a>.</li>"
                    "<li>A bug in the newly introduced updater, see <a "
                    "href=\"https://github.com/Martchus/syncthingtray/issues\">issues on GitHub</a> for potential bug reports.</li>"
                    "<li>Someone tries to distribute manipulated executables of Syncthing Tray.</li>"
                    "</ul>"
                    "<p>It is recommend to cancel the update and retry or cross-check the cause if the issue persists. If you ignore this "
                    "error you <i>may</i> install a corrupted/manipulated executable.</p>");
                verificationErrorMsgBox.execForError(error, explanation);
            }
            return error;
        });
        updateHandler.applySettings();
        QtUtilities::UpdateHandler::setMainInstance(&updateHandler);
#endif

        // init Syncthing Tray and immediately shutdown on failure
        auto parent = QObject();
        parentObject = &parent;
        if (const auto res = initSyncthingTray(windowedArg.isPresent(), waitForTrayArg.isPresent(), connectionArg)) {
            shutdownSyncthingTray();
            return res;
        }

        // trigger UI and enter event loop
        QObject::connect(&application, &QCoreApplication::aboutToQuit, &shutdownSyncthingTray);
        trigger(triggerArg.isPresent(), showWebUiArg.isPresent(), showWizardArg.isPresent());
        const auto res = application.exec();
#ifdef SYNCTHINGTRAY_SETUP_TOOLS_ENABLED
        SettingsDialog::respawnIfRestartRequested();
#endif
        return res;
    }

#if defined(Q_OS_ANDROID)
    qDebug() << "Sending arguments to already running instance";
#endif

    // trigger actions if --webui or --trigger is present but don't create a new tray icon
    const auto firstInstance = TrayWidget::instances().empty();
    if (!firstInstance && (showWebUiArg.isPresent() || triggerArg.isPresent() || showWizardArg.isPresent())) {
        trigger(triggerArg.isPresent(), showWebUiArg.isPresent(), showWizardArg.isPresent());
        return 0;
    }

    // don't create a new instance if --single-instance has been specified
    if (!firstInstance && singleInstanceArg.isPresent()) {
        return 0;
    }

    // create new/additional tray icon
    const auto res = initSyncthingTray(windowedArg.isPresent(), waitForTrayArg.isPresent(), connectionArg);
    if (!res) {
        trigger(triggerArg.isPresent(), showWebUiArg.isPresent(), showWizardArg.isPresent());
    }
    return res;
#else
    return EXIT_SUCCESS;
#endif
}

CPP_UTILITIES_MAIN_EXPORT int main(int argc, char *argv[])
{
    SET_APPLICATION_INFO;
    CMD_UTILS_CONVERT_ARGS_TO_UTF8;
    CMD_UTILS_HANDLE_VIRTUAL_TERMINAL_PROCESSING;
#ifdef Q_OS_ANDROID
    // prevent crashes on exit, see https://doc.qt.io/qt-6/android-environment-variables.html
    qputenv("QT_ANDROID_NO_EXIT_CALL", "1");
#endif
    return runApplication(argc, argv);
}
