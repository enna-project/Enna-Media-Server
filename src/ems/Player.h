#ifndef PLAYER_H
#define PLAYER_H

#include <QThread>
#include <QQueue>
#include <QMutex>
#include <QSemaphore>

#include "Data.h"

class Player : public QThread
{
    Q_OBJECT

public:
    /* ---------------------------------
     *    API FOR OTHER MODULES
     * ---------------------------------
     *
     * Control current playlist contents
     */
    EMSPlaylist getCurentPlaylist();
    int getCurrentPos();            /* If a track is active (played/paused),
                                     * return its index in the playlist,
                                     * otherwise return -1 */
    void addTrack(EMSTrack track);
    void removeTrack(EMSTrack track);
    void removeAllTracks();
    int searchTrackInPlaylist(EMSTrack);

    /*
     * Control playback
     */
    EMSPlayerStatus getStatus();
    void next();
    void prev();
    void play(EMSTrack track);
    void play();
    void pause();
    void toggle();
    void stop();
    void setRandom(bool random);
    bool getRandom();
    void setRepeat(bool repeat);
    bool getRepeat();

    /*
     * Select output audio device
     */
    void enableOutput(unsigned int id);
    void disableOutput(unsigned int id);
    QVector<EMSSndCard> getOutputs();

    /*
     * Utils
     */
    QString stateToString(EMSPlayerState state);

    /* ---------------------------------
     *    Signleton pattern
     * ---------------------------------
     * See: http://www.qtcentre.org/wiki/index.php?title=Singleton_pattern
     */
    static Player* instance()
    {
        static QMutex mutexinst;
        if (!_instance)
        {
            mutexinst.lock();

            if (!_instance)
                _instance = new Player;

            mutexinst.unlock();
        }
        return _instance;
    }

    static void drop()
    {
        static QMutex mutexinst;
        mutexinst.lock();
        delete _instance;
        _instance = 0;
        mutexinst.unlock();
    }

    void kill();

protected:
    void run() Q_DECL_OVERRIDE;

private:
    EMSPlayerStatus status;
    EMSPlaylist playlist;
    QVector<EMSSndCard> outputs;

    /* Mutex protecting shared data
     * Be careful when using this mutex, it must be used
     * to protect unblocking section
     */
    QMutex mutex;

    /* For communication thread (see run function) */
    struct mpd_connection *conn; /* Not shared */
    QQueue<EMSPlayerCmd> queue; /* Shared */
    QSemaphore cmdAvailable; /* Thread safe */

    /* Internal functions */
    void connectToMpd();
    void disconnectToMpd();
    void configureInitial();
    void updateStatus();
    void retrieveSndCardList();
    void executeCmd(EMSPlayerCmd cmd);
    QString getMPDFilename(EMSTrack track);

    /* Singleton pattern */
    static Player* _instance;
    Player(QObject *parent = 0);
    Player(const Player &);
    Player& operator=(const Player &);
    ~Player();

signals:
    void statusChanged(EMSPlayerStatus newStatus);
    void playlistChanged(EMSPlaylist newPlaylist);
    void outputsChanged(QVector<EMSSndCard> outputs);

public slots:

};

#endif // PLAYER_H
