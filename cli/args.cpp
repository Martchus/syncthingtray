#include "./args.h"

namespace Cli {

Args::Args() :
    help(parser),
    status("status", 's', "shows the status"),
    log("log", 'l', "shows the Syncthing log"),
    restart("restart", '\0', "restarts Syncthing"),
    rescan("rescan", 'r', "rescans the specified directories"),
    rescanAll("rescan-all", '\0', "rescans all directories"),
    pause("pause", '\0', "pauses the specified devices"),
    pauseAll("pause-all", '\0', "pauses all devices"),
    resume("resume", '\0', "resumes the specified devices"),
    resumeAll("resume-all", '\0', "resumes all devices"),
    dir("dir", 'd', "specifies the directory to display status info for (default is all dirs)", {"ID"}),
    dev("dev", '\0', "specifies the device to display status info for (default is all devs)", {"ID"}),
    configFile("config-file", 'f', "specifies the Syncthing config file", {"path"}),
    apiKey("api-key", 'k', "specifies the API key", {"key"}),
    url("url", 'u', "specifies the Syncthing URL, default is http://localhost:8080", {"URL"}),
    credentials("credentials", 'c', "specifies user name and password", {"user name", "password"}),
    certificate("cert", '\0', "specifies the certificate used by the Syncthing instance", {"path"})
{
    dir.setConstraints(0, -1), dev.setConstraints(0, -1);
    status.setSubArguments({&dir, &dev});

    rescan.setValueNames({"dir ID"});
    rescan.setRequiredValueCount(-1);
    pause.setValueNames({"dev ID"});
    pause.setRequiredValueCount(-1);
    resume.setValueNames({"dev ID"});
    resume.setRequiredValueCount(-1);

    parser.setMainArguments({&status, &log, &restart, &rescan, &rescanAll, &pause, &pauseAll,
                             &resume, &resumeAll,
                             &configFile, &apiKey, &url, &credentials, &certificate, &help});

    // allow setting default values via environment
    configFile.setEnvironmentVariable("SYNCTHING_CTL_CONFIG_FILE");
    apiKey.setEnvironmentVariable("SYNCTHING_CTL_API_KEY");
    url.setEnvironmentVariable("SYNCTHING_CTL_URL");
    certificate.setEnvironmentVariable("SYNCTHING_CTL_CERTIFICATE");
}

} // namespace Cli
