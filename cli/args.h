#ifndef CLI_ARGS_H
#define CLI_ARGS_H

#include <c++utilities/application/argumentparser.h>

namespace Cli {

using namespace ApplicationUtilities;

struct Args
{
    Args();
    ArgumentParser parser;
    HelpArgument help;
    OperationArgument status, log, stop, restart, rescan, rescanAll, pause, pauseAll, resume, resumeAll, waitForIdle;
    ConfigValueArgument dir, dev;
    ConfigValueArgument configFile, apiKey, url, credentials, certificate;
};

} // namespace Cli

#endif // CLI_ARGS_H
