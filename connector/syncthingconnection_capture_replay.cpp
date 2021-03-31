#include "./syncthingconnection.h"

#include <c++utilities/conversion/conversionexception.h>
#include <c++utilities/conversion/stringconversion.h>

#include <QFile>

namespace Data {

/// \cond
static QString readEnvVar(const char *name)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    return qEnvironmentVariable(name);
#else
    return QString::fromLocal8Bit(qgetenv(name));
#endif
}
/// \endcond

/*!
 * \brief Initializes capturing/replay if corresponding environment variables are set.
 */
void SyncthingConnection::initCaptureReplay()
{
    const auto captureFilePath = readEnvVar("SYNCTHING_TRAY_CAPTURE_FILE");
    if (!captureFilePath.isEmpty()) {
        m_captureFile = new QFile(captureFilePath, this);
        m_captureFile->open(QFile::WriteOnly); // TODO: error handling
    }
    const auto replyFilePath = readEnvVar("SYNCTHING_TRAY_REPLAY_FILE");
    if (!replyFilePath.isEmpty()) {
        m_replayFile = new QFile(replyFilePath, this);
        m_replayFile->open(QFile::ReadOnly); // TODO: error handling
    }
}


/*!
 * \brief Records the responsive if a capture file is initialized.
 */
void SyncthingConnection::captureResponse(std::string_view context, const QByteArray &response)
{
    if (!m_captureFile) {
        return;
    }
    const char marker[] = "--- ";
    const auto space = ' ', newLine = '\n';
    const auto timeStamp = CppUtilities::DateTime::exactGmtNow().toIsoString();
    const auto size = CppUtilities::numberToString(response.size());
    m_captureFile->write(marker, 4);
    m_captureFile->write(timeStamp.data(), timeStamp.size());
    m_captureFile->write(&space, 1);
    m_captureFile->write(context.data(), context.size());
    m_captureFile->write(&space, 1);
    m_captureFile->write(response);
    m_captureFile->write(&newLine, 1);
    m_captureFile->write(response);
    m_captureFile->write(&newLine, 1);
     // TODO: error handling
}

/*!
 * \brief Replays the captured responses if a reply file is initialized.
 */
void SyncthingConnection::replyCapturedResponses()
{
    if (!m_replayFile) {
        return;
    }
    enum State {
        Marker0, Marker1, Marker2, Marker3, TimeStamp, Size, Context,
    } state = Marker0;
    auto c = char();
    auto timeStampChars = std::string();
    auto timeStamp = CppUtilities::DateTime();
    auto sizeFactor = 1ul;
    auto size = 0ul;
    auto context = std::string();
    while (m_replayFile->getChar(&c)) {  // TODO: error handling
        switch (state) {
        case Marker0:
        case Marker1:
        case Marker2:
            switch (c) {
            case '-':
                state = static_cast<State>(state + 1);
                break;
            default:
                state = Marker0;
            }
            break;
        case Marker3:
            switch (c) {
            case ' ':
                state = TimeStamp;
                timeStampChars.clear();
                break;
            default:
                state = Marker0;
            }
            break;
        case TimeStamp:
            switch (c) {
            case ' ':
                try {
                    timeStamp = CppUtilities::DateTime::fromIsoStringGmt(timeStampChars.data());
                    state = Size;
                    sizeFactor = 0ul, size = 0ul;
                } catch (const CppUtilities::ConversionException &c) {
                    state = Marker0;  // TODO: error handling
                }
                break;
            default:
                timeStampChars += c;
            }
            break;
        case Size:
            if (c >= '0' && c <= '9') {
                size += static_cast<decltype(size)>(c) * sizeFactor;
                sizeFactor *= 10ul;
            } else if (c == ' ') {
                state = Context;
                context.clear();
            } else {
                state = Marker0; // TODO: error handling
            }
            break;
        case Context:
            switch (c) {
            case '\n':
                state = Marker0;
                m_replayFile->read(size);
                // TODO: error handling
                // TODO: add to some kind of queue for replays (by context)
                // TODO: return if queue has enough items
                break;
            default:
                context += c;
            }
            break;
        }
    }
}

} // namespace Data
