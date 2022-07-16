#ifndef SYNCTHINGFILEITEMACTIONSTATICDATA_H
#define SYNCTHINGFILEITEMACTIONSTATICDATA_H

#include <syncthingconnector/syncthingconnection.h>

/*!
 * \brief The SyncthingFileItemActionStaticData class holds objects required during the whole application's live time.
 *
 * For instance the connection to Syncthing is kept alive until Dolphin is closed to prevent re-establishing it on each and
 * every time the context menu is shown.
 */
class SyncthingFileItemActionStaticData : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString configPath READ configPath)
    Q_PROPERTY(QString currentError READ currentError WRITE setCurrentError NOTIFY currentErrorChanged RESET clearCurrentError)
    Q_PROPERTY(bool hasError READ hasError NOTIFY hasErrorChanged)
    Q_PROPERTY(bool initialized READ isInitialized)

public:
    explicit SyncthingFileItemActionStaticData();
    Data::SyncthingConnection &connection();
    const Data::SyncthingConnection &connection() const;
    const QString &configPath() const;
    const QString &currentError() const;
    bool hasError() const;
    bool isInitialized() const;

public Q_SLOTS:
    void initialize();
    bool applySyncthingConfiguration(const QString &syncthingConfigFilePath, const QString &syncthingApiKey, bool skipSavingConfig);
    void applyBrightCustomColorsSetting(bool useBrightCustomColors);
    void logConnectionStatus();
    void logConnectionError(const QString &errorMessage, Data::SyncthingErrorCategory errorCategory);
    void rescanDir(const QString &dirId, const QString &relpath = QString());
    static void showAboutDialog();
    void selectSyncthingConfig();
    void handlePaletteChanged(const QPalette &palette);
    void setCurrentError(const QString &currentError);
    void clearCurrentError();

Q_SIGNALS:
    void currentErrorChanged(const QString &error);
    void hasErrorChanged(bool hasError);

private:
    void appendNoteToError(QString &errorMessage, const QString &newSyncthingConfigFilePath) const;

    Data::SyncthingConnection m_connection;
    QString m_configFilePath;
    QString m_currentError;
    bool m_initialized;
};

inline Data::SyncthingConnection &SyncthingFileItemActionStaticData::connection()
{
    return m_connection;
}

inline const Data::SyncthingConnection &SyncthingFileItemActionStaticData::connection() const
{
    return m_connection;
}

inline const QString &SyncthingFileItemActionStaticData::configPath() const
{
    return m_configFilePath;
}

inline const QString &SyncthingFileItemActionStaticData::currentError() const
{
    return m_currentError;
}

inline bool SyncthingFileItemActionStaticData::hasError() const
{
    return !currentError().isEmpty();
}

inline bool SyncthingFileItemActionStaticData::isInitialized() const
{
    return m_initialized;
}

inline void SyncthingFileItemActionStaticData::clearCurrentError()
{
    m_currentError.clear();
}

#endif // SYNCTHINGFILEITEMACTIONSTATICDATA_H
