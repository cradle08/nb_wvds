#ifndef _CFS_FLASH_H
#define _CFS_FLASH_H

#include "contiki.h"

#ifndef CFS_MAX_FILES
#define CFS_MAX_FILES  7
#endif

// CFS的起始地址
#ifndef CFS_START_ADDR
#if BOARD_CADRE1120_VD
#define CFS_START_ADDR  (36UL<<10)
#elif BOARD_CADRE1120_AP
#define CFS_START_ADDR  0UL
#else
//#error "no support"
#endif
#endif

// CFS文件元数据大小
#define CFS_META_SIZE 0x0800UL // 2kB

// CFS文件的起始地址
#define CFS_FILES_ADDR (CFS_START_ADDR + CFS_META_SIZE)

#define FLAG_FILE_CLOSED 0
#define FLAG_FILE_OPEN   1

#define CFS_INIT_MAGIC 0xCDAB

struct filedesc {
  char name[13];
  char flag;
  int  access;
  uint32_t fileptr;
  uint32_t endptr;
  uint32_t offset;
  uint32_t maxsize;
};
/*---------------------------------------------------------------------------*/
int cfs_init(void);
int cfs_factory_reset(void);
struct filedesc * cfs_get(const char *n);
int cfs_reserv(int f, uint32_t size);

#endif /* _CFS_FLASH_H */
