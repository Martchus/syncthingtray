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
            document.getElementById('downloads-loading').innertText = 'Unable to load releases via GitHub API: ' + (xhr.responseText ?? 'unknown error');
            return;
        }
        const releases = JSON.parse(xhr.responseText);
        for (const release of releases) {
            if (!release.draft && !release.prerelease) {
                return renderRelease(release, releases);
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
    } else if (name.includes("android")) {
        return "android";
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
    } else if (name.includes("aarch64-")) {
        arch = "64-bit (ARM)";
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
    if (name.endsWith(".sig") || name.includes('initial-build')) {
        return;
    }
    const platform = determinePlatformFromAssetName(name);
    const platformList = document.getElementById("downloads-platform-" + platform) ?? document.getElementById("downloads-platform-other");
    renderAssetIntoList(name, platform, platformList, asset);
}

function renderAssetIntoList(name, platform, platformList, asset, release)
{
    const liElement = document.createElement("li");
    const aElement = document.createElement("a");
    const important = name.startsWith("syncthingtray-");
    liElement.id = "downloads-asset-" + name;
    aElement.target = "blank";
    aElement.href = asset.browser_download_url;
    aElement.appendChild(document.createTextNode(determineDisplayNameForAsset(name)));
    liElement.appendChild(aElement);
    const releaseName = release?.name;
    if (releaseName !== undefined) {
        const spanElement = document.createElement("span");
        spanElement.className = "download-remark";
        spanElement.title = "The latest release does not provide downloads for this platform yet. Therefore the download from the next most recent release is shown. Most likely the uploads of the latest release are still in progress and will show up soon.";
        spanElement.appendChild(document.createTextNode(` from release ${releaseName}`));
        liElement.appendChild(spanElement);
    }
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

function findAssetsForPlatform(platform, releases)
{
    for (const release of releases) {
        const assets = release.assets;
        if (!Array.isArray(assets)) {
            continue;
        }
        const relevantAssets = [];
        const relevantSignatures = [];
        for (const asset of assets) {
            const assetPlatform = "downloads-platform-" + determinePlatformFromAssetName(asset.name);
            if (assetPlatform === platform) {
                (asset.name.endsWith(".sig") ? relevantSignatures : relevantAssets).push(asset);
            }
        }
        if (relevantAssets.length !== 0) {
            return {assets: relevantAssets, signatures: relevantSignatures, release: release};
        }
    }
}

function renderRelease(releaseInfo, otherReleases)
{
    const releaseName = releaseInfo.name ?? "unknown";
    const releaseDate = releaseInfo.published_at ?? "unknown";
    const releaseNotes = releaseInfo.body ?? '';
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
        if (list.firstChild) {
            continue;
        }
        if (!list.classList.contains("download-list-important")) {
            list.parentElement.style.display = 'none';
            continue;
        }
        const platform = list.id;
        const assetsFromOtherRelease = findAssetsForPlatform(platform, otherReleases);
        if (assetsFromOtherRelease === undefined) {
            const liElement = document.createElement("li");
            liElement.appendChild(document.createTextNode("There are no binaries available for this platform at this point. You may find binaries of older releases on GitHub."));
            list.appendChild(liElement);
            continue;
        }
        for (const asset of assetsFromOtherRelease.assets) {
            renderAssetIntoList(asset.name, platform, list, asset, assetsFromOtherRelease.release);
        }
        for (const signature of assetsFromOtherRelease.signatures) {
            renderAssetSignature(signature);
        }
    }

    document.getElementById('downloads-loading').style.display = 'none';
    document.getElementById('downloads-release-info').style.display = 'block';

    if (releaseNotes.length > 10) {
        const releaseNotesElement = document.getElementById("downloads-release-notes");
        let formatted = releaseNotes;
        formatted = formatted.replaceAll(/\*\*([^\*]*)\*\*/gi, '<strong>$1</strong>');
        formatted = formatted.replaceAll(/\*([^\*]*)\*/gi, '<em>$1</em>');
        formatted = formatted.replaceAll(/\~\~([^\~]*)\~\~/gi, '<del>$1</del>');
        releaseNotesElement.insertAdjacentHTML("beforeend", formatted);
        releaseNotesElement.style.display = 'block';
    }
}

main();
