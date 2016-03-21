/*
 * File:   Storage.h
 * Author: xxxajk@gmail.com
 *
 * Created on February 19, 2013, 7:44 AM
 */

/* Copyright (C) 2015-2016 Andrew J. Kroll
   and
Copyright (C) 2011 Circuits At Home, LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").

Contact information
-------------------

Circuits At Home, LTD
Web      :  http://www.circuitsathome.com
e-mail   :  support@circuitsathome.com
 */

#ifndef UHS_FS_H
#define	UHS_FS_H


#include "FAT/FatFS/src/ff.h"
#include "fcntl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Generic storage structure
 */
typedef struct Storage {
        int (*Reads)(uint32_t, uint8_t *, struct Storage *, uint8_t); // multiple sector read
        int (*Writes)(uint32_t, uint8_t *, struct Storage *, uint8_t); // multiple sector write
        bool (*Status)(struct Storage *);
        DSTATUS(*Initialize)(struct Storage *);
        uint8_t(*Commit)(struct Storage *);
        uint16_t SectorSize; // Physical or translated size on the physical media
        uint32_t TotalSectors; // Total sector count. Used to guard against illegal access.
        void *private_data; // Anything you need, or nothing at all.
} storage_t;

/**
 * FAT directory information structure
 */
typedef struct {
                DWORD fsize; /* File size */
                WORD fdate; /* Last modified date */
                WORD ftime; /* Last modified time */
                FBYTE fattrib; /* Attribute */
                TCHAR fname[13]; /* Short file name (8.3 format) */
                TCHAR lfname[_MAX_LFN + 1]; /* Start point of long file name. */
} __attribute__((packed)) PFAT_DIRINFO;


/**
 * Global file-system error code
 */
extern uint8_t fs_err;

extern uint8_t fs_ready(const char *path);
extern uint8_t fs_open(const char *path, int flags);
extern uint8_t fs_opendir(const char *path);
extern int fs_close(uint8_t fd);
extern int fs_closedir(uint8_t dh);
extern int fs_read(uint8_t fd, void *data, uint16_t amount);
extern int fs_readdir(uint8_t fd, PFAT_DIRINFO *data);
extern int fs_write(uint8_t fd, const void *data, uint16_t amount);
extern uint8_t fs_unlink(const char *path);

extern uint8_t fs_sync(void);
extern uint8_t fs_flush(uint8_t fd);
extern uint8_t fs_eof(uint8_t fd);
extern uint8_t fs_truncate(uint8_t fd);

extern unsigned long fs_tell(uint8_t fd);
extern uint8_t fs_lseek(uint8_t fd, unsigned long offset, int whence);

extern uint8_t fs_rename(const char *oldpath, const char *newpath);
extern uint8_t fs_chmod(const char *path, uint8_t mode);
extern uint8_t fs_utime(const char *path, time_t timesec);
extern uint8_t fs_mkdir(const char *path, uint8_t mode);
extern uint64_t fs_getfree(const char *path);
extern uint8_t fs_stat(const char *path, FILINFO *buf);
extern char *fs_mount_lbl(uint8_t vol);
extern uint8_t fs_mountcount(void);
// Initialize every sub-system
extern void Init_Generic_Storage(void *);

#ifdef __cplusplus
}
#endif



#ifdef LOAD_GENERIC_STORAGE
#ifndef GENERIC_STORAGE_LOADED


#ifndef FAT_MAX_ERROR_RETRIES
#define FAT_MAX_ERROR_RETRIES 10
#endif

/*
 * Notes:
 * In order to assist these calls, a pointer to the Storage struct is also passed,
 * so that your driver can get at its own private information, if used.
 * The private_data pointer can point to anything that you need for your driver,
 * or nothing at all if you do not need any. Nothing in the code here will use it.
 *
 * Also, "sector" is the Logical Block Address on the media, starting at ZERO.
 * C/H/S is not, and will not be used!
 * Any translations from the LBA must be done on the storage driver side, if needed.
 * Translations include any sector offsets, logical device numbers, etc.
 * The buffer size is guaranteed to be correct providing that you set SectorSize properly.
 *
 * Success from Read and Write should return ZERO,
 * Errors should return a non-Zero integer meaningful to the storage caller.
 * Negative One is returned by this layer to indicate some other error.
 *
 */


#ifdef __UHS_BULK_STORAGE_H__

#ifndef MAX_USB_MS_DRIVERS
#define MAX_USB_MS_DRIVERS 2 // must be 1 to 4
#endif

typedef struct Pvt {
        int B; // which instance
        uint8_t lun; // which LUN
} pvt_t;


class UHS_FS_BULK_DRIVER : public UHS_Bulk_Storage {
private:
        int sto_idx;
public:
        UHS_FS_BULK_DRIVER(UHS_USB_HOST_BASE *p, int i);
        virtual uint8_t OnStart(void);
        virtual void OnPoll(void);
        virtual void OnRelease(void);
};

UHS_FS_BULK_DRIVER *UHS_USB_Storage[MAX_USB_MS_DRIVERS];

#endif

// Your driver interface(s) go here...


// TO-DO: abstract to allow multiple interfaces at once, e.g. SD on SPI and USB

#include <stdio.h>
#include "FAT/FatFS/src/ff.c"
#include "FAT/FatFS/src/option/unicode.h"
#include "FAT/FatFS/src/option/syscall.h"
#include "PCpartition/PCPartition.cpp"
#include "FAT/FAT.cpp"


#ifdef __UHS_BULK_STORAGE_H__
extern "C" {
bool UHS_USB_BulkOnly_Status(storage_t *sto);
DSTATUS UHS_USB_BulkOnly_Initialize(storage_t *sto);
int UHS_USB_BulkOnly_Read(uint32_t LBA, uint8_t *buf, storage_t *sto, uint8_t count);
int UHS_USB_BulkOnly_Write(uint32_t LBA, uint8_t *buf, storage_t *sto, uint8_t count);
uint8_t UHS_USB_BulkOnly_Commit(storage_t *sto);
}
#endif


#include "UHS_FS_INLINE.h"

#endif
#endif



#endif	/* UHS_FS_H */
