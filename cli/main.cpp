#include "./application.h"

#include "resources/config.h"

#include <c++utilities/application/argumentparser.h>
#include <c++utilities/application/commandlineutils.h>

#include <QCoreApplication>

int main(int argc, char *argv[])
{
    SET_APPLICATION_INFO;
    CMD_UTILS_HANDLE_VIRTUAL_TERMINAL_PROCESSING;
    CMD_UTILS_CONVERT_ARGS_TO_UTF8;
    QCoreApplication coreApp(argc, argv);
    Cli::Application cliApp;
    return cliApp.exec(argc, argv);
}
