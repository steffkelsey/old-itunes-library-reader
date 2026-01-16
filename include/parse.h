#ifndef PARSE_H
#define PARSE_H

#define HEADER_MAGIC "hdfm"

struct dbheader_t {
	unsigned char magic[4];
	unsigned short version;
	unsigned int filelength;
};

struct track_t {
	char name[256];
};

struct playlist_t {
	char name[256];
};

int validate_db_header(int fd, struct dbheader_t **headerOut);
int read_playlists(int fd, struct dbheader_t *, struct playlist_t **playlistsOut);
int read_tracks(int fd, struct dbheader_t *, struct track_t **tracksOut);
void list_playlists(struct dbheader_t *dbhdr, struct playlist_t *playlists);
void list_tracks(struct dbheader_t *dbhdr, struct track_t *tracks);
int show_track(struct dbheader_t *dbhdr, struct track_t *tracks, char *trackstring);
int show_playlist(struct dbheader_t *dbhdr, struct playlist_t *playlists, char *playliststring);

#endif
