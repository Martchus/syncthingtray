#ifndef SYNCTHINGTESTHELPER_H
#define SYNCTHINGTESTHELPER_H

#include <c++utilities/conversion/stringbuilder.h>

#include <cppunit/extensions/HelperMacros.h>

#include <QEventLoop>
#include <QMetaMethod>
#include <QSet>
#include <QString>
#include <QTimer>

#include <functional>
#include <ostream>

using namespace ConversionUtilities;

/*!
 * \brief Prints a QString; required to use QString with CPPUNIT_ASSERT_EQUAL_MESSAGE.
 */
inline std::ostream &operator<<(std::ostream &o, const QString &qstring)
{
    o << qstring.toLocal8Bit().data();
    return o;
}

/*!
 * \brief Prints a QString; required to use QStringList with CPPUNIT_ASSERT_EQUAL_MESSAGE.
 */
inline std::ostream &operator<<(std::ostream &o, const QStringList &qstringlist)
{
    o << qstringlist.join(QStringLiteral(", ")).toLocal8Bit().data();
    return o;
}

/*!
 * \brief Prints a QString; required to use QSet<QString> with CPPUNIT_ASSERT_EQUAL_MESSAGE.
 */
inline std::ostream &operator<<(std::ostream &o, const QSet<QString> &qstringset)
{
    o << qstringset.toList().join(QStringLiteral(", ")).toLocal8Bit().data();
    return o;
}

/*!
 * \brief Waits for the\a duration specified in ms while keeping the event loop running.
 */
inline void wait(int duration)
{
    QEventLoop loop;
    QTimer::singleShot(duration, &loop, &QEventLoop::quit);
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

    ~TemporaryConnection()
    {
        QObject::disconnect(m_connection);
    }

private:
    QMetaObject::Connection m_connection;
};

/*!
 * \brief The SignalInfo class represents a connection of a signal with a handler.
 *
 * SignalInfo objects are meant to be passed to waitForSignals() so the function can keep track
 * of emitted signals.
 */
template <typename Signal, typename Handler> class SignalInfo {
public:
    /*!
     * \brief Constructs a SignalInfo with handler and automatically connects the handler to the signal.
     * \param sender Specifies the object which will emit \a signal.
     * \param signal Specifies the signal.
     * \param handler Specifies a handler to be connected to \a signal.
     * \param correctSignalEmitted Specifies whether the correct signal has been emitted. Should be set in \a handler to indicate that the emitted signal is actually the one
     *                             the test is waiting for (and not just one which has been emitted as side-effect).
     */
    SignalInfo(
        typename QtPrivate::FunctionPointer<Signal>::Object *sender, Signal signal, const Handler &handler, bool *correctSignalEmitted = nullptr)
        : sender(sender)
        , signal(signal)
        , correctSignalEmitted(correctSignalEmitted)
        , signalEmitted(false)
    {
        // register handler if specified
        if (handler) {
            handlerConnection = QObject::connect(sender, signal, sender, handler, Qt::DirectConnection);
            if (!handlerConnection) {
                CPPUNIT_FAIL(argsToString("Unable to connect signal ", signalName().data(), " to handler"));
            }
        }
        // register own handler to detect whether signal has been emitted
        emittedConnection = QObject::connect(sender, signal, sender, [this] { signalEmitted = true; }, Qt::DirectConnection);
        if (!emittedConnection) {
            CPPUNIT_FAIL(argsToString("Unable to connect signal ", signalName().data(), " to check for signal emmitation"));
        }
    }

    SignalInfo(const SignalInfo &other) = delete;

    SignalInfo(SignalInfo &&other)
        : sender(other.sender)
        , signal(other.signal)
        , handlerConnection(other.handlerConnection)
        , emittedConnection(other.emittedConnection)
        , loopConnection(other.loopConnection)
        , correctSignalEmitted(other.correctSignalEmitted)
        , signalEmitted(other.signalEmitted)
    {
        other.handlerConnection = other.emittedConnection = other.loopConnection = QMetaObject::Connection();
    }

    /*!
     * \brief Disconnects any established connections.
     */
    ~SignalInfo()
    {
        QObject::disconnect(handlerConnection);
        QObject::disconnect(emittedConnection);
        QObject::disconnect(loopConnection);
    }

    /*!
     * \brief Returns whether the signal has been emitted.
     */
    operator bool() const
    {
        return (correctSignalEmitted && *correctSignalEmitted) || (!correctSignalEmitted && signalEmitted);
    }

    /*!
     * \brief Returns the name of the signal as string.
     */
    QByteArray signalName() const
    {
        return QMetaMethod::fromSignal(signal).name();
    }

    /*!
     * \brief Connects the signal to the specified \a loop so the loop is being interrupted when the signal
     *        has been emitted.
     */
    void connectToLoop(QEventLoop *loop) const
    {
        QObject::disconnect(loopConnection);
        loopConnection = QObject::connect(sender, signal, loop, &QEventLoop::quit, Qt::DirectConnection);
        if (!loopConnection) {
            CPPUNIT_FAIL(argsToString("Unable to connect signal ", signalName().data(), " for waiting"));
        }
    }

private:
    typename QtPrivate::FunctionPointer<Signal>::Object *sender;
    Signal signal;
    QMetaObject::Connection handlerConnection;
    QMetaObject::Connection emittedConnection;
    mutable QMetaObject::Connection loopConnection;
    bool *correctSignalEmitted = nullptr;
    bool signalEmitted;
};

/*!
 * \brief Constructs a new SignalInfo.
 */
template <typename Signal, typename Handler>
inline SignalInfo<Signal, Handler> signalInfo(typename QtPrivate::FunctionPointer<Signal>::Object *sender, Signal signal,
    const Handler &handler = Handler(), bool *correctSignalEmitted = nullptr)
{
    return SignalInfo<Signal, Handler>(sender, signal, handler, correctSignalEmitted);
}

/*!
 * \brief Connects the specified signal infos the \a loop via SignalInfo::connectToLoop().
 */
template <typename SignalInfo> inline void connectSignalInfosToLoop(QEventLoop *loop, const SignalInfo &signalInfo)
{
    signalInfo.connectToLoop(loop);
}

/*!
 * \brief Connects the specified signal infos the \a loop via SignalInfo::connectToLoop().
 */
template <typename SignalInfo, typename... SignalInfos>
inline void connectSignalInfosToLoop(QEventLoop *loop, const SignalInfo &firstSignalInfo, const SignalInfos &... remainingSignalInfos)
{
    connectSignalInfosToLoop(loop, firstSignalInfo);
    connectSignalInfosToLoop(loop, remainingSignalInfos...);
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
template <typename SignalInfo, typename... SignalInfos>
inline bool checkWhetherAllSignalsEmitted(const SignalInfo &firstSignalInfo, const SignalInfos &... remainingSignalInfos)
{
    return firstSignalInfo && checkWhetherAllSignalsEmitted(remainingSignalInfos...);
}

/*!
 * \brief Returns the names of all specified signal infos which haven't been emitted yet as comma-separated string.
 */
template <typename SignalInfo> inline QByteArray failedSignalNames(const SignalInfo &signalInfo)
{
    return !signalInfo ? signalInfo.signalName() : QByteArray();
}

/*!
 * \brief Returns the names of all specified signal infos which haven't been emitted yet as comma-separated string.
 */
template <typename SignalInfo, typename... SignalInfos>
inline QByteArray failedSignalNames(const SignalInfo &firstSignalInfo, const SignalInfos &... remainingSignalInfos)
{
    const QByteArray firstSignalName = failedSignalNames(firstSignalInfo);
    if (!firstSignalName.isEmpty()) {
        return firstSignalName + ", " + failedSignalNames(remainingSignalInfos...);
    } else {
        return failedSignalNames(remainingSignalInfos...);
    }
}

/*!
 * \brief Waits until the specified signals have been emitted when performing async operations triggered by \a action.
 * \arg action Specifies a method to trigger the action to run when waiting.
 * \arg timeout Specifies the max. time to wait. Set to zero to wait forever.
 * \arg signalInfos Specifies the signals to wait for.
 * \throws Fails if not all signals have been emitted in at least \a timeout milliseconds or when at least one of the
 *         required connections can not be established.
 */
template <typename Action, typename... SignalInfos> void waitForSignals(Action action, int timeout, const SignalInfos &... signalInfos)
{
    // use loop for waiting
    QEventLoop loop;

    // connect all signals to loop so loop is interrupted when one of the signals is emitted
    connectSignalInfosToLoop(&loop, signalInfos...);

    // perform specified action
    action();

    // no reason to enter event loop when all signals have been emitted directly
    if (checkWhetherAllSignalsEmitted(signalInfos...)) {
        return;
    }

    // also connect and start a timer if a timeout has been specified
    QTimer timer;
    if (timeout) {
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit, Qt::DirectConnection);
        timer.setSingleShot(true);
        timer.setInterval(timeout);
        timer.start();
    }

    // exec event loop as long as the right signal has not been emitted yet and there is still time
    bool allSignalsEmitted = false;
    do {
        loop.exec();
    } while (!(allSignalsEmitted = checkWhetherAllSignalsEmitted(signalInfos...)) && (!timeout || timer.isActive()));

    // check whether a timeout occured
    if (!allSignalsEmitted && timeout && !timer.isActive()) {
        CPPUNIT_FAIL(
            argsToString("Signal(s) ", failedSignalNames(signalInfos...).data(), " has/have not emmitted within at least ", timeout, " ms."));
    }
}

#endif // SYNCTHINGTESTHELPER_H
