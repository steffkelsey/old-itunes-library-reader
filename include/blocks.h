#ifndef BLOCKS_H
#define BLOCKS_H

#define HEADER_MAGIC "hdfm"

struct dbheader_t {
	unsigned char magic[4];
	unsigned short version;
	unsigned int filelength;
};


#endif
