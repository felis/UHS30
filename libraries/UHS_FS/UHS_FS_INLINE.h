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

#if defined(UHS_FS_H) && !defined(GENERIC_STORAGE_LOADED)
#define GENERIC_STORAGE_LOADED
#ifndef AJK_NI
#define AJK_NI __attribute__((noinline))
#endif


extern "C" {
        uint8_t fs_err;

        static FIL *fhandles[_FS_LOCK]; // File handles
        static DIR *dhandles[_FS_LOCK]; // Directory handles
        static PFAT *Fats[PFAT_VOLUMES];
        static storage_t *sto[PFAT_VOLUMES];
        static bool last_ready[MAX_USB_MS_DRIVERS][MASS_MAX_SUPPORTED_LUN];

        static void none_ready(int idx) {
                for(int j = 0; j < MASS_MAX_SUPPORTED_LUN; j++) {
                        last_ready[idx][j] = false;
                }
        }

        static void kill_mounts(int idx) {
                for(int f = 0; f < PFAT_VOLUMES; f++) {
                        if(Fats[f] != NULL) {
                                if(((pvt_t *)sto[f]->private_data)->B == idx) {
                                        delete Fats[f];
                                        Fats[f] = NULL;
                                        delete (pvt_t *)sto[f]->private_data;
                                        sto[f]->private_data = NULL;
                                }
                        }
                }
                none_ready(idx);
        }

#ifdef __UHS_BULK_STORAGE_H__
        // On mass storage, there is nothing to do. Just call the status function.

        DSTATUS UHS_USB_BulkOnly_Initialize(storage_t *sto) {
                if((UHS_USB_Storage[((pvt_t *)sto->private_data)->B]->WriteProtected(((pvt_t *)sto->private_data)->lun))) {
                        return STA_PROTECT;
                } else {
                        return STA_OK;
                }
        }

        bool UHS_USB_BulkOnly_Status(storage_t *sto) {
                return (UHS_USB_Storage[((pvt_t *)sto->private_data)->B]->WriteProtected(((pvt_t *)sto->private_data)->lun));
        }


        // TO-DO: read cache?

        int UHS_USB_BulkOnly_Read(uint32_t LBA, uint8_t *buf, storage_t *sto, uint8_t count) {
                uint8_t x = 0;
                while(count) {
                        int tries = FAT_MAX_ERROR_RETRIES;
                        while(tries) {
                                tries--;
                                x = (UHS_USB_Storage[((pvt_t *)sto->private_data)->B]->Read(((pvt_t *)sto->private_data)->lun, LBA, (sto->SectorSize), 1, buf));
                                if(x == UHS_BULK_ERR_DEVICE_DISCONNECTED) break;
                                if(!x) break;
                        }
                        if(x) break;
                        // TO-DO: READ caching?
                        count--;
                        buf += (sto->SectorSize);
                        LBA++;
                }
                int y = x;
                return y;
        }

        int UHS_USB_BulkOnly_Write(uint32_t LBA, uint8_t *buf, storage_t *sto, uint8_t count) {
                uint8_t x = 0;
                while(count && !x) {
                        int tries = FAT_MAX_ERROR_RETRIES;
                        while(tries) {
                                tries--;
                                x = (UHS_USB_Storage[((pvt_t *)sto->private_data)->B]->Write(((pvt_t *)sto->private_data)->lun, LBA, (sto->SectorSize), 1, buf));
                                if(x == UHS_BULK_ERR_WRITE_PROTECTED) break;
                                if(x == UHS_BULK_ERR_DEVICE_DISCONNECTED) break;
                                if(!x) break;
                        }
                        count--;
                        buf += (sto->SectorSize);
                        LBA++;
                }
                int y = x;
                return y;
        }

        uint8_t UHS_USB_BulkOnly_Commit(NOTUSED(storage_t *sto)) {
                uint8_t x = 0;
                return x;
        }


}

UHS_FS_BULK_DRIVER::UHS_FS_BULK_DRIVER(UHS_USB_HOST_BASE *p, int i) : UHS_Bulk_Storage(p) {
        sto_idx = i;
        none_ready(sto_idx);
};

uint8_t UHS_FS_BULK_DRIVER::OnStart(void) {
        none_ready(sto_idx);
        return 0;
}

void UHS_FS_BULK_DRIVER::OnPoll(void) {
        // Check LUN status, mount/unmount as needed.
        for(uint8_t lun = 0; lun <= bMaxLUN; lun++) {
                if(!last_ready[sto_idx][lun]) {
                        if(LUNIsGood(lun)) {
                                // Mount
                                int nm = (int)f_next_mount();
                                if(nm < PFAT_VOLUMES) {
                                        sto[nm]->private_data = new pvt_t;
                                        ((pvt_t *)sto[nm]->private_data)->lun = lun;
                                        ((pvt_t *)sto[nm]->private_data)->B = sto_idx;
                                        sto[nm]->Initialize = *UHS_USB_BulkOnly_Initialize;
                                        sto[nm]->Reads = *UHS_USB_BulkOnly_Read;
                                        sto[nm]->Writes = *UHS_USB_BulkOnly_Write;
                                        sto[nm]->Status = *UHS_USB_BulkOnly_Status;
                                        sto[nm]->Commit = *UHS_USB_BulkOnly_Commit;
                                        sto[nm]->TotalSectors = GetCapacity(lun);
                                        sto[nm]->SectorSize = GetSectorSize(lun);
                                        PCPartition *PT = new PCPartition;
                                        if(!PT->Start(sto[nm])) {
                                                delete (pvt_t *)sto[nm]->private_data;
                                                sto[nm]->private_data = NULL;
                                                // partitions exist
                                                part_t *apart;
                                                for(int j = 0; j < 4; j++) {
                                                        nm = f_next_mount();
                                                        if(nm < PFAT_VOLUMES) {
                                                                apart = PT->GetPart(j);
                                                                if(apart != NULL && apart->type != 0x00) {
                                                                        sto[nm]->private_data = new pvt_t;
                                                                        ((pvt_t *)sto[nm]->private_data)->lun = lun;
                                                                        ((pvt_t *)sto[nm]->private_data)->B = sto_idx;
                                                                        sto[nm]->Reads = *UHS_USB_BulkOnly_Read;
                                                                        sto[nm]->Writes = *UHS_USB_BulkOnly_Write;
                                                                        sto[nm]->Status = *UHS_USB_BulkOnly_Status;
                                                                        sto[nm]->Initialize = *UHS_USB_BulkOnly_Initialize;
                                                                        sto[nm]->Commit = *UHS_USB_BulkOnly_Commit;
                                                                        sto[nm]->TotalSectors = GetCapacity(lun);
                                                                        sto[nm]->SectorSize = GetSectorSize(lun);
                                                                        Fats[nm] = new PFAT(sto[nm], nm, apart->firstSector);
                                                                        if(Fats[nm]->MountStatus()) {
                                                                                delete Fats[nm];
                                                                                Fats[nm] = NULL;
                                                                                delete (pvt_t *)sto[nm]->private_data;
                                                                                sto[nm]->private_data = NULL;
                                                                        }
                                                                }
                                                        }
                                                }
                                        } else {
                                                // try superblock
                                                Fats[nm] = new PFAT(sto[nm], nm, 0);
                                                if(Fats[nm]->MountStatus()) {
                                                        delete Fats[nm];
                                                        Fats[nm] = NULL;
                                                        delete (pvt_t *)sto[nm]->private_data;
                                                        sto[nm]->private_data = NULL;
                                                }
                                        }
                                        delete PT;
                                }

                        }
                } else {
                        if(!LUNIsGood(lun)) {
                                // Forced unmount if media ejected
                                for(int f = 0; f < PFAT_VOLUMES; f++) {
                                        if(Fats[f] != NULL) {
                                                if((((pvt_t *)sto[f]->private_data)->B == sto_idx) && (((pvt_t *)sto[f]->private_data)->lun == lun)) {
                                                        delete Fats[f];
                                                        Fats[f] = NULL;
                                                        delete (pvt_t *)sto[f]->private_data;
                                                        sto[f]->private_data = NULL;
                                                }
                                        }
                                }
                        }
                }
                last_ready[sto_idx][lun] = LUNIsGood(lun);
        }
}

void UHS_FS_BULK_DRIVER::OnRelease(void) {
        // Forced unmount of all luns
        kill_mounts(sto_idx);
}

//UHS_FS_BULK_DRIVER *UHS_USB_Storage[MAX_USB_MS_DRIVERS];

/**
 * This must be called before using UHS USB Mass Storage. This works around a G++ bug.
 * Thanks to Lei Shi for the heads up.
 */

static void UHS_USB_BulkOnly_Init(UHS_USB_HOST_BASE *hd) {
        for(int i = 0; i < MAX_USB_MS_DRIVERS; i++) {
                UHS_USB_Storage[i] = new UHS_FS_BULK_DRIVER(hd, i);
        }
}

#endif


// YOUR DRIVERS HERE


extern "C" {
        // Allow init to happen only once in the case of multiple external inits.
        static bool Init_Generic_Storage_inited = false;

        /**
         * This must be called before using generic_storage. Calling more than once is harmless.
         */
        void Init_Generic_Storage(void *hd) {
                if(!Init_Generic_Storage_inited) {
                        Init_Generic_Storage_inited = true;
#ifdef __UHS_BULK_STORAGE_H__
                        UHS_USB_BulkOnly_Init((UHS_USB_HOST_BASE *)hd);

#endif

                        // YOUR INIT HERE

                        for(int i = 0; i < _FS_LOCK; i++) {
                                fhandles[i] = new FIL;
                                dhandles[i] = new DIR;
                                fhandles[i]->fs = NULL;
                                dhandles[i]->fs = NULL;
                        }

                        for(int i = 0; i < MAX_USB_MS_DRIVERS; i++) {
                                for(int j = 0; j < MASS_MAX_SUPPORTED_LUN; j++) {
                                        last_ready[i][j] = false;
                                }
                        }

                        for(int i = 0; i < PFAT_VOLUMES; i++) {
                                sto[i] = new storage_t;
                                sto[i]->private_data = NULL;
                                Fats[i] = NULL;
                        }
                }
        }

        char * AJK_NI fs_mount_lbl(uint8_t vol) {
                uint8_t rc;
                char *lbl = NULL;
                rc = FR_NO_PATH;
                if(Fats[vol]) {
                        rc = FR_OK;
                        lbl = (char *)malloc(1 + strlen((char *)(Fats[vol]->label)));
                        strcpy(lbl, (char *)(Fats[vol]->label));
                }
                fs_err = rc;
                return lbl;
        }

        /**
         * Return a pointer to path name with mount point stripped, unless
         * the mount point is '/'
         *
         * @param path
         * @param vol
         * @return pointer to path name
         */
        const char * AJK_NI _fs_util_trunkpath(const char *path, uint8_t vol) {

                const char *pathtrunc = path;
                char *s = fs_mount_lbl(vol);
                if(s != NULL) {
                        pathtrunc += strlen(s);
                        free(s);
                }
                return pathtrunc;
        }

        /**
         * Check if mount point is ready
         *
         * @param path mount point path
         * @return volume number, or PFAT_VOLUMES on error
         */
        uint8_t AJK_NI fs_ready(const char *path) {
                // path -> volume number
                uint8_t vol;

                vol = PFAT_VOLUMES;
                fs_err = FR_NO_PATH;
                for(uint8_t x = 0; x < PFAT_VOLUMES; x++) {
                        if(Fats[x]) {
                                if(!strcasecmp((char *)Fats[x]->label, path)) {
                                        fs_err = FR_OK;
                                        vol = Fats[x]->volmap; // should be equal to x
                                        break;
                                }
                        }
                }
                return vol;
        }

        /**
         * Convert a path to a volume number.
         *
         * @param path
         * @return volume number, or PFAT_VOLUMES on error
         *
         */
        uint8_t AJK_NI _fs_util_vol(const char *path) {
                uint8_t *ptr;
                char *fname;
                uint8_t vol = PFAT_VOLUMES;

                if((strlen(path) < 1) || *path != '/' || strstr(path, "/./") || strstr(path, "/../") || strstr(path, "//")) {
                        fs_err = FR_INVALID_NAME;
                } else {
                        fname = (char *)malloc(strlen(path) + 1);
                        *fname = 0x00;
                        strcat(fname, path);
                        ptr = (uint8_t *)fname;
                        ptr++; // skip first '/'
                        while(*ptr) {
                                if(*ptr == '/') {
                                        break;
                                }
                                ptr++;
                        }
                        if(*ptr) {
                                *ptr = 0x00;
                                vol = fs_ready((char *)fname);
                        }
                        // Could be a sub-dir in the path, so try just the root.
                        if(vol == PFAT_VOLUMES) {
                                // root of "/"
                                vol = fs_ready("/");
                        }
                        free(fname);
                }
                return vol;
        }

        /**
         * Return a pointer to path name suitable for FATfs
         *
         * @param path
         * @param vol
         * @return pointer to new path name, must be freed by caller
         */
        const char * AJK_NI _fs_util_FATpath(const char *path, uint8_t vol) {
                char *FATpath = (char *)malloc(3 + strlen(path));
                char *ptr = FATpath;
                *ptr = '0' + vol;
                ptr++;
                *ptr = ':';
                ptr++;
                *ptr = 0;
                strcat(FATpath, path);
                const char *rv = FATpath;
                return rv;
        }

        // functions

        uint8_t AJK_NI fs_opendir(const char *path) {

                uint8_t fd;
                uint8_t rc;
                const char *pathtrunc;
                uint8_t vol = _fs_util_vol(path);

                if(vol == PFAT_VOLUMES) {
                        fs_err = FR_NO_PATH;
                        return 0;
                }


                pathtrunc = _fs_util_trunkpath(path, vol);
                fd = 0;
                int i;

                for(i = 0; i < _FS_LOCK; i++) {
                        if(dhandles[i]->fs == NULL) break;
                }
                if(i < _FS_LOCK) {
                        fd = 1 + i + _FS_LOCK;
                        const char *ptr = _fs_util_FATpath(pathtrunc, vol);
                        rc = f_opendir(dhandles[i], (TCHAR *)ptr);
                        free((void *)ptr);
                        if(rc != FR_OK) {
                                dhandles[i]->fs = NULL;
                                fd = 0;
                                fs_err = FR_NO_PATH;
                        }
                }
                return fd;
        }

        /**
         * Open a file. An absolute, fully resolved canonical path is required!
         * Relative paths and links are _not_ supported.
         *
         * @param path
         * @param mode one of rRwWxX
         * @return returns file handle number, or 0 on error
         */
        uint8_t AJK_NI _fs_open(const char *path, const char *mode) {
                uint8_t fd = 0;
                uint8_t rc = FR_NO_PATH;
                const char *pathtrunc;

                uint8_t vol = _fs_util_vol(path);

                if(vol != PFAT_VOLUMES) {
                        pathtrunc = _fs_util_trunkpath(path, vol);

                        int i;

                        for(i = 0; i < _FS_LOCK; i++) {
                                if(fhandles[i]->fs == NULL) break;
                        }
                        if(i < _FS_LOCK) {
                                fd = i + 1;
                                const char *name = _fs_util_FATpath(pathtrunc, vol);
                                switch(*mode) {
                                        case 'r':
                                                rc = f_open(fhandles[i], name, FA_READ);
                                                break;
                                        case 'w':
                                                rc = f_open(fhandles[i], name, FA_WRITE | FA_CREATE_ALWAYS);
                                                break;
                                        case 'x':
                                                rc = f_open(fhandles[i], name, FA_READ | FA_WRITE);
                                                break;
                                        case 'W':
                                                rc = f_open(fhandles[i], name, FA_WRITE);
                                                break;
                                        case 'X':
                                                rc = f_open(fhandles[i], name, FA_READ | FA_WRITE | FA_CREATE_ALWAYS);
                                                break;
                                }
                                free((void *)name);
                                if(rc != FR_OK) {
                                        fhandles[i]->fs = NULL;
                                        fd = 0;
                                }
                        }
                }
                fs_err = rc;
                return fd;
        }

        /**
         * Close file
         * @param fd file handle to close
         *
         * @return FRESULT, 0 = success
         */
        int AJK_NI fs_close(uint8_t fd) {
                uint8_t rc;
                fd--;

                if(fd < _FS_LOCK && fhandles[fd]->fs != NULL) {
                        rc = f_close(fhandles[fd]);
                        fhandles[fd]->fs = NULL;
                } else rc = FR_INVALID_PARAMETER;
                fs_err = rc;
                return rc;
        }

        /**
         * Close directory
         *
         * @param dh directory handle to close
         * @return FRESULT, 0 = success
         */
        int AJK_NI fs_closedir(uint8_t dh) {
                dh -= _FS_LOCK + 1;
                int rv = FR_OK;
                if(dh < _FS_LOCK) {
                        dhandles[dh]->fs = NULL;
                } else {
                        rv = FR_INVALID_PARAMETER;
                }
                fs_err = rv;
                return rv;
        }

        int AJK_NI fs_readdir(uint8_t fd, PFAT_DIRINFO *data) {
                uint8_t rc;
                fd -= _FS_LOCK + 1;
                fs_err = FR_EOF;
                if(fd < _FS_LOCK && dhandles[fd]->fs != NULL) {
                        FILINFO finfo;
#if _USE_LFN
                        char lfn[_MAX_LFN + 1];
                        finfo.lfsize = _MAX_LFN;
#else
                        char lfn[1];
                        finfo.lfsize = 0;
#endif
                        finfo.lfname = lfn;
                        rc = f_readdir(dhandles[fd], &finfo);
                        if(rc == FR_OK) {
                                if(finfo.fname[0]) {
                                        fs_err = FR_OK;
                                        data->fsize = finfo.fsize;
                                        data->fdate = finfo.fdate;
                                        data->ftime = finfo.ftime;
                                        data->fattrib = finfo.fattrib;
                                        strcpy((char *)data->fname, (char *)finfo.fname);
                                        data->lfname[0] = 0x00;
#if _USE_LFN
                                        if(finfo.lfsize) {
                                                strcat((char *)data->lfname, (char *)finfo.lfname);
                                        }
#endif
                                }
                        }
                }
                return fs_err;
        }

        /**
         * Flush all files that have pending write data to disk.
         *
         * @return FRESULT, 0 = success
         */
        uint8_t AJK_NI fs_sync(void) {
                uint8_t rc;
                for(int i = 0; i < _FS_LOCK; i++) {
                        if(fhandles[i]->fs != NULL) {
                                f_sync(fhandles[i]);
                        }
                }
                for(int i = 0; i < PFAT_VOLUMES; i++) {
                        if(Fats[i] != NULL) {
                                f_sync_fs(Fats[i]->ffs);
                                commit_fs(Fats[i]->ffs);
                        }
                }
                rc = FR_OK;
                fs_err = rc;
                return rc;
        }

        /**
         * Flush pending writes to disk for a specific file.
         *
         * @param fd file handle
         * @return FRESULT, 0 = success
         */
        uint8_t AJK_NI fs_flush(uint8_t fd) {
                uint8_t rc;
                fd--;
                rc = FR_INVALID_PARAMETER;
                if(fd < _FS_LOCK && fhandles[fd]->fs != NULL) {
                        rc = FR_OK;
                        f_sync(fhandles[fd]);
                }
                fs_err = rc;
                return rc;
        }

        /**
         * Check to see if we are at the end of the file.
         *
         * @param fd file handle
         * @return 0 = not eof, 1 = eof
         */
        uint8_t AJK_NI fs_eof(uint8_t fd) {
                uint8_t rc;
                fd--;
                rc = FR_INVALID_PARAMETER;
                if(fd < _FS_LOCK && fhandles[fd]->fs != NULL) {
                        rc = f_eof(fhandles[fd]);
                }
                fs_err = rc;
                return rc;
        }

        /**
         * Truncate file to current position.
         *
         * @param fd file handle
         * @return FRESULT, 0 = success
         */
        uint8_t AJK_NI fs_truncate(uint8_t fd) {
                uint8_t rc;

                fd--;

                rc = FR_INVALID_PARAMETER;
                if(fd < _FS_LOCK && fhandles[fd]->fs != NULL) {
                        rc = f_truncate(fhandles[fd]);
                }
                fs_err = rc;

                return rc;
        }

        /**
         * Get current position in file
         *
         * @param fd file handle
         * @return current position in file, 0xfffffffflu == error
         */
        unsigned long AJK_NI fs_tell(uint8_t fd) {
                uint8_t rc;
                unsigned long offset = 0xfffffffflu;
                fd--;

                rc = FR_INVALID_PARAMETER;
                if(fd < _FS_LOCK && fhandles[fd]->fs != NULL) {
                        rc = FR_OK;
                        // f_tell is a macro...
                        offset = f_tell(fhandles[fd]);
                }
                fs_err = rc;

                return offset;
        }

        /**
         * Seek to position in file.
         *      SEEK_SET The offset is set to offset bytes.
         *      SEEK_CUR The offset is set to its current location plus offset bytes.
         *      SEEK_END The offset is set to the size of the file plus offset bytes.
         *
         * @param fd file handle
         * @param offset
         * @param whence one of SEEK_SET, SEEK_CUR, SEEK_END
         * @return FRESULT, 0 = success
         */
        uint8_t AJK_NI fs_lseek(uint8_t fd, unsigned long offset, int whence) {
                uint8_t rc;
                fd--;
                rc = FR_INVALID_PARAMETER;
                if(fd < _FS_LOCK && fhandles[fd]->fs != NULL) {
                        rc = f_clseek(fhandles[fd], offset, whence);
                }
                fs_err = rc;
                return rc;
        }

        /**
         * Read from file
         *
         * @param fd file handle
         * @param data pointer to a buffer large enough to hold requested amount
         * @param amount
         * @return amount actually read in, less than 0 is an error
         */
        int AJK_NI fs_read(uint8_t fd, void *data, uint16_t amount) {
                uint8_t rc;
                int count = 0;
                fd--;

                rc = FR_INVALID_PARAMETER;
                if((fd < PFAT_VOLUMES) && (fhandles[fd]->fs != NULL)) {
                        UINT bc;
                        rc = f_read(fhandles[fd], data, amount, &bc);
                        if(bc != amount) rc = FR_EOF;
                        count = bc;
                        if(rc) {
                                if(!count) count = -1;
                        }
                }
                fs_err = rc;

                return count;
        }

        /**
         * Write to file
         *
         * @param fd file handle
         * @param data pointer to a buffer
         * @param amount
         * @return amount of data actually written, less than 0 is an error.
         */
        int AJK_NI fs_write(uint8_t fd, const void *data, uint16_t amount) {
                uint8_t rc;
                int count = 0;
                fd--;

                rc = FR_INVALID_PARAMETER;
                if((fd < PFAT_VOLUMES) && (fhandles[fd]->fs != NULL)) {
                        UINT bc;
                        rc = f_write(fhandles[fd], data, amount, &bc);
                        count = bc;
                        if(rc) {
                                if(!count) count = -1;
                        }
                }
                fs_err = rc;


                return count;
        }

        /**
         * Remove (delete) file or directory. An absolute, fully resolved canonical path is required!
         * Relative paths and links are _not_ supported.
         *
         * @param path
         * @return FRESULT, 0 = success
         */
        uint8_t AJK_NI fs_unlink(const char *path) {

                const char *pathtrunc;
                uint8_t vol = _fs_util_vol(path);
                uint8_t rc;

                if(vol == PFAT_VOLUMES) {
                        rc = FR_NO_PATH;
                } else {
                        pathtrunc = _fs_util_trunkpath(path, vol);
                        const char *name = _fs_util_FATpath(pathtrunc, vol);
                        rc = f_unlink(name);
                        free((void *)name);

                }
                fs_err = rc;

                return rc;
        }

        /**
         * Change attribute flags on a file.
         *
         * @param path file or directory
         * @param mode AM_RDO|AM_ARC|AM_SYS|AM_HID in any combination
         * @return FRESULT, 0 = success
         */
        uint8_t AJK_NI fs_chmod(const char *path, uint8_t mode) {
                uint8_t rc = FR_NO_PATH;
                const char *pathtrunc;
                uint8_t vol = _fs_util_vol(path);

                if(vol != PFAT_VOLUMES) {
                        pathtrunc = _fs_util_trunkpath(path, vol);
                        const char *name = _fs_util_FATpath(pathtrunc, vol);
                        rc = f_chmod((TCHAR *)name, mode, AM_RDO | AM_ARC | AM_SYS | AM_HID);
                        free((void *)name);
                }
                fs_err = rc;
                return rc;
        }

        /**
         * Create a new directory
         *
         * @param path new directory
         * @param mode AM_RDO|AM_ARC|AM_SYS|AM_HID in any combination
         * @return FRESULT, 0 = success
         */
        uint8_t AJK_NI fs_mkdir(const char *path, uint8_t mode) {
                uint8_t rc = FR_NO_PATH;
                const char *pathtrunc;

                uint8_t vol = _fs_util_vol(path);

                if(vol != PFAT_VOLUMES) {
                        pathtrunc = _fs_util_trunkpath(path, vol);
                        const char *name = _fs_util_FATpath(pathtrunc, vol);
                        rc = f_mkdir((TCHAR *)name);
                        if((rc == FR_OK) && mode) {
                                rc = f_chmod((TCHAR *)name, mode, AM_RDO | AM_ARC | AM_SYS | AM_HID);
                        }
                        free((void *)name);
                }
                return rc;
        }

        /**
         * Modify file time stamp.
         *
         * @param path File to update time stamp
         * @param timesec time in seconds since 1/1/1900
         * @return FRESULT, 0 = success
         */
        uint8_t AJK_NI fs_utime(const char *path, time_t timesec) {
                uint8_t rc = FR_NO_PATH;
                const char *pathtrunc;
                uint8_t vol = _fs_util_vol(path);

                if(vol != PFAT_VOLUMES) {

                        pathtrunc = _fs_util_trunkpath(path, vol);
                        const char *name = _fs_util_FATpath(pathtrunc, vol);
                        FILINFO finfo;
                        finfo.fdate = timesec;
                        rc = f_utime((TCHAR *)name, &finfo);
                        free((void *)name);
                }
                fs_err = rc;
                return rc;

        }

        /**
         * Return information about a file.
         *
         * @param path File to return information about
         * @param buf pointer to FILINFO structure, which is filled in upon success.
         * @return FRESULT, 0 = success
         */
        uint8_t AJK_NI fs_stat(const char *path, FILINFO *buf) {
                uint8_t rc = FR_NO_PATH;
                const char *pathtrunc;
                uint8_t vol = _fs_util_vol(path);
                if(vol != PFAT_VOLUMES) {

                        pathtrunc = _fs_util_trunkpath(path, vol);
                        const char *name = _fs_util_FATpath(pathtrunc, vol);
                        rc = f_stat((TCHAR *)name, buf);
                        free((void *)name);
                }
                fs_err = rc;
                return rc;
        }

        /**
         * Rename a file, moving it between directories if required.
         * Does not move a file between mounted volumes.
         *
         * @param oldpath old file name
         * @param newpath new file name
         * @return FRESULT, 0 = success
         */
        uint8_t AJK_NI fs_rename(const char *oldpath, const char *newpath) {
                uint8_t rc = FR_NO_PATH;

                uint8_t vol = _fs_util_vol(oldpath);
                if(vol != PFAT_VOLUMES && vol == _fs_util_vol(newpath)) {
                        const char *oldpathtrunc = _fs_util_trunkpath(oldpath, vol);
                        const char *newpathtrunc = _fs_util_trunkpath(newpath, vol);

                        const char *oldname = _fs_util_FATpath(oldpathtrunc, vol);

                        rc = f_rename(oldname, newpathtrunc);
                        free((void *)oldname);
                }
                fs_err = rc;
                return rc;
        }

        /**
         * Get free space available on mount point.
         *
         * @param path mount point
         * @return free space available in bytes. If there is no volume or write protected, returns zero (full).
         */
        uint64_t AJK_NI fs_getfree(const char *path) {
                uint8_t rc = FR_NO_PATH;
                uint8_t vol = _fs_util_vol(path);
                uint64_t rv = 0llu;
                if(vol != PFAT_VOLUMES) {
                        FATFS *fs;
                        uint32_t tv;
                        const char *name = _fs_util_FATpath("", vol);
                        rc = f_getfree((TCHAR *)name, &tv, &fs);
                        free((void *)name);
                        rv = tv; // blocks
                        rv *= fs->csize; // sectors in each block
                        rv *= fs->pfat->storage->SectorSize; // bytes in each sector
                        if(rc) rv = 0llu;
                }
                fs_err = rc;
                return (uint64_t)rv;
        }

        uint8_t AJK_NI fs_mountcount(void) {
                uint8_t message = 0;

                fs_err = FR_OK;
                for(uint8_t x = 0; x < PFAT_VOLUMES; x++) {
                        if(Fats[x]) {
                                message++;
                        }
                }
                return message;
        }

        /**
         *
         * @param pathname path to file
         * @param flags Zero is treated as O_RDONLY, one of [0 | O_RDONLY | O_WRONLY | O_RDWR] is mandatory.
         *              Additional flags are  O_CREAT, O_APPEND and O_TRUNC.
         *              If you use O_RDONLY and O_CREAT together, it will be opened as O_RDWR.
         * @return file file system descriptor
         */
        uint8_t AJK_NI fs_open(const char *pathname, int flags) {
                int rv;
                char mode;
                char mode2;
                uint8_t s;
                // mode r
                // mode w/W
                // mode x/X (r/w)
                // W and X always create the file.
                if(!flags) flags = O_RDONLY;
                if((flags & (O_RDWR | O_CREAT)) == (O_RDONLY | O_CREAT)) flags |= O_WRONLY;
                switch(flags & (O_RDWR)) {
                        case O_RDONLY:
                                mode = 'r';
                                break;

                        case O_WRONLY:
                                mode = 'w';
                                mode2 = 'W';
                                break;

                        case O_RDWR:
                                mode = 'x';
                                mode2 = 'X';
                                break;

                        default:
                                return -1;
                }
                rv = _fs_open(pathname, &mode);
                if((rv == 0) && (flags & O_CREAT)) {
                        // try to create
                        rv = _fs_open(pathname, &mode2);
                }
                if(rv != 0) {
                        if(flags & O_WRONLY) {
                                switch(flags & (O_APPEND | O_TRUNC)) {
                                        case O_APPEND:
                                                // seek to eof
                                                s = fs_lseek(rv, 0, SEEK_END);
                                                if(s) {
                                                        fs_close(rv);
                                                        rv = -1;
                                                }
                                                break;
                                        case O_TRUNC:
                                                // truncate file
                                                s = fs_truncate(rv);
                                                if(s) {
                                                        fs_close(rv);
                                                        rv = -1;
                                                }
                                                break;
                                }
                        }
                } else {
                        rv = -1;
                }
                return rv;
        }
}

#endif
