#include "Player.h"
#include "DefaultSettings.h"
#include <QDebug>
#include <QFile>
#include <QSettings>
#include <mpd/client.h>

Player* Player::_instance = 0;

Player::Player(QObject *parent) : QObject(parent)
{
    status = EMSPlayerState::UNKNOWN;
    playlist.name = QString("current");
}

Player::~Player()
{

}
