#include "Player.h"
#include "DefaultSettings.h"
#include <QDebug>
#include <QFile>
#include <QSettings>
#include <QCoreApplication>
#include <mpd/client.h>

Player* Player::_instance = 0;

/* ---------------------------------------------------------
 *      PUBLIC PLAYER API (NOT BLOCKING - THREAD SAFE)
 * --------------------------------------------------------- */
EMSPlaylist Player::getCurentPlaylist()
{
    EMSPlaylist out;
    mutex.lock();
    out = playlist;
    mutex.unlock();
    return out;
}

int Player::getCurrentPos()
{
    int pos = -1;
    mutex.lock();
    if (status.state == STATUS_PLAY || status.state == STATUS_PAUSE)
    {
        pos = status.posInPlaylist;
    }
    mutex.unlock();
    return pos;
}

void Player::addTrack(EMSTrack track)
{
    EMSPlayerCmd cmd;
    cmd.action = ACTION_ADD;
    cmd.track = track;
    mutex.lock();
    queue.enqueue(cmd);
    mutex.unlock();
    cmdAvailable.release(1);
}

void Player::removeTrack(EMSTrack track)
{
    EMSPlayerCmd cmd;
    cmd.action = ACTION_DEL;
    cmd.track = track;
    mutex.lock();
    queue.enqueue(cmd);
    mutex.unlock();
    cmdAvailable.release(1);
}

void Player::removeAllTracks()
{
    EMSPlayerCmd cmd;
    cmd.action = ACTION_DEL_ALL;
    mutex.lock();
    queue.enqueue(cmd);
    mutex.unlock();
    cmdAvailable.release(1);
}

EMSPlayerStatus Player::getStatus()
{
    EMSPlayerStatus out;
    mutex.lock();
    out = status;
    mutex.unlock();
    return out;
}

void Player::next()
{
    EMSPlayerCmd cmd;
    cmd.action = ACTION_NEXT;
    mutex.lock();
    queue.enqueue(cmd);
    mutex.unlock();
    cmdAvailable.release(1);
}

void Player::prev()
{
    EMSPlayerCmd cmd;
    cmd.action = ACTION_PREV;
    mutex.lock();
    queue.enqueue(cmd);
    mutex.unlock();
    cmdAvailable.release(1);
}

void Player::play(EMSTrack track)
{
    EMSPlayerCmd cmd;
    cmd.action = ACTION_PLAY_POS;
    int position = searchTrackInPlaylist(track);
    if (position < 0)
    {
        qCritical() << "Asked to play a track which is not in the current playlist";
        return;
    }
    cmd.uintValue = (unsigned int) position;
    mutex.lock();
    queue.enqueue(cmd);
    mutex.unlock();
    cmdAvailable.release(1);
}

void Player::play(unsigned int position)
{
    mutex.lock();
    int size = playlist.tracks.size();
    mutex.unlock();
    if (size < 0 || position >= (unsigned int)size)
    {
        qDebug() << "Asked to play a track after the end of the current playlist";
        return;
    }

    EMSPlayerCmd cmd;
    cmd.action = ACTION_PLAY_POS;
    cmd.uintValue = position;
    mutex.lock();
    queue.enqueue(cmd);
    mutex.unlock();
    cmdAvailable.release(1);
}

void Player::play()
{
    EMSPlayerCmd cmd;
    cmd.action = ACTION_PLAY;
    mutex.lock();
    queue.enqueue(cmd);
    mutex.unlock();
    cmdAvailable.release(1);
}

void Player::seek(unsigned int percent)
{
    if (percent > 100)
    {
        qDebug() << "Invalide seek value, must be [O;100]";
        return;
    }

    int currentSongId = getCurrentPos();
    if (currentSongId < 0 || currentSongId >= playlist.tracks.size())
    {
        return;
    }
    unsigned int totalTime = playlist.tracks.at(currentSongId).duration;
    EMSPlayerCmd cmd;
    cmd.action = ACTION_SEEK;
    cmd.uintValue = (totalTime*percent) / 100;

    mutex.lock();
    queue.enqueue(cmd);
    mutex.unlock();
    cmdAvailable.release(1);
}

void Player::pause()
{
    EMSPlayerCmd cmd;
    cmd.action = ACTION_PAUSE;
    mutex.lock();
    queue.enqueue(cmd);
    mutex.unlock();
    cmdAvailable.release(1);
}

void Player::toggle()
{
    EMSPlayerCmd cmd;
    cmd.action = ACTION_TOGGLE;
    mutex.lock();
    queue.enqueue(cmd);
    mutex.unlock();
    cmdAvailable.release(1);
}

void Player::stop()
{
    EMSPlayerCmd cmd;
    cmd.action = ACTION_STOP;
    mutex.lock();
    queue.enqueue(cmd);
    mutex.unlock();
    cmdAvailable.release(1);
}

void Player::setRandom(bool randomIn)
{
    mutex.lock();
    EMSPlayerCmd cmd;
    cmd.action = ACTION_RANDOM;
    cmd.boolValue = randomIn;
    queue.enqueue(cmd);
    mutex.unlock();
    cmdAvailable.release(1);
}

bool Player::getRandom()
{
    bool out;
    mutex.lock();
    out = status.random;
    mutex.unlock();
    return out;
}

void Player::setRepeat(bool repeatIn)
{
    mutex.lock();
    EMSPlayerCmd cmd;
    cmd.action = ACTION_REPEAT;
    cmd.boolValue = repeatIn;
    queue.enqueue(cmd);
    mutex.unlock();
    cmdAvailable.release(1);
}

bool Player::getRepeat()
{
    bool out;
    mutex.lock();
    out = status.repeat;
    mutex.unlock();
    return out;
}

void Player::enableOutput(unsigned int id)
{
    mutex.lock();
    EMSPlayerCmd cmd;
    cmd.action = ACTION_ENABLE_OUTPUT;
    cmd.uintValue = id;
    queue.enqueue(cmd);
    mutex.unlock();
    cmdAvailable.release(1);
}

void Player::disableOutput(unsigned int id)
{
    mutex.lock();
    EMSPlayerCmd cmd;
    cmd.action = ACTION_DISABLE_OUTPUT;
    cmd.uintValue = id;
    queue.enqueue(cmd);
    mutex.unlock();
    cmdAvailable.release(1);
}

QVector<EMSSndCard> Player::getOutputs()
{
    QVector<EMSSndCard> out;
    mutex.lock();
    out = outputs;
    mutex.unlock();
    return out;
}

/* ---------------------------------------------------------
 *       PLAYLIST UTILS (NOT BLOCKING - THREAD SAFE)
 * --------------------------------------------------------- */
/* Look for the given track.
 * If the track has not been found, -1 is returned.
 */
int Player::searchTrackInPlaylist(EMSTrack track)
{
    mutex.lock();
    for (int i=0; i<playlist.tracks.size(); i++)
    {
        EMSTrack trackI = playlist.tracks.at(i);
        if (trackI.type != track.type)
        {
            continue;
        }
        if (trackI.type == TRACK_TYPE_DB)
        {
            if (trackI.id == track.id)
            {
                mutex.unlock();
                return i;
            }
        }
        else if (trackI.type == TRACK_TYPE_CDROM)
        {
            if (trackI.position == track.position &&
                trackI.filename == track.filename)
            {
                mutex.unlock();
                return i;
            }
        }
        else if (trackI.type == TRACK_TYPE_EXTERNAL)
        {
            if (trackI.filename == track.filename)
            {
                mutex.unlock();
                return i;
            }
        }
    }
    mutex.unlock();

    /* Not found => -1 */
    return -1;
}

QString Player::getMPDFilename(EMSTrack track)
{
    QString filename;

    if (track.type == TRACK_TYPE_DB || track.type == TRACK_TYPE_EXTERNAL)
    {
        /* File is in a storage disk. Use absolute path... */
        filename = "file://" + track.filename;
    }
    if (track.type == TRACK_TYPE_CDROM)
    {
        /* For type CDROM :
         * track.filename is the CDROM device (eg. /dev/cdrom)
         * position is the track number in the CDROM (starting from 1)
         */
        filename = "cdda://" + QString("%1/%2").arg(track.filename).arg(track.position);
    }
    return filename;
}

QString Player::stateToString(EMSPlayerState state)
{
    switch (state)
    {
        case STATUS_PAUSE:
            return QString("paused");
        case STATUS_PLAY:
            return QString("playing");
        case STATUS_STOP:
            return QString("stopped");
        default:
            return QString("unknown");
    }
}

/* ---------------------------------------------------------
 *              NETWORK COMMUNICATION (BLOCKING)
 * --------------------------------------------------------- */

/* Establish a connection with the MPD server
 * If the connection failed, the "conn" is set to NULL
 */
void Player::connectToMpd()
{
    QSettings settings;
    unsigned int retryPeriod;
    QString host;
    unsigned int port;
    unsigned int timeout;
    QString password;

    EMS_LOAD_SETTINGS(retryPeriod, "player/retry_period", EMS_MPD_CONNECTION_RETRY_PERIOD, UInt);
    EMS_LOAD_SETTINGS(host, "player/host", EMS_MPD_IP, String);
    EMS_LOAD_SETTINGS(timeout, "player/timeout", EMS_MPD_TIMEOUT, UInt);
    EMS_LOAD_SETTINGS(port, "player/port", EMS_MPD_PORT, UInt);
    EMS_LOAD_SETTINGS(password, "player/password", EMS_MPD_PASSWORD, String);

    if (conn != NULL)
    {
        disconnectToMpd();
    }

    /* Try to connect until it success */
    while (!conn && !this->isInterruptionRequested())
    {
        qDebug() << "Connecting to " << QString("%1:%2... (timeout %3ms)").arg(host).arg(port).arg(timeout);
        conn = mpd_connection_new(host.toStdString().c_str(), port, timeout);

        if (conn == NULL)
        {
            qCritical() << "MPD Connection failed.";
        }
        else if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS)
        {
            QString errorMessage = QString::fromUtf8(mpd_connection_get_error_message(conn));
            qCritical() << "MPD Connection failed : " << errorMessage;
            mpd_connection_free(conn);
            conn = NULL;
        }
        else if (!password.isEmpty())
        {
            qDebug() << "Setting password...";
            if (!mpd_run_password(conn, password.toStdString().c_str()))
            {
                qCritical() << "Setting MPD password failed.";
                mpd_connection_free(conn);
                conn = NULL;
            }
        }

        if (!conn)
        {
            qCritical() << "Retrying within " << QString("%1 ms").arg(retryPeriod);
            for (unsigned int i=0; i<retryPeriod; i++)
            {
                if (!this->isInterruptionRequested())
                    this->usleep(1000);
                else
                    break;
            }
        }
    }

    qDebug() << "Connected to " << host;
}

/* Disconnect from the MPD server
 */
void Player::disconnectToMpd()
{
    if (conn == NULL)
    {
        return;
    }

    mpd_connection_free(conn);
    conn = NULL;
}

/* This function configure the initial state of the player
 * MPD keep its own internal state, so we have to reset it
 */
void Player::configureInitial()
{
    stop();
    setRepeat(false);
    setRandom(false);
    removeAllTracks();
}

/* Execute the given command. In fact, it just will ask MPD to do it.
 * You will have to check the next "status" or "playlist" update (using the corresponding slot)
 * to know if the command has really been executed.
 */
void Player::executeCmd(EMSPlayerCmd cmd)
{
    bool waitResponse = true; /* Wait the responase by default */
    bool error = false;

    if (conn == NULL)
    {
        return;
    }

    /* Dispatch execution depending on the action */
    switch (cmd.action)
    {
        case ACTION_ADD:
        {
            if(searchTrackInPlaylist(cmd.track) >= 0)
            {
                qDebug() << "Do not add track in the playlist as it already exist";
                return;
            }
            QString filename = getMPDFilename(cmd.track);
            mpd_send_add(conn, filename.toStdString().c_str());
            break;
        }
        case ACTION_DEL:
        {
            int pos = searchTrackInPlaylist(cmd.track);
            if (pos >= 0)
            {
                mpd_send_delete(conn, pos);
            }
            break;
        }
        case ACTION_DEL_ALL:
        {
            mpd_send_clear(conn);
            break;
        }
        case ACTION_PLAY_POS:
        {
            mpd_send_play_pos(conn, cmd.uintValue);
            break;
        }
        case ACTION_PLAY:
        {
            mpd_send_play(conn);
            break;
        }
        case ACTION_SEEK:
        {
            /* Get current position */
            int songId = getCurrentPos();
            if (songId >= 0)
            {
                mpd_send_seek_pos(conn, songId, cmd.uintValue);
            }
            break;
        }
        case ACTION_PAUSE:
        {
            mpd_send_pause(conn, true);
            break;
        }
        case ACTION_TOGGLE:
        {
            mpd_send_toggle_pause(conn);
            break;
        }
        case ACTION_STOP:
        {
            mpd_send_stop(conn);
            break;
        }
        case ACTION_NEXT:
        {
            mpd_send_next(conn);
            break;
        }
        case ACTION_PREV:
        {
            mpd_send_previous(conn);
            break;
        }
        case ACTION_REPEAT:
        {
            mutex.lock();
            bool repeatTmp = status.repeat;
            mutex.unlock();
            if (cmd.boolValue != repeatTmp)
            {
                mpd_send_repeat(conn, cmd.boolValue);
            }
            break;
        }
        case ACTION_RANDOM:
        {
            mutex.lock();
            bool randomTmp = status.random;
            mutex.unlock();
            if (cmd.boolValue != randomTmp)
            {
                mpd_send_random(conn, cmd.boolValue);
            }
            break;
        }
        case ACTION_ENABLE_OUTPUT:
        {
            if (cmd.uintValue >= 1)
            {
                mpd_send_enable_output(conn, cmd.uintValue-1);
            }
            break;
        }
        case ACTION_DISABLE_OUTPUT:
        {
            if (cmd.uintValue >= 1)
            {
                mpd_send_disable_output(conn, cmd.uintValue-1);
            }
            break;
        }
        default:
        {
            qCritical() << "Unhandled action in the current command.";
            waitResponse = false;
            break;
        }
    }

    if (waitResponse && !mpd_response_finish(conn))
    {
        error = true;
        qCritical() << "MPD could not execute the current command.";
        enum mpd_error errorMpd = mpd_connection_get_error(conn);
        if (errorMpd == MPD_ERROR_SERVER ||
            errorMpd == MPD_ERROR_ARGUMENT) /* Problem with the command */
        {
            QString errorMessage = QString::fromUtf8(mpd_connection_get_error_message(conn));
            qCritical() << "Command error : " << errorMessage;
            if (!mpd_connection_clear_error(conn))
            {
                qCritical() << "This error cannot be cleared, reconnecting...";
                connectToMpd();
            }

        }
        else if (errorMpd == MPD_ERROR_TIMEOUT ||
                 errorMpd == MPD_ERROR_RESOLVER ||
                 errorMpd == MPD_ERROR_MALFORMED ||
                 errorMpd == MPD_ERROR_CLOSED ) /* Assume there is a connection problem, try to reconnect... */
        {
            QString errorMessage = QString::fromUtf8(mpd_connection_get_error_message(conn));
            qCritical() << "Connexion error : " << errorMessage;
            qCritical() << "Reconnecting...";
            connectToMpd();
            mutex.lock();
            queue.push_front(cmd);
            mutex.unlock();
            cmdAvailable.release(1);
        }
    }

    /* Post-action depending on the command
     * Do the minimum here as the whole status will be
     * retrieve here. But for playlist ADD/DEL, we need to
     * store the EMSTrack structure.
     */
    if(!error)
    {
        switch (cmd.action)
        {
            case ACTION_ADD:
            {
                EMSPlaylist newPlaylist;
                mutex.lock();
                playlist.tracks.append(cmd.track);
                newPlaylist = playlist;
                mutex.unlock();
                emit playlistChanged(newPlaylist);
                break;
            }
            case ACTION_DEL:
            {
                int pos = searchTrackInPlaylist(cmd.track);
                if (pos >= 0)
                {
                    EMSPlaylist newPlaylist;
                    mutex.lock();
                    playlist.tracks.removeAt(pos);
                    newPlaylist = playlist;
                    mutex.unlock();
                    emit playlistChanged(newPlaylist);
                }
                break;
            }
            case ACTION_DEL_ALL:
            {
                EMSPlaylist newPlaylist;
                mutex.lock();
                playlist.tracks.clear();
                newPlaylist = playlist;
                mutex.unlock();
                emit playlistChanged(newPlaylist);
                break;
            }
            case ACTION_ENABLE_OUTPUT:
            {
                mutex.lock();
                for(unsigned int i=0; i<outputs.size(); i++)
                {
                    if (outputs.at(i).id_mpd == cmd.uintValue)
                    {
                        EMSSndCard card = outputs.at(i);
                        card.enabled = true;
                        outputs.replace(i, card);
                    }
                }
                emit outputsChanged(outputs);
                mutex.unlock();
                break;
            }
            case ACTION_DISABLE_OUTPUT:
            {
                mutex.lock();
                for(unsigned int i=0; i<outputs.size(); i++)
                {
                    if (outputs.at(i).id_mpd == cmd.uintValue)
                    {
                        EMSSndCard card = outputs.at(i);
                        card.enabled = false;
                        outputs.replace(i, card);
                    }
                }
                emit outputsChanged(outputs);
                mutex.unlock();
                break;
            }
            default:
                break;
        }
    }
}

/* This function update the internal status tracking
 * the MPD status. At then end, if the status has changed, a signal is emmited.
 */
void Player::updateStatus()
{
    struct mpd_status *statusMpd;
    EMSPlayerStatus statusEMS;

    /* Get the status structure from MPD */
    mpd_send_status(conn);
    statusMpd = mpd_recv_status(conn);
    if (statusMpd == NULL)
    {
        qCritical() << "Error while trying to get MPD status";
        qCritical() << "Reconnecting...";
        connectToMpd(); /* Reconnect */
        return;
    }

    if (mpd_status_get_error(statusMpd) != NULL)
    {
        qCritical() << "MPD error: " << QString::fromUtf8(mpd_status_get_error(statusMpd));
    }

    if (!mpd_response_finish(conn))
    {
        qCritical() << "Error while trying to get MPD status" ;
        qCritical() << "Reconnecting...";
        connectToMpd();
        return;
    }

    /* Fill the internal state from the answer */
    switch (mpd_status_get_state(statusMpd))
    {
        case MPD_STATE_PAUSE:
            statusEMS.state = STATUS_PAUSE;
            break;
        case MPD_STATE_PLAY:
            statusEMS.state = STATUS_PLAY;
            break;
        case MPD_STATE_STOP:
            statusEMS.state = STATUS_STOP;
            break;
        default:
            statusEMS.state = STATUS_UNKNOWN;
            break;
    }

    statusEMS.repeat = mpd_status_get_repeat(statusMpd);
    statusEMS.random = mpd_status_get_random(statusMpd);

    if (statusEMS.state == STATUS_PLAY || statusEMS.state == STATUS_PAUSE)
    {
        if (mpd_status_get_queue_length(statusMpd) != (unsigned int)playlist.tracks.size())
        {
            qCritical() << "Error: the playlist does not have the same size as the one used by MPD!";
            /* We should not do this... */
            removeAllTracks();
        }

        statusEMS.posInPlaylist = mpd_status_get_song_pos(statusMpd);
        statusEMS.progress = mpd_status_get_elapsed_time(statusMpd);
    }

    mpd_status_free(statusMpd);

    if (statusEMS.posInPlaylist != status.posInPlaylist ||
        statusEMS.progress != status.progress ||
        statusEMS.random != status.random ||
        statusEMS.repeat != status.repeat ||
        statusEMS.state != status.state )
    {
        mutex.lock();
        status = statusEMS;
        mutex.unlock();
        emit statusChanged(statusEMS);
    }
}

/* This function get the sound card list from the MPD server
 * This list never changed but the filed "enable" could change
 */
void Player::retrieveSndCardList()
{
    mpd_malloc struct mpd_output *output;

    /* Request all outputs */
    mpd_send_outputs(conn);

    QVector<EMSSndCard> sndCards;

    while ((output = mpd_recv_output(conn)) != NULL)
    {
        EMSSndCard sndCard;
        sndCard.id_mpd = mpd_output_get_id(output)+1;
        sndCard.name = mpd_output_get_name(output);
        sndCard.enabled = mpd_output_get_enabled(output);
        mpd_output_free(output);
        qDebug() << "Find mpd output" << QString("(%1) : %2").arg(sndCard.id_mpd).arg(sndCard.name);
        sndCards.append(sndCard);
    }

    mutex.lock();
    outputs = sndCards;
    mutex.unlock();

    emit outputsChanged(sndCards);

    if (!mpd_response_finish(conn))
    {
        qCritical() << "Error while trying to get MPD outputs" ;
        qCritical() << "Reconnecting...";
        connectToMpd();
        return;
    }

}

/* ---------------------------------------------------------
 *                  THREAD MAIN FUNCTION
 * --------------------------------------------------------- */
/* Main thread function
 * This function is called once and must handle every blocking action.
 * All shared class members (= the one used by the API) MUST be protected by the global mutex.
 */
void Player::run()
{
    QSettings settings;
    unsigned int statusPeriod = settings.value("player/status_period", EMS_MPD_STATUS_PERIOD).toUInt();

    /* Blocking connection. */
    connectToMpd();

    /* Set initial state of the player */
    configureInitial();

    /* Get the list of sound cards, this list does not change as the correspond
     * to the list defined in the mpd configuration file */
    retrieveSndCardList();

    /* Execute each command, or timeout each X ms
     * In all cases, the status is retrieved at the end.
     */
    while (true)
    {
        bool cmdInQueue = cmdAvailable.tryAcquire(1, statusPeriod);

        /* Quit this thread if asked */
        if (this->isInterruptionRequested())
        {
            break;
        }

        if (cmdInQueue)
        {
            EMSPlayerCmd cmd;
            mutex.lock();
            if(queue.size() == 0)
            {
                qCritical() << "Error, no command in the queue, but the semaphore was not empty.";
                mutex.unlock();
                continue;
            }
            cmd = queue.dequeue();
            mutex.unlock();

            executeCmd(cmd);
        }

        updateStatus();
    }

    /* Clean exit */
    disconnectToMpd();
}

/* Kill this thread */
void Player::kill()
{
    this->requestInterruption();

    /* Fake a cmd, to leave properly */
    mutex.lock();
    queue.clear();
    mutex.unlock();
    cmdAvailable.release(1);
}

/* ---------------------------------------------------------
 *                 CONSTRUCTOR/DESTRUCTOR
 * --------------------------------------------------------- */

Player::Player(QObject *parent) : QThread(parent)
{
    mutex.lock();
    conn = NULL;
    status.state = EMSPlayerState::STATUS_UNKNOWN;
    status.random = false;
    status.repeat = false;
    playlist.name = QString("current");
    playlist.id = 0;
    playlist.subdir = QString("");
    playlist.tracks.clear();
    mutex.unlock();

    qRegisterMetaType<EMSPlayerStatus>("EMSPlayerStatus");
    qRegisterMetaType<EMSPlaylist>("EMSPlaylist");
}

Player::~Player()
{

}
