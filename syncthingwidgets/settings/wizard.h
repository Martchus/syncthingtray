#ifndef SETTINGS_WIZARD_H
#define SETTINGS_WIZARD_H

#include "../global.h"
#include "./wizardenums.h"

#include <QWizard>
#include <QWizardPage>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QProgressBar)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)

namespace Data {
class SyncthingConnection;
}

class WizardTests;

namespace QtGui {

class SetupDetection;

namespace Ui {
class MainConfigWizardPage;
class AutostartWizardPage;
class ApplyWizardPage;
} // namespace Ui

class SYNCTHINGWIDGETS_EXPORT Wizard : public QWizard {
    Q_OBJECT
    friend class WizardTests;

public:
    explicit Wizard(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
    ~Wizard() override;

    static Wizard *instance();
    static bool hasInstance();
    SetupDetection &setupDetection();
    MainConfiguration mainConfig() const;
    ExtraConfiguration extraConfig() const;
    bool autoStart() const;
    bool keepExistingAutoStartEntry() const;
    bool isConfigApplied() const;
    const QString &configError() const;
    Data::SyncthingConnection *appliedConnection();

public Q_SLOTS:
    bool changeSettings();
    void handleConfigurationApplied(const QString &configError = QString(), Data::SyncthingConnection *connection = nullptr);

Q_SIGNALS:
    void settingsDialogRequested();
    void openLauncherSettingsRequested();
    void openSyncthingRequested();
    void settingsChanged();
    void configApplied();

private Q_SLOTS:
    void showDetailsFromSetupDetection();
    void handleConfigurationSelected(QtGui::MainConfiguration mainConfig, QtGui::ExtraConfiguration extraConfig);
    void handleAutostartSelected(bool autostartEnabled, bool keepExisting);
    void pollForSyncthingConfig();

private:
    QString hintAboutSyncthingLog() const;

    static Wizard *s_instance;
    std::unique_ptr<SetupDetection> m_setupDetection;
    MainConfiguration m_mainConfig = MainConfiguration::None;
    ExtraConfiguration m_extraConfig = ExtraConfiguration::None;
    bool m_autoStart = false;
    bool m_autoStartKeepExisting = false;
    bool m_configApplied = false;
    QString m_configError;
    Data::SyncthingConnection *m_appliedConnection = nullptr;
    int m_elapsedPollTime;
};

inline bool Wizard::hasInstance()
{
    return s_instance != nullptr;
}

inline MainConfiguration Wizard::mainConfig() const
{
    return m_mainConfig;
}

inline ExtraConfiguration Wizard::extraConfig() const
{
    return m_extraConfig;
}

inline bool Wizard::autoStart() const
{
    return m_autoStart;
}

inline bool Wizard::keepExistingAutoStartEntry() const
{
    return m_autoStartKeepExisting;
}

inline bool Wizard::isConfigApplied() const
{
    return m_configApplied;
}

inline const QString &Wizard::configError() const
{
    return m_configError;
}

inline Data::SyncthingConnection *Wizard::appliedConnection()
{
    return m_appliedConnection;
}

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
    void showCheckAgainButton();

private Q_SLOTS:
    void tryToConnect();
    void continueIfDone();

private:
    SetupDetection *m_setupDetection;
    QString m_defaultTitle;
    QString m_defaultSubTitle;
    QProgressBar *m_progressBar;
    QPushButton *m_checkAgainButton;
};

class SYNCTHINGWIDGETS_EXPORT MainConfigWizardPage final : public QWizardPage {
    Q_OBJECT

public:
    explicit MainConfigWizardPage(QWidget *parent = nullptr);
    ~MainConfigWizardPage() override;

    bool isComplete() const override;
    void initializePage() override;
    void cleanupPage() override;
    bool validatePage() override;

Q_SIGNALS:
    void retry();
    void configurationSelected(QtGui::MainConfiguration mainConfig, QtGui::ExtraConfiguration extraConfig);

private Q_SLOTS:
    void handleSelectionChanged();

private:
    std::unique_ptr<Ui::MainConfigWizardPage> m_ui;
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    QString m_cfgSystemdUserUnitText;
    QString m_cfgSystemdSystemUnitText;
#endif
    bool m_configSelected;
};

class SYNCTHINGWIDGETS_EXPORT AutostartWizardPage final : public QWizardPage {
    Q_OBJECT

public:
    explicit AutostartWizardPage(QWidget *parent = nullptr);
    ~AutostartWizardPage() override;

    bool isComplete() const override;
    void initializePage() override;
    void cleanupPage() override;
    bool validatePage() override;

Q_SIGNALS:
    void autostartSelected(bool autostartEnabled, bool keepExisting);

private:
    std::unique_ptr<Ui::AutostartWizardPage> m_ui;
    bool m_configSelected;
};

class SYNCTHINGWIDGETS_EXPORT ApplyWizardPage final : public QWizardPage {
    Q_OBJECT

public:
    explicit ApplyWizardPage(QWidget *parent = nullptr);
    ~ApplyWizardPage() override;

    bool isComplete() const override;
    void initializePage() override;
    bool validatePage() override;

private:
    std::unique_ptr<Ui::ApplyWizardPage> m_ui;
};

class SYNCTHINGWIDGETS_EXPORT FinalWizardPage final : public QWizardPage {
    Q_OBJECT

public:
    explicit FinalWizardPage(QWidget *parent = nullptr);
    ~FinalWizardPage() override;

    bool isComplete() const override;
    void initializePage() override;
    bool validatePage() override;

public Q_SLOTS:
    void showResults();

private Q_SLOTS:
    void handleLinkActivated(const QString &href);

private:
    QVBoxLayout *m_layout;
    QLabel *m_label;
    QProgressBar *m_progressBar;
    QWidget *m_ownDeviceIdWidget;
};

} // namespace QtGui

#endif // SETTINGS_WIZARD_H
