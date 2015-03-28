Protocol
========

EMS Protocol is based on JSON.

An id is used to determine which kind of actions is send or receive. This is the
list of available message:

 * `EMS_BROWSE`
 * `EMS_PLAYER`
 * `EMS_DISK`
 * `EMS_MEDIA_INFO`

There is 2 kinds of requests :

 * Asynchronous requests
 * Synchronous requests

###Asynchronous requests

These request are used to indicates a change in EMS or in the UIs.

###Synchronous requests

These requests are used to ask a question and wait for a response. Synchronous
requests contain a msg_id which is used in the question, and are given in return
to the answer. msg_id must be increased by 1 each time a new question is sent.
The receiver sends the answer back with the same msg_id.
It's only useful in case  where multiple message are send the caller before
getting a answer back.

Asynchronous requests are always sent by EMS, and received by clients.
Synchronous requests are always sent by client to EMS. If the request is not
formated correctly, or incorrect, EMS send a message error.

After a number of (3 TBD?) successives errors, the uuid of this client is
removed from the list of known clients, and pairing with the server has to be
done again.

Synchronous request contains the following parameters :

  * `msg`: it's a unique string identifing the message sent.
  * `msg_id`: it's a unique integer value per request. It's use to inditfy the
 pair request/answer if the client send multiple request at the same time.
  * `uuid`: The UUID (as defined in RFC4122)
  * Specifics parameters depending of the message.

Synchronous answer add the following paremeter :
  * `data`: the json object containing data attached to the message.

Asynchronous message contains the following parameters :

  * `msg`: it's a unique string identifiing the message sent.
  * `data`: the Json object containing data attached to the message.

Browser
=======

This part of the protocol is used to browse EMS, and get back informations about the top level menus, and the content of the database.
menus: Array of type `menu`

`menu` Object:

 * `name`: The name of the menu ;
 * `url_scheme`: The url scheme used to browse this menu entry ;
 * `icon`: Url of the menu's icon ;
 * `enabled`: Set to `true` if the menu is active, `false` otherwise ;

> Request :

```json
{
    "msg": "EMS_BROWSE",
    "msg_id": "id",
    "uuid": "110e8400-e29b-11d4-a716-446655440000",
    "url": "menu://"
}
```

> Answer :

```json
{
    "msg": "EMS_BROWSE",
    "msg_id": "id",
    "uuid": "110e8400-e29b-11d4-a716-446655440000",
    "data": {
        "menus": [
            {
                "name": "Library",
                "url_scheme": "library://",
                "icon": "http://ip/imgs/library.png",
                "enabled": true
            },
            {
                "name": "Audio CD",
                "url_scheme": "cdda://",
                "icon": "http://ip/imgs/cdda.png",
                "enabled": false
            },
            {
                "name": "Playlists",
                "url_scheme": "playlist://",
                "icon": "http://ip/imgs/playlists.png",
                "enabled": true
            },
            {
                "name": "Settings",
                "url_scheme": "settings://",
                "icon": "http://ip/imgs/settings.png",
                "enabled": true
            }
        ]
    }
}
```

###Browsing the library

Music Library is divided into sections, which can be browsed indepedently.
Sections are retrieved by sending an `EMS_BROWSE` request on the url:
`library://music`

Available sections are the following :

 * `artists` : Browse the library by artists
 * `albums` : Browse the library by albums
 * `tracks` : Browse the library by tracks
 * `genres` : Browse the library by genres
 * `compositors` : Browse the library by genres. This section is not implemented.

url contains music string, as it could be extended later to retrieve more than music,
like video clip, or other kind of medias.

>Request :

```json
{
    "msg": "EMS_BROWSE",
    "msg_id": "id",
    "uuid": "110e8400-e29b-11d4-a716-446655440000",
    "url": "library://music"
}
```

>Answer :

```json
{
    "msg": "EMS_BROWSE",
    "msg_id": "id",
    "uuid": "110e8400-e29b-11d4-a716-446655440000",
    "data" : {
        "menus": [
            {
                "name": "Artists",
                "url": "library://music/artists"
            },
            {
                "name": "Albums",
                "url": "library://music/albums"
            },
            {
                "name": "Tracks",
                "url": "library://music/tracks"
            },
            {
                "name": "Genre",
                "url": "library://music/genres"
            },
            {
                "name": "Compositors",
                "url": "library://music/compositor"
            }
        ]
    }
}
```
###Browsing Artists

This request is used to get all artists of the music library.

>Request :

```json
{
    "msg": "EMS_BROWSE",
    "msg_id": "id",
    "url": "library://music/artists"
}
```

>Answer :

```json
{
    "msg": "EMS_BROWSE",
    "msg_id": "id",
    "artists": [
        {
            "name": "David Bowie",
            "cover": "http://ip/imgs/bowier_uuid.png"
        },
        ...
    ]
}
```

Player
======

The JSON api can control the players detected on the network.
The player state for example can be changed by sending commands. T
The player send messages by itself when his state changed.

### State of the player

This message is send by EMS when the Player state changed. For example just
after changing the state from pause to play, or when a new song is played.

 * `player_state`: represent the state of the player, it can take the following
 values : `playing`, `stopped`, `paused` or `unknown`
 * `position` : position in the current playlist
 * `progress` : progression of the song (in second)
 * `repeat` : value of the repeat setting
 * `random` : value of the random setting
 * `track` : useful data about the played track

Note that if the player state is not `playing` or `paused`, only the state is
sent.

```json
{
    "msg": "EMS_PLAYER",
    "player_state": "playing",
    "position": 0,
    "progress": 23,
    "repeat": 1,
    "random": 0,
    "track" : {
        "name": "Life on Mars",
        ... (see track browsing)
    }
}
```

This message is send periodically (each second) to update the current position
in the stream.

 * `position`: the current position in the audio stream

```json
{
    "msg": "EMS_PLAYER",
    "position": 80
}
```

To control the Player from the UIs, ui can send message containing the `action`
parameter.

`action`: can take one of this values :

 * `next` : Next song
 * `prev` : Previous song
 * `play` : Play the current song
 * `pause` : Pause the current song
 * `toggle` : Toggle the state of the player (play/pause)
 * `shuffle_on` : Set the shuffle state on
 * `shuffle_off` : Set the shuffle state off
 * `repeat_on` : Set the repeat state on
 * `repeat_off` : Set the repeat state off
 * `seek`: Set the position of the track in seconds

```json
{
    "id": "EMS_PLAYER",
    "action": "next"
}

```

### Playlist Management

EMS Player used a special playlist known as current playlist, which contains
the list of song currently playing.
It's possible to get the current playlist by sending the message:
The url `playlist://current` can be used for this purpose.

> Request :

```json
{
    "id": "EMS_BROWSE",
    "msg_id": "id",
    "url": "playlist://current"
}
```

The answer is an array of tracks :

> Answer :

```json
{
    "id": "EMS_BROWSE",
    "msg_id": "id",

    "tracks": [
        {
            "album": "21",
            "artist": "Adele",
            "track": "1st Song",
            "file": "sha1",
            "length": 215,
            "cover": "http://ip/imgs/21.png"
        },
        {
            "album": "21",
            "artist": "Adele",
            "track": "2nd Song",
            "file": "sha1",
            "length": 342,
            "cover": "http://ip/imgs/21.png"
        }
        ...
    ]
}
```

To control playlist content, UI can use EMS_PLAYLIST keyword. There are two
kinds of playlist : the current one (player) and the saved ones (database).
The field `url` shoulbe be etiher :
 * `playlist://current`
 * `playlist://[PATH]/[id]`

The field `filename` can be :
 * `cdda://` : add a CDROM in the playlist
 * `cdda://[id]` : add the track [id] of the CDROM in the playlist
 * `library://music/tracks/[id]` : add the track [id] from the DB
 * `library://music/albums/[id]` : add the album [id] from the DB
 * `file://[absolute path]` : add a track using its path. If the path is a
directory, add all the files in the playlist.

The field `action` can be :
 * `add` : add the track in the playlist
 * `del` : remove the track in the playlist
 * `clear` : delete all content in the playlist

Exemple :

> Request :

```json
{
    "id": "EMS_PLAYLIST",
    "msg_id": "id",
    "url": "playlist://current",
    "action": "add",
    "filename": "file:///media/usb0/music/track01.flac"
}
```

Disk management
===============

This messages are sent by EMS when a CD is inserted in the tray. This message is
also send by EMS when a USB Disk containing a filesystem is connected.
`disk_type` can take one of the following values :

 * `cd_audio`
 * `cd_blank`
 * `cd_rom`
 * `dvd_rom`
 * `dvd_blank`
 * `usb_disk`

an `action` is associated to this message. It can be one of the following :

 * `inserted` : a new disk has been inserted
 * `ejected` : disk has been ejected
 * `error` : an error occured. in that case, the `error_msg` parameter contains a string
 describing the error encountered.

`label` contains the label of the disk. It makes sense when an usb disk is inserted.
In case of cd/dvd this value may be blank.

```json
{
    "id": "EMS_DISK",
    "action": "inserted",
    "error_msg": "ERROR MESSAGE",
    "disk_type": "cd_audio",
    "label": "DISK LABEL"
}
```

To get the current Disk's state the EMS_DISK can be sent by UIs to EMS.
The returned value is an array of disk present on the system
This array can be void.


> Request :

```json
{
    "id": "EMS_DISK",
    "msg_id": "id",
}
```

> Answer :

```json
{
    "id": "EMS_DISK",
    "msg_id": "id",
    "disks" : [
        {
            "disk_type": "cd_audio",
            "label": ""
        },
        {
            "disk_type": "usb_disk",
            "label": "DISK LABEL"
        }
    ]
}
```

If an disk (cd or usb) is inserted, it can be ejected by sending the following
message from UIs :

```json
{
    "id": "EMS_DISK",
    "action": "eject"
}
```

Medias informations
===================

Each file in the database can be retrieved by it's sha1. To get information about
a media, a EMS_MEDIA_INFOS request can be performed. This request contains an array
of metadatas that you want to retrieved. Each items of this array contains a value to retrieved.

`key` can take the follwing values :
TBD

```json
{
    "id": "EMS_MEDIA_INFOS",
    "msg_id": "id",
    "file" : "sha1",
    "metadatas": [
        {
                "key": "value"
        },
        ...
    ]
}
```

CD RIP
======

rip command is send to EMS when a cd have to be ripped on the disk.
TBD
