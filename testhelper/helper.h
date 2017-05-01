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
 * \brief Waits for the in ms specified \a duration keeping the event loop running.
 */
inline void wait(int duration)
{
    QEventLoop loop;
    QTimer::singleShot(duration, &loop, &QEventLoop::quit);
    loop.exec();
}

inline void noop()
{
}

/*!
 * \brief Waits until the \a signal is emitted by \a sender when performing \a action and connects \a signal with \a handler if specified.
 * \arg sender Specifies the sender which is assumed to emit \a signal.
 * \arg signal Specifies the signal.
 * \arg action Specifies the action to be invoked when waiting.
 * \arg timeout Specifies the max. time to wait.
 * \arg handler Specifies a handler which will be also connected to \a signal.
 * \arg ok Specifies whether the correct signal has been emitted. Should be set in \a handler to indicate that the emitted signal is actually the one
 *         the test is waiting for (and not just one which has been emitted as side-effect).
 * \throws Fails if \a signal is not emitted in at least \a timeout milliseconds or when at least one of the required
 *         connections can not be established.
 * \remarks The handler is disconnected before the function returns.
 */
template <typename Signal, typename Action, typename Handler = std::function<void(void)> >
void waitForSignal(typename QtPrivate::FunctionPointer<Signal>::Object *sender, Signal signal, Action action, int timeout = 2500,
    Handler handler = nullptr, bool *ok = nullptr)
{
    // determine name of the signal for error messages
    const QByteArray signalName(QMetaMethod::fromSignal(signal).name());

    // use loop for waiting
    QEventLoop loop;

    // if specified, connect handler to signal
    QMetaObject::Connection handlerConnection;
    if (handler) {
        handlerConnection = QObject::connect(sender, signal, sender, handler, Qt::DirectConnection);
        if (!handlerConnection) {
            CPPUNIT_FAIL(argsToString("Unable to connect signal ", signalName.data(), " to handler"));
        }
    }

    // connect the signal to the quit slot of the loop
    if (!QObject::connect(sender, signal, &loop, &QEventLoop::quit, Qt::DirectConnection)) {
        CPPUNIT_FAIL(argsToString("Unable to connect signal ", signalName.data(), " for waiting"));
    }

    // handle case when signal is directly emitted
    bool signalDirectlyEmitted = false;
    QMetaObject::Connection signalDirectlyEmittedConnection
        = QObject::connect(sender, signal, sender, [&signalDirectlyEmitted] { signalDirectlyEmitted = true; }, Qt::DirectConnection);
    if (!signalDirectlyEmittedConnection) {
        CPPUNIT_FAIL(argsToString("Unable to connect signal ", signalName.data(), " to check for direct emmitation"));
    }

    // perform specified action
    action();

    // no reason to enter event loop when signal has been emitted directly
    if ((!ok || *ok) && signalDirectlyEmitted) {
        QObject::disconnect(signalDirectlyEmittedConnection);
        QObject::disconnect(handlerConnection);
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
    if (!ok) {
        loop.exec();
    } else {
        while (!*ok && (!timeout || timer.isActive())) {
            loop.exec();
        }
    }

    // check whether a timeout occured
    if ((!ok || !*ok) && timeout && !timer.isActive()) {
        CPPUNIT_FAIL(argsToString("Signal ", signalName.data(), " has not emmitted within at least ", timeout, " ms."));
    }

    QObject::disconnect(signalDirectlyEmittedConnection);
    QObject::disconnect(handlerConnection);
}

#endif // SYNCTHINGTESTHELPER_H
