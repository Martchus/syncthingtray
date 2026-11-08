#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SERVICE

#include "./syncthingservice.h"
#include "./utils.h"

#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD)
#include "loginmanagerinterface.h"
#include "managerinterface.h"
#include "propertiesinterface.h"
#include "serviceinterface.h"
#include "unitinterface.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QProcess>
#include <QStandardPaths>

#include <array>
#include <functional>

#elif defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_WINDOWS_SERVICE)
#include "./syncthingconfig.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMetaObject>
#include <QProcess>

#include <windows.h>

#include <sddl.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <winsvc.h>

#include <iostream>
#include <vector>
#endif

using namespace std;
#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD)
using namespace std::placeholders;
#endif
using namespace CppUtilities;

namespace Data {

/*!
 * \class SyncthingService
 * \brief The SyncthingService class controls and monitors Syncthing as a system service.
 * \remarks
 * - This class supports systemd and the Windows service management. The support for Windows service
 *   management is still experimental and not enabled by default.
 * - The systemd implementation uses systemd's D-Bus interface. So the service "org.freedesktop.systemd1"
 *   must be running on the system or user-session D-Bus. The documentation contains a section about
 *   troubleshooting the required setup.
 */

/// \cond

#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD)
QDBusArgument &operator<<(QDBusArgument &argument, const ManagerDBusUnitFileChange &unitFileChange)
{
    argument.beginStructure();
    argument << unitFileChange.type << unitFileChange.path << unitFileChange.source;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, ManagerDBusUnitFileChange &unitFileChange)
{
    argument.beginStructure();
    argument >> unitFileChange.type >> unitFileChange.path >> unitFileChange.source;
    argument.endStructure();
    return argument;
}

constexpr DateTime dateTimeFromSystemdTimeStamp(qulonglong timeStamp)
{
    return DateTime(DateTime::unixEpochStart().totalTicks() + timeStamp * 10);
}
#endif

SyncthingService *SyncthingService::s_mainInstance = nullptr;
DateTime SyncthingService::s_lastWakeUp = DateTime();
bool SyncthingService::s_fallingAsleep = false;

#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD)
OrgFreedesktopSystemd1ManagerInterface *SyncthingService::s_systemdUserInterface = nullptr;
OrgFreedesktopSystemd1ManagerInterface *SyncthingService::s_systemdSystemInterface = nullptr;
OrgFreedesktopLogin1ManagerInterface *SyncthingService::s_loginManager = nullptr;

#elif defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_WINDOWS_SERVICE)
static auto s_serviceStatus = SERVICE_STATUS{};
static auto s_serviceStatusHandle = SERVICE_STATUS_HANDLE(nullptr);
static auto s_serviceRoutine = std::function<int(void)>();
static auto s_stopRoutine = std::function<void(void)>();
static LPCWSTR s_serviceLogName = L"syncthing";

/*!
 * \brief Logs the specified Windows \a errorCode.
 */
static void logLastWindowsError(std::wstring_view context, DWORD errorCode)
{
    LPWSTR buffer = nullptr;
    const auto size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
    if (size == 0 || !buffer) {
        std::wcerr << context << L"Error code " << errorCode << '\n';
    } else {
        std::wcerr << context << std::wstring_view(buffer, static_cast<std::size_t>(size)) << '\n';
    }
    LocalFree(buffer);
}

/*!
 * \brief Converts the specified Windows \a errorCode to a QString.
 */
static QString windowsErrorString(DWORD errorCode)
{
    LPWSTR buffer = nullptr;
    const auto size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
    if (size == 0 || !buffer) {
        return QStringLiteral("Error code %1").arg(errorCode);
    }
    const auto str = QString::fromWCharArray(buffer, static_cast<qsizetype>(size)).trimmed();
    LocalFree(buffer);
    return str;
}

static void reportWindowsServiceEvent(WORD type, DWORD eventId, const QString &message)
{
    const auto source = RegisterEventSourceW(nullptr, L"SyncthingTray");
    if (!source) {
        return;
    }
    const auto wideMessage = message.toStdWString();
    auto *strings = wideMessage.c_str();
    ReportEventW(source, type, 0, eventId, nullptr, 1, 0, &strings, nullptr);
    DeregisterEventSource(source);
}

static void stopChildProcesses()
{
    const auto currentPid = GetCurrentProcessId();
    const auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return;
    }
    auto pe = PROCESSENTRY32W{};
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snapshot, &pe)) {
        do {
            if (pe.th32ParentProcessID == currentPid) {
                if (const auto childProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID)) {
                    TerminateProcess(childProcess, 0);
                    CloseHandle(childProcess);
                }
            }
        } while (Process32NextW(snapshot, &pe));
    }
    CloseHandle(snapshot);
}

/*!
 * \brief Internally called to stop the service.
 */
static void WINAPI serviceCtrlHandler(DWORD control)
{
    switch (control) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        s_serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        s_serviceStatus.dwWaitHint = 10000;
        SetServiceStatus(s_serviceStatusHandle, &s_serviceStatus);
        if (s_stopRoutine) {
            s_stopRoutine();
        }
        stopChildProcesses();
        break;
    default:
        break;
    }
}

/*!
 * \brief Internally called to run the service routine.
 */
static void WINAPI serviceMain(DWORD argc, LPWSTR *argv)
{
    const auto serviceName = (argc >= 1 && argv && argv[0]) ? argv[0] : const_cast<LPWSTR>(L"syncthing");
    s_serviceLogName = serviceName;
    s_serviceStatusHandle = RegisterServiceCtrlHandlerW(serviceName, serviceCtrlHandler);
    if (!s_serviceStatusHandle) {
        reportWindowsServiceEvent(EVENTLOG_ERROR_TYPE, 0,
            QCoreApplication::translate("Data::SyncthingService", "Unable to register service control handler for \"%1\": %2")
                .arg(QString::fromWCharArray(serviceName), windowsErrorString(GetLastError())));
        return;
    }
    s_serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    s_serviceStatus.dwCurrentState = SERVICE_START_PENDING;
    s_serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    s_serviceStatus.dwWin32ExitCode = NO_ERROR;
    s_serviceStatus.dwServiceSpecificExitCode = 0;
    s_serviceStatus.dwCheckPoint = 0;
    s_serviceStatus.dwWaitHint = 10000;
    SetServiceStatus(s_serviceStatusHandle, &s_serviceStatus);

    s_serviceStatus.dwCurrentState = SERVICE_RUNNING;
    s_serviceStatus.dwCheckPoint = 0;
    s_serviceStatus.dwWaitHint = 0;
    SetServiceStatus(s_serviceStatusHandle, &s_serviceStatus);
    reportWindowsServiceEvent(EVENTLOG_INFORMATION_TYPE, 0,
        QCoreApplication::translate("Data::SyncthingService", "Windows service \"%1\" started.").arg(QString::fromWCharArray(serviceName)));

    const auto exitCode = s_serviceRoutine ? s_serviceRoutine() : EXIT_FAILURE;
    reportWindowsServiceEvent(exitCode == EXIT_SUCCESS ? EVENTLOG_INFORMATION_TYPE : EVENTLOG_WARNING_TYPE, 0,
        QCoreApplication::translate("Data::SyncthingService", "Windows service \"%1\" stopped with exit code %2.")
            .arg(QString::fromWCharArray(serviceName))
            .arg(exitCode));
    s_serviceStatus.dwCurrentState = SERVICE_STOPPED;
    s_serviceStatus.dwWin32ExitCode = static_cast<DWORD>(exitCode);
    SetServiceStatus(s_serviceStatusHandle, &s_serviceStatus);
}

/*!
 * \brief Runs the specified \a serviceRoutine to be stopped via the specified \a stopRoutine.
 */
int runAsWindowsService(std::function<int(void)> &&serviceRoutine, std::function<void(void)> &&stopRoutine)
{
    s_serviceRoutine = std::move(serviceRoutine);
    s_stopRoutine = std::move(stopRoutine);
    SERVICE_TABLE_ENTRYW serviceTable[] = { { const_cast<LPWSTR>(L""), serviceMain }, { nullptr, nullptr } };
    if (!StartServiceCtrlDispatcherW(serviceTable)) {
        return static_cast<int>(GetLastError());
    }
    return EXIT_SUCCESS;
}

/*!
 * \brief Installs a service with the specified name.
 * \remarks
 * - Needs admin privileges.
 * - So far makes the service unconditionally point to the current executable which is
 *   expected to provide the built-in Syncthing library.
 * - QCoreApplication needs to be instantiated before calling this function.
 */
int handleInstallService(const QString &serviceName, const QString &homeDir, const QString &logFile)
{
    const auto scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
    if (!scm) {
        logLastWindowsError(L"Failed to open service manager: ", GetLastError());
        return EXIT_FAILURE;
    }
    const auto appPath = QCoreApplication::applicationFilePath();
    if (appPath.isEmpty()) {
        std::wcerr << L"Unable to determine application path.\n";
        return EXIT_FAILURE;
    }
    auto resolvedHome = homeDir;
    if (resolvedHome.isEmpty()) {
        const auto configFile = SyncthingConfig::locateConfigFile();
        if (!configFile.isEmpty()) {
            resolvedHome = QFileInfo(configFile).path();
        }
    }
    auto resolvedLogFile = logFile;
    if (resolvedLogFile.isEmpty() && !resolvedHome.isEmpty()) {
        resolvedLogFile = resolvedHome + QStringLiteral("/syncthing.log");
    }

    auto binaryPath = QStringLiteral("\"%1\" syncthing --as-service serve --no-browser --no-restart").arg(appPath);
    if (!resolvedHome.isEmpty()) {
        binaryPath += QStringLiteral(" --home \"%1\"").arg(QDir::toNativeSeparators(resolvedHome));
    }
    if (!resolvedLogFile.isEmpty()) {
        binaryPath += QStringLiteral(" --log-file \"%1\"").arg(QDir::toNativeSeparators(resolvedLogFile));
    }

    const auto displayName = QStringLiteral("Syncthing Service");
    const auto service = CreateServiceW(scm, reinterpret_cast<LPCWSTR>(serviceName.utf16()), reinterpret_cast<LPCWSTR>(displayName.utf16()),
        SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, reinterpret_cast<LPCWSTR>(binaryPath.utf16()),
        nullptr, nullptr, nullptr, nullptr, nullptr);
    if (!service) {
        const auto err = GetLastError();
        CloseServiceHandle(scm);
        logLastWindowsError(L"Failed to create service: ", err);
        return EXIT_FAILURE;
    }

    // Grant start/stop/status query permissions to Authenticated Users so standard users can start/stop without UAC elevation
    const auto sdString = L"D:(A;;GA;;;BA)(A;;GA;;;SY)(A;;CCLCSWRPWPDTLORC;;;AU)";
    auto sd = PSECURITY_DESCRIPTOR(nullptr);
    if (ConvertStringSecurityDescriptorToSecurityDescriptorW(sdString, SDDL_REVISION_1, &sd, nullptr)) {
        SetServiceObjectSecurity(service, DACL_SECURITY_INFORMATION, sd);
        LocalFree(sd);
    }

    const auto description = QStringLiteral("Syncthing securely synchronizes files between two or more computers in real time.");
    auto sdDesc = SERVICE_DESCRIPTIONW{};
    sdDesc.lpDescription = const_cast<LPWSTR>(reinterpret_cast<LPCWSTR>(description.utf16()));
    ChangeServiceConfig2W(service, SERVICE_CONFIG_DESCRIPTION, &sdDesc);

    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    std::wcout << L"Service \"" << reinterpret_cast<LPCWSTR>(serviceName.utf16()) << L"\" installed successfully.\n";
    return EXIT_SUCCESS;
}

/*!
 * \brief Uninstalls a service with the specified name.
 * \remarks
 * - Needs admin privileges.
 */
int handleUninstallService(const QString &serviceName)
{
    const auto scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) {
        logLastWindowsError(L"Failed to open service manager: ", GetLastError());
        return EXIT_FAILURE;
    }
    const auto service = OpenServiceW(scm, reinterpret_cast<LPCWSTR>(serviceName.utf16()), DELETE | SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (!service) {
        const auto err = GetLastError();
        CloseServiceHandle(scm);
        logLastWindowsError(L"Failed to open service: ", err);
        return EXIT_FAILURE;
    }
    auto status = SERVICE_STATUS{};
    ControlService(service, SERVICE_CONTROL_STOP, &status);
    if (!DeleteService(service)) {
        const auto err = GetLastError();
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        logLastWindowsError(L"Failed to delete service: ", err);
        return EXIT_FAILURE;
    }
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    std::wcout << L"Service \"" << reinterpret_cast<LPCWSTR>(serviceName.utf16()) << L"\" deleted successfully." << std::endl;
    return EXIT_SUCCESS;
}

/*!
 * \brief Enables or disables auto-start for a service with the specified name.
 * \remarks Needs admin privileges.
 */
int handleEnableService(const QString &serviceName, bool enable)
{
    const auto scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) {
        logLastWindowsError(L"Failed to open service manager: ", GetLastError());
        return EXIT_FAILURE;
    }
    const auto service = OpenServiceW(scm, reinterpret_cast<LPCWSTR>(serviceName.utf16()), SERVICE_CHANGE_CONFIG);
    if (!service) {
        const auto err = GetLastError();
        CloseServiceHandle(scm);
        logLastWindowsError(L"Failed to open service: ", err);
        return EXIT_FAILURE;
    }
    const auto startType = DWORD(enable ? SERVICE_AUTO_START : SERVICE_DEMAND_START);
    if (!ChangeServiceConfigW(
            service, SERVICE_NO_CHANGE, startType, SERVICE_NO_CHANGE, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr)) {
        const auto err = GetLastError();
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        logLastWindowsError(L"Failed to change service config: ", err);
        return EXIT_FAILURE;
    }
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    std::wcout << L"Service \"" << reinterpret_cast<LPCWSTR>(serviceName.utf16()) << L"\" " << (enable ? L"enabled" : L"disabled")
               << L" successfully.\n";
    return EXIT_SUCCESS;
}

/*!
 * \brief Starts a service with the specified name.
 * \remarks Needs admin privileges.
 */
int handleStartService(const QString &serviceName)
{
    const auto scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) {
        logLastWindowsError(L"Failed to open service manager: ", GetLastError());
        return EXIT_FAILURE;
    }
    const auto service = OpenServiceW(scm, reinterpret_cast<LPCWSTR>(serviceName.utf16()), SERVICE_START | SERVICE_QUERY_STATUS);
    if (!service) {
        const auto err = GetLastError();
        CloseServiceHandle(scm);
        logLastWindowsError(L"Failed to open service: ", err);
        return EXIT_FAILURE;
    }
    if (!StartServiceW(service, 0, nullptr)) {
        const auto err = GetLastError();
        if (err != ERROR_SERVICE_ALREADY_RUNNING) {
            CloseServiceHandle(service);
            CloseServiceHandle(scm);
            logLastWindowsError(L"Failed to start service: ", err);
            return EXIT_FAILURE;
        }
    }
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    std::wcout << L"Service \"" << reinterpret_cast<LPCWSTR>(serviceName.utf16()) << L"\" started successfully.\n";
    return EXIT_SUCCESS;
}

/*!
 * \brief Stops a service with the specified name.
 * \remarks Needs admin privileges.
 */
int handleStopService(const QString &serviceName)
{
    const auto scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) {
        logLastWindowsError(L"Failed to open service manager: ", GetLastError());
        return EXIT_FAILURE;
    }
    const auto service = OpenServiceW(scm, reinterpret_cast<LPCWSTR>(serviceName.utf16()), SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (!service) {
        const auto err = GetLastError();
        CloseServiceHandle(scm);
        logLastWindowsError(L"Failed to open service: ", err);
        return EXIT_FAILURE;
    }
    auto status = SERVICE_STATUS{};
    if (!ControlService(service, SERVICE_CONTROL_STOP, &status)) {
        const auto err = GetLastError();
        if (err != ERROR_SERVICE_NOT_ACTIVE) {
            CloseServiceHandle(service);
            CloseServiceHandle(scm);
            logLastWindowsError(L"Failed to stop service: ", err);
            return EXIT_FAILURE;
        }
    }
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    std::wcout << L"Service \"" << reinterpret_cast<LPCWSTR>(serviceName.utf16()) << L"\" stopped successfully.\n";
    return EXIT_SUCCESS;
}

static bool executeElevated(const QString &operation, const QString &unitName, QString &errorMessage)
{
    const auto appPath = QCoreApplication::applicationFilePath();
    const auto params = QStringLiteral("%1 --name \"%2\"").arg(operation, unitName);
    auto sei = SHELLEXECUTEINFOW{};
    sei.cbSize = sizeof(sei);
    sei.lpVerb = L"runas";
    sei.lpFile = reinterpret_cast<LPCWSTR>(appPath.utf16());
    sei.lpParameters = reinterpret_cast<LPCWSTR>(params.utf16());
    sei.nShow = SW_SHOWNORMAL;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    if (!ShellExecuteExW(&sei)) {
        const auto err = GetLastError();
        if (err != ERROR_CANCELLED) {
            errorMessage = windowsErrorString(err);
        }
        return false;
    }
    if (sei.hProcess) {
        WaitForSingleObject(sei.hProcess, INFINITE);
        auto exitCode = DWORD(0);
        GetExitCodeProcess(sei.hProcess, &exitCode);
        CloseHandle(sei.hProcess);
        if (exitCode != 0) {
            errorMessage = QCoreApplication::translate("Data::SyncthingService", "Process exited with code %1").arg(exitCode);
            return false;
        }
    }
    return true;
}
#endif

/// \endcond

#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_WINDOWS_SERVICE)
struct SyncthingService::NotifyState {
    std::mutex mutex;
    SyncthingService *service = nullptr;
};

namespace {
void CALLBACK serviceNotifyCallback(PVOID pParameter)
{
    auto *const notify = static_cast<SERVICE_NOTIFYW *>(pParameter);
    auto *const ctxPtr = static_cast<std::shared_ptr<SyncthingService::NotifyState> *>(notify->pContext);
    if (!ctxPtr) {
        return;
    }
    auto state = std::move(*ctxPtr);
    delete ctxPtr;
    notify->pContext = nullptr;

    auto lock = std::unique_lock<std::mutex>(state->mutex);
    auto *const service = state->service;
    lock.unlock();
    if (service) {
        QMetaObject::invokeMethod(service, "queryServiceStatus", Qt::QueuedConnection);
        QMetaObject::invokeMethod(service, "rearmServiceNotify", Qt::QueuedConnection);
    }
}
} // namespace

void SyncthingService::registerServiceNotify()
{
    unregisterServiceNotify();
    if (m_unitName.isEmpty()) {
        return;
    }
    if (!m_notifyState) {
        m_notifyState = std::make_shared<NotifyState>();
        auto lock = std::unique_lock<std::mutex>(m_notifyState->mutex);
        m_notifyState->service = this;
    }

    m_notifyScmHandle = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!m_notifyScmHandle) {
        return;
    }
    m_notifyServiceHandle
        = OpenServiceW(static_cast<SC_HANDLE>(m_notifyScmHandle), reinterpret_cast<LPCWSTR>(m_unitName.utf16()), SERVICE_QUERY_STATUS);
    if (!m_notifyServiceHandle) {
        CloseServiceHandle(static_cast<SC_HANDLE>(m_notifyScmHandle));
        m_notifyScmHandle = nullptr;
        return;
    }
    rearmServiceNotify();
}

void SyncthingService::rearmServiceNotify()
{
    if (!m_notifyServiceHandle || !m_notifyState) {
        return;
    }
    static_assert(sizeof(SERVICE_NOTIFYW) <= sizeof(m_serviceNotifyBuffer), "SERVICE_NOTIFYW buffer too small");
    auto &notify = *reinterpret_cast<SERVICE_NOTIFYW *>(m_serviceNotifyBuffer);
    notify = SERVICE_NOTIFYW{};
    notify.dwVersion = SERVICE_NOTIFY_STATUS_CHANGE;
    notify.pfnNotifyCallback = serviceNotifyCallback;
    notify.pContext = new std::shared_ptr<NotifyState>(m_notifyState);
    const auto res = NotifyServiceStatusChangeW(static_cast<SC_HANDLE>(m_notifyServiceHandle),
        SERVICE_NOTIFY_RUNNING | SERVICE_NOTIFY_START_PENDING | SERVICE_NOTIFY_STOP_PENDING | SERVICE_NOTIFY_STOPPED | SERVICE_NOTIFY_PAUSED
            | SERVICE_NOTIFY_PAUSE_PENDING | SERVICE_NOTIFY_CONTINUE_PENDING,
        &notify);
    if (res != ERROR_SUCCESS) {
        delete static_cast<std::shared_ptr<NotifyState> *>(notify.pContext);
        notify.pContext = nullptr;
    }
}

void SyncthingService::unregisterServiceNotify()
{
    auto &notify = *reinterpret_cast<SERVICE_NOTIFYW *>(m_serviceNotifyBuffer);
    if (m_notifyServiceHandle) {
        CloseServiceHandle(static_cast<SC_HANDLE>(m_notifyServiceHandle));
        m_notifyServiceHandle = nullptr;
    }
    if (m_notifyScmHandle) {
        CloseServiceHandle(static_cast<SC_HANDLE>(m_notifyScmHandle));
        m_notifyScmHandle = nullptr;
    }
    // Closing the handle cancels any pending notification, so the callback
    // will not run and cannot reclaim the context. Leave it to avoid races.
    notify.pContext = nullptr;
    notify = SERVICE_NOTIFYW{};
}
#endif

SyncthingService::~SyncthingService()
{
#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_WINDOWS_SERVICE)
    if (m_notifyState) {
        auto lock = std::unique_lock<std::mutex>(m_notifyState->mutex);
        m_notifyState->service = nullptr;
    }
#endif
}

/*!
 * \brief Creates a new SyncthingService instance.
 */
SyncthingService::SyncthingService(ServiceScope scope, QObject *parent)
    : QObject(parent)
#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD)
    , m_unit(nullptr)
    , m_service(nullptr)
    , m_properties(nullptr)
    , m_currentSystemdInterface(nullptr)
#elif defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_WINDOWS_SERVICE)
    , m_notifyScmHandle(nullptr)
    , m_notifyServiceHandle(nullptr)
#endif
    , m_scope(scope)
    , m_manuallyStopped(false)
    , m_stoppedDueToRuntimeCond(false)
    , m_unitAvailable(false)
{
#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_WINDOWS_SERVICE)
    for (auto &b : m_serviceNotifyBuffer) {
        b = 0;
    }
#endif
#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD)
    setupFreedesktopLoginInterface();

#ifdef LIB_SYNCTHING_CONNECTOR_SERVICE_MOCKED
    // let the mocked service initially be stopped and simulate start after 5 seconds, then stop after 10 seconds and start after 15 seconds
    QTimer::singleShot(5000, this, [this] {
        m_activeSince = DateTime::gmtNow() - TimeSpan::fromMilliseconds(250);
        handlePropertiesChanged(QStringLiteral("syncthing.mocked-service"),
            QVariantMap{ { QStringLiteral("ActiveState"), QStringLiteral("active") }, { QStringLiteral("SubState"), QStringLiteral("running") },
                { QStringLiteral("Description"), QStringLiteral("This service is fake.") } },
            QStringList());
    });
    QTimer::singleShot(10000, this, [this] {
        m_activeSince = DateTime();
        handlePropertiesChanged(QStringLiteral("syncthing.mocked-service"),
            QVariantMap{ { QStringLiteral("ActiveState"), QStringLiteral("inactive") }, { QStringLiteral("SubState"), QStringLiteral("dead") },
                { QStringLiteral("Description"), QStringLiteral("This service is still fake.") } },
            QStringList());
    });
    QTimer::singleShot(15000, this, [this] {
        m_activeSince = DateTime::gmtNow();
        handlePropertiesChanged(QStringLiteral("syncthing.mocked-service"),
            QVariantMap{ { QStringLiteral("ActiveState"), QStringLiteral("active") }, { QStringLiteral("SubState"), QStringLiteral("running") },
                { QStringLiteral("Description"), QStringLiteral("This service is alive again!") } },
            QStringList());
    });
#endif
#elif defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_WINDOWS_SERVICE)
    m_unitName = QStringLiteral("syncthing");
    registerServiceNotify();
    queryServiceStatus();
#endif

    connect(&m_runtimeCondition, &RuntimeCondition::supposedToRunChanged, this, [this](bool supposedToRun) {
        if (!supposedToRun) {
            stopDueToRuntimeCond();
        } else if (m_stoppedDueToRuntimeCond) {
            start();
        }
    });
}

#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD)
/*!
 * \brief Initializes m_currentSystemdInterface and its connection and service watcher for the current m_scope.
 */
void SyncthingService::setupSystemdInterface()
{
#ifndef LIB_SYNCTHING_CONNECTOR_SERVICE_MOCKED
    clearSystemdInterface();

    // ensure the static systemd interface for the current scope is initialized
    const auto isUserScope = m_scope == ServiceScope::User;
    OrgFreedesktopSystemd1ManagerInterface *&staticSystemdInterface = isUserScope ? s_systemdUserInterface : s_systemdSystemInterface;
    if (!staticSystemdInterface) {
        // register custom data types
        qDBusRegisterMetaType<ManagerDBusUnitFileChange>();
        qDBusRegisterMetaType<ManagerDBusUnitFileChangeList>();

        staticSystemdInterface = new OrgFreedesktopSystemd1ManagerInterface(QStringLiteral("org.freedesktop.systemd1"),
            QStringLiteral("/org/freedesktop/systemd1"), isUserScope ? QDBusConnection::sessionBus() : QDBusConnection::systemBus());

        // enable systemd to emit signals
        staticSystemdInterface->Subscribe();
    }

    // use the static systemd interface for the current scope
    m_currentSystemdInterface = staticSystemdInterface;
    connect(m_currentSystemdInterface, &OrgFreedesktopSystemd1ManagerInterface::UnitNew, this, &SyncthingService::handleUnitAdded);
    connect(m_currentSystemdInterface, &OrgFreedesktopSystemd1ManagerInterface::UnitRemoved, this, &SyncthingService::handleUnitRemoved);
    connect(m_currentSystemdInterface, &OrgFreedesktopSystemd1ManagerInterface::Reloading, this, &SyncthingService::handleReloading);
    m_serviceWatcher = new QDBusServiceWatcher(m_currentSystemdInterface->service(), m_currentSystemdInterface->connection());
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, &SyncthingService::handleServiceRegisteredChanged);
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, &SyncthingService::handleServiceRegisteredChanged);
#endif
}

/*!
 * \brief Initializes s_loginManager.
 */
void SyncthingService::setupFreedesktopLoginInterface()
{
#ifndef LIB_SYNCTHING_CONNECTOR_SERVICE_MOCKED
    // ensure the static login interface is initialized and use it
    if (s_loginManager) {
        return;
    }
    s_loginManager = new OrgFreedesktopLogin1ManagerInterface(
        QStringLiteral("org.freedesktop.login1"), QStringLiteral("/org/freedesktop/login1"), QDBusConnection::systemBus());
    connect(s_loginManager, &OrgFreedesktopLogin1ManagerInterface::PrepareForSleep, &SyncthingService::handlePrepareForSleep);
#endif
}

/*!
 * \brief Registers the specified D-Bus \a call to invoke \a handler when it has been concluded.
 */
template <typename HandlerType> void SyncthingService::makeAsyncCall(const QDBusPendingCall &call, HandlerType &&handler, bool removeHandler)
{
    if (m_currentSystemdInterface && removeHandler) {
        // disconnect from unit add/removed signals because these seem to be spammed when waiting for permissions
        disconnect(m_currentSystemdInterface, &OrgFreedesktopSystemd1ManagerInterface::UnitNew, this, &SyncthingService::handleUnitAdded);
        disconnect(m_currentSystemdInterface, &OrgFreedesktopSystemd1ManagerInterface::UnitRemoved, this, &SyncthingService::handleUnitRemoved);
    }
    auto *const watcher = new QDBusPendingCallWatcher(call, this);
    m_pendingCalls.emplace(watcher);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, handler);
}

/*!
 * \brief Registers a generic error handler for the specifeid D-Bus \a call.
 */
void SyncthingService::registerErrorHandler(const QDBusPendingCall &call, const char *context, bool reload, bool removeHandler)
{
    makeAsyncCall(call, bind(&SyncthingService::handleError, this, context, _1, reload), removeHandler);
}

/*!
 * \brief Determines whether the specified \a watcher is still relevant and ensures it is being deleted later.
 */
bool SyncthingService::concludeAsyncCall(QDBusPendingCallWatcher *watcher, bool reload)
{
    watcher->deleteLater();

    const auto i = m_pendingCalls.find(watcher);
    const auto resultStillRelevant = i != m_pendingCalls.cend();
    if (resultStillRelevant) {
        m_pendingCalls.erase(i);
    }
    if (m_currentSystemdInterface) {
        if (m_pendingCalls.empty()) {
            // ensure we listen to unit add/removed signals again if there are no pending calls anymore
            connect(m_currentSystemdInterface, &OrgFreedesktopSystemd1ManagerInterface::UnitNew, this, &SyncthingService::handleUnitAdded);
            connect(m_currentSystemdInterface, &OrgFreedesktopSystemd1ManagerInterface::UnitRemoved, this, &SyncthingService::handleUnitRemoved);
        }
        if (reload && !watcher->isError()) {
            if (m_scope != ServiceScope::System) {
                // reload unit files if reload flag was set (for enable/disable to make the change immediately apparent)
                reloadAllUnitFiles();
            } else {
                // we don't have the permission; at least try to refresh the unit again
                queryUnitFromSystemdInterface();
            }
        }
    }
    return resultStillRelevant;
}

/*!
 * \brief Unties the current instance from its current systemd interface.
 */
void SyncthingService::clearSystemdInterface()
{
    m_pendingCalls.clear();
    if (m_currentSystemdInterface) {
        disconnect(m_currentSystemdInterface, &OrgFreedesktopSystemd1ManagerInterface::UnitNew, this, &SyncthingService::handleUnitAdded);
        disconnect(m_currentSystemdInterface, &OrgFreedesktopSystemd1ManagerInterface::UnitRemoved, this, &SyncthingService::handleUnitRemoved);
        disconnect(m_currentSystemdInterface, &OrgFreedesktopSystemd1ManagerInterface::Reloading, this, &SyncthingService::handleReloading);
        delete m_serviceWatcher;
        m_currentSystemdInterface = nullptr;
    }
}

/*!
 * \brief Clears everything we know about the systemd unit.
 */
void SyncthingService::clearUnitData()
{
    // clean up data from previous unit
    delete m_service;
    m_service = nullptr;
    delete m_unit;
    m_unit = nullptr;
    delete m_properties;
    m_properties = nullptr;
}

/*!
 * \brief Queries m_unit from m_currentSystemdInterface.
 */
void SyncthingService::queryUnitFromSystemdInterface()
{
    clearUnitData();
    setProperties(false, QString(), QString(), QString(), QString());

#ifndef LIB_SYNCTHING_CONNECTOR_SERVICE_MOCKED
    if (!m_currentSystemdInterface) {
        setupSystemdInterface();
    }
    if (!m_currentSystemdInterface->isValid()) {
        return;
    }
    makeAsyncCall(m_currentSystemdInterface->GetUnit(m_unitName), &SyncthingService::handleUnitGet);
#endif
}
#endif

#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_WINDOWS_SERVICE)
void SyncthingService::queryServiceStatus()
{
    const auto scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) {
        if (m_unitAvailable) {
            m_unitAvailable = false;
            emit unitAvailableChanged(false);
        }
        return;
    }
    const auto service = OpenServiceW(scm, reinterpret_cast<LPCWSTR>(m_unitName.utf16()), SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG);
    if (!service) {
        CloseServiceHandle(scm);
        if (m_unitAvailable) {
            m_unitAvailable = false;
            m_activeState.clear();
            m_subState.clear();
            m_activeSince = DateTime();
            m_description.clear();
            m_unitFileState.clear();
            emit unitAvailableChanged(false);
            emit activeStateChanged(m_activeState);
            emit subStateChanged(m_subState);
            emit stateChanged(m_activeState, m_subState, m_activeSince);
            emit runningChanged(false);
            emit descriptionChanged(m_description);
            emit unitFileStateChanged(m_unitFileState);
            emit enabledChanged(false);
        }
        return;
    }

    if (!m_unitAvailable) {
        m_unitAvailable = true;
        emit unitAvailableChanged(true);
        queryServiceConfig();
    }

    auto ssp = SERVICE_STATUS_PROCESS{};
    auto bytesNeeded = DWORD(0);
    if (QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>(&ssp), sizeof(ssp), &bytesNeeded)) {
        auto newActiveState = QString();
        auto newSubState = QString();
        auto newActiveSince = DateTime();
        const auto wasRunning = isRunning();

        switch (ssp.dwCurrentState) {
        case SERVICE_RUNNING:
            newActiveState = QStringLiteral("active");
            newSubState = QStringLiteral("running");
            if (ssp.dwProcessId > 0) {
                if (const auto process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, ssp.dwProcessId)) {
                    auto creationTime = FILETIME{};
                    auto exitTime = FILETIME{};
                    auto kernelTime = FILETIME{};
                    auto userTime = FILETIME{};
                    if (GetProcessTimes(process, &creationTime, &exitTime, &kernelTime, &userTime)) {
                        auto uli = ULARGE_INTEGER{};
                        uli.LowPart = creationTime.dwLowDateTime;
                        uli.HighPart = creationTime.dwHighDateTime;
                        constexpr auto epochDiff = 116444736000000000ULL;
                        if (uli.QuadPart >= epochDiff) {
                            const auto unixTimeSec = static_cast<time_t>((uli.QuadPart - epochDiff) / 10000000ULL);
                            newActiveSince = DateTime::fromTimeStampGmt(unixTimeSec);
                        }
                    }
                    CloseHandle(process);
                }
            }
            break;
        case SERVICE_STOPPED:
            newActiveState = QStringLiteral("inactive");
            newSubState = QStringLiteral("stopped");
            break;
        case SERVICE_START_PENDING:
            newActiveState = QStringLiteral("activating");
            newSubState = QStringLiteral("starting");
            break;
        case SERVICE_STOP_PENDING:
            newActiveState = QStringLiteral("deactivating");
            newSubState = QStringLiteral("stopping");
            break;
        case SERVICE_PAUSED:
            newActiveState = QStringLiteral("active");
            newSubState = QStringLiteral("paused");
            break;
        default:
            newActiveState = QStringLiteral("unknown");
            newSubState = QStringLiteral("unknown");
            break;
        }

        const auto stateChanged = (m_activeState != newActiveState) || (m_subState != newSubState) || (m_activeSince != newActiveSince);
        m_activeState = newActiveState;
        m_subState = newSubState;
        m_activeSince = newActiveSince;
        if (stateChanged) {
            emit this->stateChanged(m_activeState, m_subState, m_activeSince);
            emit activeStateChanged(m_activeState);
            emit subStateChanged(m_subState);
        }
        if (wasRunning != isRunning()) {
            emit runningChanged(isRunning());
        }
    }

    CloseServiceHandle(service);
    CloseServiceHandle(scm);
}

void SyncthingService::queryServiceConfig()
{
    const auto scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) {
        return;
    }
    const auto service = OpenServiceW(scm, reinterpret_cast<LPCWSTR>(m_unitName.utf16()), SERVICE_QUERY_CONFIG);
    if (!service) {
        CloseServiceHandle(scm);
        return;
    }

    auto bytesNeeded = DWORD(0);
    QueryServiceConfigW(service, nullptr, 0, &bytesNeeded);
    if (bytesNeeded > 0) {
        auto buffer = std::vector<BYTE>(bytesNeeded);
        auto *const config = reinterpret_cast<QUERY_SERVICE_CONFIGW *>(buffer.data());
        if (QueryServiceConfigW(service, config, bytesNeeded, &bytesNeeded)) {
            auto newUnitFileState = QString();
            switch (config->dwStartType) {
            case SERVICE_AUTO_START:
                newUnitFileState = QStringLiteral("enabled");
                break;
            case SERVICE_DEMAND_START:
                newUnitFileState = QStringLiteral("manual");
                break;
            case SERVICE_DISABLED:
                newUnitFileState = QStringLiteral("disabled");
                break;
            default:
                newUnitFileState = QStringLiteral("unknown");
                break;
            }
            const auto wasEnabled = isEnabled();
            if (m_unitFileState != newUnitFileState) {
                m_unitFileState = newUnitFileState;
                emit unitFileStateChanged(m_unitFileState);
            }
            if (wasEnabled != isEnabled()) {
                emit enabledChanged(isEnabled());
            }
        }
    }

    bytesNeeded = 0;
    QueryServiceConfig2W(service, SERVICE_CONFIG_DESCRIPTION, nullptr, 0, &bytesNeeded);
    if (bytesNeeded > 0) {
        auto buffer = std::vector<BYTE>(bytesNeeded);
        if (QueryServiceConfig2W(service, SERVICE_CONFIG_DESCRIPTION, buffer.data(), bytesNeeded, &bytesNeeded)) {
            const auto *const desc = reinterpret_cast<SERVICE_DESCRIPTIONW *>(buffer.data());
            const auto newDescription = (desc && desc->lpDescription) ? QString::fromWCharArray(desc->lpDescription) : QString();
            if (m_description != newDescription) {
                m_description = newDescription;
                emit descriptionChanged(m_description);
            }
        }
    }

    CloseServiceHandle(service);
    CloseServiceHandle(scm);
}
#endif

/*!
 * \brief Sets the \a unitName of the systemd user service to be controlled/monitored, e.g. "syncthing.service".
 */
void SyncthingService::setUnitName(const QString &unitName)
{
    if (m_unitName == unitName) {
        return;
    }
    m_unitName = unitName;

#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD)
    if (!m_unitName.isEmpty()) {
        queryUnitFromSystemdInterface();
    }
#elif defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_WINDOWS_SERVICE)
    registerServiceNotify();
    queryServiceStatus();
#endif
    emit unitNameChanged(unitName);
}

/*!
 * \brief Sets the \a scope and \a unitName (see scope() and unitName()).
 */
void SyncthingService::setScopeAndUnitName(ServiceScope scope, const QString &unitName)
{
    const auto scopeChanged = m_scope != scope;
    const auto unitNameChanged = m_unitName != unitName;
    if (!scopeChanged && !unitNameChanged) {
        return;
    }
#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD)
    if (scopeChanged) {
        m_scope = scope;
        clearSystemdInterface();
    }
    if (unitNameChanged) {
        m_unitName = unitName;
    }
    if (!unitName.isEmpty()) {
        queryUnitFromSystemdInterface();
    }
#elif defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_WINDOWS_SERVICE)
    m_scope = scope;
    m_unitName = unitName;
    queryServiceStatus();
#endif
    if (scopeChanged) {
        emit this->scopeChanged(scope);
    }
    if (unitNameChanged) {
        emit this->unitNameChanged(unitName);
    }
}

/*!
 * \brief Returns a display name for this service.
 * \remarks
 * So far this just returns "user/system … unit …" but if this class was extended to support different types of
 * services it might return different names.
 */
QString SyncthingService::displayName() const
{
#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_WINDOWS_SERVICE)
    return tr("service \"%1\"").arg(unitName());
#else
    if (isUserScope()) {
        return tr("user unit \"%1\"").arg(unitName());
    } else {
        return tr("system unit \"%1\"").arg(unitName());
    }
#endif
}

/*!
 * \brief Returns whether a supported service manager (systemd and specifically its D-Bus interface for user services) is available.
 * \remarks The availability might not be instantly detected and may change at any time. Use the serviceManagerAvailableChanged()
 *          to react to availability changes.
 */
bool SyncthingService::isServiceManagerAvailable() const
{
#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD)
#ifndef LIB_SYNCTHING_CONNECTOR_SERVICE_MOCKED
    return m_currentSystemdInterface && m_currentSystemdInterface->isValid();
#else
    return true;
#endif
#elif defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_WINDOWS_SERVICE)
    if (const auto scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT)) {
        CloseServiceHandle(scm);
        return true;
    }
    return false;
#endif
}

/*!
 * \brief Returns whether the unit specified with \a unitName is available.
 * \remarks
 * - The availability might not be instantly detected and may change at any time. Use the unitAvailableChanged()
 *   to react to availability changes.
 * - Unless this function returns true the other unit-related functions will return false/empty values.
 */
bool SyncthingService::isUnitAvailable() const
{
#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD)
#ifndef LIB_SYNCTHING_CONNECTOR_SERVICE_MOCKED
    return m_unitAvailable || (m_unit && m_unit->isValid());
#else
    return true;
#endif
#elif defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_WINDOWS_SERVICE)
    return m_unitAvailable;
#endif
}

/*!
 * \brief Sets the scope the current instance is tuned to.
 */
void SyncthingService::setScope(ServiceScope scope)
{
    if (m_scope == scope) {
        return;
    }
    m_scope = scope;
#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD)
    clearSystemdInterface();
    queryUnitFromSystemdInterface();
#elif defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_WINDOWS_SERVICE)
    queryServiceStatus();
#endif
    emit scopeChanged(scope);
}

/*!
 * \brief Starts the unit if \a running is true and stops the unit if \a running is false.
 */
void SyncthingService::setRunning(bool running)
{
#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD)
#ifndef LIB_SYNCTHING_CONNECTOR_SERVICE_MOCKED
    m_manuallyStopped = !running;
    m_stoppedDueToRuntimeCond = false;
    if (!m_currentSystemdInterface) {
        setupSystemdInterface();
    }
    if (running) {
        registerErrorHandler(
            m_currentSystemdInterface->StartUnit(m_unitName, QStringLiteral("replace")), QT_TR_NOOP_UTF8("start unit"), m_activeState.isEmpty());
    } else {
        registerErrorHandler(m_currentSystemdInterface->StopUnit(m_unitName, QStringLiteral("replace")), QT_TR_NOOP_UTF8("stop unit"));
    }
#endif
#elif defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_WINDOWS_SERVICE)
    m_manuallyStopped = !running;
    m_stoppedDueToRuntimeCond = false;
    const auto scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) {
        emit errorOccurred(tr("open service manager"), tr("Windows Service Error"), windowsErrorString(GetLastError()));
        return;
    }
    const auto desiredAccess = DWORD(running ? (SERVICE_START | SERVICE_QUERY_STATUS) : (SERVICE_STOP | SERVICE_QUERY_STATUS));
    const auto service = OpenServiceW(scm, reinterpret_cast<LPCWSTR>(m_unitName.utf16()), desiredAccess);
    if (!service) {
        const auto err = GetLastError();
        CloseServiceHandle(scm);
        if (err == ERROR_ACCESS_DENIED) {
            auto errorMessage = QString();
            if (!executeElevated(running ? QStringLiteral("start-service") : QStringLiteral("stop-service"), m_unitName, errorMessage)) {
                if (!errorMessage.isEmpty()) {
                    emit errorOccurred(running ? tr("start service") : tr("stop service"), tr("Operation Failed"), errorMessage);
                }
                return;
            }
            queryServiceStatus();
            return;
        }
        emit errorOccurred(running ? tr("start service") : tr("stop service"), tr("Windows Service Error"), windowsErrorString(err));
        return;
    }
    if (running) {
        if (!StartServiceW(service, 0, nullptr)) {
            const auto err = GetLastError();
            if (err != ERROR_SERVICE_ALREADY_RUNNING) {
                emit errorOccurred(tr("start service"), tr("Windows Service Error"), windowsErrorString(err));
            }
        }
    } else {
        auto status = SERVICE_STATUS{};
        if (!ControlService(service, SERVICE_CONTROL_STOP, &status)) {
            const auto err = GetLastError();
            if (err != ERROR_SERVICE_NOT_ACTIVE) {
                emit errorOccurred(tr("stop service"), tr("Windows Service Error"), windowsErrorString(err));
            }
        }
    }
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    queryServiceStatus();
#endif
}

/*!
 * \brief Enables the unit if \a enabled is true and disables the unit if \a enabled is false.
 */
void SyncthingService::setEnabled(bool enabled)
{
#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD)
#ifndef LIB_SYNCTHING_CONNECTOR_SERVICE_MOCKED
    if (!m_currentSystemdInterface) {
        setupSystemdInterface();
    }
    if (enabled) {
        registerErrorHandler(m_currentSystemdInterface->EnableUnitFiles(QStringList(m_unitName), false, true), QT_TR_NOOP_UTF8("enable unit"), true);
    } else {
        registerErrorHandler(m_currentSystemdInterface->DisableUnitFiles(QStringList(m_unitName), false), QT_TR_NOOP_UTF8("disable unit"), true);
    }
#endif
#elif defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_WINDOWS_SERVICE)
    const auto scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) {
        emit errorOccurred(tr("open service manager"), tr("Windows Service Error"), windowsErrorString(GetLastError()));
        return;
    }
    const auto service = OpenServiceW(scm, reinterpret_cast<LPCWSTR>(m_unitName.utf16()), SERVICE_CHANGE_CONFIG);
    if (!service) {
        const auto err = GetLastError();
        CloseServiceHandle(scm);
        if (err == ERROR_ACCESS_DENIED) {
            auto errorMessage = QString();
            if (!executeElevated(enabled ? QStringLiteral("enable-service") : QStringLiteral("disable-service"), m_unitName, errorMessage)) {
                if (!errorMessage.isEmpty()) {
                    emit errorOccurred(enabled ? tr("enable service") : tr("disable service"), tr("Operation Failed"), errorMessage);
                }
                return;
            }
            queryServiceConfig();
            return;
        }
        emit errorOccurred(enabled ? tr("enable service") : tr("disable service"), tr("Windows Service Error"), windowsErrorString(err));
        return;
    }
    const auto startType = DWORD(enabled ? SERVICE_AUTO_START : SERVICE_DEMAND_START);
    if (!ChangeServiceConfigW(
            service, SERVICE_NO_CHANGE, startType, SERVICE_NO_CHANGE, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr)) {
        const auto err = GetLastError();
        if (err == ERROR_ACCESS_DENIED) {
            CloseServiceHandle(service);
            CloseServiceHandle(scm);
            auto errorMessage = QString();
            if (!executeElevated(enabled ? QStringLiteral("enable-service") : QStringLiteral("disable-service"), m_unitName, errorMessage)) {
                if (!errorMessage.isEmpty()) {
                    emit errorOccurred(enabled ? tr("enable service") : tr("disable service"), tr("Operation Failed"), errorMessage);
                }
                return;
            }
            queryServiceConfig();
            return;
        }
        emit errorOccurred(enabled ? tr("enable service") : tr("disable service"), tr("Windows Service Error"), windowsErrorString(err));
    }
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    queryServiceConfig();
#endif
}

/*!
 * \brief Reload all unit files.
 */
void SyncthingService::reloadAllUnitFiles()
{
#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD)
    registerErrorHandler(m_currentSystemdInterface->Reload(), QT_TR_NOOP_UTF8("reload all unit files"), false, false);
#elif defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_WINDOWS_SERVICE)
    queryServiceStatus();
    queryServiceConfig();
#endif
}

#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD)
namespace {

enum class TerminalArgFormat {
    DashE,
    DashDash,
    Direct,
};

struct TerminalSpec {
    const char *binaryName;
    TerminalArgFormat format;
};

constexpr auto terminalSpecs = std::array<TerminalSpec, 12>{
    TerminalSpec{ "xdg-terminal-exec", TerminalArgFormat::Direct },
    TerminalSpec{ "konsole", TerminalArgFormat::DashE },
    TerminalSpec{ "gnome-terminal", TerminalArgFormat::DashDash },
    TerminalSpec{ "xfce4-terminal", TerminalArgFormat::DashDash },
    TerminalSpec{ "mate-terminal", TerminalArgFormat::DashDash },
    TerminalSpec{ "kitty", TerminalArgFormat::Direct },
    TerminalSpec{ "foot", TerminalArgFormat::Direct },
    TerminalSpec{ "alacritty", TerminalArgFormat::DashE },
    TerminalSpec{ "x-terminal-emulator", TerminalArgFormat::DashE },
    TerminalSpec{ "xterm", TerminalArgFormat::DashE },
    TerminalSpec{ "urxvt", TerminalArgFormat::DashE },
    TerminalSpec{ "terminator", TerminalArgFormat::DashE },
};

} // namespace
#endif

/*!
 * \brief Opens the logs.
 */
void SyncthingService::openLogs() const
{
#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD)
    if (m_unitName.trimmed().isEmpty()) {
        emit const_cast<SyncthingService *>(this)->errorOccurred(tr("open logs"), tr("Invalid configuration"), tr("No unit name configured."));
        return;
    }

    const auto journalctlPath = QStandardPaths::findExecutable(QStringLiteral("journalctl"));
    if (journalctlPath.isEmpty()) {
        emit const_cast<SyncthingService *>(this)->errorOccurred(
            tr("open logs"), tr("Command not found"), tr("The \"journalctl\" executable could not be found."));
        return;
    }

    auto journalctlArgs = QStringList();
    journalctlArgs.reserve(6);
    if (m_scope == ServiceScope::User) {
        journalctlArgs << QStringLiteral("--user");
    }
    journalctlArgs << QStringLiteral("-u") << m_unitName;
    journalctlArgs << QStringLiteral("--follow") << QStringLiteral("--no-tail");
    if (isRunning() && !m_activeSince.isNull()) {
        journalctlArgs << QStringLiteral("--since") << QStringLiteral("@%1").arg(m_activeSince.toTimeStamp());
    }

    auto terminalPath = QString();
    auto format = TerminalArgFormat::DashE;
    auto terminalArgs = QStringList();

    if (const auto terminalEnv = qEnvironmentVariable("TERMINAL"); !terminalEnv.isEmpty()) {
        if (const auto tokens = SyncthingProcess::splitArguments(terminalEnv); !tokens.isEmpty()) {
            const auto candidate = tokens.first();
            terminalPath = QStandardPaths::findExecutable(candidate);
            if (terminalPath.isEmpty()) {
                terminalPath = candidate;
            }
            terminalArgs = tokens.mid(1);
            for (const auto &spec : terminalSpecs) {
                if (terminalPath.endsWith(QLatin1String(spec.binaryName))) {
                    format = spec.format;
                    break;
                }
            }
        }
    }

    if (terminalPath.isEmpty()) {
        for (const auto &spec : terminalSpecs) {
            if (const auto candidatePath = QStandardPaths::findExecutable(QString::fromLatin1(spec.binaryName)); !candidatePath.isEmpty()) {
                terminalPath = candidatePath;
                format = spec.format;
                break;
            }
        }
    }

    if (terminalPath.isEmpty()) {
        emit const_cast<SyncthingService *>(this)->errorOccurred(tr("open logs"), tr("Terminal not found"),
            tr("No supported terminal application was found in PATH. Please set the TERMINAL environment variable."));
        return;
    }

    switch (format) {
    case TerminalArgFormat::DashE:
        terminalArgs << QStringLiteral("-e") << journalctlPath << journalctlArgs;
        break;
    case TerminalArgFormat::DashDash:
        terminalArgs << QStringLiteral("--") << journalctlPath << journalctlArgs;
        break;
    case TerminalArgFormat::Direct:
        terminalArgs << journalctlPath << journalctlArgs;
        break;
    }

    if (!QProcess::startDetached(terminalPath, terminalArgs)) {
        emit const_cast<SyncthingService *>(this)->errorOccurred(
            tr("open logs"), tr("Launch failed"), tr("Unable to launch terminal \"%1\".").arg(terminalPath));
    }
#elif defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_WINDOWS_SERVICE)
    if (!QProcess::startDetached(QStringLiteral("cmd.exe"), { QStringLiteral("/c"), QStringLiteral("start"), QStringLiteral("services.msc") })) {
        emit const_cast<SyncthingService *>(this)->errorOccurred(tr("open logs"), tr("Launch failed"), tr("Unable to launch \"services.msc\"."));
    }
#endif
}

void SyncthingService::installService()
{
#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_WINDOWS_SERVICE)
    const auto appPath = QCoreApplication::applicationFilePath();
    auto params = QStringLiteral("install-service --name \"%1\"").arg(m_unitName);
    const auto configFile = SyncthingConfig::locateConfigFile();
    if (!configFile.isEmpty()) {
        const auto homeDir = QFileInfo(configFile).path();
        params += QStringLiteral(" --home \"%1\"").arg(QDir::toNativeSeparators(homeDir));
        params += QStringLiteral(" --log-file \"%1\\syncthing.log\"").arg(QDir::toNativeSeparators(homeDir));
    }
    auto sei = SHELLEXECUTEINFOW{};
    sei.cbSize = sizeof(sei);
    sei.lpVerb = L"runas";
    sei.lpFile = reinterpret_cast<LPCWSTR>(appPath.utf16());
    sei.lpParameters = reinterpret_cast<LPCWSTR>(params.utf16());
    sei.nShow = SW_SHOWNORMAL;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    if (!ShellExecuteExW(&sei)) {
        const auto err = GetLastError();
        if (err != ERROR_CANCELLED) {
            emit errorOccurred(tr("install service"), tr("UAC Elevation Failed"), windowsErrorString(err));
        }
        return;
    }
    if (sei.hProcess) {
        WaitForSingleObject(sei.hProcess, INFINITE);
        auto exitCode = DWORD(0);
        GetExitCodeProcess(sei.hProcess, &exitCode);
        CloseHandle(sei.hProcess);
        if (exitCode != 0) {
            emit errorOccurred(tr("install service"), tr("Installation Failed"), tr("Process exited with code %1").arg(exitCode));
        }
    }
    queryServiceStatus();
#endif
}

void SyncthingService::uninstallService()
{
#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_WINDOWS_SERVICE)
    auto errorMessage = QString();
    if (!executeElevated(QStringLiteral("uninstall-service"), m_unitName, errorMessage)) {
        if (!errorMessage.isEmpty()) {
            emit errorOccurred(tr("uninstall service"), tr("Operation Failed"), errorMessage);
        }
        return;
    }
    queryServiceStatus();
#endif
}

#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD)
/*!
 * \brief Handles when a new unit is added to react if it matches the name of the unit we're monitoring.
 */
void SyncthingService::handleUnitAdded(const QString &unitName, const QDBusObjectPath &unitPath)
{
    if (unitName == m_unitName) {
        setUnit(unitPath);
    }
}

/*!
 * \brief Handles when a unit is removed to react if it matches the name of the unit we're monitoring.
 */
void SyncthingService::handleUnitRemoved(const QString &unitName, const QDBusObjectPath &unitPath)
{
    Q_UNUSED(unitPath)
    if (unitName == m_unitName) {
        setUnit(QDBusObjectPath());
    }
}

/*!
 * \brief Handles when unit files have been reloaded.
 */
void SyncthingService::handleReloading(bool started)
{
    if (!started) {
        queryUnitFromSystemdInterface();
    }
}

/*!
 * \brief Consumes the results of the s_manager->GetUnit() call (in setUnitName()).
 */
void SyncthingService::handleUnitGet(QDBusPendingCallWatcher *watcher)
{
    if (!concludeAsyncCall(watcher)) {
        return;
    }
    setUnit(QDBusPendingReply<QDBusObjectPath>(*watcher).value());
}

/*!
 * \brief Consumes the results of the s_manager->GetUnitFileState() call (in setUnitName()).
 */
void SyncthingService::handleGetUnitFileState(QDBusPendingCallWatcher *watcher)
{
    if (!concludeAsyncCall(watcher)) {
        return;
    }
    auto fileState = QString();
    if (!watcher->isError()) {
        if (const auto &args = watcher->reply().arguments(); !args.empty()) {
            fileState = args.at(0).toString();
        }
    }
    clearUnitData();
    setProperties(!fileState.isEmpty(), QString(), QString(), fileState, QString());
}

/*!
 * \brief Handles when properties of the monitored unit change.
 */
void SyncthingService::handlePropertiesChanged(
    const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
#ifndef LIB_SYNCTHING_CONNECTOR_SERVICE_MOCKED
    if (interface != m_unit->interface()) {
        return;
    }
#else
    if (interface != QStringLiteral("syncthing.mocked-service")) {
        return;
    }
#endif
    handlePropertyChanged(m_activeSince, QStringLiteral("ActiveEnterTimestamp"), changedProperties, invalidatedProperties);

    const auto wasRunningBefore = isRunning();
    const auto activeStateChanged = handlePropertyChanged(
        m_activeState, &SyncthingService::activeStateChanged, QStringLiteral("ActiveState"), changedProperties, invalidatedProperties);
    const auto subStateChanged
        = handlePropertyChanged(m_subState, &SyncthingService::subStateChanged, QStringLiteral("SubState"), changedProperties, invalidatedProperties);
    if (activeStateChanged || subStateChanged) {
        emit stateChanged(m_activeState, m_subState, m_activeSince);
    }
    const bool currentlyRunning = isRunning();
    if (wasRunningBefore != currentlyRunning) {
        if (currentlyRunning) {
            m_manuallyStopped = false;
            m_stoppedDueToRuntimeCond = false;
        }
        emit runningChanged(currentlyRunning);
    }

    const bool wasEnabledBefore = isEnabled();
    handlePropertyChanged(
        m_unitFileState, &SyncthingService::unitFileStateChanged, QStringLiteral("UnitFileState"), changedProperties, invalidatedProperties);
    if (wasEnabledBefore != isEnabled()) {
        emit enabledChanged(isEnabled());
    }

    handlePropertyChanged(
        m_description, &SyncthingService::descriptionChanged, QStringLiteral("Description"), changedProperties, invalidatedProperties);
}

/*!
 * \brief Handles D-Bus errors.
 */
void SyncthingService::handleError(const char *context, QDBusPendingCallWatcher *watcher, bool reload)
{
    if (!concludeAsyncCall(watcher, reload)) {
        return;
    }
    const QDBusError error = watcher->error();
    if (error.isValid()) {
        emit errorOccurred(tr(context), error.name(), error.message());
    }
}

/*!
 * \brief Handles when the service availability changes.
 */
void SyncthingService::handleServiceRegisteredChanged(const QString &service)
{
    if (m_currentSystemdInterface && service == m_currentSystemdInterface->service()) {
        emit serviceManagerAvailableChanged(m_currentSystemdInterface->isValid());
    }
}

/*!
 * \brief Logs the moment before when standby is enabled and the time of the last standby-wakeup.
 */
void SyncthingService::handlePrepareForSleep(bool rightBefore)
{
    if (!(s_fallingAsleep = rightBefore)) {
        s_lastWakeUp = DateTime::gmtNow();
    }
}

/*!
 * \brief Internal helper to handle property changes for QString-properties.
 */
bool SyncthingService::handlePropertyChanged(QString &variable, void (SyncthingService::*signal)(const QString &), const QString &propertyName,
    const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    const auto valueVariant = changedProperties.find(propertyName);
    if (valueVariant != changedProperties.end()) {
        auto valueString = valueVariant->toString();
        if (valueString != variable) {
            emit(this->*signal)(variable = std::move(valueString));
            return true;
        }
    } else if (invalidatedProperties.contains(propertyName) && !variable.isEmpty()) {
        variable.clear();
        emit(this->*signal)(variable);
        return true;
    }
    return false;
}

/*!
 * \brief Internal helper to handle property changes for DateTime-properties.
 */
bool SyncthingService::handlePropertyChanged(
    DateTime &variable, const QString &propertyName, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    const auto valueVariant = changedProperties.find(propertyName);
    if (valueVariant != changedProperties.end()) {
        bool ok;
        const qulonglong valueInt = valueVariant->toULongLong(&ok);
        if (ok) {
            variable = dateTimeFromSystemdTimeStamp(valueInt);
            return true;
        }
    } else if (invalidatedProperties.contains(propertyName) && !variable.isNull()) {
        variable = DateTime();
        return true;
    }
    return false;
}

/*!
 * \brief Sets the current unit data.
 */
void SyncthingService::setUnit(const QDBusObjectPath &objectPath)
{
    clearUnitData();

    if (!m_currentSystemdInterface) {
        setProperties(false, QString(), QString(), QString(), QString());
        return;
    }

    const auto path = objectPath.path();
    if (path.isEmpty()) {
        // fallback to querying unit file state
        makeAsyncCall(m_currentSystemdInterface->GetUnitFileState(m_unitName), &SyncthingService::handleGetUnitFileState);
        return;
    }

    // init unit
    m_unit = new OrgFreedesktopSystemd1UnitInterface(m_currentSystemdInterface->service(), path, m_currentSystemdInterface->connection());
    m_unit->setTimeout(2000);
    if (m_unit->isValid()) {
        m_activeSince = dateTimeFromSystemdTimeStamp(m_unit->activeEnterTimestamp());
        setProperties(true, m_unit->activeState(), m_unit->subState(), m_unit->unitFileState(), m_unit->description());
        // handle runtime conditions: if the runtime conditions are not met, then …
        if (!m_runtimeCondition.isSupposedToRun()) {
            if (isRunning()) {
                // stop an already running service immediately
                stopDueToRuntimeCond();
            } else {
                // consider an already stopped service as stopped due to runtime conditions; so we will start it as soon as the runtime
                // conditions are met again
                m_stoppedDueToRuntimeCond = true;
            }
        }
    } else {
        // fallback to querying unit file state
        makeAsyncCall(m_currentSystemdInterface->GetUnitFileState(m_unitName), &SyncthingService::handleGetUnitFileState);
    }

    // init properties
    m_properties = new OrgFreedesktopDBusPropertiesInterface(m_currentSystemdInterface->service(), path, m_currentSystemdInterface->connection());
    connect(m_properties, &OrgFreedesktopDBusPropertiesInterface::PropertiesChanged, this, &SyncthingService::handlePropertiesChanged);
}

/*!
 * \brief Updates the properties for the current unit.
 */
void SyncthingService::setProperties(
    bool unitAvailable, const QString &activeState, const QString &subState, const QString &unitFileState, const QString &description)
{
    if (m_unitAvailable != unitAvailable) {
        emit unitAvailableChanged(m_unitAvailable = unitAvailable);
    }

    const bool running = isRunning();
    bool anyStateChanged = false;
    if (m_activeState != activeState) {
        emit activeStateChanged(m_activeState = activeState);
        anyStateChanged = true;
    }
    if (m_subState != subState) {
        emit subStateChanged(m_subState = subState);
        anyStateChanged = true;
    }
    if (anyStateChanged) {
        emit stateChanged(m_activeState, m_subState, m_activeSince);
    }
    if (running != isRunning()) {
        emit runningChanged(isRunning());
    }

    const bool enabled = isEnabled();
    if (m_unitFileState != unitFileState) {
        emit unitFileStateChanged(m_unitFileState = unitFileState);
    }
    if (enabled != isEnabled()) {
        emit enabledChanged(isEnabled());
    }
    if (m_description != description) {
        emit descriptionChanged(m_description = description);
    }
}
#endif

/*!
 * \brief Internal helper to stop the service when the runtime conditions are no longer met.
 */
void SyncthingService::stopDueToRuntimeCond()
{
    if (isRunning()) {
        setRunning(false);
    }
    m_stoppedDueToRuntimeCond = true;
}

} // namespace Data

#endif
