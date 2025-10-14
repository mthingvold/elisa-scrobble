#include "scrobblemanager.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

#include "elisaapplication.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>

class ScrobbleManagerPrivate
{
public:
    ElisaApplication *ea = nullptr;
    ScrobbleManager::ScrobbleProvider currentProvider;
};

ScrobbleManager::ScrobbleManager(QObject *parent)
    : QObject{parent}
    , d(std::make_unique<ScrobbleManagerPrivate>())
{
    manager = new QNetworkAccessManager(this);
    // connect(manager, &QNetworkAccessManager::finished, this, &ScrobbleManager::onFinished);
}

ScrobbleManager::~ScrobbleManager()
{
}

ElisaApplication *ScrobbleManager::elisaApplication() const
{
    return d->ea;
}

void ScrobbleManager::setElisaApplication(ElisaApplication *elisaApplication)
{
    if (d->ea == elisaApplication) {
        return;
    }

    d->ea = elisaApplication;
    Q_EMIT elisaApplicationChanged();
}

void ScrobbleManager::connectToScrobbler(ScrobbleProvider provider)
{
    qDebug() << "Connecting to scrobbler provider:" << static_cast<int>(provider);

    switch (provider) {
    case ScrobbleProvider::LastFM:
        connectToLastFM();
        break;
    case ScrobbleProvider::ListenBrainz:
        connectToListenBrainz();
        break;
    default:
        exit(1);
    }
}

void ScrobbleManager::connectToLastFM()
{
    retrieveLastFmToken();
}

void ScrobbleManager::connectToListenBrainz()
{
    exit(0);
}

QVariantMap ScrobbleManager::persistentState() const
{
    QVariantMap currentState;

    // currentState[QStringLiteral("playList")] = d->mPlayListModel->getEntriesForRestore();
    // currentState[QStringLiteral("shuffleMode")] = d->mShuffleMode;
    // currentState[QStringLiteral("randomMapping")] = getRandomMappingForRestore();
    // currentState[QStringLiteral("currentTrack")] = d->mCurrentPlayListPosition;
    // currentState[QStringLiteral("repeatMode")] = d->mRepeatMode;

    return currentState;
}

void ScrobbleManager::setPersistentState(const QVariantMap &persistentStateValue)
{
    // qDebug(orgKdeElisaScrobbleManager()) << "MediaPlayListProxyModel::setPersistentState" << persistentStateValue;

    // auto playListIt = persistentStateValue.find(QStringLiteral("playList"));
    // if (playListIt != persistentStateValue.end()) {
    //     d->mPlayListModel->enqueueRestoredEntries(playListIt.value().toList());
    // }

    // auto shuffleModeStoredValue = persistentStateValue.find(QStringLiteral("shuffleMode"));
    // auto shuffleRandomMappingIt = persistentStateValue.find(QStringLiteral("randomMapping"));
    // if (shuffleModeStoredValue != persistentStateValue.end() && shuffleRandomMappingIt != persistentStateValue.end()) {
    //     restoreShuffleMode(shuffleModeStoredValue->value<Shuffle>(), shuffleRandomMappingIt.value().toList());
    // }

    // auto playerCurrentTrack = persistentStateValue.find(QStringLiteral("currentTrack"));
    // if (playerCurrentTrack != persistentStateValue.end()) {
    //     auto newIndex = index(playerCurrentTrack->toInt(), 0);
    //     if (newIndex.isValid() && (newIndex != d->mCurrentTrack)) {
    //         d->mCurrentTrack = newIndex;
    //         notifyCurrentTrackChanged();
    //     }
    // }

    // auto repeatPlayStoredValue = persistentStateValue.find(QStringLiteral("repeatPlay"));
    // if (repeatPlayStoredValue != persistentStateValue.end() && repeatPlayStoredValue->value<bool>()) {
    //     setRepeatMode(Repeat::Playlist);
    // }

    // auto repeatModeStoredValue = persistentStateValue.find(QStringLiteral("repeatMode"));
    // if (repeatModeStoredValue != persistentStateValue.end()) {
    //     setRepeatMode(repeatModeStoredValue->value<Repeat>());
    // }

    Q_EMIT persistentStateChanged();
}

void ScrobbleManager::retrieveLastFmToken()
{
    // Construct the URL
    QUrl url(QStringLiteral("https://ws.audioscrobbler.com/2.0/"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("method"), QStringLiteral("auth.gettoken"));
    query.addQueryItem(QStringLiteral("api_key"), scrobbleApiKey);
    query.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    url.setQuery(query);
    qDebug() << "Network request:";

    // Create the network request3
    QNetworkRequest request(url);

    // Send the GET request
    QNetworkReply *reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        retrieveTokensFinished(reply);
        reply->deleteLater();
    });
}

void ScrobbleManager::retrieveTokensFinished(QNetworkReply *reply)
{
    QObject::disconnect(reply, nullptr, this, nullptr);
    if (reply->error() == QNetworkReply::NoError) {
        // Read the response data
        QByteArray responseData = reply->readAll();

        // Parse the JSON response
        QJsonDocument jsonResponse = QJsonDocument::fromJson(responseData);
        if (!jsonResponse.isNull() && jsonResponse.isObject()) {
            QJsonObject jsonObject = jsonResponse.object();
            if (jsonObject.contains(QStringLiteral("token"))) {
                if (!scrobbleListenerServer) { }
                QString token = jsonObject[QStringLiteral("token")].toString();
                qDebug() << "Token received:" << token;
                QUrl url(QStringLiteral("https://www.last.fm/api/auth/"));
                QUrlQuery query;
                // query.addQueryItem(QStringLiteral("method"), QStringLiteral("auth.gettoken"));
                query.addQueryItem(QStringLiteral("api_key"), scrobbleApiKey);
                // query.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
                query.addQueryItem(QStringLiteral("token"), token);
                url.setQuery(query);
                QDesktopServices::openUrl(url);
            } else {
                qDebug() << "Token not found in response.";
            }
        } else {
            qDebug() << "Invalid JSON response.";
        }
    } else {
        qDebug() << "Error:" << reply->errorString();
    }

    // Clean up the reply object
    reply->deleteLater();
}

void ScrobbleManager::setupSession(QString token)
{
    // Construct the URL
    QUrl url(QStringLiteral("http://ws.audioscrobbler.com/2.0/"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("api_key"), scrobbleApiKey);
    query.addQueryItem(QStringLiteral("method"), QStringLiteral("auth.getSession"));
    query.addQueryItem(QStringLiteral("token"), token);
    query.addQueryItem(QStringLiteral(""), QLatin1String(scrobbleApiSecret.toUtf8()));
    for (auto item : query.queryItems()) {
        qDebug() << "First: " << item.first << " Second: " << item.second;
    }

    // query.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    url.setQuery(query);

    // Create the network request3
    QNetworkRequest request(url);

    // Send the GET request
    QNetworkReply *reply = manager->get(request);
    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        setupSessionFinished(reply);
    });
}

void ScrobbleManager::setupSessionFinished(QNetworkReply *reply)
{
    qDebug() << "Network replyived:";
    QObject::disconnect(reply, nullptr, this, nullptr);
    if (reply->error() == QNetworkReply::NoError) {
        // Read the response data
        QByteArray responseData = reply->readAll();

        // Parse the JSON response
        QJsonDocument jsonResponse = QJsonDocument::fromJson(responseData);
        if (!jsonResponse.isNull() && jsonResponse.isObject()) {
            QJsonObject jsonObject = jsonResponse.object();
            if (jsonObject.contains(QStringLiteral("token"))) {
                if (!scrobbleListenerServer) { }
                QString token = jsonObject[QStringLiteral("token")].toString();
                qDebug() << "Token received:" << token;
                QUrl url(QStringLiteral("http://www.last.fm/api/auth/"));
                QUrlQuery query;
                // query.addQueryItem(QStringLiteral("method"), QStringLiteral("auth.gettoken"));
                query.addQueryItem(QStringLiteral("api_key"), scrobbleApiKey);
                // query.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
                query.addQueryItem(QStringLiteral("token"), token);
                url.setQuery(query);
                QDesktopServices::openUrl(url);
                setupSession(token);
            } else {
                qDebug() << "Token not found in response.";
            }
        } else {
            qDebug() << "Invalid JSON response.";
        }
    } else {
        qDebug() << "Error:" << reply->errorString();
    }

    // Clean up the reply object
    reply->deleteLater();
}
// void ScrobbleManager::Scrobble(const Song &song) {
void ScrobbleManager::scrobble()
{
    trackScrobbled = true;
    qDebug() << "Scrobbling: " << mCurrentTrack.data(MediaPlayList::AlbumRole) << mCurrentTrack.data(MediaPlayList::ArtistRole)
             << mCurrentTrack.data(MediaPlayList::TitleRole) << mCurrentTrack.data(MediaPlayList::TrackNumberRole);

    QUrl url(QStringLiteral("http://ws.audioscrobbler.com/2.0/"));
    QUrlQuery query;
    // query.addQueryItem(QStringLiteral("method"), QStringLiteral("auth.gettoken"));
    query.addQueryItem(QStringLiteral("artist"), scrobbleApiKey);
    query.addQueryItem(QStringLiteral("track"), scrobbleApiKey);
    query.addQueryItem(QStringLiteral("timestamp"), scrobbleApiKey);
    query.addQueryItem(QStringLiteral("album"), scrobbleApiKey);
    query.addQueryItem(QStringLiteral("chosenByUser"), QStringLiteral("1"));
    query.addQueryItem(QStringLiteral("trackNumber"), scrobbleApiKey);
    query.addQueryItem(QStringLiteral("albumArtist"), scrobbleApiKey);

    query.addQueryItem(QStringLiteral("duration"), scrobbleApiKey);
    query.addQueryItem(QStringLiteral("api_key"), scrobbleApiKey);
    query.addQueryItem(QStringLiteral("api_sig"), scrobbleApiKey);
    query.addQueryItem(QStringLiteral("sk"), scrobbleApiKey);

    // if (song.id() != song_playing_.id() || song.url() != song_playing_.url() || !song.is_metadata_good()) return;

    // scrobbled_ = true;

    // cache_->Add(song, timestamp_);

    // if (settings_->offline()) return;

    // if (!authenticated()) {
    //     if (settings_->show_error_dialog()) {
    //         Q_EMIT ErrorMessage(tr("Scrobbler %1 is not authenticated!").arg(name_));
    //     }
    //     return;
    // }

    // StartSubmit(true);
}

void ScrobbleManager::positionChanged(qint64 pos)
{
    if (!trackScrobbled && (pos - seekPos > mDuration / 2 || pos - seekPos > maxDurationBeforeScrobble)) {
        scrobble();
    }
}

void ScrobbleManager::currentTrackChanged(const QPersistentModelIndex &currentTrack)
{
    if (!currentTrack.isValid() || currentTrack == mCurrentTrack) {
        qDebug() << "TRACKS ARE EQUAL?" << "\n";
        // return;
    }
    seekPos = 0;
    mCurrentTrack = currentTrack;
    trackScrobbled = false;

    // qDebug() << "\nHello 2" << currentTrack.isValid() << currentTrack.data(MediaPlayList::AlbumRole) << currentTrack.data(MediaPlayList::ArtistRole)
    //          << currentTrack.data(MediaPlayList::TitleRole) << currentTrack.data(MediaPlayList::TrackNumberRole)
    //     << "\n\n\n"
    //          << currentTrack.data(MediaPlayList::YearRole);

    // qDebug() << "\nHel" << mCurrentTrack.isValid() << mCurrentTrack.data(MediaPlayList::AlbumRole) << mCurrentTrack.data(MediaPlayList::ArtistRole)
    //          << mCurrentTrack.data(MediaPlayList::TitleRole) << mCurrentTrack.data(MediaPlayList::TrackNumberRole)
    //          << "\n\n\n"
    //          << mCurrentTrack.data(MediaPlayList::YearRole);
}

void ScrobbleManager::seekChanged(qint64 newPos)
{
    seekPos = newPos;
    if (trackScrobbled && newPos < mDuration * 0.1) {
        trackScrobbled = false;
    }
}

void ScrobbleManager::audioDurationChanged(qint64 duration)
{
    // qDebug() << "Duration set: " << duration;
    mDuration = duration;
}

#include "moc_scrobblemanager.cpp"
