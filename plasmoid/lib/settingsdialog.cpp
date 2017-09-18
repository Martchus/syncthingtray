#include "./settingsdialog.h"
#include "./syncthingapplet.h"

#include "ui_appearanceoptionpage.h"

#include "../../widgets/settings/settingsdialog.h"

#include <qtutilities/settingsdialog/optioncategory.h>
#include <qtutilities/settingsdialog/optionpage.h>
#include <qtutilities/settingsdialog/settingsdialog.h>

#include <KConfigGroup>

#include <QCoreApplication>
#include <QFormLayout>
#include <QKeySequenceEdit>
#include <QVBoxLayout>

using namespace Dialogs;
using namespace Data;
using namespace QtGui;

namespace Plasmoid {

// ShortcutOptionPage
ShortcutOptionPage::ShortcutOptionPage(SyncthingApplet &applet, QWidget *parentWidget)
    : ShortcutOptionPageBase(parentWidget)
    , m_applet(&applet)
{
}

ShortcutOptionPage::~ShortcutOptionPage()
{
}

bool ShortcutOptionPage::apply()
{
    if (hasBeenShown()) {
        m_applet->setGlobalShortcut(m_globalShortcutEdit->keySequence());
    }
    return true;
}

void ShortcutOptionPage::reset()
{
    if (hasBeenShown()) {
        m_globalShortcutEdit->setKeySequence(m_applet->globalShortcut());
    }
}

QWidget *ShortcutOptionPage::setupWidget()
{
    QWidget *widget = new QWidget();
    widget->setWindowTitle(QCoreApplication::translate("Plasmoid::ShortcutOptionPage", "Shortcuts"));
    widget->setWindowIcon(QIcon::fromTheme(QStringLiteral("configure-shortcuts")));
    QVBoxLayout *mainLayout = new QVBoxLayout(widget);
    QFormLayout *formLayout = new QFormLayout(widget);
    formLayout->addRow(
        QCoreApplication::translate("Plasmoid::ShortcutOptionPage", "Global shortcut"), m_globalShortcutEdit = new QKeySequenceEdit(widget));
    mainLayout->addLayout(formLayout);
    widget->setLayout(mainLayout);
    return widget;
}

// AppearanceOptionPage
AppearanceOptionPage::AppearanceOptionPage(SyncthingApplet &applet, QWidget *parentWidget)
    : AppearanceOptionPageBase(parentWidget)
    , m_applet(&applet)
{
}

AppearanceOptionPage::~AppearanceOptionPage()
{
}

bool AppearanceOptionPage::apply()
{
    if (hasBeenShown()) {
        KConfigGroup config = m_applet->config();

        config.writeEntry<QSize>("size", QSize(ui()->widthSpinBox->value(), ui()->heightSpinBox->value()));
        config.writeEntry<bool>("brightColors", ui()->brightTextColorsCheckBox->isChecked());
    }
    return true;
}

void AppearanceOptionPage::reset()
{
    if (hasBeenShown()) {
        const KConfigGroup config = m_applet->config();

        const QSize size(config.readEntry<QSize>("size", QSize(25, 25)));
        ui()->widthSpinBox->setValue(size.width());
        ui()->heightSpinBox->setValue(size.height());

        ui()->brightTextColorsCheckBox->setChecked(config.readEntry<bool>("brightColors", false));
    }
}

QtGui::SettingsDialog *setupSettingsDialog(SyncthingApplet &applet)
{
    // setup categories
    QList<Dialogs::OptionCategory *> categories;
    Dialogs::OptionCategory *category;

    category = new OptionCategory;
    category->setDisplayName(QCoreApplication::translate("Plasmoid::SettingsDialog", "Plasmoid"));
    category->assignPages(QList<Dialogs::OptionPage *>()
        << new ConnectionOptionPage(applet.connection()) << new NotificationsOptionPage(GuiType::Plasmoid) << new AppearanceOptionPage(applet)
        << new ShortcutOptionPage(applet));
    category->setIcon(QIcon::fromTheme(QStringLiteral("plasma")));
    categories << category;

    // most startup options don't make much sense for a Plasmoid, so merge webview with startup
    auto *const webViewPage = new WebViewOptionPage;
    webViewPage->widget()->setWindowTitle(QCoreApplication::translate("Plasmoid::SettingsDialog", "Web view"));
    webViewPage->widget()->setWindowIcon(
        QIcon::fromTheme(QStringLiteral("internet-web-browser"), QIcon(QStringLiteral(":/icons/hicolor/scalable/apps/internet-web-browser.svg"))));
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    auto *const systemdPage = new SystemdOptionPage;
    systemdPage->widget()->setWindowIcon(
        QIcon::fromTheme(QStringLiteral("system-run"), QIcon(QStringLiteral(":/icons/hicolor/scalable/apps/system-run.svg"))));
#endif

    category = new OptionCategory;
    category->setDisplayName(QCoreApplication::translate("Plasmoid::SettingsDialog", "Extras"));
    category->assignPages(QList<Dialogs::OptionPage *>() << webViewPage
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
                                                         << systemdPage
#endif
    );
    category->setIcon(
        QIcon::fromTheme(QStringLiteral("preferences-other"), QIcon(QStringLiteral(":/icons/hicolor/scalable/apps/preferences-other.svg"))));
    categories << category;

    return new ::QtGui::SettingsDialog(categories);
}
} // namespace Plasmoid
