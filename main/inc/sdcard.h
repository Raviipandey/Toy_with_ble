#ifndef __SDCARD_H__
#define __SDCARD_H__

#include "main.h"

class SdCard
{
public:
    void open();
    void close();

    int read(uint8_t *, size_t, FILE *);
    int write(uint8_t *, size_t, FILE *);

    uint64_t getFreeMemory();
    bool exist(char *);
    char **list_files(const char *path, int *file_count);
    void free_file_list(char **file_names, int file_count);

private:
    void setupFiles();
};

void deleteEntireFolder(char *folderName);

#endif