#ifndef SYNCTHINGWIDGETS_MODELS_H
#define SYNCTHINGWIDGETS_MODELS_H

#include "./syncthingdata.h"

#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
#include "./diffhighlighter.h"
#endif

#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
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

#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
#include <QJSValue>
#include <QtQmlIntegration/qqmlintegration.h>

QT_FORWARD_DECLARE_CLASS(QJSEngine)
#endif

QT_FORWARD_DECLARE_CLASS(QQmlEngine)

namespace QtGui {

class SYNCTHINGWIDGETS_EXPORT SyncthingModels : public QObject {
    Q_OBJECT
    Q_PROPERTY(Data::SyncthingDirectoryModel *dirModel READ dirModel CONSTANT)
    Q_PROPERTY(Data::SyncthingSortFilterModel *sortFilterDirModel READ sortFilterDirModel CONSTANT)
    Q_PROPERTY(Data::SyncthingDeviceModel *devModel READ devModel CONSTANT)
    Q_PROPERTY(Data::SyncthingSortFilterModel *sortFilterDevModel READ sortFilterDevModel CONSTANT)
    Q_PROPERTY(Data::SyncthingRecentChangesModel *changesModel READ changesModel CONSTANT)
    Q_PROPERTY(bool scanSupported READ isScanSupported CONSTANT)
#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
    Q_PROPERTY(bool savingConfig READ isSavingConfig NOTIFY savingConfigChanged)
    QML_ELEMENT
    QML_SINGLETON
#endif

public:
    explicit SyncthingModels(Data::SyncthingConnection &data, QQmlEngine *engine = nullptr, QObject *parent = nullptr);
    explicit SyncthingModels(SyncthingData &data, QQmlEngine *engine = nullptr, QObject *parent = nullptr);
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
    Data::SyncthingConnection::QueryResult &pendingConfigChange()
    {
        return m_pendingConfigChange;
    }
    bool isSavingConfig() const
    {
        return m_pendingConfigChange.reply != nullptr;
    }
    /*!
     * \brief Whether scanPath() is available.
     */
    static constexpr bool isScanSupported()
    {
#ifdef Q_OS_ANDROID
        return true;
#else
        return false;
#endif
    }
#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
    void setEngine(QQmlEngine *engine)
    {
        m_engine = engine;
    }
#endif

    // functions used by different UIs
    Q_INVOKABLE QString formatDataSize(quint64 size) const;
    Q_INVOKABLE void setBrightColors(bool brightColors);

    // functions only used by mobile UI
#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
    Q_INVOKABLE QString externalFilesDir() const;
    Q_INVOKABLE QStringList externalStoragePaths() const;
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
    Q_INVOKABLE bool showQrCode(Icon *icon);
    Q_INVOKABLE QString resolveUrl(const QUrl &url);
    Q_INVOKABLE bool shouldIgnorePermissions(const QString &path);
    Q_INVOKABLE Data::SyncthingFileModel *createFileModel(const QString &dirId, QObject *parent);
    Q_INVOKABLE QtGui::DiffHighlighter *createDiffHighlighter(QTextDocument *parent);
    Q_INVOKABLE bool postSyncthingConfig(const QJsonObject &rawConfig, const QJSValue &callback = QJSValue());
    Q_INVOKABLE bool invokeDirAction(const QString &dirId, const QString &action);
    Q_INVOKABLE bool requestFromSyncthing(
        const QString &verb, const QString &path, const QVariantMap &parameters, const QJSValue &callback = QJSValue());
    Q_INVOKABLE QString formatTraffic(quint64 total, double rate) const;
    Q_INVOKABLE bool hasDevice(const QString &id);
    Q_INVOKABLE bool hasDir(const QString &id);
    Q_INVOKABLE QString deviceDisplayName(const QString &id) const;
    Q_INVOKABLE QString dirDisplayName(const QString &id) const;
    Q_INVOKABLE QVariantList computeDirsNeedingItems(const QModelIndex &devProxyModelIndex) const;
    Q_INVOKABLE QVariant isPopulated(const QString &path) const;
#endif

Q_SIGNALS:
#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
    void info(const QString &infoMessage, const QString &details = QString());
    void error(const QString &errorMessage, const QString &details = QString());
    void hapticFeedbackRequested();
    void savingConfigChanged(bool isSavingConfig);
#endif

private:
    Data::SyncthingConnection &m_connection;
    Data::SyncthingDirectoryModel m_dirModel;
    Data::SyncthingSortFilterModel m_sortFilterDirModel;
    Data::SyncthingDeviceModel m_devModel;
    Data::SyncthingSortFilterModel m_sortFilterDevModel;
    Data::SyncthingRecentChangesModel m_recentChangesModel;
    Data::SyncthingConnection::QueryResult m_pendingConfigChange;
#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
    QQmlEngine *m_engine;
#endif
};

inline void SyncthingModels::setBrightColors(bool brightColors)
{
    m_dirModel.setBrightColors(brightColors);
    m_devModel.setBrightColors(brightColors);
    m_recentChangesModel.setBrightColors(brightColors);
}

} // namespace QtGui

#endif // SYNCTHINGWIDGETS_MODELS_H
