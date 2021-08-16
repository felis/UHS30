#if !defined(__UHS_PRINTER_H__)
#define __UHS_PRINTER_H__

#define UHS_PRINTER_MAX_ENDPOINTS 4 //endpoint 0, bulk_IN(PRINTER), bulk_OUT(PRINTER), 1284
#define UHS_PRINTER_EVENT_PACKET_SIZE 64

#define UHS_PRINTER_POLL_RATE    8

#if DEBUG_PRINTF_EXTRA_HUGE
#ifdef DEBUG_PRINTF_EXTRA_HUGE_PRINTER_HOST
#define UHS_PRINTER_HOST_DEBUG(...) printf(__VA_ARGS__)
#else
#define UHS_PRINTER_HOST_DEBUG(...) VOID0
#endif
#else
#define UHS_PRINTER_HOST_DEBUG(...) VOID0
#endif

/* Quirks: various printer quirks are handled by this table & its flags. */

struct quirk_printer_struct {
        uint16_t vendorId;
        uint16_t productId;
        uint8_t quirks;
}__attribute__((packed));

#define     UHS_PRINTER_QUIRK_BIDIR 0x01U /* reports bidir but requires unidirectional mode (no INs/reads) */
#define  UHS_PRINTER_QUIRK_USB_INIT 0x02U /* needs vendor USB init string */
#define UHS_PRINTER_QUIRK_BAD_CLASS 0x04U /* descriptor uses vendor-specific Class or SubClass */

const struct quirk_printer_struct quirk_printers[] PROGMEM = {
        { 0x03f0U, 0x0004U, UHS_PRINTER_QUIRK_BIDIR}, /* HP DeskJet 895C */
        { 0x03f0U, 0x0104U, UHS_PRINTER_QUIRK_BIDIR}, /* HP DeskJet 880C */
        { 0x03f0U, 0x0204U, UHS_PRINTER_QUIRK_BIDIR}, /* HP DeskJet 815C */
        { 0x03f0U, 0x0304U, UHS_PRINTER_QUIRK_BIDIR}, /* HP DeskJet 810C/812C */
        { 0x03f0U, 0x0404U, UHS_PRINTER_QUIRK_BIDIR}, /* HP DeskJet 830C */
        { 0x03f0U, 0x0504U, UHS_PRINTER_QUIRK_BIDIR}, /* HP DeskJet 885C */
        { 0x03f0U, 0x0604U, UHS_PRINTER_QUIRK_BIDIR}, /* HP DeskJet 840C */
        { 0x03f0U, 0x0804U, UHS_PRINTER_QUIRK_BIDIR}, /* HP DeskJet 816C */
        { 0x03f0U, 0x1104U, UHS_PRINTER_QUIRK_BIDIR}, /* HP Deskjet 959C */
        { 0x0409U, 0xefbeU, UHS_PRINTER_QUIRK_BIDIR}, /* NEC Picty900 (HP OEM) */
        { 0x0409U, 0xbef4U, UHS_PRINTER_QUIRK_BIDIR}, /* NEC Picty760 (HP OEM) */
        { 0x0409U, 0xf0beU, UHS_PRINTER_QUIRK_BIDIR}, /* NEC Picty920 (HP OEM) */
        { 0x0409U, 0xf1beU, UHS_PRINTER_QUIRK_BIDIR}, /* NEC Picty800 (HP OEM) */
        { 0x0482U, 0x0010U, UHS_PRINTER_QUIRK_BIDIR}, /* Kyocera Mita FS 820, by zut <kernel@zut.de> */
        { 0x04f9U, 0x000dU, UHS_PRINTER_QUIRK_BIDIR}, /* Brother Industries, Ltd HL-1440 Laser Printer */
        { 0x04b8U, 0x0202U, UHS_PRINTER_QUIRK_BAD_CLASS}, /* Seiko Epson Receipt Printer M129C */
        { 0, 0, 0}
};

struct UHS_PRINTER_STATUS {
        uint8_t Reserved0 : 3; // Reserved
        uint8_t not_error : 1; // 1 = No Error, 0 = Error
        uint8_t selected : 1; // 1 = Selected, 0 = Not Selected
        uint8_t paper_out : 1; // 1 = Paper Empty, 0 = Paper Not Empty
        uint8_t Reserved1 : 2; // Reserved
} __attribute__ ((packed));

class UHS_PRINTER : public UHS_USBInterface {
protected:
        volatile bool ready; // device ready indicator
        uint8_t qPollRate; // How fast to poll maximum
        uint16_t vid; //Vendor ID
        uint16_t pid; //Product ID

public:
        static const uint8_t epDataOutIndex = 1; // DataOUT endpoint index(PRINTER)
        static const uint8_t epDataInIndex = 2; // DataIn endpoint index(PRINTER)
        volatile UHS_EpInfo epInfo[UHS_PRINTER_MAX_ENDPOINTS];
        volatile UHS_PRINTER_STATUS status;
        uint8_t quirks;
        uint8_t bAlternateSetting = 255; // impossible?
        uint8_t Interface = 255; // impossible?

        UHS_PRINTER(UHS_USB_HOST_BASE *p);

        inline uint16_t idVendor() {
                return vid;
        }

        inline uint16_t idProduct() {
                return pid;
        }

        uint8_t Start(void);
        bool OKtoEnumerate(ENUMERATION_INFO *ei);
        uint8_t SetInterface(ENUMERATION_INFO *ei);
        void DriverDefaults(void);

        bool Polling(void) {
                return bPollEnable;
        }

        void Poll(void);

        uint8_t GetAddress() {
                return bAddress;
        };

        bool isReady() {
                pUsb->DisablePoll();
                bool rv = ready;
                pUsb->EnablePoll();
                return rv;
        };

        uint8_t printer_type(void);
        uint8_t select_printer(void);
        int16_t printer_selected(void);
        uint8_t quirk_check(ENUMERATION_INFO *ei);
        uint8_t check_status(void);
        // Methods for receiving and sending data
        uint8_t write(uint16_t len, uint8_t *data);
        uint8_t read(uint16_t *len, uint8_t *data);

};

#if defined(LOAD_UHS_PRINTER) && !defined(UHS_PRINTER_LOADED)
#include "UHS_PRINTER_INLINE.h"
#endif
#endif // __UHS_PRINTER_H__
