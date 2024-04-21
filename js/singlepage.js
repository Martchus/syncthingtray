export let sections = {};
export let sectionNames = [];

/// \brief 'main()' function which initializes the single page app.
export function initPage(pageSections)
{
    sections = pageSections;
    sectionNames = Object.keys(sections);
    handleHashChange();
    document.body.onhashchange = handleHashChange;
}

let preventHandlingHashChange = false;
let preventSectionInitializer = false;

function splitHashParts()
{
    const currentHash = location.hash.substr(1);
    const hashParts = currentHash.split('?');
    for (let i = 0, len = hashParts.length; i != len; ++i) {
        hashParts[i] = decodeURIComponent(hashParts[i]);
    }
    return hashParts;
}


/// \brief Shows the current section and hides other sections.
function handleHashChange()
{
    if (preventHandlingHashChange) {
        return;
    }

    const hashParts = splitHashParts();
    const currentSectionName = hashParts.shift() || 'intro-section';
    if (!currentSectionName.endsWith('-section')) {
        return;
    }

    sectionNames.forEach(function (sectionName) {
        const sectionData = sections[sectionName];
        const sectionElement = document.getElementById(sectionName + '-section');
        if (sectionElement.id === currentSectionName) {
            const sectionInitializer = sectionData.initializer;
            if (sectionInitializer === undefined || preventSectionInitializer || sectionInitializer(sectionElement, sectionData, hashParts)) {
                sectionElement.style.display = 'block';
            }
        } else {
            const sectionDestructor = sectionData.destructor;
            if (sectionDestructor === undefined || sectionDestructor(sectionElement, sectionData, hashParts)) {
                sectionElement.style.display = 'none';
            }
        }
        const navLinkElement = document.getElementById(sectionName + '-nav-link');
        if (sectionElement.id === currentSectionName) {
            navLinkElement.classList.add('active');
        } else {
            navLinkElement.classList.remove('active');
        }
    });
}
