// inline library loading
#define LOAD_USB_HOST_SYSTEM
#define LOAD_UHS_KINETIS_FS_HOST

// Redirect debugging
#define USB_HOST_SERIAL Serial1

#include <Arduino.h>
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif

#include <stdio.h>

#define TESTdsize 512
#define TESTcycles (1048576/TESTdsize)


#ifndef printf_P
#define printf_P(...) printf(__VA_ARGS__)
#endif

#include <dyn_SWI.h>

#include <UHS_host.h>
#include <UHS_KINETIS_FS_HOST.h>
UHS_KINETIS_FS_HOST KINETIS_Usb;

#if defined(CORE_TEENSY)
extern "C" {

        int _write(int fd, const char *ptr, int len) {
                int j;
                for(j = 0; j < len; j++) {
                        if(fd == 1)
                                Serial1.write(*ptr++);
                        else if(fd == 2)
                                USB_HOST_SERIAL.write(*ptr++);
                }
                return len;
        }

        int _read(int fd, char *ptr, int len) {
                if(len > 0 && fd == 0) {
                        while(!Serial1.available());
                        *ptr = Serial1.read();
                        return 1;
                }
                return 0;
        }

#include <sys/stat.h>

        int _fstat(int fd, struct stat *st) {
                memset(st, 0, sizeof (*st));
                st->st_mode = S_IFCHR;
                st->st_blksize = 1024;
                return 0;
        }

        int _isatty(int fd) {
                return (fd < 3) ? 1 : 0;
        }

}

// Else we are using CMSIS DAP (possibly) or debug output
#endif // TEENSY_CORE

void setup() {
        //while(!Serial1);
        Serial1.begin(115200);
        delay(10000);
        Serial1.println("Start.");
        Init_dyn_SWI();

        // It locks around here! -Pedvide

        printf("\r\n\r\nSWI_IRQ_NUM %i\r\n", SWI_IRQ_NUM);
        while(KINETIS_Usb.Init(1000) != 0);
        printf("\r\n\r\nUSB HOST READY.\r\n");

        pinMode(LED_BUILTIN, OUTPUT);
}

uint8_t current_state = 128;
uint8_t last_state = 255;

uint8_t d;

void loop() {
#if !USB_HOST_SHIELD_USE_ISR
        KINETIS_Usb.Task();
#endif

        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

        current_state = KINETIS_Usb.getUsbTaskState();
        if(current_state != last_state) {
                last_state = current_state;
                printf("USB HOST state %2.2x\r\n", current_state);
        }



        if (Serial1.available() > 0) {

                d = Serial1.read();
                if(d=='s') {
                        Serial1.print("USB0_INTEN: ");
                        Serial1.println(USB0_INTEN, HEX);
                        Serial1.print("USB0_CTL: ");
                        Serial1.println(USB0_CTL, HEX);
                } else if(d=='p') { //
                }
        }

}
