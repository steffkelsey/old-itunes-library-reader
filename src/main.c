#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>

#include "common.h"
#include "file.h"
#include "parse.h"

void print_usage(char *argv[]) {
  printf("Usage: %s -f <database file>\n", argv[0]);
  printf("\t -f - [required] path to database file\n");
  printf("\t -l - list the tracks\n");
  printf("\t -t - show information of named track\n");
  printf("\t -p - list the playlists\n");
  printf("\t -s - show information of named playlist\n");
  return;
}

int main(int argc, char *argv[]) { 
  char *trackstring = NULL;
  char *playliststring = NULL;
  char *filepath = NULL;
  bool listtracks = false;
  bool listplaylists = false;
  int c;

  int dbfd = -1;
  struct dbheader_t *dbhdr = NULL;
  struct track_t *tracks = NULL;
  struct playlist_t *playlists = NULL;

  while ((c = getopt(argc, argv, "lpf:t:s:")) != -1) {
    switch (c) {
      case 'f':
        filepath = optarg;
        break;
      case 'l':
        listtracks = true;
        break;
      case 'p':
        listplaylists = true;
        break;
      case 't':
        trackstring = optarg;
        break;
      case 's':
        playliststring = optarg;
        break;
      case '?':
        printf("Unknown option -%c\n", c);
        break;
      default:
        return STATUS_ERROR;
    } 
  }

  if (filepath == NULL) {
    printf("Filepath is a required argument\n");
    print_usage(argv);

    return 0;
  }

  dbfd = open_db_file(filepath);
  if (dbfd == STATUS_ERROR) {
    printf("Unable to open database file\n");
    return -1;
  }

  if (validate_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
   printf("Failed to validate database header\n");
   return -1;
  }
   

  if (trackstring) {
    if (show_track(dbhdr, tracks, trackstring) == STATUS_ERROR) {
      printf("Failed to show track\n");
      return -1;
    }
  }

  if (playliststring) {
    if (show_playlist(dbhdr, playlists, playliststring) == STATUS_ERROR) {
      printf("Failed to show playlist\n");
      return -1;
    }
  }

  if (listtracks) {
    list_tracks(dbhdr, tracks);
  }

  if (listplaylists) {
    list_playlists(dbhdr, playlists);
  }

  free(dbhdr);
  dbhdr = NULL;

  free(tracks);
  tracks = NULL;
	
  free(playlists);
  playlists = NULL;

  return 0;
}
