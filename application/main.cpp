#include "./settings.h"
#include "../gui/tray.h"

#include "resources/config.h"

#include <c++utilities/application/argumentparser.h>
#include <c++utilities/application/commandlineutils.h>
#include <c++utilities/application/failure.h>

#include <qtutilities/resources/qtconfigarguments.h>
#include <qtutilities/resources/resources.h>
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
#ifndef QT_NO_SYSTEMTRAYICON
            if(QSystemTrayIcon::isSystemTrayAvailable()) {
                application.setQuitOnLastWindowClosed(false);
                TrayIcon trayIcon;
                trayIcon.show();
                res = application.exec();
            } else {
                QMessageBox::critical(nullptr, QApplication::applicationName(), QApplication::translate("main", "The system tray is (currently) not available."));
                res = -1;
            }
#else
            QMessageBox::critical(nullptr, QApplication::applicationName(), QApplication::translate("main", "The Qt libraries have not been built with tray icon support."));
            res = -2;
#endif
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

