#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <syncthingmodel/syncthingstatusselectionmodel.h>

#include <syncthingwidgets/settings/settingsdialog.h>

#include <qtutilities/settingsdialog/optionpage.h>

QT_FORWARD_DECLARE_CLASS(QKeySequenceEdit)

namespace QtGui {
class SettingsDialog;
}

namespace Data {
class SyncthingStatusSelectionModel;
}

namespace Plasmoid {
class SyncthingApplet;

BEGIN_DECLARE_OPTION_PAGE_CUSTOM_CTOR(ShortcutOptionPage)
public:
ShortcutOptionPage(SyncthingApplet &applet, QWidget *parentWidget = nullptr);

private:
DECLARE_SETUP_WIDGETS
SyncthingApplet *m_applet;
QKeySequenceEdit *m_globalShortcutEdit;
END_DECLARE_OPTION_PAGE

BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE_CUSTOM_CTOR(AppearanceOptionPage)
public:
AppearanceOptionPage(SyncthingApplet &applet, QWidget *parentWidget = nullptr);
Data::SyncthingStatusSelectionModel *passiveStatusSelection();

private:
DECLARE_SETUP_WIDGETS
SyncthingApplet *m_applet;
Data::SyncthingStatusSelectionModel m_passiveStatusSelection;
END_DECLARE_OPTION_PAGE

inline Data::SyncthingStatusSelectionModel *AppearanceOptionPage::passiveStatusSelection()
{
    return &m_passiveStatusSelection;
}

class SettingsDialog : public QtGui::SettingsDialog {
public:
    SettingsDialog(Plasmoid::SyncthingApplet &applet);

    AppearanceOptionPage *appearanceOptionPage();

private:
    AppearanceOptionPage *m_appearanceOptionPage;
};

inline AppearanceOptionPage *SettingsDialog::appearanceOptionPage()
{
    return m_appearanceOptionPage;
}

QtGui::SettingsDialog *setupSettingsDialog(Plasmoid::SyncthingApplet &applet);
} // namespace Plasmoid

#endif // SETTINGSDIALOG_H
