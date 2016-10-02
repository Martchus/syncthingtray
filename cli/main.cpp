#include "./application.h"

#include "resources/config.h"

#include <c++utilities/application/argumentparser.h>

#include <QCoreApplication>

int main(int argc, char *argv[])
{
    SET_APPLICATION_INFO;
    QCoreApplication coreApp(argc, argv);
    Cli::Application cliApp;
    return cliApp.exec(argc, argv);
}
