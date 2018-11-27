#ifndef SYNCTHINGFILEITEMACTION_H
#define SYNCTHINGFILEITEMACTION_H

#include "../connector/syncthingconnection.h"

#include <KAbstractFileItemActionPlugin>
#include <KFileItemListProperties>

#include <QAction>
#include <QWidget>
#include <QWidgetAction>

QT_FORWARD_DECLARE_CLASS(QLabel)

class KFileItemListProperties;

class SyncthingMenuAction : public QAction {
    Q_OBJECT

public:
    explicit SyncthingMenuAction(const KFileItemListProperties &properties = KFileItemListProperties(),
        const QList<QAction *> &actions = QList<QAction *>(), QWidget *parentWidget = nullptr);

public Q_SLOTS:
    void updateStatus(Data::SyncthingStatus status);

private:
    KFileItemListProperties m_properties;
};

class SyncthingInfoAction;

class SyncthingInfoWidget : public QWidget {
    Q_OBJECT

public:
    explicit SyncthingInfoWidget(const SyncthingInfoAction *action, QWidget *parent = nullptr);

private Q_SLOTS:
    void updateFromSender();
    void updateFromAction(const SyncthingInfoAction *action);

private:
    QLabel *const m_textLabel;
    QLabel *const m_iconLabel;
};

class SyncthingInfoAction : public QWidgetAction {
    Q_OBJECT

public:
    explicit SyncthingInfoAction(QObject *parent = nullptr);

protected:
    QWidget *createWidget(QWidget *parent) override;
};

class SyncthingDirActions : public QObject {
    Q_OBJECT
    friend QList<QAction *> &operator<<(QList<QAction *> &, SyncthingDirActions &);

public:
    explicit SyncthingDirActions(const Data::SyncthingDir &dir, QObject *parent = nullptr);

public Q_SLOTS:
    void updateStatus(const std::vector<Data::SyncthingDir> &dirs);
    bool updateStatus(const Data::SyncthingDir &dir);

private:
    QString m_dirId;
    QAction m_infoAction;
    SyncthingInfoAction m_statusAction;
    SyncthingInfoAction m_globalStatusAction;
    SyncthingInfoAction m_localStatusAction;
    SyncthingInfoAction m_lastScanAction;
    SyncthingInfoAction m_rescanIntervalAction;
    SyncthingInfoAction m_errorsAction;
};

QList<QAction *> &operator<<(QList<QAction *> &actions, SyncthingDirActions &dirActions);

class SyncthingFileItemActionStaticData : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString configPath READ configPath)
    Q_PROPERTY(QString currentError READ currentError WRITE setCurrentError NOTIFY currentErrorChanged RESET clearCurrentError)
    Q_PROPERTY(bool hasError READ hasError NOTIFY hasErrorChanged)
    Q_PROPERTY(bool initialized READ isInitialized)

public:
    explicit SyncthingFileItemActionStaticData();
    ~SyncthingFileItemActionStaticData();
    Data::SyncthingConnection &connection();
    const Data::SyncthingConnection &connection() const;
    const QString &configPath() const;
    const QString &currentError() const;
    bool hasError() const;
    bool isInitialized() const;

public Q_SLOTS:
    void initialize();
    bool applySyncthingConfiguration(const QString &syncthingConfigFilePath);
    void logConnectionStatus();
    void logConnectionError(const QString &errorMessage, Data::SyncthingErrorCategory errorCategory);
    void rescanDir(const QString &dirId, const QString &relpath = QString());
    static void showAboutDialog();
    void selectSyncthingConfig();
    void setCurrentError(const QString &currentError);
    void clearCurrentError();

Q_SIGNALS:
    void currentErrorChanged(const QString &error);
    void hasErrorChanged(bool hasError);

private:
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

class SyncthingFileItemAction : public KAbstractFileItemActionPlugin {
    Q_OBJECT

public:
    SyncthingFileItemAction(QObject *parent, const QVariantList &args);
    QList<QAction *> actions(const KFileItemListProperties &fileItemInfo, QWidget *parentWidget) override;
    static QList<QAction *> createActions(const KFileItemListProperties &fileItemInfo, QWidget *parentWidget);
    static SyncthingFileItemActionStaticData &staticData();

private:
    static SyncthingFileItemActionStaticData s_data;
};

inline SyncthingFileItemActionStaticData &SyncthingFileItemAction::staticData()
{
    return s_data;
}

#endif // SYNCTHINGFILEITEMACTION_H
