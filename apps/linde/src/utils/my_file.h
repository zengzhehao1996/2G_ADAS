#ifndef __MY_FILE_H__
#define __MY_FILE_H__

#include <kernel.h>

typedef struct
{
    const char     *filename;
    struct k_mutex mutex;
    int            timeOut;
}classFile_t;

void mountFS(void);
void unmountFs(void);
bool initFile(classFile_t*ps);
bool writeDataToFile(classFile_t* ps, void *pdata, int len);
int  readDataFromFile(classFile_t* ps, void *pdata, int len);
void deleteFile(classFile_t* ps);
void onlyMountFs(void);
bool isFatfsMounted(void);

#endif