#ifndef __FOTA_H__
#define __FOTA_H__
#include "my_md5.h"
#include <kernel.h>

#define FOTA_CFG_PATH   "fota_config"
#define FOTA_FILE_PATH  "fota.bin"

enum
{
    NONE = 0,
    DOWNLOADING = 1,
    DONE = 3,
    SOLIDIFY = 4
};

typedef struct
{
    uint8_t status;
    uint8_t major;
    uint8_t minor;
    uint8_t tiny;
    uint8_t path;
    uint8_t md5[16];
    uint32_t fotaSize;
}fotaConfig_t;

void deleteFotaFile(void);
int  pushBytesToFotaFile(char * data,int len, uint32_t off);
int  getFotaFileSize(void);

bool doFotaUpgrade(void);
int  fotaSolidify(void);

bool saveFotaCFG(fotaConfig_t *p);
bool loadFotaCFG(fotaConfig_t *p);
bool updateFotaCFG(uint8_t state);
void deleteFotaCFG(void);

bool getFOTAfileMd5(md5_byte_t *outmd5);


#endif