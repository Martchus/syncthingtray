#include "./singleinstance.h"

#include "../gui/trayicon.h"
#include "../gui/traywidget.h"

#include "../../widgets/misc/syncthinglauncher.h"
#include "../../widgets/settings/settings.h"

#include "../../connector/syncthingprocess.h"
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
#include "../../connector/syncthingservice.h"
#endif

#include "resources/config.h"
#include "resources/qtconfig.h"

#include <c++utilities/application/argumentparser.h>
#include <c++utilities/application/commandlineutils.h>
#include <c++utilities/misc/parseerror.h>

#include <qtutilities/resources/importplugin.h>
#include <qtutilities/resources/qtconfigarguments.h>
#include <qtutilities/resources/resources.h>
#include <qtutilities/settingsdialog/qtsettings.h>

#include <QApplication>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QSettings>
#include <QStringBuilder>

#include <iostream>

using namespace std;
using namespace CppUtilities;
using namespace QtGui;
using namespace Data;

ENABLE_QT_RESOURCES_OF_STATIC_DEPENDENCIES

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
void handleSystemdServiceError(const QString &context, const QString &name, const QString &message)
{
    auto *const msgBox = new QMessageBox;
    msgBox->setAttribute(Qt::WA_DeleteOnClose);
    msgBox->setIcon(QMessageBox::Critical);
    msgBox->setText(QCoreApplication::translate("main", "Unable to ") + context);
    msgBox->setInformativeText(name % QStringLiteral(":\n") % message);
    msgBox->show();
}
#endif

int initSyncthingTray(bool windowed, bool waitForTray, const Argument &connectionConfigArg)
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
        auto *const trayIcon = new TrayIcon(QString::fromLocal8Bit(connectionConfig), QApplication::instance());
        trayIcon->show();
        widget = &trayIcon->trayMenu().widget();
    }

    // show "first launch" message box
    if (!settings.firstLaunch) {
        return 0;
    }
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText(QCoreApplication::translate("main", "You must configure how to connect to Syncthing when using Syncthing Tray the first time."));
    msgBox.setInformativeText(QCoreApplication::translate(
        "main", "Note that the settings dialog allows importing URL, credentials and API-key from the local Syncthing configuration."));
    msgBox.exec();
    widget->showSettingsDialog();
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

void trigger(bool tray, bool webUi)
{
    if (TrayWidget::instances().empty() || !(tray || webUi)) {
        return;
    }
    auto *const trayWidget = TrayWidget::instances().front();
    if (webUi) {
        trayWidget->showWebUi();
    }
    if (tray) {
        trayWidget->showUsingPositioningSettings();
    }
}

void shutdownSyncthingTray()
{
    Settings::save();
    Settings::Launcher::terminate();
}

int runApplication(int argc, const char *const *argv)
{
    // setup argument parser
    SET_APPLICATION_INFO;
    CMD_UTILS_CONVERT_ARGS_TO_UTF8;
    ArgumentParser parser;
    // Qt configuration arguments
    QT_CONFIG_ARGUMENTS qtConfigArgs;
    Argument windowedArg("windowed", 'w', "opens the tray menu as a regular window");
    windowedArg.setCombinable(true);
    Argument showWebUiArg("webui", '\0', "instantly shows the web UI - meant for creating shortcut to web UI");
    showWebUiArg.setCombinable(true);
    Argument triggerArg("trigger", '\0', "instantly shows the left-click tray menu - meant for creating a shortcut");
    triggerArg.setCombinable(true);
    Argument waitForTrayArg("wait", '\0',
        "wait until the system tray becomes available instead of showing an error message if the system tray is not available on start-up");
    waitForTrayArg.setCombinable(true);
    ConfigValueArgument connectionArg("connection", '\0', "specifies one or more connection configurations to be used", { "config name" });
    connectionArg.setRequiredValueCount(Argument::varValueCount);
    ConfigValueArgument configPathArg("config-dir-path", '\0', "specifies the path to the configuration directory", { "path" });
    configPathArg.setEnvironmentVariable(PROJECT_VARNAME_UPPER "_CONFIG_DIR");
    ConfigValueArgument newInstanceArg("new-instance", '\0', "disable the usual single-process behavior");
    Argument &widgetsGuiArg = qtConfigArgs.qtWidgetsGuiArg();
    widgetsGuiArg.addSubArgument(&windowedArg);
    widgetsGuiArg.addSubArgument(&showWebUiArg);
    widgetsGuiArg.addSubArgument(&triggerArg);
    widgetsGuiArg.addSubArgument(&waitForTrayArg);
    widgetsGuiArg.addSubArgument(&connectionArg);
    widgetsGuiArg.addSubArgument(&configPathArg);
    widgetsGuiArg.addSubArgument(&newInstanceArg);

    parser.setMainArguments({ &qtConfigArgs.qtWidgetsGuiArg(), &parser.noColorArg(), &parser.helpArg() });
    parser.parseArgs(argc, argv);
    if (!qtConfigArgs.qtWidgetsGuiArg().isPresent()) {
        return 0;
    }

    // handle override for config dir
    if (const char *const configPathDir = configPathArg.firstValue()) {
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, QString::fromLocal8Bit(configPathDir));
    }

    // check whether runApplication() has been called for the first time
    static auto firstRun = true;
    if (firstRun) {
        firstRun = false;

        // do first-time initializations
        SET_QT_APPLICATION_INFO;
        QApplication application(argc, const_cast<char **>(argv));
        QGuiApplication::setQuitOnLastWindowClosed(false);
        SingleInstance singleInstance(argc, argv, newInstanceArg.isPresent());
        networkAccessManager().setParent(&singleInstance);
        QObject::connect(&singleInstance, &SingleInstance::newInstance, &runApplication);
        Settings::restore();
        Settings::values().qt.apply();
        qtConfigArgs.applySettings(true);
        LOAD_QT_TRANSLATIONS;
        SyncthingLauncher launcher;
        SyncthingLauncher::setMainInstance(&launcher);
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
        SyncthingService service;
        SyncthingService::setMainInstance(&service);
        Settings::values().systemd.setupService(service);
        QObject::connect(&service, &SyncthingService::errorOccurred, &handleSystemdServiceError);
#endif

        // init Syncthing Tray and immediately shutdown on failure
        if (const auto res = initSyncthingTray(windowedArg.isPresent(), waitForTrayArg.isPresent(), connectionArg)) {
            shutdownSyncthingTray();
            return res;
        }

        // trigger UI and enter event loop
        QObject::connect(&application, &QCoreApplication::aboutToQuit, &shutdownSyncthingTray);
        trigger(triggerArg.isPresent(), showWebUiArg.isPresent());
        return application.exec();
    }

    // trigger actions if --webui or --trigger is present but don't create a new tray icon
    if (!TrayWidget::instances().empty() && (showWebUiArg.isPresent() || triggerArg.isPresent())) {
        trigger(triggerArg.isPresent(), showWebUiArg.isPresent());
        return 0;
    }

    // create new/additional tray icon
    const auto res = initSyncthingTray(windowedArg.isPresent(), waitForTrayArg.isPresent(), connectionArg);
    if (!res) {
        trigger(triggerArg.isPresent(), showWebUiArg.isPresent());
    }
    return res;
}

int main(int argc, char *argv[])
{
    return runApplication(argc, argv);
}
