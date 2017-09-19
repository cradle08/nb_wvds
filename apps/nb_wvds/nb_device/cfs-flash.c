#include "cfs/cfs.h"
#include "cfs-flash.h"
#include "lib/crc16.h"
//#include "m25pe.h"
//#include "c2195.h"
#include <string.h>
#include <stddef.h>

#define FLASH_A 0
#define FLASH_B 1

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static struct filedesc *files = NULL;
static struct filedesc *file = NULL;
static int filen = 0;
static int filei = 0;

struct cfs_metadata {
  uint16_t magic;
  uint8_t  count;
  uint8_t  reserva[13];
  struct filedesc files[CFS_MAX_FILES];
  uint8_t  reservb[14];
  uint16_t crc;
} cfs_meta;
/*---------------------------------------------------------------------------*/
static struct filedesc * cfs_create(const char *n);

/*---------------------------------------------------------------------------*/
int cfs_arch_read(uint32_t addr, uint8_t *buf, uint16_t len)
{
  int r = 0;
#if BOARD_CADRE1120_VD
  r = m25pe_read(FLASH_A, addr, buf, len);
#elif BOARD_CADRE1120_AP
  r = m25pe_read(FLASH_B, addr, buf, len);
#else
//#error "no support"
#endif
  return r;
}

int cfs_arch_write(uint32_t addr, uint8_t *buf, uint16_t len)
{
  int r = 0;
#if BOARD_CADRE1120_VD
  r = m25pe_write(FLASH_A, addr, buf, len);
#elif BOARD_CADRE1120_AP
  r = m25pe_write(FLASH_B, addr, buf, len);
#else
//#error "no support"
#endif
  return r;
}
/*---------------------------------------------------------------------------*/
int cfs_init(void)
{
  uint16_t ccrc;
  cfs_arch_read(CFS_START_ADDR, (uint8_t*)&cfs_meta, sizeof(struct cfs_metadata));
  ccrc = crc16_data((uint8_t*)&cfs_meta, sizeof(struct cfs_metadata)-2, 0x0000);
  if ((cfs_meta.magic != CFS_INIT_MAGIC) || (ccrc != cfs_meta.crc)) {
    cfs_factory_reset();
  }
  files = &cfs_meta.files[0];
  return 0;
}

int cfs_factory_reset(void)
{
  memset(&cfs_meta, 0, sizeof(struct cfs_metadata));
  cfs_meta.magic = CFS_INIT_MAGIC;
  cfs_meta.count = 0;
  cfs_meta.crc = crc16_data((uint8_t*)&cfs_meta, sizeof(struct cfs_metadata)-2, 0x0000);
  cfs_arch_write(CFS_START_ADDR, (uint8_t*)&cfs_meta, sizeof(struct cfs_metadata));
  return 0;
}
/*---------------------------------------------------------------------------*/
int cfs_open(const char *name, int flags)
{
  file = cfs_get(name);
  if (file == NULL) {
    if (flags & CFS_WRITE) {
      file = cfs_create(name);
      if (file == NULL) {
        PRINTF("warn cfs open %s\n", name);
        return -1;
      }
    } else {
      PRINTF("warn cfs open %s\n", name);
      return -1;
    }
  }

  if(file->flag == FLAG_FILE_CLOSED) {
    file->flag = FLAG_FILE_OPEN;
    file->access = flags;
    if(flags & CFS_APPEND) {
      file->fileptr = file->endptr;
    } else {
      file->fileptr = 0;
    }
    PRINTF("cfs opened %d:%s\n", filei, n);
    return filei;
  } else {
    return -1;
  }
}

void cfs_close(int f)
{
  file = &files[f];
  file->flag = FLAG_FILE_CLOSED;
  file->fileptr = 0;
  cfs_meta.crc = crc16_data((uint8_t*)&cfs_meta, sizeof(struct cfs_metadata)-2, 0x0000);
  cfs_arch_write(CFS_START_ADDR, (uint8_t*)&cfs_meta, sizeof(struct cfs_metadata));
  filei = -1;
  PRINTF("cfs closed fd %d\n", f);
}

int cfs_read(int f, void *buf, unsigned int len)
{
  int r = 0;

  file = &files[f];
  if((file->flag == FLAG_FILE_OPEN) && (file->access & CFS_READ)) {
    if(file->fileptr + len >= file->endptr) {
      len = file->endptr - file->fileptr;
    }
    PRINTF("cfs read fd %d, addr 0x%04X, len %d\n", f, (file->offset + file->fileptr), len);
    r = cfs_arch_read(CFS_FILES_ADDR + file->offset + file->fileptr, (uint8_t*)buf, len);
    if (r != 0)
      return -1;
    file->fileptr += len;
    return len;
  } else {
    PRINTF("warn cfs read fd %d, len %d\n", f, len);
    return -1;
  }
}

int cfs_write(int f, const void *buf, unsigned int len)
{
  int r = 0;

  file = &files[f];
  if((file->flag == FLAG_FILE_OPEN) && (file->access & CFS_WRITE)) {
    if(file->fileptr + len > file->maxsize) {
      PRINTF("warn cfs write truncated %d+%d>%d\n", file->fileptr, len, file->maxsize);
      len = file->maxsize - file->fileptr;
    }
    if (len == 0)
      return -1;
    PRINTF("cfs write fd %d, addr 0x%04X, len %d\n", f, (file->offset + file->fileptr), len);
    r = cfs_arch_write(CFS_FILES_ADDR + file->offset + file->fileptr, (uint8_t*)buf, len);
    if (r != 0)
      return -1;
    file->fileptr += len;
    if(file->fileptr > file->endptr) {
      file->endptr = file->fileptr;
    }
    return len;
  } else {
    PRINTF("warn cfs write fd %d, len\n", f, len);
    return -1;
  }
}

cfs_offset_t cfs_seek(int f, cfs_offset_t o, int w)
{
  file = &files[f];
  if(file->flag == FLAG_FILE_OPEN) {
    if(w == CFS_SEEK_SET) {
      file->fileptr = o;
    } else if(w == CFS_SEEK_CUR) {
      file->fileptr += o;
    } else if(w == CFS_SEEK_END) {
      file->fileptr = file->endptr + o;
    }
    if(file->fileptr <= file->maxsize) {
      if(file->fileptr > file->endptr) {
        file->endptr = file->fileptr;
      }
      return file->fileptr;
    }
  }
  return -1;
}

/*---------------------------------------------------------------------------*/
struct filedesc *
cfs_get(const char *n)
{
  struct filedesc *f = NULL;
  uint8_t i;

  for (i = 0; i < CFS_MAX_FILES; i++) {
    if (strncmp(files[i].name, n, strlen(n)) == 0) {
      PRINTF("cfs got %s, %d:0x%04X\n", n, i, files[i].offset);
      filei = i;
      f = &files[i];
      break;
    }
  }

  return f;
}

static struct filedesc *
cfs_create(const char *n)
{
  struct filedesc *f = NULL;
  uint8_t i;
  uint32_t end = 0;

  filei = -1;
  filen = 0;
  for (i = 0; i < CFS_MAX_FILES; i++) {
    if (strlen(files[i].name) == 0) {
      break;
    } else if (strncmp(files[i].name, n, strlen(n)) == 0) {
      PRINTF("cfs got %s, %d:0x%04X\n", n, i, files[i].offset);
      filei = i;
      f = &files[i];
      break;
    } else {
      ++filen;
      if (files[i].offset + files[i].maxsize > end)
        end = files[i].offset + files[i].maxsize;
    }
  }
  if (f == NULL) {
    if (end < FLASH_SIZE-1) {
      filei = filen;
      f = &files[filen];
      strncpy(f->name, n, strlen(n));
      f->offset = end;
      f->maxsize = 0;
      PRINTF("cfs new %s, %d:0x%04X\n", n, filen, f->offset);
    }
  }

  return f;
}

int
cfs_reserv(int f, uint32_t size)
{
  file = &files[f];
  file->maxsize = size;
  PRINTF("cfs reserv %s 0x[%04X,%04X]\n", file->name, file->offset, (file->offset + file->maxsize));
  return 0;
}
