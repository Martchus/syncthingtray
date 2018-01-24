#include "./args.h"

#include "resources/config.h"

namespace Cli {

Args::Args()
    : help(parser)
    , status("status", 's', "shows the status (for all dirs and devs if none specified)")
    , log("log", 'l', "shows the Syncthing log")
    , stop("stop", '\0', "stops Syncthing")
    , restart("restart", '\0', "restarts Syncthing")
    , rescan("rescan", 'r', "rescans the specified directories")
    , rescanAll("rescan-all", '\0', "rescans all directories")
    , pause("pause", '\0', "pauses the specified devices")
    , resume("resume", '\0', "resumes the specified devices")
    , waitForIdle("wait-for-idle", 'w', "waits until the specified dirs/devs are idling")
    , pwd("pwd", 'p', "operates in the current working directory")
    , statusPwd("status", 's', "prints the status of the current working directory")
    , rescanPwd("rescan", 'r', "rescans the current working directory")
    , pausePwd("pause", 'p', "pauses the current working directory")
    , resumePwd("resume", '\0', "resumes the current working directory")
    , dir("dir", 'd', "specifies a directory", { "ID" })
    , dev("dev", '\0', "specifies a device", { "ID" })
    , allDirs("all-dirs", '\0', "apply operation for all directories")
    , allDevs("all-devs", '\0', "apply operation for all devices")
    , atLeast("at-least", 'a', "specifies for how many milliseconds Syncthing must idle (prevents exiting to early in case of flaky status)",
          { "number" })
    , timeout("timeout", 't', "specifies how many milliseconds to wait at most", { "number" })
    , configFile("config-file", 'f', "specifies the Syncthing config file to read API key and URL from, when not explicitely specified", { "path" })
    , apiKey("api-key", 'k', "specifies the API key", { "key" })
    , url("url", 'u', "specifies the Syncthing URL, default is http://localhost:8080", { "URL" })
    , credentials("credentials", 'c', "specifies user name and password", { "user name", "password" })
    , certificate("cert", '\0', "specifies the certificate used by the Syncthing instance", { "path" })
{
    dir.setConstraints(0, Argument::varValueCount);
    dir.setValueCompletionBehavior(
        ValueCompletionBehavior::PreDefinedValues | ValueCompletionBehavior::Directories | ValueCompletionBehavior::InvokeCallback);
    dev.setConstraints(0, Argument::varValueCount);
    dev.setValueCompletionBehavior(ValueCompletionBehavior::PreDefinedValues | ValueCompletionBehavior::InvokeCallback);
    status.setSubArguments({ &dir, &dev, &allDirs, &allDevs });
    status.setExample(PROJECT_NAME " status # shows all dirs and devs\n" PROJECT_NAME " status --dir dir1 --dir dir2 --dev dev1 --dev dev2");
    waitForIdle.setSubArguments({ &dir, &dev, &allDirs, &allDevs, &atLeast, &timeout });
    waitForIdle.setExample(PROJECT_NAME " wait-for-idle --timeout 1800000 --at-least 5000 && systemctl poweroff\n" PROJECT_NAME
                                        " wait-for-idle --dir dir1 --dir dir2 --dev dev1 --dev dev2 --at-least 5000");
    pwd.setSubArguments({ &statusPwd, &rescanPwd, &pausePwd, &resumePwd });

    rescan.setValueNames({ "dir ID" });
    rescan.setRequiredValueCount(Argument::varValueCount);
    rescan.setValueCompletionBehavior(
        ValueCompletionBehavior::PreDefinedValues | ValueCompletionBehavior::Directories | ValueCompletionBehavior::InvokeCallback);
    rescan.setExample(PROJECT_NAME " rescan dir1 dir2 dir4 dir5");
    pause.setSubArguments({ &dir, &dev, &allDirs, &allDevs });
    pause.setExample(PROJECT_NAME " pause --dir dir1 --dir dir2 --dev dev1 --dev dev2\n" PROJECT_NAME " pause --all-dirs");
    resume.setSubArguments({ &dir, &dev, &allDirs, &allDevs });
    resume.setExample(PROJECT_NAME " resume --dir dir1 --dir dir2 --dev dev1 --dev dev2\n" PROJECT_NAME " resume --all-devs");

    configFile.setExample(PROJECT_NAME " status --dir dir1 --config-file ~/.config/syncthing/config.xml");
    credentials.setExample(PROJECT_NAME " status --dir dir1 --credentials name supersecret");

    parser.setMainArguments({ &status, &log, &stop, &restart, &rescan, &rescanAll, &pause, &resume, &waitForIdle, &pwd, &configFile, &apiKey, &url,
        &credentials, &certificate, &noColor, &help });

    // allow setting default values via environment
    configFile.setEnvironmentVariable("SYNCTHING_CTL_CONFIG_FILE");
    apiKey.setEnvironmentVariable("SYNCTHING_CTL_API_KEY");
    apiKey.setValueCompletionBehavior(ValueCompletionBehavior::None);
    url.setEnvironmentVariable("SYNCTHING_CTL_URL");
    url.setValueCompletionBehavior(ValueCompletionBehavior::None);
    certificate.setEnvironmentVariable("SYNCTHING_CTL_CERTIFICATE");
}

} // namespace Cli
