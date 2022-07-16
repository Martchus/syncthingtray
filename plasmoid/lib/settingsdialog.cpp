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

void addPlasmoidSpecificNote(QLayout *layout, QWidget *parent)
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

bool AppearanceOptionPage::apply()
{
    KConfigGroup config = m_applet->config();
    config.writeEntry<QSize>("size", QSize(ui()->widthSpinBox->value(), ui()->heightSpinBox->value()));
    config.writeEntry("passiveStates", m_passiveStatusSelection.toVariantList());

    return true;
}

void AppearanceOptionPage::reset()
{
    const KConfigGroup config = m_applet->config();
    const auto size(config.readEntry<>("size", QSize(25, 25)));
    ui()->widthSpinBox->setValue(size.width());
    ui()->heightSpinBox->setValue(size.height());
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
    category->setDisplayName(QCoreApplication::translate("Plasmoid::SettingsDialog", "Plasmoid"));
    category->assignPages({ new ConnectionOptionPage(applet.connection()), new NotificationsOptionPage(GuiType::Plasmoid), m_appearanceOptionPage,
        new IconsOptionPage, new ShortcutOptionPage(applet) });
    category->setIcon(QIcon::fromTheme(QStringLiteral("plasma")));
    categories << category;

    // most startup options don't make much sense for a Plasmoid, so merge webview with startup
    auto *const webViewPage = new WebViewOptionPage;
    webViewPage->widget()->setWindowTitle(QCoreApplication::translate("Plasmoid::SettingsDialog", "Web view"));
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    auto *const systemdPage = new SystemdOptionPage;
#endif

    category = new OptionCategory;
    category->setDisplayName(QCoreApplication::translate("Plasmoid::SettingsDialog", "Extras"));
    category->assignPages({ webViewPage
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
