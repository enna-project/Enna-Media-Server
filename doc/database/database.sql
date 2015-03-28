BEGIN TRANSACTION;
-- Main table storing the tracks
-- 1 track == 1 music file
-- If foreign key "album_id" is null => Unknown album
CREATE TABLE "tracks" (
	`id`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
	`album_id`	INTEGER NOT NULL DEFAULT 0,
	`position`	INTEGER,
	`name`	TEXT NOT NULL,
	`sha1`	TEXT NOT NULL UNIQUE,
	`format`	INTEGER NOT NULL,
	`sample_rate`	INTEGER,
	`duration`	INTEGER NOT NULL,
	`format_parameters`	TEXT,
	FOREIGN KEY(album_id) REFERENCES albums(id)
);

CREATE INDEX tracks_sha1_index ON tracks(sha1);

-- This table is only used to stored paths of a music file in case where
-- there are several time the same file
CREATE TABLE "files" (
	`filename`	TEXT NOT NULL PRIMARY KEY,
	`track_id`	INTEGER NOT NULL,
	FOREIGN KEY(track_id) REFERENCES tracks(id) ON DELETE CASCADE
);

CREATE TABLE "albums" (
	`id`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
	`name`	TEXT NOT NULL,
	`cover`	TEXT
);

CREATE INDEX albums_name_index ON albums(name);

-- Default album => ID = 0
-- Used for all tracks which are not linked to an album
INSERT INTO "albums" VALUES (0, 'Unknown Album', '');

CREATE TABLE "artists" (
	`id`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
	`name`	TEXT NOT NULL UNIQUE,
	`picture`	TEXT
);

CREATE INDEX artists_name_index ON artists(name);

CREATE TABLE "tracks_artists" (
	`track_id`	INTEGER NOT NULL,
	`artist_id`	INTEGER NOT NULL,
	PRIMARY KEY (track_id, artist_id),
	FOREIGN KEY(track_id) REFERENCES tracks(id) ON DELETE CASCADE,
	FOREIGN KEY(artist_id) REFERENCES artists(id) ON DELETE CASCADE
);

CREATE TABLE "genres" (
	`id`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
	`name`	TEXT NOT NULL UNIQUE,
	`picture`	TEXT
);

CREATE INDEX genres_name_index ON genres(name);

CREATE TABLE "tracks_genres" (
	`track_id`	INTEGER NOT NULL,
	`genre_id`	INTEGER NOT NULL,
	PRIMARY KEY (track_id, genre_id),
	FOREIGN KEY(track_id) REFERENCES tracks(id) ON DELETE CASCADE,
	FOREIGN KEY(genre_id) REFERENCES genres(id) ON DELETE CASCADE
);

-- Playlist management
CREATE TABLE "playlists" (
	`id`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
	`name`	TEXT NOT NULL,
	`subdir`	TEXT
);

CREATE TABLE "playlists_tracks" (
	`playlist_id`	INTEGER NOT NULL,
	`track_id`	INTEGER NOT NULL,
	PRIMARY KEY (playlist_id, track_id),
	FOREIGN KEY(playlist_id) REFERENCES playlists(id) ON DELETE CASCADE,
	FOREIGN KEY(track_id) REFERENCES tracks(id) ON DELETE CASCADE
);

-- Authorized clients
CREATE TABLE "authorized_clients" (
	`uuid`	TEXT NOT NULL PRIMARY KEY,
	`hostname`	TEXT,
	`username`	TEXT
);

-- Admin. config
CREATE TABLE "configuration" (
	`config_name`	TEXT NOT NULL PRIMARY KEY,
	`config_value`	TEXT
);


COMMIT;
