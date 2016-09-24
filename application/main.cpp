#include "./settings.h"
#include "./singleinstance.h"

#include "../gui/trayicon.h"
#include "../gui/traywidget.h"

#include "../data/syncthingprocess.h"

#include "resources/config.h"

#include <c++utilities/application/argumentparser.h>
#include <c++utilities/application/commandlineutils.h>
#include <c++utilities/application/failure.h>

#include <qtutilities/resources/qtconfigarguments.h>
#include <qtutilities/resources/resources.h>
#include <qtutilities/resources/importplugin.h>
#include <qtutilities/settingsdialog/qtsettings.h>

#include <QApplication>
#include <QMessageBox>

#include <iostream>

using namespace std;
using namespace ApplicationUtilities;
using namespace QtGui;
using namespace Data;

int initSyncthingTray(bool windowed, bool waitForTray)
{
    if(windowed) {
        if(Settings::launchSynchting()) {
            syncthingProcess().startSyncthing();
        }
        auto *trayWidget = new TrayWidget;
        trayWidget->setAttribute(Qt::WA_DeleteOnClose);
        trayWidget->show();
    } else {
#ifndef QT_NO_SYSTEMTRAYICON
        if(QSystemTrayIcon::isSystemTrayAvailable() || waitForTray) {
            if(Settings::launchSynchting()) {
                syncthingProcess().startSyncthing();
            }
            auto *trayIcon = new TrayIcon;
            trayIcon->show();
            if(Settings::firstLaunch()) {
                QMessageBox msgBox;
                msgBox.setIcon(QMessageBox::Information);
                msgBox.setText(QCoreApplication::translate("main", "You must configure how to connect to Syncthing when using Syncthing Tray the first time."));
                msgBox.setInformativeText(QCoreApplication::translate("main", "Note that the settings dialog allows importing URL, credentials and API-key from the local Syncthing configuration."));
                msgBox.exec();
                trayIcon->trayMenu().widget()->showSettingsDialog();
            }
        } else {
            QMessageBox::critical(nullptr, QApplication::applicationName(), QApplication::translate("main", "The system tray is (currently) not available. You could open the tray menu as a regular window using the -w flag, though."));
            return -1;
        }
#else
        QMessageBox::critical(nullptr, QApplication::applicationName(), QApplication::translate("main", "The Qt libraries have not been built with tray icon support. You could open the tray menu as a regular window using the -w flag, though."));
        return -2;
#endif
    }
    return 0;
}

int runApplication(int argc, const char *const *argv)
{
    static bool firstRun = true;

    // setup argument parser
    SET_APPLICATION_INFO;
    ArgumentParser parser;
    HelpArgument helpArg(parser);
    // Qt configuration arguments
    QT_CONFIG_ARGUMENTS qtConfigArgs;
    Argument windowedArg("windowed", 'w', "opens the tray menu as a regular window");
    windowedArg.setCombinable(true);
    Argument showWebUi("webui", '\0', "instantly shows the web UI - meant for creating shortcut to web UI");
    showWebUi.setCombinable(true);
    Argument waitForTrayArg("wait", '\0', "wait until the system tray becomes available instead of showing an error message if the system tray is not available on start-up");
    waitForTrayArg.setCombinable(true);
    qtConfigArgs.qtWidgetsGuiArg().addSubArgument(&windowedArg);
    qtConfigArgs.qtWidgetsGuiArg().addSubArgument(&showWebUi);
    qtConfigArgs.qtWidgetsGuiArg().addSubArgument(&waitForTrayArg);
    parser.setMainArguments({&qtConfigArgs.qtWidgetsGuiArg(), &helpArg});
    try {
        parser.parseArgs(argc, argv);
        if(qtConfigArgs.qtWidgetsGuiArg().isPresent()) {
            if(firstRun) {
                firstRun = false;

                SET_QT_APPLICATION_INFO;
                QApplication application(argc, const_cast<char **>(argv));
                QGuiApplication::setQuitOnLastWindowClosed(false);
                SingleInstance singleInstance(argc, argv);
                QObject::connect(&singleInstance, &SingleInstance::newInstance, &runApplication);

                Settings::restore();
                Settings::qtSettings().apply();
                qtConfigArgs.applySettings(true);

                LOAD_QT_TRANSLATIONS;
                QtUtilitiesResources::init();

                int res = initSyncthingTray(windowedArg.isPresent(), waitForTrayArg.isPresent());
                if(!res) {
                    if(!TrayWidget::instances().empty() && showWebUi.isPresent()) {
                        TrayWidget::instances().front()->showWebUi();
                    }
                    res = application.exec();
                }

                Settings::save();
                QtUtilitiesResources::cleanup();
                return res;
            } else {
                if(!TrayWidget::instances().empty() && showWebUi.isPresent()) {
                    // if --webui is present don't create a new tray icon, just show the web UI one of the present ones
                    TrayWidget::instances().front()->showWebUi();
                } else {
                    return initSyncthingTray(windowedArg.isPresent(), waitForTrayArg.isPresent());
                }
            }
        }
    } catch(const Failure &ex) {
        CMD_UTILS_START_CONSOLE;
        cout << "Unable to parse arguments. " << ex.what() << "\nSee --help for available commands." << endl;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    return runApplication(argc, argv);
}

