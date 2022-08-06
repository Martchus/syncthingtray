#ifndef SETTINGS_WIZARD_H
#define SETTINGS_WIZARD_H

#include "../global.h"

#include <QWizard>
#include <QWizardPage>

namespace QtGui {

class SYNCTHINGWIDGETS_EXPORT Wizard : public QWizard {
    Q_OBJECT

public:
    explicit Wizard(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
    ~Wizard() override;

    static Wizard *instance();

Q_SIGNALS:
    void settingsRequested();

private:
    static Wizard *s_instance;
};

class SYNCTHINGWIDGETS_EXPORT WelcomeWizardPage : public QWizardPage {
    Q_OBJECT

public:
    explicit WelcomeWizardPage(QWidget *parent = nullptr);

    bool isComplete() const override;
};

} // namespace QtGui

#endif // SETTINGS_WIZARD_H
