#ifndef SETTINGS_WIZARD_H
#define SETTINGS_WIZARD_H

#include "../global.h"

#include <QWizard>
#include <QWizardPage>

#include <memory>

namespace QtGui {

class SetupDetection;

namespace Ui {
class MainConfigWizardPage;
}

class SYNCTHINGWIDGETS_EXPORT Wizard : public QWizard {
    Q_OBJECT

public:
    explicit Wizard(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
    ~Wizard() override;

    static Wizard *instance();
    SetupDetection &setupDetection();

Q_SIGNALS:
    void settingsRequested();

private Q_SLOTS:
    void showDetailsFromSetupDetection();

private:
    static Wizard *s_instance;
    std::unique_ptr<SetupDetection> m_setupDetection;
};

class SYNCTHINGWIDGETS_EXPORT WelcomeWizardPage final : public QWizardPage {
    Q_OBJECT

public:
    explicit WelcomeWizardPage(QWidget *parent = nullptr);

    bool isComplete() const override;
};

class SYNCTHINGWIDGETS_EXPORT DetectionWizardPage final : public QWizardPage {
    Q_OBJECT

public:
    explicit DetectionWizardPage(QWidget *parent = nullptr);

    bool isComplete() const override;
    void initializePage() override;
    void cleanupPage() override;

public Q_SLOTS:
    void refresh();

private Q_SLOTS:
    void tryToConnect();
    void continueIfDone();

private:
    SetupDetection *m_setupDetection;
};

class SYNCTHINGWIDGETS_EXPORT MainConfigWizardPage final : public QWizardPage {
    Q_OBJECT

public:
    explicit MainConfigWizardPage(QWidget *parent = nullptr);
    ~MainConfigWizardPage() override;

    bool isComplete() const override;
    void initializePage() override;
    void cleanupPage() override;

Q_SIGNALS:
    void retry();

private Q_SLOTS:
    void handleSelectionChanged();

private:
    std::unique_ptr<Ui::MainConfigWizardPage> m_ui;
    bool m_configSelected;
};

} // namespace QtGui

#endif // SETTINGS_WIZARD_H
