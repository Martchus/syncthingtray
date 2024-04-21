import * as AjaxHelper from './ajaxhelper.js'
import * as SinglePage from './singlepage.js';

function main()
{
    SinglePage.initPage({
        'intro': {
        },
        'downloads': {
            initializer: initializeDownloadsSection,
            state: {params: undefined},
        },
        'doc': {
        },
        'contact': {
        },
    });
}

function initializeDownloadsSection()
{
    if (window.downloadsInitialized) {
        return true;
    }
    const query = new URLSearchParams(window.location.search);
    queryReleases();
    renderUserAgent(query.get("useragent") ?? window.navigator.userAgent);
    return window.downloadsInitialized = true;
}

function renderUserAgent(userAgent)
{
    const platform = determinePlatformFromUserAgent(userAgent);
    const platformCheckbox = document.getElementById("downloads-checkbox-" + platform);
    if (platformCheckbox) {
        platformCheckbox.checked = true;
    }
}

function queryReleases()
{
    AjaxHelper.queryRoute("GET", "https://api.github.com/repos/Martchus/syncthingtray/releases", (xhr, ok) => {
        if (!ok) {
            return;
        }
        const releases = JSON.parse(xhr.responseText);
        for (const release of releases) {
            if (!release.draft) {
                return renderRelease(release);
            }
        }
    });
}

function determinePlatformFromAssetName(name)
{
    if (name.includes("mingw32")) {
        return name.includes("-qt5") && !name.includes("-qt6") ? "windows" : "windows10";
    } else if (name.includes("pc-linux-gnu")) {
        return "pc-linux-gnu";
    }
}

function determinePlatformFromUserAgent(userAgent)
{
    if (userAgent.includes("Linux")) {
        return "pc-linux-gnu";
    } else if (userAgent.includes("Windows")) {
        return "windows10";
    }
}

function determineDisplayNameForAsset(name)
{
    let arch;
    if (name.includes("i686-")) {
        arch = "32-bit (Intel/AMD)";
    } else if (name.includes("x86_64-")) {
        arch = "64-bit (Intel/AMD)";
    }
    let component;
    if (name.startsWith("syncthingctl-")) {
        component = "Additional command-line client for Syncthing";
    } else if (name.startsWith("syncthingtray-")) {
        component = "Tray application";
    }
    if (arch && component) {
        return `${arch}: ${component}`;
    }
    return name;
}

function renderAsset(asset)
{
    const name = asset.name;
    if (name.endsWith(".sig")) {
        return;
    }
    const platform = determinePlatformFromAssetName(name);
    const platformList = document.getElementById("downloads-platform-" + platform) ?? document.getElementById("downloads-platform-other");
    const liElement = document.createElement("li");
    const aElement = document.createElement("a");
    const important = name.startsWith("syncthingtray-");
    liElement.id = "downloads-asset-" + name;
    aElement.target = "blank";
    aElement.href = asset.browser_download_url;
    aElement.appendChild(document.createTextNode(determineDisplayNameForAsset(name)));
    liElement.appendChild(aElement);
    if (important) {
        aElement.style.fontWeight = "bold";
        platformList.prepend(liElement);
    } else {
        platformList.appendChild(liElement);
    }
}

function renderAssetSignature(asset)
{
    const name = asset.name;
    if (!name.endsWith(".sig")) {
        return;
    }
    const nameWithoutSig = name.substr(0, name.length - 4);
    const liElement = document.getElementById("downloads-asset-" + nameWithoutSig);
    if (!liElement) {
        return;
    }
    const aElement = document.createElement("a");
    aElement.target = "blank";
    aElement.href = asset.browser_download_url;
    aElement.appendChild(document.createTextNode("signature"));
    liElement.appendChild(document.createTextNode(" ("));
    liElement.appendChild(aElement);
    liElement.appendChild(document.createTextNode(")"));
}

function renderRelease(releaseInfo)
{
    const releaseName = releaseInfo.name ?? "unknown";
    const releaseDate = releaseInfo.published_at ?? "unknown";
    document.getElementById("downloads-latest-release").innerText = `${releaseName} from ${releaseDate}`;

    const assets = Array.isArray(releaseInfo.assets) ? releaseInfo.assets : [];
    for (const asset of assets) {
        renderAsset(asset);
    }
    for (const asset of assets) {
        renderAssetSignature(asset);
    }
    const lists = document.querySelectorAll(".downloads-platform ul");
    for (const list of lists) {
        if (!list.firstChild) {
            list.parentElement.style.display = 'none';
        }
    }

    document.getElementById('downloads-loading').style.display = 'none';
    document.getElementById('downloads-release-info').style.display = 'block';
}

main();
