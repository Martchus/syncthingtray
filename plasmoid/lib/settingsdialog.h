#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <qtutilities/settingsdialog/optionpage.h>

QT_FORWARD_DECLARE_CLASS(QKeySequenceEdit)

namespace QtGui {
class SettingsDialog;
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

private:
SyncthingApplet *m_applet;
END_DECLARE_OPTION_PAGE

QtGui::SettingsDialog *setupSettingsDialog(Plasmoid::SyncthingApplet &applet);
} // namespace Plasmoid

#endif // SETTINGSDIALOG_H
