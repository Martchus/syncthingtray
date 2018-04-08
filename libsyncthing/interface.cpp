#include "./interface.h"

#include "libsyncthinginternal.h"

namespace LibSyncthing {

inline _GoString_ gostr(const std::string &str)
{
    return _GoString_{ str.data(), static_cast<std::ptrdiff_t>(str.size()) };
}

void runSyncthing(const RuntimeOptions &options)
{
    ::runSyncthing(gostr(options.configDir), gostr(options.guiAddress), gostr(options.guiApiKey), gostr(options.logFile), options.verbose);
}

void generate(const std::string &generateDir)
{
    ::generate(gostr(generateDir));
}

} // namespace LibSyncthing
