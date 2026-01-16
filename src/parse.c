#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "parse.h"

void list_tracks(struct dbheader_t *dbhdr, struct track_t *tracks) {
  if (NULL == dbhdr) return;

  printf("list_tracks called!\n");
}

void list_playlists(struct dbheader_t *dbhdr, struct playlist_t *playlists) {
  if (NULL == dbhdr) return;

  printf("list_playlists called!\n");
}

int show_track(struct dbheader_t *dbhdr, struct track_t *tracks, char *trackstring) {
  if (NULL == dbhdr) return STATUS_ERROR;

  printf("show_track called!\n");

  return STATUS_SUCCESS;
}

int show_playlist(struct dbheader_t *dbhdr, struct playlist_t *playlists, char *playliststring) {
  if (NULL == dbhdr) return STATUS_ERROR;

  printf("show_playlist called!\n");

  return STATUS_SUCCESS;
}

int read_tracks(int fd, struct dbheader_t *dbhdr, struct track_t **tracksOut) {
  if (fd < 0) {
    printf("Got a bad FD from the user\n");
    return STATUS_ERROR;
  }
  
  printf("read_tracks called!\n");

  return STATUS_SUCCESS;
}

int read_playlists(int fd, struct dbheader_t *dbhdr, struct playlist_t **playlistsOut) {
  if (fd < 0) {
    printf("Got a bad FD from the user\n");
    return STATUS_ERROR;
  }
  
  printf("read_playlists called!\n");

  return STATUS_SUCCESS;
}

int validate_db_header(int fd, struct dbheader_t **headerOut) {
  if (fd < 0) {
    printf("Got a bad FD from the user\n");
    return STATUS_ERROR;
  }
  
  struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
  if (header == NULL) {
    printf("Malloc failed to create db header\n");
    return STATUS_ERROR;
  }

  if (read(fd, header, sizeof(int)) != sizeof(int)) {
    perror("read");
    free(header);
    return STATUS_ERROR;
  }
  
  if (strcmp(header->magic, HEADER_MAGIC) != 0) {
    printf("Improper header magic\n");
    free(header);
    return STATUS_ERROR;
  }

  //if (header->version != 1) {
  //  printf("Improper header version\n");
  //  free(header);
  //  return STATUS_ERROR;
  //}

  //struct stat dbstat = {0};
  //fstat(fd, &dbstat);
  //printf("header->filesize: %d, st_size: %d\n", header->filesize, dbstat.st_size);
  //if (header->filesize != dbstat.st_size) {
  //  printf("Corrupted database\n");
  //  free(header);
  //  return STATUS_ERROR;
  //}

  *headerOut = header;
}
