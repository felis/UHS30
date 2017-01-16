
// Load the USB Host System core
#define LOAD_USB_HOST_SYSTEM
// Load the Kinetis core
#define LOAD_UHS_KINETIS_FS_HOST
// Use USB hub
#define LOAD_UHS_HUB

// Patch printf so we can use it.
#define LOAD_UHS_PRINTF_HELPER
//#define DEBUG_PRINTF_EXTRA_HUGE 1
//#define DEBUG_PRINTF_EXTRA_HUGE_UHS_HOST 1
//#define DEBUG_PRINTF_EXTRA_HUGE_USB_HUB 1
//#define DEBUG_PRINTF_EXTRA_HUGE_USB_HOST_SHIELD 1
//#define DEBUG_PRINTF_EXTRA_HUGE_ACM_HOST 1
//#define UHS_DEBUG_USB_ADDRESS 1
// Redirect debugging and printf
#define USB_HOST_SERIAL Serial1


// The data rate on the device must be < host, 115200/2 = 57600
#define HOST_SERIAL_SPEED 115200
#define CLIENT_SERIAL_SPEED 57600

// These all get combined under UHS_CDC_ACM multiplexer.
// Each should only add a trivial amount of code.
// XR21B1411 can run in a pure CDC-ACM mode, as can PROLIFIC.
// FTDI has a large code and data footprint. Avoid this chip if you can.
#define LOAD_UHS_CDC_ACM
#define LOAD_UHS_CDC_ACM_XR21B1411
// This needs testing.
#define LOAD_UHS_CDC_ACM_PROLIFIC
// This needs testing.
#define LOAD_UHS_CDC_ACM_FTDI

#include <Arduino.h>
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif

#include <UHS_host.h>

// maximum size 65535
#define RD_BUF_MAX 1024
#define WR_BUF_MAX 1024

class MY_ACM : public UHS_CDC_ACM {
public:
        uint8_t wrbuf[WR_BUF_MAX];
        uint8_t rdbuf[RD_BUF_MAX];
        volatile uint16_t wr_head;
        volatile uint16_t wr_tail;
        volatile uint16_t rd_head;
        volatile uint16_t rd_tail;
        volatile uint8_t lstat;

        MY_ACM(UHS_USB_HOST_BASE *p) : UHS_CDC_ACM(p) {
                wr_head = 0;
                wr_tail = 0;
                rd_head = 0;
                rd_tail = 0;
                lstat = 0;
        };
        uint8_t OnStart(void);
        void OnRelease(void);
        void OnPoll(void);

        uint16_t wr_available(void);
        uint16_t rd_available(void);
        uint8_t *get_wr_buf();
        uint8_t *get_rd_buf();
        void UpdateWrite(uint16_t amount);
        void UpdateRead(uint16_t amount);
};

/**
 *
 * @return pointer to beginning of the next chunk to read
 */
uint8_t *MY_ACM::get_rd_buf() {
        return &rdbuf[rd_tail];
}

/**
 *
 * @return the size of a contiguous chunk user can read
 */
uint16_t MY_ACM::rd_available(void) {
        uint16_t x = 0;
        pUsb->DisablePoll();
        x = rd_head - rd_tail;
        pUsb->EnablePoll();
        return x;
}

/**
 *
 * @param amount how many bytes got taken
 */
void MY_ACM::UpdateRead(uint16_t amount) {
        pUsb->DisablePoll();
        rd_tail += amount;
        if(rd_tail == rd_head) {
                // Empty, reset indexes to zero
                rd_tail = 0;
                rd_head = 0;
        }
        pUsb->EnablePoll();
}

/**
 * Must be called with polling disabled
 * @return pointer to beginning of the next chunk to write
 */
uint8_t *MY_ACM::get_wr_buf() {
        return &wrbuf[wr_head];
}

/**
 * Must be called with polling disabled
 * @param amount how many bytes got stored
 */
void MY_ACM::UpdateWrite(uint16_t amount) {
        wr_head += amount;
        if(wr_head == WR_BUF_MAX) wr_head = 0;
}

/**
 * Must be called with polling disabled
 * @return the size of a contiguous chunk user can write
 */
uint16_t MY_ACM::wr_available(void) {
        uint16_t x = 0;
        if(wr_head >= wr_tail) {
                x = WR_BUF_MAX - wr_head;
        } else if(wr_head < wr_tail) {
                x = wr_tail - wr_head;
        }
        return x;
};

/**
 * Called at end of interface polling service.
 * This is where all the magic happens to transfer data to/from USB uart.
 * No user interaction is required.
 */
void MY_ACM::OnPoll(void) {
        uint16_t x;
        uint8_t rcode;
        boolean txe = false;
readmore:
        do {
                // IMPORTANT NOTE: reads must ALWAYS be 64 bytes, writes can be ANY size.
                // Pull USB to read buffer, but only in a contiguous chunk.
                x = WR_BUF_MAX - rd_head;
                if(x >= 64) {
                        if(x > epInfo[epDataInIndex].maxPktSize) x = epInfo[epDataInIndex].maxPktSize;
                        // get data chunk (if available) from USB uart directly into the queue
                        rcode = Read(&x, &rdbuf[rd_head]);
                        // bail out early if something bad happens.
                        if(rcode) {
                                x = 0;
                                if(rcode != UHS_HOST_ERROR_NAK) return;
                        }
                        if(x > 0) {
                                rd_head += x;
                        }
#if defined(LOAD_UHS_CDC_ACM_FTDI)
                        if(adaptor == UHS_USB_ACM_FTDI) {
                                // we can cheat here and use the FTDI status bit
                                txe = ((st_line & FTDI_SIO_TEMT) == FTDI_SIO_TEMT);
                        }
#endif
                } else x = 0;
        } while(x);

#if defined(LOAD_UHS_CDC_ACM_FTDI)
        if(adaptor != UHS_USB_ACM_FTDI) {
                txe = true; // depend on NAK
        }
#else
        txe = true; // depend on NAK
#endif

        // Push write buffer to USB
        // Normal devices using CDC-ACM should NAK, FTDI will wedge.
        if(wr_head != wr_tail && txe) {
                txe = false;
#if 1
                if(wr_tail < wr_head) {
                        x = wr_head - wr_tail;
                } else {
                        x = WR_BUF_MAX - wr_tail;
                }
                // prevent needing to send a ZLP
                if(x > (epInfo[epDataOutIndex].maxPktSize - 1)) x = epInfo[epDataOutIndex].maxPktSize - 1;
                txe = false;
                rcode = Write(x, &wrbuf[wr_tail]);
                // bail out early if NAK or something bad happens.
                if(rcode) return; // NAK or something else. NAK will not adjust write buffer.
                wr_tail += x;
                if(wr_tail == WR_BUF_MAX) wr_tail = 0;
#else
                rcode = Write(1, &wrbuf[wr_tail]);
                // bail out early if NAK or something bad happens.
                if(rcode) return; // NAK or something else. NAK will not adjust write buffer.
                wr_tail++;
                if(wr_tail == WR_BUF_MAX) wr_tail = 0;
#endif
                goto readmore;
        }
}

/**
 * Tells the user that the device has been unplugged.
 */
void MY_ACM::OnRelease(void) {

        if(bAddress) printf("\r\n\r\nDisconnected.\r\n\r\n");
}

/**
 * Sets up the UART, tells the user device is ready.
 *
 * @return 0 for success
 */
uint8_t MY_ACM::OnStart(void) {
        uint8_t rcode;
#if defined(LOAD_UHS_CDC_ACM_FTDI)
        // If we are an FTDI, set the latency timer to a very low number here to improve performance.
        if(adaptor == UHS_USB_ACM_FTDI) {
                rcode = pUsb->ctrlReq(bAddress, mkSETUP_PKT8(bmREQ_VENDOR_OUT, FTDI_SIO_SET_LATENCY_TIMER, 0, 2, 0, 0), 0, NULL);
                if(rcode) {
                        printf("FTDI: Set latency timer error %x\r\n", rcode);
                        return rcode;
                }

        }
#endif

        // set qPollRate every iteration
        qPollRate = 1;
        // if full immediately abort
        epInfo[epDataOutIndex].bmNakPower = 1;
        // if empty immediately abort
        epInfo[epDataInIndex].bmNakPower = 1;

        // Set DTR = 1 RTS = 1
        rcode = SetControlLineState(3);

        if(rcode) {
                printf("SetControlLineState %x\r\n", rcode);
                return rcode;
        }
        UHS_CDC_LINE_CODING lc;
        lc.dwDTERate = CLIENT_SERIAL_SPEED;
        lc.bCharFormat = 0;
        lc.bParityType = 0;
        lc.bDataBits = 8;

        rcode = SetLineCoding(&lc);

        if(rcode) {
                printf("SetLineCoding %x\r\n", rcode);
                return rcode;
        }

        wr_head = 0;
        wr_tail = 0;
        rd_head = 0;
        rd_tail = 0;
        // Tell the user that the device has connected
        printf("\r\n\r\nConnected.\r\n\r\n");
        fflush(stdout);
        return 0;
}


UHS_KINETIS_FS_HOST *KINETIS_Usb;
UHS_USBHub *hub_KINETIS1;
UHS_USBHub *hub_KINETIS2;
MY_ACM *Acm;

void setup() {
        // This is so you can be ensured the dev board has power,
        // since the Teensy lacks a power indicator LED.
        // It also flashes at each stage, and during I/O in the loop.
        // If the code wedges at any point, you'll see the LED stuck on.
        pinMode(LED_BUILTIN, OUTPUT);
        pinMode(6, OUTPUT);
        digitalWriteFast(6, LOW);
        pinMode(2, OUTPUT);
        digitalWriteFast(2, LOW);
        digitalWriteFast(LED_BUILTIN, HIGH);
        delay(250);
        digitalWriteFast(LED_BUILTIN, LOW);
        delay(250);
        digitalWriteFast(LED_BUILTIN, HIGH);
        delay(250);

        // USB data switcher, PC -> device.
        pinMode(5, OUTPUT),
                digitalWriteFast(5, HIGH);

        KINETIS_Usb = new UHS_KINETIS_FS_HOST();
        hub_KINETIS1 = new UHS_USBHub(KINETIS_Usb);
        digitalWriteFast(LED_BUILTIN, LOW);
        delay(250);
        digitalWriteFast(LED_BUILTIN, HIGH);
        delay(250);
        hub_KINETIS2 = new UHS_USBHub(KINETIS_Usb);
        Acm = new MY_ACM(KINETIS_Usb);
        digitalWriteFast(LED_BUILTIN, LOW);
        delay(250);
        digitalWriteFast(LED_BUILTIN, HIGH);
        delay(250);
        while(!USB_HOST_SERIAL);
        digitalWriteFast(LED_BUILTIN, LOW);
        delay(250);
        USB_HOST_SERIAL.begin(HOST_SERIAL_SPEED);

        USB_HOST_SERIAL.print("\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\nStarting buffered CDC-ACM test program...\r\n");
        while(KINETIS_Usb->Init(1000) != 0);
        // now printf is available
        printf("\r\n\r\nWaiting for Connection...\r\n");
}

void loop() {
        static int16_t rcvd;
        static int16_t chunk_size;
        static uint8_t *chunk;
        if(Acm->isReady()) {
                /* read a chunk from serial terminal keyboard, write to USB uart */
                rcvd = USB_HOST_SERIAL.available();
                if(rcvd > 0) {
                        Acm->pUsb->DisablePoll();
                        chunk_size = Acm->wr_available();
                        if(chunk_size > 0) {
                                chunk = Acm->get_wr_buf();
                                // trim rcvd to available chunk size.
                                if(chunk_size < rcvd) rcvd = chunk_size;
                                // write chunk directly into write buffer.
                                USB_HOST_SERIAL.readBytes(chunk, rcvd);
                                // Tell USB to send it
                                Acm->UpdateWrite(rcvd);
                        }
                        Acm->pUsb->EnablePoll();
                }

                /*
                 * read a chunk from USB uart, display it.
                 */
                rcvd = USB_HOST_SERIAL.availableForWrite();
                Acm->pUsb->DisablePoll();
                chunk_size = Acm->rd_available();
                Acm->pUsb->EnablePoll();
                if(chunk_size > 0 && rcvd > 0) {
                        // More than zero bytes received, and serial has room, display the text.
                        chunk = Acm->get_rd_buf();
                        // trim chunk_size to available serial buffer.
                        if(rcvd < chunk_size) chunk_size = rcvd;
                        // Display chunk directly from the read buffer.
                        USB_HOST_SERIAL.write(chunk, chunk_size);
                        Acm->pUsb->DisablePoll();
                        Acm->UpdateRead(chunk_size);
                        Acm->pUsb->EnablePoll();
                }
        }
}
