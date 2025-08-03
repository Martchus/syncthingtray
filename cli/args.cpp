#include "./args.h"

#include "resources/config.h"

namespace Cli {

Args::Args()
    : status("status", 's', "shows the overall status and/or folder/device specific status")
    , log("log", 'l', "shows the Syncthing log")
    , stop("stop", '\0', "stops Syncthing")
    , restart("restart", '\0', "restarts Syncthing")
    , rescan("rescan", 'r', "rescans the specified folders")
    , rescanAll("rescan-all", '\0', "rescans all folders")
    , pause("pause", '\0', "pauses the specified folders and devices")
    , resume("resume", '\0', "resumes the specified folders and devices")
    , waitForIdle("wait-for-idle", 'w',
          "waits until the specified dirs/devs are idling\nnote: Directories are considered idling if they are locally up-to-date and NOT "
          "otherwise busy with e.g. scanning. Devices are considered idling if they are disconnected or all directories shared with the device are "
          "remotely up-to-date.")
    , pwd("pwd", 'p', "operates in the current working directory")
    , cat("cat", '\0', "prints the Syncthing configuration as JSON")
    , edit("edit", '\0', "allows editing the Syncthing configuration using an external editor")
    , statusPwd("status", 's', "prints the status of the current working directory")
    , rescanPwd("rescan", 'r', "rescans the current working directory")
    , pausePwd("pause", 'p', "pauses the current working directory")
    , resumePwd("resume", '\0', "resumes the current working directory")
    , script("script", '\0', "runs the specified UTF-8 encoded ECMAScript on the configuration rather than opening an editor", { "path" })
    , jsLines("js-lines", '\0', "runs the specified ECMAScript lines on the configuration rather than opening an editor", { "line" })
    , dryRun("dry-run", '\0', "writes the altered configuration to stdout instead of posting it to Syncthing")
    , stats("stats", '\0', "shows overall statistics")
    , dir("dir", 'd', "specifies a folder by ID", { "ID" })
    , dev("dev", '\0', "specifies a device by ID or name", { "ID/name" })
    , allDirs("all-dirs", '\0', "applies the operation for all folders")
    , allDevs("all-devs", '\0', "applies the operation for all devices")
    , atLeast("at-least", 'a', "specifies for how many milliseconds Syncthing must idle (prevents exiting too early in case of flaky status)",
          { "number" })
    , timeout("timeout", 't', "specifies how many milliseconds to wait at most", { "number" })
    , requireDevsConnected(
          "require-devs-connected", '\0', "requires all specified devices to be connected (by default disconnected devices are considered idling)")
    , editor("editor", '\0', "specifies the editor to be opened", { "editor name", "editor option" })
    , configFile("config-file", 'f', "specifies the Syncthing config file to read API key and URL from, when not explicitly specified", { "path" })
    , apiKey("api-key", 'k', "specifies the API key", { "key" })
    , url("url", 'u', "specifies the Syncthing URL, default is http://localhost:8080", { "URL" })
    , credentials("credentials", 'c', "specifies user name and password", { "user name", "password" })
    , certificate("cert", '\0', "specifies the certificate used by the Syncthing instance", { "path" })
    , requestTimeout("request-timeout", '\0', "specifies the transfer timeout for network requests in milliseconds", { "timeout" })
    , generalTimeout("general-timeout", '\0', "specifies how long to wait for Syncthing in general", { "timeout" })
{
    dir.setConstraints(0, Argument::varValueCount);
    dir.setValueCompletionBehavior(
        ValueCompletionBehavior::PreDefinedValues | ValueCompletionBehavior::Directories | ValueCompletionBehavior::InvokeCallback);
    dev.setConstraints(0, Argument::varValueCount);
    dev.setValueCompletionBehavior(ValueCompletionBehavior::PreDefinedValues | ValueCompletionBehavior::InvokeCallback);
    status.setSubArguments({ &stats, &dir, &dev, &allDirs, &allDevs });
    status.setExample(PROJECT_NAME " status # shows all dirs and devs\n" PROJECT_NAME " status --dir dir1 --dir dir2 --dev dev1 --dev dev2");
    waitForIdle.setSubArguments({ &dir, &dev, &allDirs, &allDevs, &atLeast, &timeout, &requireDevsConnected });
    waitForIdle.setExample(PROJECT_NAME " wait-for-idle --timeout 1800000 --at-least 5000 --all-devs --all-dirs && systemctl poweroff\n" PROJECT_NAME
                                        " wait-for-idle --dir dir1 --dir dir2 --dev dev1 --dev dev2 --at-least 5000");
    pwd.setSubArguments({ &statusPwd, &rescanPwd, &pausePwd, &resumePwd });

    for (auto *arg : { &editor, &script, &jsLines }) {
        arg->setCombinable(false);
    }
    jsLines.setRequiredValueCount(Argument::varValueCount);
    edit.setSubArguments({ &editor, &script, &jsLines, &dryRun });

    rescan.setValueNames({ "dir ID" });
    rescan.setRequiredValueCount(Argument::varValueCount);
    rescan.setValueCompletionBehavior(
        ValueCompletionBehavior::PreDefinedValues | ValueCompletionBehavior::Directories | ValueCompletionBehavior::InvokeCallback);
    rescan.setExample(PROJECT_NAME " rescan dir1 dir2 dir4 dir5");
    pause.setSubArguments({ &dir, &dev, &allDirs, &allDevs });
    pause.setExample(PROJECT_NAME " pause --dir dir1 --dir dir2 --dev dev1 --dev dev2\n" PROJECT_NAME " pause --all-dirs");
    resume.setSubArguments({ &dir, &dev, &allDirs, &allDevs });
    resume.setExample(PROJECT_NAME " resume --dir dir1 --dir dir2 --dev dev1 --dev dev2\n" PROJECT_NAME " resume --all-devs");

    editor.setEnvironmentVariable("EDITOR");
    editor.setRequiredValueCount(Argument::varValueCount);

    configFile.setExample(PROJECT_NAME " status --dir dir1 --config-file ~/.config/syncthing/config.xml");
    credentials.setExample(PROJECT_NAME " status --dir dir1 --credentials name supersecret");

    parser.setMainArguments({ &status, &log, &stop, &restart, &rescan, &rescanAll, &pause, &resume, &waitForIdle, &pwd, &cat, &edit, &configFile,
        &apiKey, &url, &credentials, &certificate, &requestTimeout, &generalTimeout, &parser.noColorArg(), &parser.helpArg() });

    // allow setting default values via environment
    configFile.setEnvironmentVariable("SYNCTHING_CTL_CONFIG_FILE");
    apiKey.setEnvironmentVariable("SYNCTHING_CTL_API_KEY");
    apiKey.setValueCompletionBehavior(ValueCompletionBehavior::None);
    url.setEnvironmentVariable("SYNCTHING_CTL_URL");
    url.setValueCompletionBehavior(ValueCompletionBehavior::None);
    certificate.setEnvironmentVariable("SYNCTHING_CTL_CERTIFICATE");
}

} // namespace Cli
