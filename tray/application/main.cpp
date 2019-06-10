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
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText(QCoreApplication::translate("main", "Unable to ") + context);
    msgBox.setInformativeText(name % QStringLiteral(":\n") % message);
    msgBox.exec();
}
#endif

int initSyncthingTray(bool windowed, bool waitForTray, const char *connectionConfig)
{
    // get settings
    auto &settings = Settings::values();
    const auto connectionConfigQStr(connectionConfig ? QString::fromLocal8Bit(connectionConfig) : QString());

    // handle "windowed" case
    if (windowed) {
        settings.launcher.autostart();
        auto *const trayWidget = new TrayWidget();
        trayWidget->setAttribute(Qt::WA_DeleteOnClose);
        trayWidget->show();
        trayWidget->applySettings(connectionConfigQStr);
        return 0;
    }

#ifndef QT_NO_SYSTEMTRAYICON
    // check whether system tray is available
    if (!QSystemTrayIcon::isSystemTrayAvailable() && !waitForTray) {
        QMessageBox::critical(nullptr, QApplication::applicationName(),
            QApplication::translate(
                "main", "The system tray is (currently) not available. You could open the tray menu as a regular window using the -w flag, though."));
        return -1;
    }

    // show tray icon
    settings.launcher.autostart();
    auto *const trayIcon = new TrayIcon(connectionConfigQStr, QApplication::instance());
    trayIcon->show();
    if (!settings.firstLaunch) {
        return 0;
    }

    // show "first launch" message box
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText(QCoreApplication::translate("main", "You must configure how to connect to Syncthing when using Syncthing Tray the first time."));
    msgBox.setInformativeText(QCoreApplication::translate(
        "main", "Note that the settings dialog allows importing URL, credentials and API-key from the local Syncthing configuration."));
    msgBox.exec();
    trayIcon->trayMenu().widget().showSettingsDialog();
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
        trayWidget->showAtCursor();
    }
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
    ConfigValueArgument connectionArg("connection", '\0', "specifies the connection configuration to be used", { "config name" });
    Argument &widgetsGuiArg = qtConfigArgs.qtWidgetsGuiArg();
    widgetsGuiArg.addSubArgument(&windowedArg);
    widgetsGuiArg.addSubArgument(&showWebUiArg);
    widgetsGuiArg.addSubArgument(&triggerArg);
    widgetsGuiArg.addSubArgument(&waitForTrayArg);
    widgetsGuiArg.addSubArgument(&connectionArg);

    parser.setMainArguments({ &qtConfigArgs.qtWidgetsGuiArg(), &parser.noColorArg(), &parser.helpArg() });
    parser.parseArgs(argc, argv);
    if (!qtConfigArgs.qtWidgetsGuiArg().isPresent()) {
        return 0;
    }

    // check whether runApplication() has been called for the first time
    static auto firstRun = true;
    if (firstRun) {
        firstRun = false;

        // do first-time initializations
        SET_QT_APPLICATION_INFO;
        QApplication application(argc, const_cast<char **>(argv));
        QGuiApplication::setQuitOnLastWindowClosed(false);
        SingleInstance singleInstance(argc, argv);
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
        service.setUnitName(Settings::values().systemd.syncthingUnit);
        QObject::connect(&service, &SyncthingService::errorOccurred, &handleSystemdServiceError);
#endif

        // show (first) tray icon and enter main event loop
        auto res = initSyncthingTray(windowedArg.isPresent(), waitForTrayArg.isPresent(), connectionArg.firstValue());
        if (!res) {
            trigger(triggerArg.isPresent(), showWebUiArg.isPresent());
            res = application.exec();
        }

        // perform cleanup, then terminate
        Settings::Launcher::terminate();
        Settings::save();
        return res;
    }

    // trigger actions if --webui or --trigger is present but don't create a new tray icon
    if (!TrayWidget::instances().empty() && (showWebUiArg.isPresent() || triggerArg.isPresent())) {
        trigger(triggerArg.isPresent(), showWebUiArg.isPresent());
        return 0;
    }

    // create new/additional tray icon
    const auto res = initSyncthingTray(windowedArg.isPresent(), waitForTrayArg.isPresent(), connectionArg.firstValue());
    if (!res) {
        trigger(triggerArg.isPresent(), showWebUiArg.isPresent());
    }
    return res;
}

int main(int argc, char *argv[])
{
    return runApplication(argc, argv);
}
