#if !defined(__UHS_Printer_H__)
#define __UHS_Printer_H__

#define UHS_Printer_MAX_ENDPOINTS 3 //endpoint 0, bulk_IN(Printer), bulk_OUT(Printer)
#define UHS_Printer_EVENT_PACKET_SIZE 64

#define UHS_Printer_POLL_RATE    8

#if DEBUG_PRINTF_EXTRA_HUGE
#ifdef DEBUG_PRINTF_EXTRA_HUGE_Printer_HOST
#define UHS_Printer_HOST_DEBUG(...) printf(__VA_ARGS__)
#else
#define UHS_Printer_HOST_DEBUG(...) VOID0
#endif
#else
#define UHS_Printer_HOST_DEBUG(...) VOID0
#endif

class UHS_Printer : public UHS_USBInterface {
protected:
        volatile bool ready; // device ready indicator
        uint8_t qPollRate; // How fast to poll maximum
        uint16_t vid; //Vendor ID
        uint16_t pid; //Product ID

        /* Printer Event packet buffer */
        uint8_t *pktbuf;
        UHS_ByteBuffer Printerbuf;

       

public:
        static const uint8_t epDataInIndex = 1; // DataIn endpoint index(Printer)
        static const uint8_t epDataOutIndex = 2; // DataOUT endpoint index(Printer)
        volatile UHS_EpInfo epInfo[UHS_Printer_MAX_ENDPOINTS];

        UHS_Printer(UHS_USB_HOST_BASE *p);

        // Methods for receiving and sending data
        uint8_t SendRawData(uint16_t bytes_send, uint8_t *dataptr);
        inline uint16_t idVendor() { return vid; }
        inline uint16_t idProduct() { return pid; }

        uint8_t Start(void);
        bool OKtoEnumerate(ENUMERATION_INFO *ei);
        uint8_t SetInterface(ENUMERATION_INFO *ei);
        void DriverDefaults(void);

        bool Polling(void) {
                return bPollEnable;
        }

        

        uint8_t GetAddress() {
                return bAddress;
        };

        bool isReady() {
                pUsb->DisablePoll();
                bool rv = ready;
                pUsb->EnablePoll();
                return rv;
        };
};

#if defined(LOAD_UHS_Printer) && !defined(UHS_Printer_LOADED)
#include "UHS_Printer_INLINE.h"
#endif
#endif // __UHS_Printer_H__
