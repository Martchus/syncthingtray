#ifndef CLI_ARGS_H
#define CLI_ARGS_H

#include <c++utilities/application/argumentparser.h>

namespace Cli {

using namespace ApplicationUtilities;

struct Args {
    Args();
    ArgumentParser parser;
    HelpArgument help;
    NoColorArgument noColor;
    OperationArgument status, log, stop, restart, rescan, rescanAll, pause, resume, waitForIdle, pwd, cat;
    OperationArgument statusPwd, rescanPwd, pausePwd, resumePwd;
    ConfigValueArgument dir, dev, allDirs, allDevs;
    ConfigValueArgument atLeast, timeout;
    ConfigValueArgument configFile, apiKey, url, credentials, certificate;
};

} // namespace Cli

#endif // CLI_ARGS_H
