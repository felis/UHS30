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

#include "macro_logic.h"
#include <ISR_safe_memory.h>
#include "FAT/FatFS/src/ff.h"
#include "fcntl.h"
#ifdef UHS_USE_SDCARD
#ifndef __UHS_BULK_STORAGE_H__
#endif
#endif
#if !defined(NOTUSED)
#define NOTUSED(...)  __VA_ARGS__ __attribute__((unused))
#endif

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
                int driver_type;
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

        extern void Init_Generic_Storage(
#ifdef __UHS_BULK_STORAGE_H__
                void *
#endif
#ifdef UHS_USE_SDCARD
#ifdef __UHS_BULK_STORAGE_H__
                ,
#endif
                int [], int []
#endif
                );
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
#define MAX_USB_MS_DRIVERS 2 // must be 1 to 4 minus any SDcards
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
#else
#ifndef MAX_USB_MS_DRIVERS
#define MAX_USB_MS_DRIVERS 0
#endif

#endif

#ifdef UHS_USE_SDCARD

#if !defined(UHS_SD_CARDS)
#define UHS_MAX_SD_CARDS 1
#endif
/** init timeout ms */
uint16_t const UHS_SD_INIT_TIMEOUT = 2000;
/** erase timeout ms */
uint16_t const UHS_SD_ERASE_TIMEOUT = 10000;
/** read timeout ms */
uint16_t const UHS_SD_READ_TIMEOUT = 300;
/** write time out ms */
uint16_t const UHS_SD_WRITE_TIMEOUT = 600;

/** timeout error for command CMD0 */
uint8_t const UHS_SD_CARD_ERROR_CMD0 = 0X1;
/** CMD8 was not accepted - not a valid SD card*/
uint8_t const UHS_SD_CARD_ERROR_CMD8 = 0X2;
/** card returned an error response for CMD17 (read block) */
uint8_t const UHS_SD_CARD_ERROR_CMD17 = 0X3;
/** card returned an error response for CMD24 (write block) */
uint8_t const UHS_SD_CARD_ERROR_CMD24 = 0X4;
/**  WRITE_MULTIPLE_BLOCKS command failed */
uint8_t const UHS_SD_CARD_ERROR_CMD25 = 0X05;
/** card returned an error response for CMD58 (read OCR) */
uint8_t const UHS_SD_CARD_ERROR_CMD58 = 0X06;
/** SET_WR_BLK_ERASE_COUNT failed */
uint8_t const UHS_SD_CARD_ERROR_ACMD23 = 0X07;
/** card's ACMD41 initialization process timeout */
uint8_t const UHS_SD_CARD_ERROR_ACMD41 = 0X08;
/** card returned a bad CSR version field */
uint8_t const UHS_SD_CARD_ERROR_BAD_CSD = 0X09;
/** erase block group command failed */
uint8_t const UHS_SD_CARD_ERROR_ERASE = 0X0A;
/** card not capable of single block erase */
uint8_t const UHS_SD_CARD_ERROR_ERASE_SINGLE_BLOCK = 0X0B;
/** Erase sequence timed out */
uint8_t const UHS_SD_CARD_ERROR_ERASE_TIMEOUT = 0X0C;
/** card returned an error token instead of read data */
uint8_t const UHS_SD_CARD_ERROR_READ = 0X0D;
/** read CID or CSD failed */
uint8_t const UHS_SD_CARD_ERROR_READ_REG = 0X0E;
/** timeout while waiting for start of read data */
uint8_t const UHS_SD_CARD_ERROR_READ_TIMEOUT = 0X0F;
/** card did not accept STOP_TRAN_TOKEN */
uint8_t const UHS_SD_CARD_ERROR_STOP_TRAN = 0X10;
/** card returned an error token as a response to a write operation */
uint8_t const UHS_SD_CARD_ERROR_WRITE = 0X11;
/** attempt to write protected block zero */
uint8_t const UHS_SD_CARD_ERROR_WRITE_BLOCK_ZERO = 0X12;
/** card did not go ready for a multiple block write */
uint8_t const UHS_SD_CARD_ERROR_WRITE_MULTIPLE = 0X13;
/** card returned an error to a CMD13 status check after a write */
uint8_t const UHS_SD_CARD_ERROR_WRITE_PROGRAMMING = 0X14;
/** timeout occurred during write programming */
uint8_t const UHS_SD_CARD_ERROR_WRITE_TIMEOUT = 0X15;
/** incorrect rate selected */
uint8_t const UHS_SD_CARD_ERROR_SCK_RATE = 0X16;
/** Standard capacity V1 SD card */
uint8_t const UHS_SD_CARD_TYPE_SD1 = 1;
/** Standard capacity V2 SD card */
uint8_t const UHS_SD_CARD_TYPE_SD2 = 2;
/** High Capacity SD card */
uint8_t const UHS_SD_CARD_TYPE_SDHC = 3;
/** GO_IDLE_STATE - init card in spi mode if CS low */
uint8_t const UHS_SD_CMD0 = 0X00;
/** SEND_IF_COND - verify SD Memory Card interface operating condition.*/
uint8_t const UHS_SD_CMD8 = 0X08;
/** SEND_CSD - read the Card Specific Data (CSD register) */
uint8_t const UHS_SD_CMD9 = 0X09;
/** SEND_CID - read the card identification information (CID register) */
uint8_t const UHS_SD_CMD10 = 0X0A;
/** SEND_STATUS - read the card status register */
uint8_t const UHS_SD_CMD13 = 0X0D;
/** READ_BLOCK - read a single data block from the card */
uint8_t const UHS_SD_CMD17 = 0X11;
/** WRITE_BLOCK - write a single data block to the card */
uint8_t const UHS_SD_CMD24 = 0X18;
/** WRITE_MULTIPLE_BLOCK - write blocks of data until a STOP_TRANSMISSION */
uint8_t const UHS_SD_CMD25 = 0X19;
/** ERASE_WR_BLK_START - sets the address of the first block to be erased */
uint8_t const UHS_SD_CMD32 = 0X20;
/** ERASE_WR_BLK_END - sets the address of the last block of the continuous
    range to be erased*/
uint8_t const UHS_SD_CMD33 = 0X21;
/** ERASE - erase all previously selected blocks */
uint8_t const UHS_SD_CMD38 = 0X26;
/** APP_CMD - escape for application specific command */
uint8_t const UHS_SD_CMD55 = 0X37;
/** READ_OCR - read the OCR register of a card */
uint8_t const UHS_SD_CMD58 = 0X3A;
/** SET_WR_BLK_ERASE_COUNT - Set the number of write blocks to be
     pre-erased before writing */
uint8_t const UHS_SD_ACMD23 = 0X17;
/** SD_SEND_OP_COMD - Sends host capacity support information and
    activates the card's initialization process */
uint8_t const UHS_SD_ACMD41 = 0X29;
/** status for card in the ready state */
uint8_t const UHS_SD_R1_READY_STATE = 0X00;
/** status for card in the idle state */
uint8_t const UHS_SD_R1_IDLE_STATE = 0X01;
/** status bit for illegal command */
uint8_t const UHS_SD_R1_ILLEGAL_COMMAND = 0X04;
/** start data token for read or write single block*/
uint8_t const UHS_SD_DATA_START_BLOCK = 0XFE;
/** stop token for write multiple blocks*/
uint8_t const UHS_SD_STOP_TRAN_TOKEN = 0XFD;
/** start data token for write multiple blocks*/
uint8_t const UHS_SD_WRITE_MULTIPLE_TOKEN = 0XFC;
/** mask for data response tokens after a write block operation */
uint8_t const UHS_SD_DATA_RES_MASK = 0X1F;
/** write data accepted token */
uint8_t const UHS_SD_DATA_RES_ACCEPTED = 0X05;

typedef struct UHS_SD_CID {
        // byte 0
        uint8_t mid; // Manufacturer ID
        // byte 1-2
        char oid[2]; // OEM/Application ID
        // byte 3-7
        char pnm[5]; // Product name
        // byte 8
        unsigned prv_m : 4; // Product revision n.m
        unsigned prv_n : 4;
        // byte 9-12
        uint32_t psn; // Product serial number
        // byte 13
        unsigned mdt_year_high : 4; // Manufacturing date
        unsigned reserved : 4;
        // byte 14
        unsigned mdt_month : 4;
        unsigned mdt_year_low : 4;
        // byte 15
        unsigned always1 : 1;
        unsigned crc : 7;
} UHS_SD_cid_t;

typedef struct UHS_SD_CSDV1 {
        // byte 0
        unsigned reserved1 : 6;
        unsigned csd_ver : 2;
        // byte 1
        uint8_t taac;
        // byte 2
        uint8_t nsac;
        // byte 3
        uint8_t tran_speed;
        // byte 4
        uint8_t ccc_high;
        // byte 5
        unsigned read_bl_len : 4;
        unsigned ccc_low : 4;
        // byte 6
        unsigned c_size_high : 2;
        unsigned reserved2 : 2;
        unsigned dsr_imp : 1;
        unsigned read_blk_misalign : 1;
        unsigned write_blk_misalign : 1;
        unsigned read_bl_partial : 1;
        // byte 7
        uint8_t c_size_mid;
        // byte 8
        unsigned vdd_r_curr_max : 3;
        unsigned vdd_r_curr_min : 3;
        unsigned c_size_low : 2;
        // byte 9
        unsigned c_size_mult_high : 2;
        unsigned vdd_w_cur_max : 3;
        unsigned vdd_w_curr_min : 3;
        // byte 10
        unsigned sector_size_high : 6;
        unsigned erase_blk_en : 1;
        unsigned c_size_mult_low : 1;
        // byte 11
        unsigned wp_grp_size : 7;
        unsigned sector_size_low : 1;
        // byte 12
        unsigned write_bl_len_high : 2;
        unsigned r2w_factor : 3;
        unsigned reserved3 : 2;
        unsigned wp_grp_enable : 1;
        // byte 13
        unsigned reserved4 : 5;
        unsigned write_partial : 1;
        unsigned write_bl_len_low : 2;
        // byte 14
        unsigned reserved5 : 2;
        unsigned file_format : 2;
        unsigned tmp_write_protect : 1;
        unsigned perm_write_protect : 1;
        unsigned copy : 1;
        unsigned file_format_grp : 1;
        // byte 15
        unsigned always1 : 1;
        unsigned crc : 7;
} UHS_SD_csd1_t;

typedef struct UHS_SD_CSDV2 {
        // byte 0
        unsigned reserved1 : 6;
        unsigned csd_ver : 2;
        // byte 1
        uint8_t taac;
        // byte 2
        uint8_t nsac;
        // byte 3
        uint8_t tran_speed;
        // byte 4
        uint8_t ccc_high;
        // byte 5
        unsigned read_bl_len : 4;
        unsigned ccc_low : 4;
        // byte 6
        unsigned reserved2 : 4;
        unsigned dsr_imp : 1;
        unsigned read_blk_misalign : 1;
        unsigned write_blk_misalign : 1;
        unsigned read_bl_partial : 1;
        // byte 7
        unsigned reserved3 : 2;
        unsigned c_size_high : 6;
        // byte 8
        uint8_t c_size_mid;
        // byte 9
        uint8_t c_size_low;
        // byte 10
        unsigned sector_size_high : 6;
        unsigned erase_blk_en : 1;
        unsigned reserved4 : 1;
        // byte 11
        unsigned wp_grp_size : 7;
        unsigned sector_size_low : 1;
        // byte 12
        unsigned write_bl_len_high : 2;
        unsigned r2w_factor : 3;
        unsigned reserved5 : 2;
        unsigned wp_grp_enable : 1;
        // byte 13
        unsigned reserved6 : 5;
        unsigned write_partial : 1;
        unsigned write_bl_len_low : 2;
        // byte 14
        unsigned reserved7 : 2;
        unsigned file_format : 2;
        unsigned tmp_write_protect : 1;
        unsigned perm_write_protect : 1;
        unsigned copy : 1;
        unsigned file_format_grp : 1;
        // byte 15
        unsigned always1 : 1;
        unsigned crc : 7;
} UHS_SD_csd2_t;

union UHS_SD_csd_t {
        UHS_SD_csd1_t v1;
        UHS_SD_csd2_t v2;
};

#define SDISR_BODY(x) UHS_SD_Storage[x]->IRQ();
#define MAKE_SD_ISR_REFS() AJK_MAKE_FUNS(static void SD_ISR, void, UHS_MAX_SD_CARDS, SDISR_BODY)
typedef struct SDPvt {
        int B; // which instance
} SDpvt_t;

class UHS_SD {
protected:

        uint8_t errorCode_;
        uint8_t type_;
        uint8_t status_;

        //uint32_t block_;
        //uint8_t inBlock_;
        //uint16_t offset_;
        //uint8_t partialBlockRead_;

        uint8_t cardAcmd(uint8_t cmd, uint32_t arg) {
                cardCommand(UHS_SD_CMD55, 0);
                return cardCommand(cmd, arg);
        }

        void type(uint8_t value) {
                type_ = value;
        }

        void error(uint8_t code) {
                errorCode_ = code;
        }

        uint8_t errorCode(void) const {
                return errorCode_;
        }

        uint8_t errorData(void) const {
                return status_;
        }

        uint8_t readCSD(UHS_SD_csd_t* csd) {
                return readRegister(UHS_SD_CMD9, csd);
        }

        uint8_t readCID(UHS_SD_cid_t* cid) {
                return readRegister(UHS_SD_CMD10, cid);
        }

        uint8_t type(void) const {
                return type_;
        }

        bool waitNotBusy(uint16_t timeoutMillis);
        //void readEnd(void);
        uint8_t cardCommand(uint8_t cmd, uint32_t arg);
        bool readRegister(uint8_t cmd, void* buf);
        bool waitStartBlock(void);

        //uint8_t sendWriteCommand(uint32_t blockNumber, uint32_t eraseCount);

        //uint8_t partialBlockRead(void) const {
        //        return partialBlockRead_;
        //}

        uint32_t cardSize(void);




        //uint8_t readBlock(uint32_t block, uint8_t* dst);
        //uint8_t readData(uint32_t block, uint16_t offset, uint16_t count, uint8_t* dst);




        //uint8_t erase(uint32_t firstBlock, uint32_t lastBlock);
        //uint8_t eraseSingleBlockEnable(void);

        bool writeBlock(uint32_t blockNumber, const uint8_t* src);
        //uint8_t writeData(const uint8_t* src);
        bool writeData(uint8_t token, const uint8_t* src);
        //uint8_t writeStart(uint32_t blockNumber, uint32_t eraseCount);
        //uint8_t writeStop(void);




public:
        SPISettings settings;
        int present;
        int cs;
        bool last_present;
        uint32_t CurrentCapacity;


        UHS_SD(int _present, int _cs);

        bool Present(void) {
                return (digitalRead(present) == LOW ? true : false);
        };
        uint8_t Read(uint32_t addr, uint16_t bsize, uint8_t blocks, uint8_t *buf);
        uint8_t Write(uint32_t addr, uint16_t bsize, uint8_t blocks, const uint8_t *buf);

        uint32_t GetCapacity(void) {
                return CurrentCapacity;
        };

        uint16_t GetSectorSize(void) {
                return 512;
        };

        // Note: This actually means that it is OK to write (true).
        // i.e. The logic is reversed as if sensing a LOW on a pin.
        bool WriteProtected(void) {
                return true;
        };
};

class UHS_FS_SD_DRIVER : public UHS_SD {
private:
        int sto_idx;
public:
        UHS_FS_SD_DRIVER(int _present, int _cs, int i);
        void IRQ(void);
};

UHS_FS_SD_DRIVER *UHS_SD_Storage[UHS_MAX_SD_CARDS];

#else
#if !defined(UHS_SD_CARDS)
#define UHS_MAX_SD_CARDS 0
#endif
#endif




// Your driver interface(s) go here...


// Add up all interface maximums...
#if !defined(PFAT_VOLUMES)
#define PFAT_VOLUMES (UHS_MAX_SD_CARDS + MAX_USB_MS_DRIVERS)
#endif

#include <stdio.h>
#include "FAT/FatFS/src/ff.c"
#include "FAT/FatFS/src/option/unicode.h"
#include "FAT/FatFS/src/option/syscall.h"
#include "PCpartition/PCPartition.cpp"
#include "FAT/FAT.cpp"


extern "C" {
#ifdef __UHS_BULK_STORAGE_H__
        bool UHS_USB_BulkOnly_Status(storage_t *sto);
        DSTATUS UHS_USB_BulkOnly_Initialize(storage_t *sto);
        int UHS_USB_BulkOnly_Read(uint32_t LBA, uint8_t *buf, storage_t *sto, uint8_t count);
        int UHS_USB_BulkOnly_Write(uint32_t LBA, uint8_t *buf, storage_t *sto, uint8_t count);
        uint8_t UHS_USB_BulkOnly_Commit(storage_t *sto);
#endif
#ifdef UHS_USE_SDCARD
        bool UHS_SD_Status(storage_t *sto);
        DSTATUS UHS_SD_Initialize(storage_t *sto);
        int UHS_SD_Read(uint32_t LBA, uint8_t *buf, storage_t *sto, uint8_t count);
        int UHS_SD_Write(uint32_t LBA, uint8_t *buf, storage_t *sto, uint8_t count);
        uint8_t UHS_SD_Commit(storage_t *sto);
#endif
}


#include "UHS_FS_INLINE.h"

#endif
#endif



#endif	/* UHS_FS_H */
