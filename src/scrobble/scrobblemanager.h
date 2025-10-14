#ifndef SCROBBLEMANAGER_H
#define SCROBBLEMANAGER_H

#include "elisaLib_export.h"

#include "datatypes.h"
#include "elisautils.h"
#include "mediaplaylist.h"

#include <QAbstractProxyModel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QQmlEngine>
#include <QTcpServer>

class ElisaApplication;
class ScrobbleManagerPrivate;
// Q_DECLARE_OPAQUE_POINTER(ElisaApplication*)

class ScrobbleManager : public QObject
{
    Q_OBJECT

    QML_ELEMENT

    Q_PROPERTY(ElisaApplication *elisaApplication READ elisaApplication WRITE setElisaApplication NOTIFY elisaApplicationChanged)

    // Q_PROPERTY(TracksListener* tracksListener
    //            READ tracksListener
    //            NOTIFY tracksListenerChanged)

    Q_PROPERTY(QVariantMap persistentState READ persistentState WRITE setPersistentState NOTIFY persistentStateChanged)
    // Q_PROPERTY(
    //                READ scrobbleView
    //                    WRITE setScrobble
    //                        NOTIFY scrobbleChanged)

public:
    enum class ScrobbleProvider {
        None,
        LastFM,
        ListenBrainz,
    };
    Q_ENUM(ScrobbleProvider)

    explicit ScrobbleManager(QObject *parent = nullptr);

    ~ScrobbleManager() override;

    [[nodiscard]] ElisaApplication *elisaApplication() const;

    [[nodiscard]] QVariantMap persistentState() const;

    //[[nodiscard]] TracksListener* tracksListener() const;

    void setPersistentState(const QVariantMap &persistentState);

    void retrieveLastFmToken();

Q_SIGNALS:

    void viewDatabaseChanged();

    void applicationIsTerminating();

    // void tracksListenerChanged();

    // void importedTracksCountChanged();

    void elisaApplicationChanged();

    void persistentStateChanged();

    // void removeTracksInError(const QList<QUrl> &tracks);

    // void displayTrackError(const QString &fileName);

    // void indexerBusyChanged();

    // void clearDatabase();

    // void clearedDatabase();

    // void fileSystemIndexerActiveChanged();

    // void androidIndexerActiveChanged();

    // void androidIndexerAvailableChanged();

    // void refreshDatabase();
public Q_SLOTS:
    void connectToScrobbler(ScrobbleProvider provider);
    void setElisaApplication(ElisaApplication *elisaApplication);
    void scrobble();
    void positionChanged(qint64 position);
    void seekChanged(qint64 newPos);
    void currentTrackChanged(const QPersistentModelIndex &currentTrack);
    void audioDurationChanged(qint64 duration);

    // void enqueue(const QUrl &entryUrl,
    //              ElisaUtils::PlayListEnqueueMode enqueueMode,
    //              ElisaUtils::PlayListEnqueueTriggerPlay triggerPlay);
private Q_SLOTS:
    void retrieveTokensFinished(QNetworkReply *reply);
    void setupSessionFinished(QNetworkReply *reply);

private:
    qint64 trackPlaytime;
    qint64 seekPos;
    qint64 mDuration;
    bool trackScrobbled = false;
    QPersistentModelIndex mCurrentTrack;

    void connectToLastFM();
    void connectToListenBrainz();
    QNetworkAccessManager *manager;
    std::unique_ptr<ScrobbleManagerPrivate> d;

    QTcpServer *scrobbleListenerServer;

    void setupSession(QString token);
    //  QNetworkReply *reply;

    const QString scrobbleApiKey = QStringLiteral("0bc3abb05a8b9a668960bf39677a9f6d");
    const QString scrobbleApiSecret = QStringLiteral("helloworld");
    const qint64 maxDurationBeforeScrobble = 240000;
    // signals:
};

#endif // SCROBBLEMANAGER_H
