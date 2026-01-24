#include "./syncthingmodels.h"

namespace QtGui {

SyncthingData::SyncthingData(QObject *parent)
    : QObject(parent)
    , m_notifier(m_connection)
{
    m_connection.setPollingFlags(SyncthingConnection::PollingFlags::MainEvents | SyncthingConnection::PollingFlags::Errors);
}

SyncthingData::~SyncthingData()
{
}

#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
template <typename SyncthingClass> static SyncthingClass *dataObjectFromProperty(QQmlEngine *qmlEngine, QJSEngine *engine, const char *propertyName)
{
    Q_UNUSED(qmlEngine)
    auto *const dataObject = engine->property(propertyName).value<SyncthingClass *>();
    QJSEngine::setObjectOwnership(dataObject, QJSEngine::CppOwnership);
    return dataObject;
}

SyncthingData *SyncthingData::create(QQmlEngine *qmlEngine, QJSEngine *engine)
{
    return dataObjectFromProperty<SyncthingData>(qmlEngine, engine, "connector");
}
#endif

SyncthingModels::SyncthingModels(Data::SyncthingConnection &connection, QObject *parent = nullptr)
    : QObject(parent)
    , m_dirModel(connection)
    , m_sortFilterDirModel(&m_dirModel)
    , m_devModel(connection)
    , m_sortFilterDevModel(&m_devModel)
    , m_recentChangesModel(connection)
{

}

SyncthingModels::SyncthingModels(SyncthingData &data, QObject *parent)
    : SyncthingMOdels(data.connection(), parent)
{
}

SyncthingModels::~SyncthingModels()
{
}


qint64 App::databaseSize(const QString &path, const QString &extension) const
{
    const auto dir = QDir(m_syncthingDataDir % QChar('/') % path);
    const auto files = dir.entryInfoList({ extension }, QDir::Files);
    return std::accumulate(files.begin(), files.end(), qint64(), [](auto size, const auto &file) { return size + file.size(); });
}

QVariant App::formattedDatabaseSize(const QString &path, const QString &extension) const
{
    const auto size = databaseSize(path, extension);
    return size > 0 ? QVariant(QString::fromStdString(CppUtilities::dataSizeToString(static_cast<std::uint64_t>(size)))) : QVariant();
}

QVariantMap App::statistics() const
{
    auto stats = QVariantMap();
    statistics(stats);
    return stats;
}

void App::statistics(QVariantMap &res) const
{
#ifdef Q_OS_ANDROID
    res[QStringLiteral("extFilesDir")] = externalFilesDir();
    res[QStringLiteral("extStoragePaths")] = externalStoragePaths();
#endif
    if (!m_connectToLaunched) {
        return;
    }
    res[QStringLiteral("stConfigDir")] = m_syncthingConfigDir;
    res[QStringLiteral("stDataDir")] = m_syncthingDataDir;
    res[QStringLiteral("stLevelDbSize")] = formattedDatabaseSize(QStringLiteral("index-v0.14.0.db"), QStringLiteral("*.ldb"));
    res[QStringLiteral("stLevelDbMigratedSize")] = formattedDatabaseSize(QStringLiteral("index-v0.14.0.db-migrated"), QStringLiteral("*.ldb"));
    res[QStringLiteral("stSQLiteDbSize")] = formattedDatabaseSize(QStringLiteral("index-v2"), QStringLiteral("*.db*"));
}

bool App::openPath(const QString &path)
{
    if (QtUtilities::openLocalFileOrDir(path)) {
        return true;
    }
    emit error(tr("Unable to open \"%1\"").arg(path));
    return false;
}

bool App::openPath(const QString &dirId, const QString &relativePath)
{
    const auto fullPath = m_connection.fullPath(dirId, relativePath);
    if (fullPath.isEmpty()) {
        emit error(tr("Unable to open \"%1\"").arg(relativePath));
    } else if (openPath(fullPath)) {
        return true;
    }
    return false;
}

/*!
 * \brief Triggers a scan of \a path via the underlying OS.
 * \remarks So far only supported under Android where it is used to discover media files.
 */
bool App::scanPath(const QString &path)
{
#ifdef Q_OS_ANDROID
    if (QJniObject(QNativeInterface::QAndroidApplication::context())
            .callMethod<jboolean>("scanPath", "(Ljava/lang/String;)Z", QJniObject::fromString(path))) {
        return true;
    }
#else
    Q_UNUSED(path)
#endif
    emit error(tr("Scanning is not supported."));
    return false;
}

bool App::copyText(const QString &text)
{
    if (auto *const clipboard = QGuiApplication::clipboard()) {
        clipboard->setText(text);
#ifdef Q_OS_ANDROID
        // do not emit info as Android will show a message about it automatically
        performHapticFeedback();
#else
        emit info(tr("Copied value"));
#endif
        return true;
    }
    emit info(tr("Unable to copy value"));
    return false;
}

bool App::copyPath(const QString &dirId, const QString &relativePath)
{
    const auto fullPath = m_connection.fullPath(dirId, relativePath);
    if (fullPath.isEmpty()) {
        emit error(tr("Unable to copy \"%1\"").arg(relativePath));
    } else if (copyText(fullPath)) {
        return true;
    }
    return false;
}

QString App::getClipboardText() const
{
    if (auto *const clipboard = QGuiApplication::clipboard()) {
        return clipboard->text();
    }
    return QString();
}

bool App::loadIgnorePatterns(const QString &dirId, QObject *textArea)
{
    auto res = m_connection.ignores(dirId, [this, textArea](SyncthingIgnores &&ignores, QString &&error) {
        if (!error.isEmpty()) {
            emit this->error(tr("Unable to load ignore patterns: ") + error);
        }
        textArea->setProperty("text", ignores.ignore.join(QChar('\n')));
        textArea->setProperty("enabled", true);
    });
    connect(textArea, &QObject::destroyed, res.reply, &QNetworkReply::deleteLater);
    connect(this, &QObject::destroyed, [connection = res.connection] { disconnect(connection); });
    return true;
}

bool App::saveIgnorePatterns(const QString &dirId, QObject *textArea)
{
    textArea->setProperty("enabled", false);
    const auto text = textArea->property("text");
    if (text.userType() != QMetaType::QString) {
        textArea->setProperty("enabled", true);
        return false;
    }
    auto res = m_connection.setIgnores(
        dirId, SyncthingIgnores{ .ignore = text.toString().split(QChar('\n')), .expanded = QStringList() }, [this, textArea](QString &&error) {
            if (!error.isEmpty()) {
                emit this->error(tr("Unable to save ignore patterns: ") + error);
            }
            textArea->setProperty("enabled", true);
        });
    connect(textArea, &QObject::destroyed, res.reply, &QNetworkReply::deleteLater);
    connect(this, &QObject::destroyed, [connection = res.connection] { disconnect(connection); });
    return true;
}

bool App::openIgnorePatterns(const QString &dirId)
{
    return openPath(dirId, QStringLiteral(".stignore"));
}

bool App::loadErrors(QObject *listView)
{
    listView->setProperty("model", QVariant::fromValue(new Data::SyncthingErrorModel(m_connection, listView)));
    return true;
}

bool App::showLog(QObject *textArea)
{
    textArea->setProperty("text", QString());
    connect(this, &App::logsAvailable, textArea, [textArea](const QString &newLogs) {
        QMetaObject::invokeMethod(textArea, "insert", Q_ARG(int, textArea->property("length").toInt()), Q_ARG(QString, newLogs));
    });
#ifdef Q_OS_ANDROID
    connect(textArea, &QObject::destroyed, this, [this] { sendMessageToService(ServiceAction::CloseLog); });
    sendMessageToService(ServiceAction::FollowLog);
#else
    emit replayLogRequested();
#endif
    return true;
}

void App::clearLog()
{
#ifdef Q_OS_ANDROID
    sendMessageToService(ServiceAction::ClearLog);
#else
    emit clearLogRequested();
#endif
}

bool App::showQrCode(Icon *icon)
{
    if (m_connection.myId().isEmpty()) {
        return false;
    }
    connect(&m_connection, &Data::SyncthingConnection::qrCodeAvailable, icon,
        [icon, requestedId = m_connection.myId(), this](const QString &id, const QByteArray &data) {
            if (id == requestedId) {
                disconnect(&m_connection, nullptr, icon, nullptr);
                icon->setSource(QImage::fromData(data));
            }
        });
    m_connection.requestQrCode(m_connection.myId());
    return true;
}

bool App::loadDirErrors(const QString &dirId, QObject *view)
{
    auto connection = connect(&m_connection, &Data::SyncthingConnection::dirStatusChanged, view, [this, dirId, view](const Data::SyncthingDir &dir) {
        if (dir.id != dirId) {
            return;
        }
        auto array = m_engine->newArray(static_cast<quint32>(dir.itemErrors.size()));
        auto index = quint32();
        for (const auto &itemError : dir.itemErrors) {
            auto error = m_engine->newObject();
            error.setProperty(QStringLiteral("path"), itemError.path);
            error.setProperty(QStringLiteral("message"), itemError.message);
            array.setProperty(index++, error);
        }
        view->setProperty("model", array.toVariant());
        view->setProperty("enabled", true);
    });
    connect(this, &QObject::destroyed, [connection] { disconnect(connection); });
    m_connection.requestDirPullErrors(dirId);
    return true;
}

bool App::loadStatistics(const QJSValue &callback)
{
    if (!callback.isCallable()) {
        return false;
    }
    auto query = m_connection.requestJsonData(
        QByteArrayLiteral("GET"), QStringLiteral("svc/report"), QUrlQuery(), QByteArray(), [this, callback](QJsonDocument &&doc, QString &&error) {
            auto report = doc.object().toVariantMap();
            report[QStringLiteral("uptime")] = QString::fromStdString(CppUtilities::TimeSpan::fromSeconds(report[QStringLiteral("uptime")].toDouble())
                    .toString(CppUtilities::TimeSpanOutputFormat::WithMeasures));
            report[QStringLiteral("memoryUsage")] = QString::fromStdString(
                CppUtilities::dataSizeToString(static_cast<std::uint64_t>(report.take(QStringLiteral("memoryUsageMiB")).toDouble() * 1024 * 1024)));
            report[QStringLiteral("processRSS")] = QString::fromStdString(
                CppUtilities::dataSizeToString(static_cast<std::uint64_t>(report.take(QStringLiteral("processRSSMiB")).toDouble() * 1024 * 1024)));
            report[QStringLiteral("memorySize")] = QString::fromStdString(
                CppUtilities::dataSizeToString(static_cast<std::uint64_t>(report[QStringLiteral("memorySize")].toDouble() * 1024 * 1024)));
            report[QStringLiteral("totSize")] = QString::fromStdString(
                CppUtilities::dataSizeToString(static_cast<std::uint64_t>(report.take(QStringLiteral("totMiB")).toDouble() * 1024 * 1024)));
            statistics(report);
            callback.call(QJSValueList({ m_engine->toScriptValue(report), QJSValue(std::move(error)) }));
        });
    connect(this, &QObject::destroyed, query.reply, &QNetworkReply::deleteLater);
    connect(this, &QObject::destroyed, [c = query.connection] { disconnect(c); });
    return true;
}

bool App::showError(const QString &errorMessage)
{
    emit error(errorMessage);
    return true;
}

Data::SyncthingFileModel *App::createFileModel(const QString &dirId, QObject *parent)
{
    auto row = int();
    auto dirInfo = m_connection.findDirInfo(dirId, row);
    if (!dirInfo) {
        return nullptr;
    }
    auto model = new Data::SyncthingFileModel(m_connection, *dirInfo, parent);
    connect(model, &Data::SyncthingFileModel::notification, this,
        [this](const QString &type, const QString &message, const QString &details = QString()) {
            type == QStringLiteral("error") ? emit error(message, details) : emit info(message, details);
        });
    return model;
}

DiffHighlighter *App::createDiffHighlighter(QTextDocument *parent)
{
    return new DiffHighlighter(parent);
}

QString App::resolveUrl(const QUrl &url)
{
#if defined(Q_OS_ANDROID)
    const auto urlString = url.toString(QUrl::FullyEncoded);
    const auto path = QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<jstring>("resolveUri", urlString).toString();
    if (path.isEmpty()) {
        showToast(tr("Unable to resolve URL \"%1\".").arg(urlString));
    }
    return path.isEmpty() ? urlString : path;
#elif defined(Q_OS_WINDOWS)
    auto path = url.path();
    while (path.startsWith(QChar('/'))) {
        path.remove(0, 1);
    }
    return path;
#else
    return url.path();
#endif
}

bool App::shouldIgnorePermissions(const QString &path)
{
#ifdef Q_OS_ANDROID
    // QStorageInfo only returns "fuse" on Android but we can assume that permissions should be generally ignored
    Q_UNUSED(path)
    return true;
#else
    static const auto problematicFileSystems = QSet<QByteArray>({ QByteArrayLiteral("fat"), QByteArrayLiteral("vfat"), QByteArrayLiteral("exfat") });
    const auto storageInfo = QStorageInfo(path);
    return storageInfo.isValid() && problematicFileSystems.contains(storageInfo.fileSystemType());
#endif
}


bool App::postSyncthingConfig(const QJsonObject &rawConfig, const QJSValue &callback)
{
    if (m_pendingConfigChange.reply) {
        emit error(tr("Another config change is still pending."));
        return false;
    }
    m_pendingConfigChange = m_connection.postConfigFromJsonObject(rawConfig, [this, callback](QString &&error) {
        m_pendingConfigChange.reply = nullptr;
        emit savingConfigChanged(false);
        invalidateStatus();
        if (callback.isCallable()) {
            callback.call(QJSValueList{ error });
        }
    });
    connect(this, &QObject::destroyed, m_pendingConfigChange.reply, &QNetworkReply::deleteLater);
    connect(this, &QObject::destroyed, [c = m_pendingConfigChange.connection] { disconnect(c); });
    emit savingConfigChanged(true);
    invalidateStatus();
    return true;
}

bool App::invokeDirAction(const QString &dirId, const QString &action)
{
    if (action == QLatin1String("override")) {
        m_connection.requestOverride(dirId);
        return true;
    } else if (action == QLatin1String("revert")) {
        m_connection.requestRevert(dirId);
        return true;
    }
    return false;
}

bool QtGui::App::requestFromSyncthing(const QString &verb, const QString &path, const QVariantMap &parameters, const QJSValue &callback)
{
    auto params = QUrlQuery();
    for (const auto &parameter : parameters.asKeyValueRange()) {
        params.addQueryItem(parameter.first, parameter.second.toString());
    }
    auto query = m_connection.requestJsonData(verb.toUtf8(), path, params, QByteArray(), [this, callback](QJsonDocument &&doc, QString &&error) {
        if (callback.isCallable()) {
            callback.call(QJSValueList({ m_engine->toScriptValue(doc.object()), QJSValue(std::move(error)) }));
        }
    });
    connect(this, &QObject::destroyed, query.reply, &QNetworkReply::deleteLater);
    connect(this, &QObject::destroyed, [c = query.connection] { disconnect(c); });
    return true;
}

QString App::formatDataSize(quint64 size) const
{
    return QString::fromStdString(CppUtilities::dataSizeToString(size));
}

QString App::formatTraffic(quint64 total, double rate) const
{
    return trafficString(total, rate);
}

bool QtGui::App::hasDevice(const QString &id)
{
    auto row = 0;
    return m_connection.findDevInfo(id, row) != nullptr;
}

bool QtGui::App::hasDir(const QString &id)
{
    auto row = 0;
    return m_connection.findDirInfo(id, row) != nullptr;
}

QString App::deviceDisplayName(const QString &id) const
{
    auto row = 0;
    auto info = m_connection.findDevInfo(id, row);
    return info != nullptr ? info->displayName() : id;
}

QString App::dirDisplayName(const QString &id) const
{
    auto row = 0;
    auto info = m_connection.findDirInfo(id, row);
    return info != nullptr ? info->displayName() : id;
}

QVariantList App::computeDirsNeedingItems(const QModelIndex &devProxyModelIndex) const
{
    const auto *const devInfo = m_devModel.devInfo(m_sortFilterDevModel.mapToSource(devProxyModelIndex));
    auto dirs = QVariantList();
    if (!devInfo) {
        return dirs;
    }
    dirs.reserve(static_cast<QStringList::size_type>(devInfo->completionByDir.size()));
    for (const auto &[dirId, completion] : devInfo->completionByDir) {
        if (completion.needed.items < 1) {
            continue;
        }
        auto row = 0;
        auto dirNeedInfo = QVariantMap();
        auto *const dirInfo = m_connection.findDirInfo(dirId, row);
        dirNeedInfo.insert(QStringLiteral("dirId"), dirId);
        dirNeedInfo.insert(QStringLiteral("dirName"), dirInfo ? dirInfo->displayName() : dirId);
        dirNeedInfo.insert(QStringLiteral("items"), completion.needed.items);
        dirNeedInfo.insert(QStringLiteral("bytes"), completion.needed.bytes);
        dirs.emplace_back(std::move(dirNeedInfo));
    }
    return dirs;
}

/*!
 * \brief Opens the Syncthing config file in the standard editor.
 */
bool QtGui::App::openSyncthingConfigFile()
{
    return openPath(m_syncthingConfigDir + QStringLiteral("/config.xml"));
}

/*!
 * \brief Opens the Syncthing log file in the standard editor.
 */
bool QtGui::App::openSyncthingLogFile()
{
    return openPath(syncthingLogFilePath());
}

/*!
 * \brief Opens the specified \a url externally, e.g. using an external web browser or a custom browser tab.
 */
bool QtGui::App::openUrlExternally(const QUrl &url, bool viaQt)
{
#ifdef Q_OS_ANDROID
    if (!viaQt) {
        return QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<jboolean>("openUrl", url.toString());
    }
#else
    Q_UNUSED(viaQt)
#endif
    return QDesktopServices::openUrl(url);
}

/*!
 * \brief Returns whether \a path is populated if it points to a directory usable as Syncthing home.
 */
QVariant App::isPopulated(const QString &path) const
{
    auto ec = std::error_code();
    const auto stdPath = std::filesystem::path(SYNCTHING_APP_STRING_CONVERSION(path));
    const auto status = std::filesystem::symlink_status(stdPath, ec);
    const auto existing = std::filesystem::exists(status);
    return (existing && !std::filesystem::is_directory(status)) ? QVariant() : QVariant(existing && !std::filesystem::is_empty(stdPath, ec));
}

#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
SyncthingModels *SyncthingModels::create(QQmlEngine *qmlEngine, QJSEngine *engine)
{
    return dataObjectFromProperty<SyncthingModels>(qmlEngine, engine, "models");
}



#endif

} // namespace QtGui
