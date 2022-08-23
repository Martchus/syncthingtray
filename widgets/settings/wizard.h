#ifndef SETTINGS_WIZARD_H
#define SETTINGS_WIZARD_H

#include "../global.h"

#include <syncthingconnector/syncthingconfig.h>

#include <QByteArray>
#include <QProcess>
#include <QTimer>
#include <QWizard>
#include <QWizardPage>

#include <optional>

QT_FORWARD_DECLARE_CLASS(QProgressBar)
QT_FORWARD_DECLARE_CLASS(QLabel)

namespace Data {
class SyncthingConnection;
class SyncthingService;
class SyncthingProcess;
class SyncthingLauncher;
} // namespace Data

namespace QtGui {

class DetectionWizardPage;

class SYNCTHINGWIDGETS_EXPORT Wizard : public QWizard {
    Q_OBJECT

public:
    explicit Wizard(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
    ~Wizard() override;

    static Wizard *instance();
    DetectionWizardPage *detectionPage() const;

Q_SIGNALS:
    void settingsRequested();

private:
    static Wizard *s_instance;
    DetectionWizardPage *m_detectionPage;
};

inline DetectionWizardPage *Wizard::detectionPage() const
{
    return m_detectionPage;
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

private Q_SLOTS:
    void tryToConnect();
    void handleConnectionStatusChanged();
    void handleConnectionError(const QString &error);
    void handleLauncherExit(int exitCode, QProcess::ExitStatus exitStatus);
    void handleLauncherError(QProcess::ProcessError error);
    void handleLauncherOutput(const QByteArray &output);
    void handleTimeout();
    void continueWithSummaryIfDone();
    void showSummary();

private:
    QString m_configFilePath;
    QString m_certPath;
    QStringList m_connectionErrors;
    Data::SyncthingConfig m_config;
    Data::SyncthingConnection *m_connection;
    Data::SyncthingService *m_userService;
    Data::SyncthingService *m_systemService;
    Data::SyncthingLauncher *m_launcher;
    QTimer m_timeoutTimer;

    QProgressBar *m_progressBar;
    QLabel *m_logLabel;
    std::optional<int> m_launcherExitCode;
    std::optional<QProcess::ExitStatus> m_launcherExitStatus;
    std::optional<QProcess::ProcessError> m_launcherError;
    QByteArray m_launcherOutput;
    bool m_timedOut;
    bool m_configOk;
};

} // namespace QtGui

#endif // SETTINGS_WIZARD_H
