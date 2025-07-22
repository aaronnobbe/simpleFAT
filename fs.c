#include "fs.h"
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BLOCK_SIZE 512
#define NUM_BLOCKS ((FSSIZE - 65536) / BLOCK_SIZE)
#define FAT_ENTRIES (NUM_BLOCKS)
#define ROOT_ENTRIES 128

unsigned char* fs;
unsigned short* fat;
DirEntry* root;
unsigned char* datablocks;

void mapfs(int fd) {
    fs = mmap(NULL, FSSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fs == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }
    fat = (unsigned short*) fs;
    root = (DirEntry*) (fs + FAT_ENTRIES * sizeof(unsigned short));
    datablocks = fs + 65536;
}

void unmapfs(){
  munmap(fs, FSSIZE);
}


void formatfs(){
    // zero out fat
    for (int i = 0; i < FAT_ENTRIES; i++) {
        fat[i] = 0;
    }

    // zero out root dir
    for (int i = 0; i < ROOT_ENTRIES; i++) {
        memset(&root[i], 0, sizeof(DirEntry));
    }

    // mark block 0 used for root
    fat[0] = USHRT_MAX;
}


void loadfs(){

}

int find_free_block() {
    for (int i = 1; i < FAT_ENTRIES; i++) {
        if (fat[i] == 0) return i;
    }
    return -1;
}

int find_free_root_entry() {
    for (int i = 0; i < ROOT_ENTRIES; i++) {
        if (root[i].name[0] == '\0') return i;
    }
    return -1;
}

void lsfs() {
    for (int i = 0; i < ROOT_ENTRIES; i++) {
        if (root[i].name[0] != '\0') {
            printf("%s\t%s\t%u bytes\n", root[i].name, root[i].is_dir ? "dir" : "file", root[i].size);
        }
    }
}

void addfile(char* fname) {
    int fd = open(fname, O_RDONLY);
    if (fd < 0) {
        perror("could not open input file");
        return;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("stat failed");
        close(fd);
        return;
    }

    int idx = find_free_root_entry();
    if (idx < 0) {
        printf("no space in root dir\n");
        close(fd);
        return;
    }

    strncpy(root[idx].name, fname, 255);
    root[idx].is_dir = 0;
    root[idx].size = st.st_size;

    int prev_block = -1;
    int first_block = -1;
    int read_bytes;
    char buf[BLOCK_SIZE];

    while ((read_bytes = read(fd, buf, BLOCK_SIZE)) > 0) {
        int block = find_free_block();
        if (block < 0) {
            printf("no space left in fs\n");
            break;
        }

        memcpy(datablocks + block * BLOCK_SIZE, buf, read_bytes);

        if (prev_block != -1) {
            fat[prev_block] = block;
        } else {
            first_block = block;
        }
        prev_block = block;
    }

    if (prev_block != -1) {
        fat[prev_block] = USHRT_MAX;
    }

    root[idx].first_block = first_block;

    close(fd);
}

void adddir(char* dname){
    int idx = find_free_root_entry();
    if (idx < 0) {
        printf("no space in root dir\n");
        return;
    }
    strncpy(root[idx].name, dname, 255);
    root[idx].is_dir = 1;
    root[idx].size = 0;
    root[idx].first_block = 0;
}

void removefile(char* fname){
    for (int i = 0; i < ROOT_ENTRIES; i++) {
        if (strcmp(root[i].name, fname) == 0) {
            int block = root[i].first_block;
            while (block != USHRT_MAX && block != 0) {
                int next = fat[block];
                fat[block] = 0;
                block = next;
            }
            memset(&root[i], 0, sizeof(DirEntry));
            printf("removed %s\n", fname);
            return;
        }
    }
    printf("file not found\n");
}


void extractfile(char* fname){
    for (int i = 0; i < ROOT_ENTRIES; i++) {
        if (strcmp(root[i].name, fname) == 0 && !root[i].is_dir) {
            int block = root[i].first_block;
            unsigned int remaining = root[i].size;
            while (block != USHRT_MAX && remaining > 0) {
                int to_write = remaining > BLOCK_SIZE ? BLOCK_SIZE : remaining;
                write(1, datablocks + block * BLOCK_SIZE, to_write);
                remaining -= to_write;
                block = fat[block];
            }
            return;
        }
    }
    printf("file not found\n");
}
