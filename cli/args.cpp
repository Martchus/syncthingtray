#include "./args.h"

namespace Cli {

Args::Args() :
    help(parser),
    status("status", 's', "shows the status"),
    log("log", 'l', "shows the Syncthing log"),
    stop("stop", '\0', "stops Syncthing"),
    restart("restart", '\0', "restarts Syncthing"),
    rescan("rescan", 'r', "rescans the specified directories"),
    rescanAll("rescan-all", '\0', "rescans all directories"),
    pause("pause", '\0', "pauses the specified devices"),
    pauseAllDevs("pause-all-devs", '\0', "pauses all devices"),
    pauseAllDirs("pause-all-dirs", '\0', "pauses all directories"),
    resume("resume", '\0', "resumes the specified devices"),
    resumeAllDevs("resume-all-devs", '\0', "resumes all devices"),
    resumeAllDirs("resume-all-dirs", '\0', "resumes all directories"),
    waitForIdle("wait-for-idle", 'w', "waits until the specified dirs/devs are idling"),
    pwd("pwd", 'p', "operates in the current working directory"),
    statusDir("dir", 'd', "specifies the directoies (default is all dirs)", {"ID"}),
    statusDev("dev", '\0', "specifies the devices (default is all devs)", {"ID"}),
    pauseDir("dir", 'd', "specifies the directories", {"ID"}),
    pauseDev("dev", '\0', "specifies the devices", {"ID"}),
    configFile("config-file", 'f', "specifies the Syncthing config file", {"path"}),
    apiKey("api-key", 'k', "specifies the API key", {"key"}),
    url("url", 'u', "specifies the Syncthing URL, default is http://localhost:8080", {"URL"}),
    credentials("credentials", 'c', "specifies user name and password", {"user name", "password"}),
    certificate("cert", '\0', "specifies the certificate used by the Syncthing instance", {"path"})
{
    for(Argument *arg : {&statusDir, &statusDev, &pauseDev, &pauseDir}) {
        arg->setConstraints(0, -1);
    }
    status.setSubArguments({&statusDir, &statusDev});
    waitForIdle.setSubArguments({&statusDir, &statusDev});
    pwd.setValueNames({"status/rescan/pause/resume"});
    pwd.setRequiredValueCount(1);

    rescan.setValueNames({"dir ID"});
    rescan.setRequiredValueCount(-1);
    pause.setSubArguments({&pauseDir, &pauseDev});
    resume.setSubArguments({&pauseDir, &pauseDev});

    parser.setMainArguments({&status, &log, &stop, &restart, &rescan, &rescanAll, &pause, &pauseAllDevs, &pauseAllDirs, &resume, &resumeAllDevs,
                             &resumeAllDirs, &waitForIdle, &pwd, &configFile, &apiKey, &url, &credentials, &certificate, &help});

    // allow setting default values via environment
    configFile.setEnvironmentVariable("SYNCTHING_CTL_CONFIG_FILE");
    apiKey.setEnvironmentVariable("SYNCTHING_CTL_API_KEY");
    url.setEnvironmentVariable("SYNCTHING_CTL_URL");
    certificate.setEnvironmentVariable("SYNCTHING_CTL_CERTIFICATE");
}

} // namespace Cli
