#include "./settingsdialog.h"
#include "./syncthingapplet.h"

#include "ui_appearanceoptionpage.h"

#include <syncthingwidgets/settings/settingsdialog.h>

#include <qtutilities/settingsdialog/optioncategory.h>
#include <qtutilities/settingsdialog/optioncategorymodel.h>
#include <qtutilities/settingsdialog/optionpage.h>
#include <qtutilities/settingsdialog/settingsdialog.h>

#include <KConfigGroup>

#include <QCoreApplication>
#include <QFormLayout>
#include <QKeySequenceEdit>
#include <QVBoxLayout>

using namespace Data;
using namespace QtGui;
using namespace QtUtilities;

namespace Plasmoid {

static void addPlasmoidSpecificNote(QLayout *layout, QWidget *parent)
{
    auto *const infoLabel = new QLabel(
        QCoreApplication::translate("Plasmoid::Settings", "The settings on this page are specific to the current instance of the Plasmoid."), parent);
    infoLabel->setWordWrap(true);
    QFont infoFont(infoLabel->font());
    infoFont.setBold(true);
    infoLabel->setFont(infoFont);
    auto *const line = new QFrame(parent);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);
    layout->addWidget(infoLabel);
}

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
    m_applet->setGlobalShortcut(m_globalShortcutEdit->keySequence());
    return true;
}

void ShortcutOptionPage::reset()
{
    m_globalShortcutEdit->setKeySequence(m_applet->globalShortcut());
}

QWidget *ShortcutOptionPage::setupWidget()
{
    auto *const widget = new QWidget();
    widget->setWindowTitle(QCoreApplication::translate("Plasmoid::ShortcutOptionPage", "Shortcuts"));
    widget->setWindowIcon(QIcon::fromTheme(QStringLiteral("configure-shortcuts")));
    widget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    auto *const mainLayout = new QVBoxLayout(widget);
    auto *const formLayout = new QFormLayout;
    formLayout->addRow(
        QCoreApplication::translate("Plasmoid::ShortcutOptionPage", "Global shortcut"), m_globalShortcutEdit = new QKeySequenceEdit(widget));
    mainLayout->addLayout(formLayout);
    mainLayout->addStretch(1);
    addPlasmoidSpecificNote(mainLayout, widget);
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

/*!
 * \brief Writes the appearance config to the applet-specific config.
 */
bool AppearanceOptionPage::apply()
{
    auto config = m_applet->config();
    config.writeEntry("size", QSize(ui()->widthSpinBox->value(), ui()->heightSpinBox->value()));
    config.writeEntry("showTabTexts", ui()->showTabTextsCheckBox->isChecked());
    config.writeEntry("showDownloads", ui()->showDownloadsCheckBox->isChecked());
    config.writeEntry("preferIconsFromTheme", ui()->preferIconsFromThemeCheckBox->isChecked());
    config.writeEntry("defaultTab", QtGui::AppearanceOptionPage::comboBoxIndexToTabIndex(ui()->defaultTabComboBox->currentIndex()));
    config.writeEntry("passiveStates", m_passiveStatusSelection.toVariantList());
    return true;
}

/*!
 * \brief Restores the appearance config from the applet-specific config.
 * \remarks
 * This function reads config entries mainly using the readEntry() overloads that take template parameters. When I remember correctly, in KF5
 * this needed explicit use of `<>`. So I am not removing the `<>` even though it doesn't seem to be required when using KF6.
 */
void AppearanceOptionPage::reset()
{
    const auto config = m_applet->config();
    const auto size = config.readEntry<>("size", QSize(25, 25));
    ui()->widthSpinBox->setValue(size.width());
    ui()->heightSpinBox->setValue(size.height());
    ui()->showTabTextsCheckBox->setChecked(config.readEntry<>("showTabTexts", false));
    ui()->showDownloadsCheckBox->setChecked(config.readEntry<>("showDownloads", false));
    ui()->preferIconsFromThemeCheckBox->setChecked(config.readEntry<>("preferIconsFromTheme", false));
    ui()->defaultTabComboBox->setCurrentIndex(QtGui::AppearanceOptionPage::tabIndexToComboBoxIndex(config.readEntry<>("defaultTab", 0)));
    m_passiveStatusSelection.applyVariantList(config.readEntry("passiveStates", QVariantList()));
}

QWidget *AppearanceOptionPage::setupWidget()
{
    auto *const widget = AppearanceOptionPageBase::setupWidget();
    addPlasmoidSpecificNote(ui()->verticalLayout, widget);
    ui()->passiveListView->setModel(&m_passiveStatusSelection);
    return widget;
}

SettingsDialog::SettingsDialog(Plasmoid::SyncthingApplet &applet)
{
    // setup categories
    QList<OptionCategory *> categories;
    OptionCategory *category;

    category = new OptionCategory;
    m_appearanceOptionPage = new AppearanceOptionPage(applet);
    translateCategory(category, [] { return QCoreApplication::translate("Plasmoid::SettingsDialog", "Plasmoid"); });
    category->assignPages({ new ConnectionOptionPage(applet.connection()), new NotificationsOptionPage(GuiType::Plasmoid), m_appearanceOptionPage,
        new IconsOptionPage, new ShortcutOptionPage(applet) });
    category->setIcon(QIcon::fromTheme(QStringLiteral("plasma")));
    categories << category;

    // most startup options don't make much sense for a Plasmoid, so merge webview with startup
    auto *const generalWebViewPage = new GeneralWebViewOptionPage;
    auto *const builtinWebViewPage = new BuiltinWebViewOptionPage;
    auto setWindowTitle = [generalWebViewPage, builtinWebViewPage] {
        generalWebViewPage->widget()->setWindowTitle(QCoreApplication::translate("Plasmoid::SettingsDialog", "General web view settings"));
        builtinWebViewPage->widget()->setWindowTitle(QCoreApplication::translate("Plasmoid::SettingsDialog", "Built-in web view"));
    };
    setWindowTitle();
    connect(this, &QtUtilities::SettingsDialog::retranslationRequired, this, std::move(setWindowTitle));
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    auto *const systemdPage = new SystemdOptionPage;
#endif

    category = new OptionCategory;
    translateCategory(category, [] { return QCoreApplication::translate("Plasmoid::SettingsDialog", "Extras"); });
    category->assignPages({ generalWebViewPage, builtinWebViewPage
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
        ,
        systemdPage
#endif
    });
    category->setIcon(
        QIcon::fromTheme(QStringLiteral("preferences-other"), QIcon(QStringLiteral(":/icons/hicolor/scalable/apps/preferences-other.svg"))));
    categories << category;
    categoryModel()->setCategories(categories);
}

} // namespace Plasmoid
