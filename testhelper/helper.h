#ifndef SYNCTHINGTESTHELPER_H
#define SYNCTHINGTESTHELPER_H

#include "./global.h"

#include <c++utilities/conversion/stringbuilder.h>

#ifndef SYNCTHINGTESTHELPER_FOR_CLI
#include <cppunit/extensions/HelperMacros.h>
#endif

#include <QEventLoop>
#include <QMetaMethod>
#include <QSet>
#include <QString>
#include <QTimer>

#include <functional>
#include <ostream>

#ifndef SYNCTHINGTESTHELPER_FOR_CLI
#define SYNCTHINGTESTHELPER_TIMEOUT(timeout) static_cast<int>(timeout * ::CppUtilities::timeoutFactor)
#else
#define SYNCTHINGTESTHELPER_TIMEOUT(timeout) timeout
#endif

using namespace CppUtilities;

/*!
 * \brief Prints a QString; required to use QString with CPPUNIT_ASSERT_EQUAL_MESSAGE.
 */
inline std::ostream &operator<<(std::ostream &o, const QString &qstring)
{
    return o << qstring.toLocal8Bit().data();
}

/*!
 * \brief Prints a QStringList; required to use QStringList with CPPUNIT_ASSERT_EQUAL_MESSAGE.
 */
inline std::ostream &operator<<(std::ostream &o, const QStringList &qstringlist)
{
    return o << qstringlist.join(QStringLiteral(", ")).toLocal8Bit().data();
}

/*!
 * \brief Prints a QSet<QString>; required to use QSet<QString> with CPPUNIT_ASSERT_EQUAL_MESSAGE.
 */
inline std::ostream &operator<<(std::ostream &o, const QSet<QString> &qstringset)
{
    return o << qstringset.values().join(QStringLiteral(", ")).toLocal8Bit().data();
}

namespace CppUtilities {

extern SYNCTHINGTESTHELPER_EXPORT double timeoutFactor;

/*!
 * \brief Waits for the \a duration specified in ms while keeping the event loop running.
 */
inline void wait(int duration)
{
    QEventLoop loop;
    QTimer::singleShot(SYNCTHINGTESTHELPER_TIMEOUT(duration), &loop, &QEventLoop::quit);
    loop.exec();
}

/*!
 * \brief Does nothing - meant to be used in waitForSignals() if no action needs to be triggered.
 */
inline void noop()
{
}

/*!
 * \brief The TemporaryConnection class disconnects a QMetaObject::Connection when being destroyed.
 */
class TemporaryConnection {
public:
    TemporaryConnection(QMetaObject::Connection connection)
        : m_connection(connection)
    {
    }

    TemporaryConnection(const TemporaryConnection &other) = delete;

    TemporaryConnection(TemporaryConnection &&other)
        : m_connection(std::move(other.m_connection))
    {
    }

    ~TemporaryConnection()
    {
        QObject::disconnect(m_connection);
    }

private:
    QMetaObject::Connection m_connection;
};

/*!
 * \brief Returns whether the \a object is actually callable.
 *
 * This is supposed to be the case if \a object evaluates to true in boolean context (eg. std::function) or
 * if there's no conversion to bool (eg. lambda).
 */
template <typename T, Traits::EnableIf<Traits::HasOperatorBool<T>> * = nullptr> inline bool isActuallyCallable(const T &object)
{
    return object ? true : false;
}

/*!
 * \brief Returns whether the \a object is actually callable.
 *
 * This is supposed to be the case if \a object evaluates to true in boolean context (eg. std::function) or
 * if there's no conversion to bool (eg. lambda).
 */
template <typename T, Traits::DisableIf<Traits::HasOperatorBool<T>> * = nullptr> inline bool isActuallyCallable(const T &)
{
    return true;
}

/*!
 * \brief The SignalInfo class represents a connection of a signal with a handler.
 *
 * SignalInfo objects are meant to be passed to waitForSignals() so the function can keep track
 * of emitted signals.
 */
template <typename Signal, typename Handler> class SignalInfo {
public:
    /*!
     * \brief Constructs a dummy SignalInfo which will never be considered emitted.
     */
    SignalInfo()
        : m_sender(nullptr)
        , m_signal(nullptr)
        , m_correctSignalEmitted(nullptr)
        , m_signalEmitted(false)
    {
    }

    /*!
     * \brief Constructs a SignalInfo with handler and automatically connects the handler to the signal.
     * \param sender Specifies the object which will emit \a signal.
     * \param signal Specifies the signal.
     * \param handler Specifies a handler to be connected to \a signal.
     * \param correctSignalEmitted Specifies whether the correct signal has been emitted. Should be set in \a handler to indicate that the emitted signal is actually the one
     *                             the test is waiting for (and not just one which has been emitted as side-effect).
     */
    SignalInfo(const typename QtPrivate::FunctionPointer<Signal>::Object *sender, Signal signal, const Handler &handler,
        bool *correctSignalEmitted = nullptr)
        : m_sender(sender)
        , m_signal(signal)
        , m_correctSignalEmitted(correctSignalEmitted)
        , m_signalEmitted(false)
    {
        // register handler if specified
        if (isActuallyCallable(handler)) {
            m_handlerConnection = QObject::connect(sender, signal, sender, handler, Qt::DirectConnection);
#ifndef SYNCTHINGTESTHELPER_FOR_CLI
            if (!m_handlerConnection) {
                CPPUNIT_FAIL(argsToString("Unable to connect signal ", signalName().data(), " to handler"));
            }
#endif
        }
        // register own handler to detect whether signal has been emitted
        m_emittedConnection = QObject::connect(
            sender, signal, sender, [this] { m_signalEmitted = true; }, Qt::DirectConnection);
#ifndef SYNCTHINGTESTHELPER_FOR_CLI
        if (!m_emittedConnection) {
            CPPUNIT_FAIL(argsToString("Unable to connect signal ", signalName().data(), " to check for signal emmitation"));
        }
#endif
    }

    SignalInfo(const SignalInfo &other) = delete;

    SignalInfo(SignalInfo &&other)
        : m_sender(other.m_sender)
        , m_signal(other.m_signal)
        , m_handlerConnection(other.m_handlerConnection)
        , m_emittedConnection(other.m_emittedConnection)
        , m_loopConnection(other.m_loopConnection)
        , m_correctSignalEmitted(other.m_correctSignalEmitted)
        , m_signalEmitted(other.m_signalEmitted)
    {
        other.m_handlerConnection = other.m_emittedConnection = other.m_loopConnection = QMetaObject::Connection();
    }

    /*!
     * \brief Disconnects any established connections.
     */
    ~SignalInfo()
    {
        QObject::disconnect(m_handlerConnection);
        QObject::disconnect(m_emittedConnection);
        QObject::disconnect(m_loopConnection);
    }

    /*!
     * \brief Returns whether the signal has been emitted.
     */
    operator bool() const
    {
        return (m_correctSignalEmitted && *m_correctSignalEmitted) || (!m_correctSignalEmitted && m_signalEmitted);
    }

    /*!
     * \brief Returns the name of the signal as string.
     */
    QByteArray signalName() const
    {
        return QMetaMethod::fromSignal(m_signal).name();
    }

    /*!
     * \brief Connects the signal to the specified \a loop so the loop is being interrupted when the signal
     *        has been emitted.
     */
    void connectToLoop(QEventLoop *loop) const
    {
        if (!m_sender) {
            return;
        }
        QObject::disconnect(m_loopConnection);
        m_loopConnection = QObject::connect(m_sender, m_signal, loop, &QEventLoop::quit, Qt::DirectConnection);
#ifndef SYNCTHINGTESTHELPER_FOR_CLI
        if (!m_loopConnection) {
            CPPUNIT_FAIL(argsToString("Unable to connect signal ", signalName().data(), " for waiting"));
        }
#endif
    }

private:
    const typename QtPrivate::FunctionPointer<Signal>::Object *m_sender;
    Signal m_signal;
    QMetaObject::Connection m_handlerConnection;
    QMetaObject::Connection m_emittedConnection;
    mutable QMetaObject::Connection m_loopConnection;
    bool *m_correctSignalEmitted = nullptr;
    bool m_signalEmitted;
};

/*!
 * \brief Constructs a new SignalInfo.
 */
template <typename Signal, typename Handler>
inline auto signalInfo(typename QtPrivate::FunctionPointer<Signal>::Object *sender, Signal signal, const Handler &handler = Handler(),
    bool *correctSignalEmitted = nullptr)
{
    return SignalInfo<Signal, Handler>(sender, signal, handler, correctSignalEmitted);
}

/*!
 * \brief Constructs a new SignalInfo.
 */
template <typename Signal> inline auto signalInfo(typename QtPrivate::FunctionPointer<Signal>::Object *sender, Signal signal)
{
    return SignalInfo<Signal, std::function<void(void)>>(sender, signal, std::function<void(void)>(), nullptr);
}

/*!
 * \brief Constructs a new SignalInfo.
 */
inline auto dummySignalInfo()
{
    return SignalInfo<decltype(&QObject::destroyed), std::function<void(void)>>();
}

/*!
 * \brief Connects the specified signal info the \a loop via SignalInfo::connectToLoop().
 */
template <typename SignalInfo> inline void connectSignalInfoToLoop(QEventLoop *loop, const SignalInfo &signalInfo)
{
    signalInfo.connectToLoop(loop);
}

/*!
 * \brief Connects the specified signal info the \a loop via SignalInfo::connectToLoop().
 */
template <typename SignalInfo, typename... Signalinfo>
inline void connectSignalInfoToLoop(QEventLoop *loop, const SignalInfo &firstSignalInfo, const Signalinfo &...remainingSignalinfo)
{
    connectSignalInfoToLoop(loop, firstSignalInfo);
    connectSignalInfoToLoop(loop, remainingSignalinfo...);
}

/*!
 * \brief Checks whether all specified signals have been emitted.
 */
template <typename SignalInfo> inline bool checkWhetherAllSignalsEmitted(const SignalInfo &signalInfo)
{
    return signalInfo;
}

/*!
 * \brief Checks whether all specified signals have been emitted.
 */
template <typename SignalInfo, typename... Signalinfo>
inline bool checkWhetherAllSignalsEmitted(const SignalInfo &firstSignalInfo, const Signalinfo &...remainingSignalinfo)
{
    return firstSignalInfo && checkWhetherAllSignalsEmitted(remainingSignalinfo...);
}

/*!
 * \brief Returns the names of all specified signal info which haven't been emitted yet as comma-separated string.
 */
template <typename SignalInfo> inline QByteArray failedSignalNames(const SignalInfo &signalInfo)
{
    return !signalInfo ? signalInfo.signalName() : QByteArray();
}

/*!
 * \brief Returns the names of all specified signal info which haven't been emitted yet as comma-separated string.
 */
template <typename SignalInfo, typename... Signalinfo>
inline QByteArray failedSignalNames(const SignalInfo &firstSignalInfo, const Signalinfo &...remainingSignalinfo)
{
    const QByteArray firstSignalName = failedSignalNames(firstSignalInfo);
    if (!firstSignalName.isEmpty()) {
        return firstSignalName + ", " + failedSignalNames(remainingSignalinfo...);
    } else {
        return failedSignalNames(remainingSignalinfo...);
    }
}

/*!
 * \brief Waits until the specified signals have been emitted when performing async operations triggered by \a action.
 * \arg action Specifies a method to trigger the action to run when waiting.
 * \arg timeout Specifies the max. time to wait. Set to zero to wait forever.
 * \arg signalinfo Specifies the signals to wait for.
 * \throws Fails if not all signals have been emitted in at least \a timeout milliseconds or when at least one of the
 *         required connections can not be established.
 * \returns Returns true if all \a signalinfo have been omitted before the \a timeout exceeded.
 */
template <typename Action, typename... Signalinfo> bool waitForSignals(Action action, int timeout, const Signalinfo &...signalinfo)
{
    return waitForSignalsOrFail(action, timeout, dummySignalInfo(), signalinfo...);
}

/*!
 * \brief Waits until the specified signals have been emitted when performing async operations triggered by \a action. Aborts when \a failure is emitted.
 * \arg action Specifies a method to trigger the action to run when waiting.
 * \arg timeout Specifies the max. time to wait in ms. Set to zero to wait forever.
 * \arg failure Specifies the signal indicating an error occurred.
 * \arg signalinfo Specifies the signals to wait for.
 * \throws Fails if not all signals have been emitted in at least \a timeout milliseconds or when at least one of the
 *         required connections can not be established.
 * \returns Returns true if all \a signalinfo have been omitted before \a failure as been emitted or the \a timeout exceeded.
 */
template <typename Action, typename SignalInfo, typename... Signalinfo>
bool waitForSignalsOrFail(Action action, int timeout, const SignalInfo &failure, const Signalinfo &...signalinfo)
{
    // use loop for waiting
    QEventLoop loop;

    // connect all signals to loop so loop is interrupted when one of the signals is emitted
    connectSignalInfoToLoop(&loop, failure);
    connectSignalInfoToLoop(&loop, signalinfo...);

    // perform specified action
    action();

    // no reason to enter event loop when all signals have been emitted directly
    if (checkWhetherAllSignalsEmitted(failure)) {
        return false;
    }
    if (checkWhetherAllSignalsEmitted(signalinfo...)) {
        return true;
    }

    // also connect and start a timer if a timeout has been specified
    QTimer timer;
    if (timeout) {
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit, Qt::DirectConnection);
        timer.setSingleShot(true);
        timer.setInterval(SYNCTHINGTESTHELPER_TIMEOUT(timeout));
        timer.start();
    }

    // exec event loop as long as the right signal has not been emitted yet and there is still time
    bool allSignalsEmitted = false, failureEmitted = false;
    do {
        loop.exec();
    } while (!(failureEmitted = checkWhetherAllSignalsEmitted(failure)) && !(allSignalsEmitted = checkWhetherAllSignalsEmitted(signalinfo...))
        && (!timeout || timer.isActive()));

    // check whether a timeout occurred
    const bool timeoutFailed(!allSignalsEmitted && timeout && !timer.isActive());
#ifndef SYNCTHINGTESTHELPER_FOR_CLI
    if (failureEmitted) {
        CPPUNIT_FAIL(
            argsToString("Signal(s) ", failedSignalNames(signalinfo...).data(), " has/have not emitted before ", failure.signalName().data(), '.'));
    } else if (timeoutFailed) {
        CPPUNIT_FAIL(argsToString("Signal(s) ", failedSignalNames(signalinfo...).data(), " has/have not emitted within at least ", timer.interval(),
            " ms (set environment variable SYNCTHING_TEST_TIMEOUT_FACTOR to increase the timeout).",
            timeoutFactor != 1.0 ? argsToString(" (original timeout: ", timeout, " ms)") : std::string()));
    }
#endif
    return !failureEmitted && !timeoutFailed;
}

} // namespace CppUtilities

#endif // SYNCTHINGTESTHELPER_H
