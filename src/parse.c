#include "parse.h"

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <zlib.h>

#include "common.h"
#include "crypto.h"

void list_tracks(struct db_t *db, struct track_t *tracks) {
  if (NULL == db) return;

  printf("list_tracks called!\n");
}

void list_playlists(struct db_t *db, struct playlist_t *playlists) {
  if (NULL == db) return;

  printf("list_playlists called!\n");
}

int show_track(struct db_t *db, struct track_t *tracks, char *trackstring) {
  if (NULL == db) return STATUS_ERROR;

  printf("show_track called!\n");

  return STATUS_SUCCESS;
}

int show_playlist(struct db_t *db, struct playlist_t *playlists, char *playliststring) {
  if (NULL == db) return STATUS_ERROR;

  printf("show_playlist called!\n");

  return STATUS_SUCCESS;
}

int read_tracks(struct db_t *db, struct track_t **tracksOut) {
  if (NULL == db) {
    printf("Got a bad db from the user\n");
    return STATUS_ERROR;
  }
  
  printf("read_tracks called!\n");

  return STATUS_SUCCESS;
}

int read_playlists(struct db_t *db, struct playlist_t **playlistsOut) {
  if (NULL == db) {
    printf("Got a bad db from the user\n");
    return STATUS_ERROR;
  }
  
  printf("read_playlists called!\n");

  return STATUS_SUCCESS;
}

int validate_db_header(int fd, struct db_t **dbOut) {
  if (fd < 0) {
    printf("Got a bad FD from the user\n");
    return STATUS_ERROR;
  }
  
  struct db_t *db = calloc(1, sizeof(struct db_t));
  if (db == NULL) {
    printf("Malloc failed to create db\n");
    return STATUS_ERROR;
  }

  db->header = calloc(1, sizeof(struct dbheader_t));
  if (db->header == NULL) {
    printf("Malloc failed to create db header\n");
    return STATUS_ERROR;
  }

  // The bytestoread is everything in dbheader_t except for the pointer to the version string
  int bytestoread = sizeof(db->header->magic) + sizeof(db->header->headerlength) + sizeof(db->header->filelength) + sizeof(db->header->unknown) + sizeof(unsigned char);

  if (read(fd, db->header, bytestoread) != bytestoread) {
    perror("read");
    free(db);
    return STATUS_ERROR;
  }
  
  if (strcmp(db->header->magic, HEADER_MAGIC) != 0) {
    printf("Improper header magic\n");
    free(db);
    return STATUS_ERROR;
  }

  db->header->headerlength = ntohl(db->header->headerlength);
  db->header->filelength = ntohl(db->header->filelength);
  db->header->unknown = ntohl(db->header->unknown);

  // Make sure the file is the correct length
  struct stat dbstat = {0};
  fstat(fd, &dbstat);
  if (db->header->filelength != dbstat.st_size) {
    printf("Corrupted database\n");
    free(db);
    return STATUS_ERROR;
  }
  
  // Get the version string
  db->header->version = (unsigned char *)malloc((1 + (int)db->header->versionlength) * sizeof(unsigned char));
  if (db->header->version == NULL) {
    printf("Memory allocation failed!\n");
    return STATUS_ERROR;
  }
  db->header->version[db->header->versionlength] = '\0';
  if (read(fd, db->header->version, (int)db->header->versionlength * sizeof(unsigned char)) != (int)db->header->versionlength * sizeof(unsigned char)) {
    perror("read");
    free(db);
    return STATUS_ERROR;
  }

  // Seek to the end of the header
  // First, find the current position
  off_t pos = lseek(fd, 0, SEEK_CUR);
  pos = pos + db->header->headerlength - pos;
  lseek(fd, pos, SEEK_SET);
  bytestoread = db->header->filelength - pos;
  
  // make a buffer for the db (trimming out the header)
  db->data = (unsigned char *)malloc(bytestoread * sizeof(unsigned char));
  if (db->data == NULL) {
    printf("Memory allocation failed!\n");
    return STATUS_ERROR;
  }
  
  // read the rest of the file into the buffer
  if (read(fd, db->data, bytestoread * sizeof(unsigned char)) != bytestoread * sizeof(unsigned char)) {
    perror("read");
    free(db);
    return STATUS_ERROR;
  }

  // The decrypted length is less than the original
  int encryptedlength = bytestoread;
  encryptedlength -= encryptedlength % 16;

  // Decrypt the data
  // Create a buffer for the decrypted data
  unsigned char *decrypted = malloc(encryptedlength * sizeof(unsigned char));
  if (decrypted == NULL) {
    printf("Memory allocation failed!\n");
    return STATUS_ERROR;
  }

  unsigned char *key = (unsigned char *)"BHUILuilfghuila3";

  // decrypt to the new buffer
  int decrypted_length = aes_ecb_nopad_decrypt(db->data, encryptedlength, key, decrypted);
  if (decrypted_length == -1) {
    printf("Error decrypting data\n");
    return STATUS_ERROR;
  }
  
  // Copy the decrypted data over the original data, stopping short and 
  // preserving the end.
  memcpy(db->data, decrypted, decrypted_length * sizeof(unsigned char));

  free(decrypted);
  decrypted = NULL;

  // inflate the decrypted buffer
  unsigned char *inflated = NULL;
  int inflated_length = inflate_data(db->data, bytestoread, &inflated);
  if (inflated_length == -1) {
    printf("Error inflating data\n");
    return STATUS_ERROR;
  }

  //printf("inflated_length: %d\n", inflated_length);
  // Change the filelength in the header to account for the new inflated length
  db->header->filelength = db->header->headerlength + inflated_length;

  // Change the pointer so the data is using the inflated data
  free(db->data);
  db->data = NULL;
  db->data = inflated;
  
  *dbOut = db;
}

int inflate_data(unsigned char *input, int inputlength, unsigned char **output) {
  z_stream strm;

  // Allocate inflate state
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = (unsigned int)(inputlength * sizeof(unsigned char));
  strm.next_in = input;

  // The magic number 32 added to windowBits enables gzip decoding
  // windowBits parameter should be 15 + 32 for gzip format (or -15 for raw deflate)
  if (inflateInit2(&strm, 15 + 32) != Z_OK) {
    perror("inflateInit2");
    return STATUS_ERROR;
  }

  // Dynamically allocate output memory and decompress in chunks
  int decompressed_size = 0;
  size_t current_chunk_size = CHUNK;

  /* decompress until deflate stream ends */
  int ret = -1;
  do {
    // Reallocate memory for output buffer
    unsigned char *temp_buffer = realloc(*output, decompressed_size + current_chunk_size);
    if (temp_buffer == NULL) {
      inflateEnd(&strm);
      return Z_MEM_ERROR;
    }
    *output = temp_buffer;
    strm.avail_out = current_chunk_size;
    strm.next_out = *output + decompressed_size;

    // Decompress a chunk
    ret = inflate(&strm, Z_NO_FLUSH);
    switch (ret) {
      case Z_STREAM_ERROR:
      case Z_DATA_ERROR:
      case Z_MEM_ERROR:
        inflateEnd(&strm);
        // Free memory in case of error
        free(*output); 
        *output = NULL;
        return STATUS_ERROR;
      }

      decompressed_size += (current_chunk_size - strm.avail_out);

      // If we filled the current chunk and not done, double chunk size for next iteration
      if (strm.avail_out == 0 && ret != Z_STREAM_END) {
        current_chunk_size *= 2;
      }

  } while (ret != Z_STREAM_END);

  // Clean up
  inflateEnd(&strm);

  // Resize the memory to the exact final size
  unsigned char *final_buffer = realloc(*output, decompressed_size);
  if (final_buffer != NULL) {
    *output = final_buffer;
  }
  
  return decompressed_size/sizeof(unsigned char);
}

int parse_library(struct db_t *db, struct track_t **tracksOut, struct playlist_t **playlistsOut) {
  // Get each block header and print them out
  int data_length = db->header->filelength - db->header->headerlength;
  unsigned char *data_ptr = db->data;
  unsigned char *end_ptr = db->data + (data_length * sizeof(unsigned char));

  //printf("data length: %d\n", data_length);

  // Tracking total bytes counted in the loop
  int byte_cnt = 0;
  // For tracking if we're going through sub blocks of a block in the loop
  int sub_block_bytes_remaining = 0;

  int total_track_num = -1;
  int total_playlist_num = -1;
  int track_cnt = 0;
  int playlist_cnt = 0;
  struct track_t *tracks = NULL;
  struct playlist_t *playlists = NULL;
  int playlist_track_id_cnt = 0;
  while (data_ptr < end_ptr) {
    unsigned char block_type[5] = {0}; // Leave space for null char at the end
    int block_length = 0;
    int consumed = 0;
    //printf("%d ", byte_cnt);
    // Read the block type 
    memcpy(block_type, data_ptr, 4 * sizeof(unsigned char));
    data_ptr += 4;
    consumed += 4;
    byte_cnt += 4;
    // Read the block length int
    memcpy(&block_length, data_ptr, sizeof(unsigned int));
    data_ptr += 4;
    consumed += 4;
    byte_cnt += 4;
    // Convert to host long (endianess)
    block_length = ntohl(block_length);

    //printf("type: %s\n", block_type);
    //printf("length: %d\n", block_length);

    // some blocks have the total length defined in another byte
    if (strcmp(block_type, "htlm") == 0) {
      // TODO there might be blocks of these per hdsm block
      // In early tests, the first one to appear has been the good one
      if (total_track_num < 0) {
        // find the total number of tracks
        memcpy(&total_track_num, data_ptr, sizeof(unsigned int));
        data_ptr += 4;
        consumed += 4;
        byte_cnt += 4;
        total_track_num = ntohl(total_track_num);
        //printf("TOTAL NUMBER OF TRACKS: %d\n", total_track_num);
        // allocate the memory for this number of tracks
        tracks = calloc(total_track_num, sizeof(struct track_t));
        if (tracks == NULL) {
          printf("Malloc failed to create tracks\n");
          return STATUS_ERROR;
        }
      }
    } else if (strcmp(block_type, "htim") == 0) {
      int record_length = 0;
      memcpy(&record_length, data_ptr, sizeof(unsigned int));
      data_ptr += 4;
      consumed += 4;
      byte_cnt += 4;
      record_length = ntohl(record_length);
      int num_hohm_sub_blocks = 0;
      memcpy(&num_hohm_sub_blocks, data_ptr, sizeof(unsigned int));
      data_ptr += 4;
      consumed += 4;
      byte_cnt += 4;
      num_hohm_sub_blocks = ntohl(num_hohm_sub_blocks);
      int song_id = 0;
      memcpy(&song_id, data_ptr, sizeof(unsigned int));
      data_ptr += 4;
      consumed += 4;
      byte_cnt += 4;
      song_id = ntohl(song_id);
      //printf("song_id: %d\n", song_id);
      
      // create a new track
      track_cnt++;
      tracks[track_cnt-1].id = song_id;

      // Mark that we're going to go through sub blocks
      sub_block_bytes_remaining = record_length - block_length; 
    } else if (strcmp(block_type, "hplm") == 0) { 
      // TODO there might be one of these per hdsm block
      // In early tests, it seems like the first one to appear is the good one
      if (total_playlist_num < 0) {
        memcpy(&total_playlist_num, data_ptr, sizeof(unsigned int));
        data_ptr += 4;
        consumed += 4;
        byte_cnt += 4;
        total_playlist_num = ntohl(total_playlist_num);
        //printf("TOTAL NUMBER OF PLAYLISTS: %d\n", total_playlist_num);
        // Allocate the memory for this number of playlists
        if (total_playlist_num > 0) {
          playlists = calloc(total_playlist_num, sizeof(struct playlist_t));
          if (playlists == NULL) {
            printf("Malloc failed to create playlists\n");
            return STATUS_ERROR;
          }
        }
      }
    } else if (strcmp(block_type, "hpim") == 0) { 
      // For finding the number of items in a playlist
      // Expect to be followed by hptm block(s)
      // Skip 8 bytes
      data_ptr += 8;
      consumed += 8;
      byte_cnt += 8;
      // Get the number of items in the playlist
      int num_items = 0;
      memcpy(&num_items, data_ptr, sizeof(unsigned int));
      data_ptr += 4;
      consumed += 4;
      byte_cnt += 4;
      num_items = ntohl(num_items);
      //printf("Num items in playlist: %d\n", num_items);
      // Create a new playlist for adding info from hohm
      playlist_cnt++;
      playlists[playlist_cnt-1].id = playlist_cnt-1;
      // Reset the track_id cnt
      playlist_track_id_cnt = 0;
      // Allocate memory for the list of track_ids
      if (num_items > 0) {
        playlists[playlist_cnt-1].track_ids = calloc(num_items, sizeof(int));
        if (playlists[playlist_cnt-1].track_ids == NULL) {
          printf("Malloc failed to create track_ids for playlist\n");
          return STATUS_ERROR;
        }
      }
    } else if (strcmp(block_type, "hptm") == 0) { 
      // Get the song_id
      // Skip 16 bytes
      data_ptr += 16;
      consumed += 16;
      byte_cnt += 16;
      // Get the song_id to add to the current playlist
      int list_song_id = 0;
      memcpy(&list_song_id, data_ptr, sizeof(unsigned int));
      data_ptr += 4;
      consumed += 4;
      byte_cnt += 4;
      list_song_id = ntohl(list_song_id);
      //printf("Song_id to add to playlist: %d\n", list_song_id);
      // add it to the current playlist
      playlists[playlist_cnt-1].track_ids[playlist_track_id_cnt] = list_song_id;
      playlist_track_id_cnt++;
    } else if (strcmp(block_type, "hohm") == 0) {
      //if (sub_block_bytes_remaining > 0) {
      //  printf("sub_block_bytes_remaining: %d\n", sub_block_bytes_remaining);
      //}
      int record_length = 0;
      memcpy(&record_length, data_ptr, sizeof(unsigned int));
      data_ptr += 4;
      consumed += 4;
      byte_cnt += 4;
      record_length = ntohl(record_length);
      int hohm_type = 0;
      memcpy(&hohm_type, data_ptr, sizeof(unsigned int));
      data_ptr += 4;
      consumed += 4;
      byte_cnt += 4;
      hohm_type = ntohl(hohm_type);
      int ret = 0;
      switch (hohm_type) {
        case 0x02: // track title
          unsigned char *track_name = NULL;
          ret = parseGenericHohm(&data_ptr, &track_name);
          if (ret == STATUS_ERROR) {
            printf("Failed to parse hohm block type track name\n");
            return STATUS_ERROR;
          }
          consumed += ret;
          byte_cnt += ret;
          //printf("track name: %s\n", track_name);
          strncpy(tracks[track_cnt-1].name, track_name, sizeof(tracks[track_cnt-1].name));
          free(track_name);
          track_name = NULL;
          break;
        case 0x03: // album title
          unsigned char *album_name = NULL;
          ret = parseGenericHohm(&data_ptr, &album_name);
          if (ret == STATUS_ERROR) {
            printf("Failed to parse hohm block type album name\n");
            return STATUS_ERROR;
          }
          consumed += ret;
          byte_cnt += ret;
          //printf("album name: %s\n", album_name);
          strncpy(tracks[track_cnt-1].album, album_name, sizeof(tracks[track_cnt-1].album));
          free(album_name);
          album_name = NULL;
          break;
        case 0x04: // artist name
          unsigned char *artist_name = NULL;
          ret = parseGenericHohm(&data_ptr, &artist_name);
          if (ret == STATUS_ERROR) {
            printf("Failed to parse hohm block type artist name\n");
            return STATUS_ERROR;
          }
          consumed += ret;
          byte_cnt += ret;
          //printf("artist name: %s\n", artist_name);
          strncpy(tracks[track_cnt-1].artist, artist_name, sizeof(tracks[track_cnt-1].artist));
          free(artist_name);
          artist_name = NULL;
          break;
        case 0x0D: // file location
          unsigned char *file_location = NULL;
          ret = parseGenericHohm(&data_ptr, &file_location);
          if (ret == STATUS_ERROR) {
            printf("Failed to parse hohm block type file location\n");
            return STATUS_ERROR;
          }
          consumed += ret;
          byte_cnt += ret;
          //printf("file location: %s\n", file_location);
          strncpy(tracks[track_cnt-1].file_location, file_location, sizeof(tracks[track_cnt-1].file_location));
          free(file_location);
          file_location = NULL;
          break;
        case 0x64: // playlist name
          unsigned char *playlist_name = NULL;
          ret = parseGenericHohm(&data_ptr, &playlist_name);
          if (ret == STATUS_ERROR) {
            printf("Failed to parse hohm block type playlist name\n");
            return STATUS_ERROR;
          }
          consumed += ret;
          byte_cnt += ret;
          //printf("playlist name: %s\n", playlist_name);
          strncpy(playlists[playlist_cnt-1].name, playlist_name, sizeof(playlists[playlist_cnt-1].name));
          free(playlist_name);
          playlist_name = NULL;
          break;
        // Known ones to skip  
        case 0x12C: // Has data - General Title / Album Title / Podcast Title
        case 0x12D: // Has data - Another Artist
        case 0x190: // Has data - Artist/Author
        case 0x1F7:
        case 0x6b:
        case 0x65:
        case 0x66:
        case 0x67:
        case 0x69:
          //printf("skip!\n");
          break;
        default: 
          //unsigned char *data_str = NULL;
          //ret = parseGenericHohm(&data_ptr, &data_str);
          //if (ret == STATUS_ERROR) {
          //  printf("Failed to parse hohm block type generic\n");
          //  return STATUS_ERROR;
          //}
          //consumed += ret;
          //byte_cnt += ret;
          //printf("0x%02X - data str: %s\n", hohm_type, data_str);
          //free(data_str);
          //data_str = NULL;
          break;
      }
      block_length = record_length;
      //if (sub_block_bytes_remaining > 0) {
      //  printf("parsed %d sub block bytes...\n", block_length);
      //}
      sub_block_bytes_remaining -= block_length;
    }

    // Jump ahead to the next block 
    data_ptr += (block_length - consumed);
    byte_cnt += (block_length - consumed);
    if (block_length <= 0) {
      printf("Bad block length! Aborting...\n");
      return STATUS_ERROR;
    }
  }

  data_ptr = NULL;
  end_ptr = NULL;

  // Save the total number of tracks into the header
   db->header->trackcount = track_cnt;
  // Save the total number of playlists into the header
   db->header->playlistcount = playlist_cnt;
  // Set the pointer for tracksOut
  *tracksOut = tracks;
  // Set the pointer for playlistsOut
  *playlistsOut = playlists;

  //printf("Num tracks: %d, Track CNT: %d\n", total_track_num, track_cnt);
  //printf("Num playlists: %d, Playlist CNT: %d\n", total_playlist_num, playlist_cnt);
}

int parseGenericHohm (unsigned char **data_ptr, unsigned char **dataOut) {
  int byte_cnt = 0;
  // Generic hohm parsing
  // Byte Length Comment
  // 0    12     ???
  // 12   4      N = length of data string
  // 16   8      ?
  // 24   N      data
  *data_ptr += 12;
  byte_cnt += 12;
  int data_length = 0;
  memcpy(&data_length, *data_ptr, sizeof(unsigned int));
  *data_ptr += 4;
  byte_cnt += 4;
  data_length = ntohl(data_length);
  *data_ptr += 8;
  byte_cnt += 8;
  unsigned char *data = malloc((1 + data_length) * sizeof(unsigned char));
  if (data == NULL) {
    printf("Unable to allocate memory\n");
    return STATUS_ERROR;
  }
  data[data_length] = '\0';
  memcpy(data, *data_ptr, data_length * sizeof(unsigned char));
  
  *dataOut = data;
  return byte_cnt;
}
