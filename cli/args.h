#ifndef CLI_ARGS_H
#define CLI_ARGS_H

#include <c++utilities/application/argumentparser.h>

namespace Cli {

using namespace CppUtilities;

struct Args {
    Args();
    ArgumentParser parser;
    OperationArgument status, log, stop, restart, rescan, rescanAll, pause, resume, waitForIdle, pwd, cat, edit;
    OperationArgument statusPwd, rescanPwd, pausePwd, resumePwd;
    ConfigValueArgument script, jsLines, dryRun;
    ConfigValueArgument stats, dir, dev, allDirs, allDevs;
    ConfigValueArgument atLeast, timeout;
    ConfigValueArgument editor;
    ConfigValueArgument configFile, apiKey, url, credentials, certificate;
};

} // namespace Cli

#endif // CLI_ARGS_H
