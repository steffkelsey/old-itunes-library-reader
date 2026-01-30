#ifndef PARSE_H
#define PARSE_H

#define HEADER_MAGIC "hdfm"
#define CHUNK 16384 // Buffer size for zlib chunks

struct dbheader_t {
	unsigned char magic[4];
	unsigned int headerlength;
	unsigned int filelength;
	unsigned int unknown;
	unsigned char versionlength;
	unsigned char *version;
	unsigned int trackcount;
	unsigned int playlistcount;
};

struct db_t {
  struct dbheader_t *header;
  unsigned char *data;
};

struct track_t {
  int id;
	char name[256];
	char artist[256];
	char album[256];
  char file_location[256];
  int file_location_length;
};

struct playlist_t {
  int id;
	char name[256];
  int *track_ids;
  unsigned int trackcount;
};

int validate_db_header(int fd, struct db_t **dbOut);
int inflate_data(unsigned char *input, int inputlength, unsigned char **output);
int parse_library(struct db_t *db, struct track_t **tracksOut, struct playlist_t **playlistsOut);
int parseGenericHohm (unsigned char **data_ptr, unsigned char **dataOut, int *outlength);
void list_playlists(struct db_t *db, struct playlist_t *playlists);
void list_tracks(struct db_t *db, struct track_t *tracks);
int show_track(struct db_t *db, struct track_t *tracks, char *trackstring);
int show_playlist(struct db_t *db, struct playlist_t *playlists, char *playliststring, struct track_t *tracks);
int export_playlist(struct db_t *db, struct playlist_t *playlists, char *playliststring, struct track_t *tracks, int fd, char *findreplacestring);

#endif
