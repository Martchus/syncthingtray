#include "./settings.h"
#include "../gui/trayicon.h"
#include "../gui/traywidget.h"

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

int main(int argc, char *argv[])
{
    SET_APPLICATION_INFO;
    // setup argument parser
    ArgumentParser parser;
    HelpArgument helpArg(parser);
    // Qt configuration arguments
    QT_CONFIG_ARGUMENTS qtConfigArgs;
    Argument windowedArg("windowed", 'w', "opens the tray menu as a regular window");
    windowedArg.setCombinable(true);
    qtConfigArgs.qtWidgetsGuiArg().addSubArgument(&windowedArg);
    parser.setMainArguments({&qtConfigArgs.qtWidgetsGuiArg(), &helpArg});
    try {
        parser.parseArgs(argc, argv);
        if(qtConfigArgs.qtWidgetsGuiArg().isPresent()) {
            SET_QT_APPLICATION_INFO;
            QApplication application(argc, argv);
            Settings::restore();
            Settings::qtSettings().apply();
            qtConfigArgs.applySettings(true);
            LOAD_QT_TRANSLATIONS;
            QtUtilitiesResources::init();
            int res;
            if(windowedArg.isPresent()) {
                TrayWidget trayWidget;
                trayWidget.show();
                res = application.exec();
            } else {
#ifndef QT_NO_SYSTEMTRAYICON
                if(QSystemTrayIcon::isSystemTrayAvailable()) {
                    application.setQuitOnLastWindowClosed(false);
                    TrayIcon trayIcon;
                    trayIcon.show();
                    if(Settings::firstLaunch()) {
                        trayIcon.trayMenu().widget()->showSettingsDialog();
                        QMessageBox msgBox;
                        msgBox.setIcon(QMessageBox::Information);
                        msgBox.setText(QCoreApplication::translate("main", "You must configure how to connect to Syncthing when using Syncthing Tray the first time."));
                        msgBox.setInformativeText(QCoreApplication::translate("main", "Note that the settings dialog allows importing URL, credentials and API-key from the local Syncthing configuration."));
                        msgBox.exec();
                    }
                    res = application.exec();
                } else {
                    QMessageBox::critical(nullptr, QApplication::applicationName(), QApplication::translate("main", "The system tray is (currently) not available. You could open the tray menu as a regular window using the -w flag, though."));
                    res = -1;
                }
#else
                QMessageBox::critical(nullptr, QApplication::applicationName(), QApplication::translate("main", "The Qt libraries have not been built with tray icon support. You could open the tray menu as a regular window using the -w flag, though."));
                res = -2;
#endif
            }
            Settings::save();
            QtUtilitiesResources::cleanup();
            return res;
        }
    } catch(const Failure &ex) {
        CMD_UTILS_START_CONSOLE;
        cout << "Unable to parse arguments. " << ex.what() << "\nSee --help for available commands." << endl;
    }

    return 0;
}

