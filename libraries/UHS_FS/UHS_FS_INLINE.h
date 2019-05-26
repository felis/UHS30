/* Copyright (C) 2015-2016 Andrew J. Kroll
   and
Copyright (C) 2011 Circuits At Home, LTD. All rights reserved.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

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

#ifdef __UHS_BULK_STORAGE_H__
        static bool last_ready[MAX_USB_MS_DRIVERS][MASS_MAX_SUPPORTED_LUN];

        static void none_ready(int idx) {
                for(int j = 0; j < MASS_MAX_SUPPORTED_LUN; j++) {
                        last_ready[idx][j] = false;
                }
        }

#endif

#ifdef UHS_USE_SDCARD

        MAKE_SD_ISR_REFS();

        void (* UHS_FS_SD_ISR[UHS_MAX_SD_CARDS])(void) = {
                AJK_MAKE_LIST(SD_ISR, UHS_MAX_SD_CARDS)
        };

        static bool SD_last_ready[UHS_MAX_SD_CARDS];

        static void SD_none_ready(int idx) {
                SD_last_ready[idx] = false;
        }
#endif

#ifdef __UHS_BULK_STORAGE_H__

        static void kill_mounts(int idx, int drv) {
                for(int f = 0; f < PFAT_VOLUMES; f++) {
                        if(Fats[f] != NULL) {
                                if(sto[f]->driver_type == drv) {
                                        if(drv == 0) {
                                                if(((pvt_t *)sto[f]->private_data)->B == idx) {
                                                        delete Fats[f];
                                                        Fats[f] = NULL;
                                                        delete (pvt_t *)sto[f]->private_data;
                                                        sto[f]->private_data = NULL;
                                                        none_ready(idx);
                                                }
                                        }
                                }
                        }
                }
        }
#endif

#ifdef UHS_USE_SDCARD

        DSTATUS UHS_SD_Initialize(storage_t *sto) {
                if((UHS_SD_Storage[((SDpvt_t *)sto->private_data)->B]->WriteProtected())) {
                        return STA_PROTECT;
                } else {
                        return STA_OK;
                }
        }

        bool UHS_SD_Status(storage_t *sto) {
                return (UHS_SD_Storage[((SDpvt_t *)sto->private_data)->B]->WriteProtected());
        }

        // TO-DO: cache?

        int UHS_SD_Read(uint32_t LBA, uint8_t *buf, storage_t *sto, uint8_t count) {
                uint8_t x = 0;
                while(count) {
                        int tries = FAT_MAX_ERROR_RETRIES;
                        while(tries) {
                                tries--;
                                x = (UHS_SD_Storage[((SDpvt_t *)sto->private_data)->B]->Read(LBA, (sto->SectorSize), 1, buf));
                                //if(x == UHS_BULK_ERR_DEVICE_DISCONNECTED) break;
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

        // TO-DO: cache?

        int UHS_SD_Write(uint32_t LBA, uint8_t *buf, storage_t *sto, uint8_t count) {
                uint8_t x = 0;
                while(count && !x) {
                        int tries = FAT_MAX_ERROR_RETRIES;
                        while(tries) {
                                tries--;
                                x = (UHS_SD_Storage[((SDpvt_t *)sto->private_data)->B]->Write(LBA, (sto->SectorSize), 1, buf));
                                //if(x == UHS_BULK_ERR_WRITE_PROTECTED) break;
                                //if(x == UHS_BULK_ERR_DEVICE_DISCONNECTED) break;
                                if(!x) break;
                        }
                        // TO-DO: WRITE caching?
                        count--;
                        buf += (sto->SectorSize);
                        LBA++;
                }
                int y = x;
                return y;
        }

        uint8_t UHS_SD_Commit(NOTUSED(storage_t *sto)) {
                uint8_t x = 0;
                return x;
        }
#endif

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

        // TO-DO: cache?

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

        // TO-DO: cache?

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
                        // TO-DO: WRITE caching?
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
#endif
}


#ifdef __UHS_BULK_STORAGE_H__

UHS_FS_BULK_DRIVER::UHS_FS_BULK_DRIVER(UHS_USB_HOST_BASE *p, int i) : UHS_Bulk_Storage(p) {
        sto_idx = i;
        none_ready(sto_idx);
}

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
                                        sto[nm]->driver_type = 0;
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
                                                                        sto[nm]->Initialize = *UHS_USB_BulkOnly_Initialize;
                                                                        sto[nm]->Reads = *UHS_USB_BulkOnly_Read;
                                                                        sto[nm]->Writes = *UHS_USB_BulkOnly_Write;
                                                                        sto[nm]->Status = *UHS_USB_BulkOnly_Status;
                                                                        sto[nm]->Commit = *UHS_USB_BulkOnly_Commit;
                                                                        sto[nm]->TotalSectors = GetCapacity(lun);
                                                                        sto[nm]->SectorSize = GetSectorSize(lun);
                                                                        sto[nm]->driver_type = 0;

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
                                                if(sto[f]->driver_type == 0) {
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
                }
                last_ready[sto_idx][lun] = LUNIsGood(lun);
        }
}

void UHS_FS_BULK_DRIVER::OnRelease(void) {
        // Forced unmount of all luns
        kill_mounts(sto_idx, 0);
}

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

#ifdef UHS_USE_SDCARD

UHS_SD::UHS_SD(int _present, int _cs) {
        present = _present;
        cs = _cs;
        pinMode(cs, OUTPUT);
        digitalWrite(cs, HIGH);
        pinMode(present, INPUT_PULLUP);
}

bool UHS_SD::waitNotBusy(uint16_t timeoutMillis) {
        uint16_t t0 = millis();
        do {
                if(SPI.transfer(0xFF) == 0XFF) return true;
        } while(((uint16_t)millis() - t0) < timeoutMillis);
        return false;

}

uint8_t UHS_SD::cardCommand(uint8_t cmd, uint32_t arg) {
        //readEnd();
        //chipSelectLow();

        waitNotBusy(300);
        SPI.transfer(cmd | 0x40);
        for(int8_t s = 24; s >= 0; s -= 8) SPI.transfer(arg >> s);
        uint8_t crc = 0XFF;
        if(cmd == UHS_SD_CMD0) crc = 0X95; // correct crc for CMD0 with arg 0
        if(cmd == UHS_SD_CMD8) crc = 0X87; // correct crc for CMD8 with arg 0X1AA
        SPI.transfer(crc);
        for(uint8_t i = 0; ((status_ = SPI.transfer(0xFF)) & 0X80) && i != 0XFF; i++);
        return status_;
}

bool UHS_SD::readRegister(uint8_t cmd, void* buf) {
        uint8_t* dst = reinterpret_cast<uint8_t*>(buf);
        // Trans
        SPI.beginTransaction(settings);
        digitalWrite(cs, LOW);
        if(cardCommand(cmd, 0)) {
                error(UHS_SD_CARD_ERROR_READ_REG);
                goto fail;
        }
        if(!waitStartBlock()) goto fail;
        // transfer data
        for(uint16_t i = 0; i < 16; i++) dst[i] = SPI.transfer(0xFF);
        SPI.transfer(0xFF); // get first crc byte
        SPI.transfer(0xFF); // get second crc byte
        digitalWrite(cs, HIGH);
        SPI.endTransaction();
        return true;

fail:
        digitalWrite(cs, HIGH);
        SPI.endTransaction();
        return false;
}

bool UHS_SD::waitStartBlock(void) {
        uint16_t t0 = millis();

        while((status_ = SPI.transfer(0xFF)) == 0XFF) {
                if(((uint16_t)millis() - t0) > UHS_SD_READ_TIMEOUT) {
                        error(UHS_SD_CARD_ERROR_READ_TIMEOUT);
                        goto fail;
                }
        }
        if(status_ != UHS_SD_DATA_START_BLOCK) {
                error(UHS_SD_CARD_ERROR_READ);
                goto fail;
        }
        return true;

fail:
        digitalWrite(cs, HIGH);

        return false;
}

uint32_t UHS_SD::cardSize(void) {
        UHS_SD_csd_t csd;
        if(!readCSD(&csd)) return 0;
        if(csd.v1.csd_ver == 0) {
                uint8_t read_bl_len = csd.v1.read_bl_len;
                uint16_t c_size = (csd.v1.c_size_high << 10)
                        | (csd.v1.c_size_mid << 2) | csd.v1.c_size_low;
                uint8_t c_size_mult = (csd.v1.c_size_mult_high << 1)
                        | csd.v1.c_size_mult_low;
                return (uint32_t)(c_size + 1) << (c_size_mult + read_bl_len - 7);
        } else if(csd.v2.csd_ver == 1) {
                uint32_t c_size = ((uint32_t)csd.v2.c_size_high << 16)
                        | (csd.v2.c_size_mid << 8) | csd.v2.c_size_low;
                return (c_size + 1) << 10;
        } else {
                error(UHS_SD_CARD_ERROR_BAD_CSD);

                return 0;
        }
}

uint8_t UHS_SD::Read(uint32_t addr, uint16_t bsize, uint8_t blocks, uint8_t *buf) {
        if(bsize != 512) return FR_DISK_ERR;
        uint16_t j = 0;
        uint32_t xaddr;
        while(blocks--) {
                xaddr = addr;
                // use address if not SDHC card
                if(type() != UHS_SD_CARD_TYPE_SDHC) xaddr <<= 9;
                SPI.beginTransaction(settings);
                digitalWrite(cs, LOW);
                // Trans
                if(cardCommand(UHS_SD_CMD17, xaddr)) {
                        error(UHS_SD_CARD_ERROR_CMD17);
                        goto fail;
                }
                if(!waitStartBlock()) {
                        goto fail;
                }
                // transfer data
                for(uint16_t i = 0; i < bsize; i++, j++) {
                        buf[j] = SPI.transfer(0xFF);
                }
                addr++;
                digitalWrite(cs, HIGH);
                SPI.endTransaction();
        }
        return 0;

fail:
        digitalWrite(cs, HIGH);
        SPI.endTransaction();
        return FR_DISK_ERR;
}

bool UHS_SD::writeData(uint8_t token, const uint8_t* src) {
        SPI.transfer(token);
        for(uint16_t i = 0; i < 512; i++) {
                SPI.transfer(src[i]);
        }
        SPI.transfer(0xff); // dummy crc
        SPI.transfer(0xff); // dummy crc

        status_ = SPI.transfer(0xff);
        if((status_ & UHS_SD_DATA_RES_MASK) != UHS_SD_DATA_RES_ACCEPTED) {
                error(UHS_SD_CARD_ERROR_WRITE);
                digitalWrite(cs, HIGH);
                return false;
        }
        return true;
}

bool UHS_SD::writeBlock(uint32_t blockNumber, const uint8_t* src) {

        // use address if not SDHC card
        if(type() != UHS_SD_CARD_TYPE_SDHC) blockNumber <<= 9;
        // Trans
        SPI.beginTransaction(settings);
        digitalWrite(cs, LOW);
        if(cardCommand(UHS_SD_CMD24, blockNumber)) {
                error(UHS_SD_CARD_ERROR_CMD24);
                goto fail;
        }
        if(!writeData(UHS_SD_DATA_START_BLOCK, src)) goto fail;

        // wait for flash programming to complete
        if(!waitNotBusy(UHS_SD_WRITE_TIMEOUT)) {
                error(UHS_SD_CARD_ERROR_WRITE_TIMEOUT);
                goto fail;
        }
        // response is r2 so get and check two bytes for nonzero
        if(cardCommand(UHS_SD_CMD13, 0) || SPI.transfer(0xFF)) {
                error(UHS_SD_CARD_ERROR_WRITE_PROGRAMMING);
                goto fail;
        }
        digitalWrite(cs, HIGH);
        SPI.endTransaction();
        return true;

fail:
        digitalWrite(cs, HIGH);
        SPI.endTransaction();
        return false;
}

uint8_t UHS_SD::Write(uint32_t addr, uint16_t bsize, uint8_t blocks, const uint8_t *buf) {
        if(bsize != 512) return true;
        while(blocks--) {
                if(!writeBlock(addr, buf)) return FR_DISK_ERR;
                addr++;
                buf += 512;
        }
        return 0;
}

UHS_FS_SD_DRIVER::UHS_FS_SD_DRIVER(int _present, int _cs, int i) : UHS_SD(_present, _cs) {

        sto_idx = i;
        SD_none_ready(sto_idx);
        SD_last_ready[sto_idx] = !Present(); // Invert to force probe if SDcard is present.
}

/**
 *  This is equal to mass storage OnPoll detect phase
 */
void UHS_FS_SD_DRIVER::IRQ(void) {
        delay(100); // debounce...
        bool sense = Present();
        if(sense != SD_last_ready[sto_idx]) {
                SD_last_ready[sto_idx] = sense;
                if(sense) {
                        uint16_t t0 = (uint16_t)millis();
                        uint32_t arg;

                        //printf("Card %d Inserted\r\n", sto_idx);
                        bool ok = false;
                        for(int j = 0; j < 10; j++) {
                                // needs to retry for reliability.
                                // Do card detection.
                                // This equals USB connection check.
                                settings = SPISettings(400000, MSBFIRST, SPI_MODE0);
                                SPI.beginTransaction(settings);
                                digitalWrite(cs, HIGH);
                                for(uint8_t i = 0; i < 10; i++) SPI.transfer(0xff);
                                digitalWrite(cs, LOW);
                                while((status_ = cardCommand(UHS_SD_CMD0, 0)) != UHS_SD_R1_IDLE_STATE) {
                                        if(((uint16_t)(millis() - t0)) > UHS_SD_INIT_TIMEOUT) {
                                                error(UHS_SD_CARD_ERROR_CMD0);
                                                goto fail;
                                        }
                                }
                                //printf("CSDV\r\n");
                                // check SD version
                                if((cardCommand(UHS_SD_CMD8, 0x1AA) & UHS_SD_R1_ILLEGAL_COMMAND)) {
                                        type(UHS_SD_CARD_TYPE_SD1);
                                } else {
                                        // only need last byte of r7 response
                                        for(uint8_t i = 0; i < 4; i++) status_ = SPI.transfer(0xff);
                                        if(status_ != 0XAA) {
                                                error(UHS_SD_CARD_ERROR_CMD8);
                                                goto fail;
                                        }
                                        type(UHS_SD_CARD_TYPE_SD2);
                                }
                                //printf("ACV %d\r\n",(type() == UHS_SD_CARD_TYPE_SD2 ? 1 : 0));
                                // initialize card and send host supports SDHC if SD2
                                arg = type() == UHS_SD_CARD_TYPE_SD2 ? 0X40000000 : 0;

                                while((status_ = cardAcmd(UHS_SD_ACMD41, arg)) != UHS_SD_R1_READY_STATE) {
                                        // check for timeout
                                        if(((uint16_t)(millis() - t0)) > UHS_SD_INIT_TIMEOUT) {
                                                error(UHS_SD_CARD_ERROR_ACMD41);
                                                goto fail;
                                        }
                                }
                                //printf("OCR\r\n");
                                // if SD2 read OCR register to check for SDHC card
                                if(type() == UHS_SD_CARD_TYPE_SD2) {
                                        if(cardCommand(UHS_SD_CMD58, 0)) {
                                                error(UHS_SD_CARD_ERROR_CMD58);
                                                goto fail;
                                        }
                                        if((SPI.transfer(0xff) & 0XC0) == 0XC0) type(UHS_SD_CARD_TYPE_SDHC);
                                        // discard rest of ocr - contains allowed voltage range
                                        for(uint8_t i = 0; i < 3; i++) SPI.transfer(0xff);
                                }

                                digitalWrite(cs, HIGH);
                                SPI.endTransaction();
                                ok = true;
                                break;
                                fail:
                                digitalWrite(cs, HIGH);
                                SPI.endTransaction();
                        }
                        if(!ok) {
                                //printf("Failed to init card\r\n");
                                digitalWrite(cs, HIGH);
                                SPI.endTransaction();
                                return;
                        }
                        // card OK
                        // Spec, 0 to 25MHz, try for top speed.
                        settings = SPISettings(25000000, MSBFIRST, SPI_MODE0);
                        CurrentCapacity = cardSize();
                        //printf("Card sectors: %ld\r\n", GetCapacity());

                        int nm = (int)f_next_mount();
                        if(nm < PFAT_VOLUMES) {
                                sto[nm]->private_data = new SDpvt_t;
                                ((SDpvt_t *)sto[nm]->private_data)->B = sto_idx;
                                sto[nm]->Initialize = *UHS_SD_Initialize;
                                sto[nm]->Reads = *UHS_SD_Read;
                                sto[nm]->Writes = *UHS_SD_Write;
                                sto[nm]->Status = *UHS_SD_Status;
                                sto[nm]->Commit = *UHS_SD_Commit;
                                sto[nm]->TotalSectors = GetCapacity();
                                sto[nm]->SectorSize = GetSectorSize();
                                sto[nm]->driver_type = 1;
                                PCPartition *PT = new PCPartition;
                                if(!PT->Start(sto[nm])) {
                                        delete (SDpvt_t *)sto[nm]->private_data;
                                        sto[nm]->private_data = NULL;
                                        // partitions exist
                                        //printf("Partitions exist...\r\n");
                                        part_t *apart;
                                        for(int j = 0; j < 4; j++) {
                                                nm = f_next_mount();
                                                if(nm < PFAT_VOLUMES) {
                                                        apart = PT->GetPart(j);
                                                        if(apart != NULL && apart->type != 0x00) {
                                                                sto[nm]->private_data = new SDpvt_t;
                                                                ((SDpvt_t *)sto[nm]->private_data)->B = sto_idx;
                                                                sto[nm]->Initialize = *UHS_SD_Initialize;
                                                                sto[nm]->Reads = *UHS_SD_Read;
                                                                sto[nm]->Writes = *UHS_SD_Write;
                                                                sto[nm]->Status = *UHS_SD_Status;
                                                                sto[nm]->Commit = *UHS_SD_Commit;
                                                                sto[nm]->TotalSectors = GetCapacity();
                                                                sto[nm]->SectorSize = GetSectorSize();
                                                                sto[nm]->driver_type = 1;
                                                                Fats[nm] = new PFAT(sto[nm], nm, apart->firstSector);
                                                                if(Fats[nm]->MountStatus()) {
                                                                        //printf("Killing %d\r\n", nm);
                                                                        delete Fats[nm];
                                                                        Fats[nm] = NULL;
                                                                        delete (SDpvt_t *)sto[nm]->private_data;
                                                                        sto[nm]->private_data = NULL;
                                                                } else {
                                                                        //printf("%d is ok\r\n", nm);
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
                                                delete (SDpvt_t *)sto[nm]->private_data;
                                                sto[nm]->private_data = NULL;
                                        }
                                }
                        }
                } else {
                        //printf("Card %d Removed\r\n", sto_idx);
                        for(int f = 0; f < PFAT_VOLUMES; f++) {
                                if(Fats[f] != NULL) {
                                        if(sto[f]->driver_type == 1) {

                                                if((((SDpvt_t *)sto[f]->private_data)->B == sto_idx)) {
                                                        delete Fats[f];
                                                        Fats[f] = NULL;
                                                        delete (SDpvt_t *)sto[f]->private_data;
                                                        sto[f]->private_data = NULL;
                                                }
                                        }
                                }
                        }
                }
        }
        return;

}

#endif
// YOUR DRIVERS HERE

extern "C" {
        // Allow init to happen only once in the case of multiple external inits.

        static bool Init_Generic_Storage_inited = false;

        /**
         * This must be called before using generic_storage. Calling more than once is harmless.
         */
        void Init_Generic_Storage(
#ifdef __UHS_BULK_STORAGE_H__
                void *hd
#endif
#ifdef UHS_USE_SDCARD
#ifdef __UHS_BULK_STORAGE_H__
                ,
#endif
                int _pr[], int _cs[]
#endif
                ) {
                if(!Init_Generic_Storage_inited) {
                        Init_Generic_Storage_inited = true;
#ifdef __UHS_BULK_STORAGE_H__
                        UHS_USB_BulkOnly_Init((UHS_USB_HOST_BASE *)hd);

#endif


                        for(int i = 0; i < _FS_LOCK; i++) {
                                fhandles[i] = new FIL;
                                dhandles[i] = new DIR;
                                fhandles[i]->fs = NULL;
                                dhandles[i]->fs = NULL;
                        }
#ifdef __UHS_BULK_STORAGE_H
                        for(int i = 0; i < MAX_USB_MS_DRIVERS; i++) {
                                for(int j = 0; j < MASS_MAX_SUPPORTED_LUN; j++) {
                                        last_ready[i][j] = false;
                                }
                        }
#endif
                        for(int i = 0; i < PFAT_VOLUMES; i++) {
                                sto[i] = new storage_t;
                                sto[i]->private_data = NULL;
                                Fats[i] = NULL;
                        }
#ifdef UHS_USE_SDCARD
                        SPI.begin();
                        uint8_t intr;
                        // Enumerate SDcards
                        for(int i = 0; i < UHS_MAX_SD_CARDS; i++) {
                                intr = digitalPinToInterrupt(_pr[i]);
                                SD_last_ready[i] = false;
                                // attach SDcard
                                UHS_SD_Storage[i] = new UHS_FS_SD_DRIVER(_pr[i], _cs[i], i);
                                //printf("usingInterrupt... pin %d, %d\r\n",_pr[i], intr);
                                //delay(1000);
                                SPI.usingInterrupt(intr);
                                //printf("usingInterrupt OK\r\n");
                                //delay(1000);
                                UHS_SD_Storage[i]->IRQ(); // Initial SDcard Probe.
                                attachInterrupt(intr, UHS_FS_SD_ISR[i], CHANGE);
                        }
#endif
                // YOUR INIT HERE
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
                UINT bc;
                int count = 0;
                fd--;

                rc = FR_INVALID_PARAMETER;
                if((fd < PFAT_VOLUMES) && (fhandles[fd]->fs != NULL)) {
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
                        uint32_t tv = 0;
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
