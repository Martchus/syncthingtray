#include "./wizard.h"
#include "./settings.h"

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

#include <QCommandLinkButton>
#include <QDesktopServices>
#include <QFrame>
#include <QLabel>
#include <QUrl>
#include <QVBoxLayout>

#include <string_view>

namespace QtGui {

Wizard *Wizard::s_instance = nullptr;

Wizard::Wizard(QWidget *parent, Qt::WindowFlags flags)
    : QWizard(parent, flags)
{
    setWindowTitle(tr("Setup wizard - ") + QStringLiteral(APP_NAME));

    const auto &settings = Settings::values();
    if (settings.firstLaunch || settings.fakeFirstLaunch) {
        addPage(new WelcomeWizardPage());
    }
}

Wizard::~Wizard()
{
    if (this == s_instance) {
        s_instance = nullptr;
    }
}

Wizard *Wizard::instance()
{
    if (!s_instance) {
        s_instance = new Wizard();
        s_instance->setAttribute(Qt::WA_DeleteOnClose, true);
    }
    return s_instance;
}

WelcomeWizardPage::WelcomeWizardPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Welcome to ") + QStringLiteral(APP_NAME));
    setSubTitle(tr("It looks like you're launching %1 for the first time.").arg(QStringLiteral(APP_NAME)));

    auto *const infoLabel = new QLabel(this);
    infoLabel->setText(tr(
        "You must configure how to connect to Syncthing and how to launch Syncthing (if that's wanted) when using Syncthing Tray the first time.  A "
        "guided/automated setup is still in the works so the manual setup is currently the only option."));
    infoLabel->setWordWrap(true);

    auto *const showSettingsCommand = new QCommandLinkButton(this);
    showSettingsCommand->setText(tr("Configure connection and launcher settings manually"));
    showSettingsCommand->setDescription(
        tr("Note that the connection settings allow importing URL, credentials and API-key from the local Syncthing configuration.")
            .arg(QStringLiteral(APP_NAME)));
    showSettingsCommand->setIcon(QIcon::fromTheme(QStringLiteral("preferences-other")));
    connect(showSettingsCommand, &QCommandLinkButton::clicked, this, [this] {
        if (auto *const wizard = qobject_cast<Wizard *>(this->wizard())) {
            emit wizard->settingsRequested();
            wizard->close();
        }
    });

    auto *const line = new QFrame(this);
    line->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    auto *const showDocsCommand = new QCommandLinkButton(this);
    showDocsCommand->setText(tr("Show Syncthing's documentation"));
    showDocsCommand->setDescription(tr("It contains general information about configuring Syncthing.").arg(QStringLiteral(APP_NAME)));
    showDocsCommand->setIcon(QIcon::fromTheme(QStringLiteral("help-contents")));
    connect(showDocsCommand, &QCommandLinkButton::clicked, this, [] { QDesktopServices::openUrl(QStringLiteral("https://docs.syncthing.net/")); });

    auto *const showReadmeCommand = new QCommandLinkButton(this);
    showReadmeCommand->setText(tr("Show %1's README").arg(QStringLiteral(APP_NAME)));
    showReadmeCommand->setDescription(tr("It contains documentation about this GUI integration specifically.").arg(QStringLiteral(APP_NAME)));
    showReadmeCommand->setIcon(showDocsCommand->icon());
    connect(showReadmeCommand, &QCommandLinkButton::clicked, this, [] {
        if constexpr (std::string_view(APP_VERSION).find('-') == std::string_view::npos) {
            QDesktopServices::openUrl(QStringLiteral(APP_URL "/blob/v" APP_VERSION "/README.md"));
        } else {
            QDesktopServices::openUrl(QStringLiteral(APP_URL "/blob/master/README.md"));
        }
    });

    auto *const layout = new QVBoxLayout;
    layout->addWidget(infoLabel);
    layout->addWidget(showSettingsCommand);
    layout->addStretch();
    layout->addWidget(line);
    layout->addWidget(showDocsCommand);
    layout->addWidget(showReadmeCommand);
    setLayout(layout);
}

bool WelcomeWizardPage::isComplete() const
{
    return false;
}

} // namespace QtGui
