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
  printf("\t -i - show information of named playlist\n");
  printf("\t -e - export named playlist as m3u\n");
  printf("\t -o - [required for -e] m3u output file path\n");
  printf("\t -s - substitution string in file location during export. Format: #find#replace#\n");
  return;
}

int main(int argc, char *argv[]) { 
  char *trackstring = NULL;
  char *playliststring = NULL;
  char *exportstring = NULL;
  char *findreplacestring = NULL;
  char *outputpath = NULL;
  char *filepath = NULL;
  bool listtracks = false;
  bool listplaylists = false;
  int c;

  int dbfd = -1;
  int ofd = -1;
  struct db_t *db = NULL;
  struct track_t *tracks = NULL;
  struct playlist_t *playlists = NULL;

  while ((c = getopt(argc, argv, "lpf:t:i:e:o:s:")) != -1) {
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
      case 'i':
        playliststring = optarg;
        break;
      case 'e':
        exportstring = optarg;
        break;
      case 'o':
        outputpath = optarg;
        break;
      case 's':
        findreplacestring = optarg;
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

  if (outputpath) {
    ofd = open_m3u_file(outputpath);
    if (ofd == STATUS_ERROR) {
      printf("Unable to create/open m3u file\n");
      return -1;
    }
  }

  if (validate_db_header(dbfd, &db) == STATUS_ERROR) {
   printf("Failed to validate database header\n");
   return -1;
  }
   
  printf("iTunes Version: %s\n", db->header->version);
  
  // Parse the library (many blocks)
  if (parse_library(db, &tracks, &playlists) == STATUS_ERROR) {
    printf("Failed to parse library\n");
    return -1;
  }

  if (exportstring) {
    if (ofd < 0) {
      printf("-e (Export) requires -o (Output file path)\n");
      return -1;
    }
    if (export_playlist(db, playlists, exportstring, tracks, ofd, findreplacestring) == STATUS_ERROR) {
      printf("Failed to export playlist\n");
      return -1;
    }
  }

  if (listtracks) {
    list_tracks(db, tracks);
  }

  if (listplaylists) {
    list_playlists(db, playlists);
  }

  if (trackstring) {
    if (show_track(db, tracks, trackstring) == STATUS_ERROR) {
      printf("Failed to show track\n");
      return -1;
    }
  }

  if (playliststring) {
    if (show_playlist(db, playlists, playliststring, tracks) == STATUS_ERROR) {
      printf("Failed to show playlist\n");
      return -1;
    }
  }

  free(tracks);
  tracks = NULL;
	
  for (int i = 0; i < db->header->playlistcount; i++) {
    free(playlists[i].track_ids);
    playlists[i].track_ids = NULL;
  }
  free(playlists);
  playlists = NULL;

  free(db->header->version);
  db->header->version = NULL;

  free(db->header);
  db->header = NULL;

  free(db->data);
  db->data = NULL;

  free(db);
  db = NULL;

  return 0;
}
