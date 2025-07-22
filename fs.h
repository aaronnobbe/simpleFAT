#ifndef __FS_H__
#define __FS_H__
#include <sys/mman.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#define FSSIZE 10000000

typedef struct {
    char name[256]; // file or dir
    char is_dir;    // 0 = file, 1 = dir
    unsigned int size; // file sizes
    unsigned short first_block; // index of 1st block
} DirEntry;

extern unsigned char* fs;

void mapfs(int fd);
void unmapfs();
void formatfs();
void loadfs();
void lsfs();
void addfile(char* fname);
void adddir(char* fname);
void removefile(char* fname);
void extractfile(char* fname);

#endif
