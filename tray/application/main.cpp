#ifdef GUI_QTQUICK
#define QT_UTILITIES_GUI_QTQUICK
#endif

#include "./singleinstance.h"

#include "../gui/trayicon.h"
#include "../gui/traywidget.h"

#ifdef GUI_QTQUICK
#include "../gui/app.h"
#endif

#include <syncthingwidgets/misc/syncthinglauncher.h>
#include <syncthingwidgets/settings/settings.h>

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

#include <QApplication>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QSettings>
#include <QStringBuilder>

#if defined(GUI_QTQUICK) && defined(SYNCTHINGTRAY_HAS_WEBVIEW)
#include <QtWebView/QtWebView>
#endif

#include <iostream>

#ifdef Q_OS_ANDROID
#include <QDebug>
#include <QSslSocket>
#endif

using namespace std;
using namespace CppUtilities;
using namespace QtGui;
using namespace Data;

// import static icon engine plugin
#ifdef QT_FORK_AWESOME_ICON_ENGINE_STATIC
#include <QtPlugin>
Q_IMPORT_PLUGIN(ForkAwesomeIconEnginePlugin)
#endif

ENABLE_QT_RESOURCES_OF_STATIC_DEPENDENCIES

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

static int runApplication(int argc, const char *const *argv)
{
    // setup argument parser
    auto parser = ArgumentParser();
    auto qtConfigArgs = QT_CONFIG_ARGUMENTS();
    auto windowedArg = ConfigValueArgument("windowed", 'w', "opens the tray menu as a regular window");
    auto showWebUiArg = ConfigValueArgument("webui", '\0', "instantly shows the web UI - meant for creating shortcut to web UI");
    auto triggerArg = ConfigValueArgument("trigger", '\0', "instantly shows the left-click tray menu - meant for creating a shortcut");
    auto showWizardArg = ConfigValueArgument("show-wizard", '\0', "instantly shows the setup  wizard");
    auto assumeFirstLaunchArg = ConfigValueArgument("assume-first-launch", '\0', "assumes first launch");
    assumeFirstLaunchArg.setFlags(Argument::Flags::Deprecated, true); // hide as it is debug-only
    auto wipArg = ConfigValueArgument("wip", '\0', "enables WIP features");
    wipArg.setFlags(Argument::Flags::Deprecated, true); // hide as it is debug-only
    auto insecureArg = ConfigValueArgument("insecure", '\0', "allow any self-signed certificate");
    insecureArg.setFlags(Argument::Flags::Deprecated, true); // hide as it is only for development
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

    parser.setMainArguments({ &qtConfigArgs.qtWidgetsGuiArg(),
#ifdef GUI_QTQUICK
        &qtConfigArgs.qtQuickGuiArg(),
#endif
#ifdef SYNCTHINGTRAY_USE_LIBSYNCTHING
        &cliArg, &syncthingArg,
#endif
        &parser.noColorArg(), &parser.helpArg(), &quitArg });

    // parse arguments
    parser.parseArgs(argc, argv);

    // quit already running application if quit is present
    static auto firstRun = true;
    if (quitArg.isPresent() && !firstRun) {
        std::cerr << EscapeCodes::Phrases::Info << "Quitting as told by another instance" << EscapeCodes::Phrases::EndFlush;
        QCoreApplication::quit();
        return EXIT_SUCCESS;
    }

#ifdef GUI_QTQUICK
    if (qtConfigArgs.qtQuickGuiArg().isPresent()) {
#ifdef SYNCTHINGTRAY_HAS_WEBVIEW
        QtWebView::initialize();
#endif
        SET_QT_APPLICATION_INFO;
        auto app = QApplication(argc, const_cast<char **>(argv));
        auto &settings = Settings::values();
        Settings::restore();
        settings.qt.disableNotices();
        settings.qt.apply();
        qtConfigArgs.applySettings(true);
        qtConfigArgs.applySettingsForQuickGui();
        networkAccessManager().setParent(&app);

        auto quickApp = App();
        quickApp.connection()->connect(settings.connection.primary);
        return app.exec();
    }
#endif

    // quit unless Qt Widgets GUI should be shown
    if (!qtConfigArgs.qtWidgetsGuiArg().isPresent()) {
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
        auto application = QApplication(argc, const_cast<char **>(argv));
        QGuiApplication::setQuitOnLastWindowClosed(false);
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
#if defined(Q_OS_ANDROID) && !defined(QT_NO_SSL)
        qDebug() << "TLS support available: " << QSslSocket::supportsSsl();
#endif
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
        return application.exec();
    }

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
}

int main(int argc, char *argv[])
{
    SET_APPLICATION_INFO;
    CMD_UTILS_CONVERT_ARGS_TO_UTF8;
    CMD_UTILS_HANDLE_VIRTUAL_TERMINAL_PROCESSING;
    return runApplication(argc, argv);
}
