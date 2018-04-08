#ifndef LIBSYNCTHING_INTERFACE_H
#define LIBSYNCTHING_INTERFACE_H

#include "./global.h"

#include <string>

namespace LibSyncthing {

struct RuntimeOptions {
    std::string configDir;
    std::string guiAddress;
    std::string guiApiKey;
    std::string logFile;
    bool verbose = false;
};

void LIB_SYNCTHING_EXPORT runSyncthing(const RuntimeOptions &options);
void LIB_SYNCTHING_EXPORT generate(const std::string &generateDir);

} // namespace LibSyncthing

#endif // LIBSYNCTHING_INTERFACE_H
