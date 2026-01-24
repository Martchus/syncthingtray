#ifndef SYNCTHING_DATA_MODELS_H
#define SYNCTHING_DATA_MODELS_H

#include "./syncthinglauncher.h"
#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
#include "./diffhighlighter.h"
#include "../quick/quickicon.h"
#endif

#include <syncthingmodel/syncthingdevicemodel.h>
#include <syncthingmodel/syncthingdirectorymodel.h>
#include <syncthingmodel/syncthingrecentchangesmodel.h>
#include <syncthingmodel/syncthingsortfiltermodel.h>
#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
#include <syncthingmodel/syncthingfilemodel.h>
#endif

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingnotifier.h>

#include <QObject>
#include <QtVersion>

#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
#include <QJSValue>
#include <QtQmlIntegration/qqmlintegration.h>

QT_FORWARD_DECLARE_CLASS(QQmlEngine)
QT_FORWARD_DECLARE_CLASS(QJSEngine)
#endif

namespace QtGui {

class SYNCTHINGWIDGETS_EXPORT SyncthingData : public QObject {
    Q_OBJECT
    Q_PROPERTY(Data::SyncthingConnection *connection READ connection CONSTANT)
    Q_PROPERTY(Data::SyncthingNotifier *notifier READ notifier CONSTANT)
    Q_PROPERTY(QString syncthingVersion READ syncthingVersion CONSTANT)
    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)
#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
    QML_ELEMENT
    QML_SINGLETON
#endif

public:
    explicit SyncthingData(QObject *parent = nullptr);
    ~SyncthingData() override;
#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
    static SyncthingData *create(QQmlEngine *, QJSEngine *engine);
#endif

    Data::SyncthingConnection *connection()
    {
        return &m_connection;
    }
    Data::SyncthingNotifier *notifier()
    {
        return &m_notifier;
    }
    QString syncthingVersion() const
    {
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
        return Data::SyncthingLauncher::libSyncthingVersionInfo().remove(0, 11);
#else
        return tr("not available");
#endif
    }
    QString qtVersion() const
    {
        return QString::fromUtf8(qVersion());
    }

#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
#endif

private:
    Data::SyncthingConnection m_connection;
    Data::SyncthingNotifier m_notifier;
};

class SYNCTHINGWIDGETS_EXPORT SyncthingModels : public QObject {
    Q_OBJECT
    Q_PROPERTY(Data::SyncthingDirectoryModel *dirModel READ dirModel CONSTANT)
    Q_PROPERTY(Data::SyncthingSortFilterModel *sortFilterDirModel READ sortFilterDirModel CONSTANT)
    Q_PROPERTY(Data::SyncthingDeviceModel *devModel READ devModel CONSTANT)
    Q_PROPERTY(Data::SyncthingSortFilterModel *sortFilterDevModel READ sortFilterDevModel CONSTANT)
    Q_PROPERTY(Data::SyncthingRecentChangesModel *changesModel READ changesModel CONSTANT)
#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
    QML_ELEMENT
    QML_SINGLETON
#endif

public:
    explicit SyncthingModels(Data::SyncthingConnection &data, QObject *parent = nullptr);
    explicit SyncthingModels(SyncthingData &data, QObject *parent = nullptr);
    ~SyncthingModels() override;
#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
    static SyncthingModels *create(QQmlEngine *, QJSEngine *engine);
#endif

    Data::SyncthingDirectoryModel *dirModel()
    {
        return &m_dirModel;
    }
    Data::SyncthingSortFilterModel *sortFilterDirModel()
    {
        return &m_sortFilterDirModel;
    }
    Data::SyncthingDeviceModel *devModel()
    {
        return &m_devModel;
    }
    Data::SyncthingSortFilterModel *sortFilterDevModel()
    {
        return &m_sortFilterDevModel;
    }
    Data::SyncthingRecentChangesModel *changesModel()
    {
        return &m_recentChangesModel;
    }

#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
    Q_INVOKABLE bool openSyncthingConfigFile();
    Q_INVOKABLE bool openSyncthingLogFile();
    Q_INVOKABLE bool openUrlExternally(const QUrl &url, bool viaQt = false);
    Q_INVOKABLE bool openPath(const QString &path);
    Q_INVOKABLE bool openPath(const QString &dirId, const QString &relativePath);
    Q_INVOKABLE bool scanPath(const QString &path);
    Q_INVOKABLE bool copyText(const QString &text);
    Q_INVOKABLE bool copyPath(const QString &dirId, const QString &relativePath);
    Q_INVOKABLE QString getClipboardText() const;
    Q_INVOKABLE bool loadIgnorePatterns(const QString &dirId, QObject *textArea);
    Q_INVOKABLE bool saveIgnorePatterns(const QString &dirId, QObject *textArea);
    Q_INVOKABLE bool openIgnorePatterns(const QString &dirId);
    Q_INVOKABLE bool loadErrors(QObject *listView);
    Q_INVOKABLE bool showLog(QObject *textArea);
    Q_INVOKABLE void clearLog();
    Q_INVOKABLE bool showQrCode(Icon *icon);
    Q_INVOKABLE bool loadDirErrors(const QString &dirId, QObject *view);
    Q_INVOKABLE bool loadStatistics(const QJSValue &callback);
    Q_INVOKABLE bool showError(const QString &errorMessage);
    Q_INVOKABLE QString resolveUrl(const QUrl &url);
    Q_INVOKABLE bool shouldIgnorePermissions(const QString &path);
    Q_INVOKABLE Data::SyncthingFileModel *createFileModel(const QString &dirId, QObject *parent);
    Q_INVOKABLE QtGui::DiffHighlighter *createDiffHighlighter(QTextDocument *parent);
    qint64 databaseSize(const QString &path, const QString &extension) const;
    QVariant formattedDatabaseSize(const QString &path, const QString &extension) const;
    QVariantMap statistics() const;
    void statistics(QVariantMap &res) const;
    Q_INVOKABLE bool postSyncthingConfig(const QJsonObject &rawConfig, const QJSValue &callback = QJSValue());
    Q_INVOKABLE bool invokeDirAction(const QString &dirId, const QString &action);
    Q_INVOKABLE bool requestFromSyncthing(
        const QString &verb, const QString &path, const QVariantMap &parameters, const QJSValue &callback = QJSValue());
    Q_INVOKABLE QString formatDataSize(quint64 size) const;
    Q_INVOKABLE QString formatTraffic(quint64 total, double rate) const;
    Q_INVOKABLE bool hasDevice(const QString &id);
    Q_INVOKABLE bool hasDir(const QString &id);
    Q_INVOKABLE QString deviceDisplayName(const QString &id) const;
    Q_INVOKABLE QString dirDisplayName(const QString &id) const;
    Q_INVOKABLE QVariantList computeDirsNeedingItems(const QModelIndex &devProxyModelIndex) const;
    Q_INVOKABLE QVariant isPopulated(const QString &path) const;
#endif

private:
    Data::SyncthingDirectoryModel m_dirModel;
    Data::SyncthingSortFilterModel m_sortFilterDirModel;
    Data::SyncthingDeviceModel m_devModel;
    Data::SyncthingSortFilterModel m_sortFilterDevModel;
    Data::SyncthingRecentChangesModel m_recentChangesModel;
};

} // namespace QtGui

#endif // SYNCTHING_DATA_MODELS_H
