#ifndef PLAYER_H
#define PLAYER_H

#include <QObject>
#include <QMutex>

#include "Data.h"

class Player : public QObject
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
    bool getCurrentTrack(EMSTrack *track);
    bool addTrack(EMSTrack track);
    bool removeTrack(EMSTrack track);
    bool removeAllTracks();

    /*
     * Control playback
     */
    EMSPlayerState getStatus();
    bool next();
    bool prev();
    bool play(unsigned int pos=0);
    bool stop();

    /*
     * Playlist settings
     */
    void setRandom(bool random);
    bool getRandom();

    /* ---------------------------------
     *    Signleton pattern
     * ---------------------------------
     * See: http://www.qtcentre.org/wiki/index.php?title=Singleton_pattern
     */
    static Player* instance()
    {
        static QMutex mutex;
        if (!_instance)
        {
            mutex.lock();

            if (!_instance)
                _instance = new Player;

            mutex.unlock();
        }
        return _instance;
    }

    static void drop()
    {
        static QMutex mutex;
        mutex.lock();
        delete _instance;
        _instance = 0;
        mutex.unlock();
    }

private:
    EMSPlayerState status;
    EMSPlaylist playlist;

    /* Singleton pattern */
    static Player* _instance;
    Player(QObject *parent = 0);
    Player(const Player &);
    Player& operator=(const Player &);
    ~Player();

signals:
    void statusChanged(EMSPlayerState newStatus);
    void playlistChanged(EMSPlaylist newPlaylist);

public slots:

};

#endif // PLAYER_H
