// Patch printf so we can use it.
#define LOAD_UHS_PRINTF_HELPER
// Load the USB Host System core
#define LOAD_USB_HOST_SYSTEM
// Load USB Host Shield
#define LOAD_USB_HOST_SHIELD
// Load Printer class driver
#define LOAD_UHS_Printer

#define ENABLE_UHS_DEBUGGING 1
#define DEBUG_PRINTF_EXTRA_HUGE 1
#define DEBUG_PRINTF_EXTRA_HUGE_UHS_HOST 1
#define DEBUG_PRINTF_EXTRA_HUGE_USB_HOST_SHIELD 1
#define DEBUG_PRINTF_EXTRA_HUGE_Printer_HOST 1
#define UHS_DEVICE_WINDOWS_USB_SPEC_VIOLATION_DESCRIPTOR_DEVICE 1

#define PRINTER_CLEAR_COMMAND_LEN    65

#include <Arduino.h>
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif

#include <UHS_host.h>

MAX3421E_HOST *UHS_Usb;
UHS_Printer *Printer;
bool connected;
String s;
bool isReady1;
// Poll USB Printer Controller and send to serial Printer
void setup() {
        connected = false;
        while(!Serial) {
                yield();
        }
        Serial.begin(115200);
        delay(100);
        Serial.println(F("USB Printer Converter example."));
        UHS_Usb = new MAX3421E_HOST();
        Printer = new UHS_Printer(UHS_Usb);
        while(UHS_Usb->Init(1000) != 0);
}

void loop() {
        if(Printer->isReady()) {
                if(!connected) {
                        connected = true;
                        Serial.println(F("Connected to Printer\r\n"));
                }
                Printer_write();
        } else {
                if(connected) {
                        connected = false;
                        Serial.println(F("\r\nDisconnected from Printer\r\n"));
                }
        }
}

void Printer_write()
{
 if(Serial.available())
   {
   s = Serial.readStringUntil('\n');
   if(s.length()>0)
   {
      isReady1=true;
   }
   else {
   isReady1=false;
   }
    if(isReady1)
    {
      switch(s[0])
      {        
         case '$': // LCD         
         PRINTER_request(s.substring(1));       
         break;         
         default:                               
         break;
      }
      isReady1=false;
    }
   }
}

void PRINTER_request(String str)
{
  unsigned int f, a, i = 0, cnt, len;
  unsigned int d;
  signed char c;
  char byte_array[80];
  byte rcode = 0;
  byte rcode1 = 0;
  
  
    cnt = 0;
    i = 0;
    memset(byte_array, '\0', sizeof(byte_array));
    len = str.length();
    for (f = 0; f <= len; f++) //no. of characters // Loop for No. of character's
    {
        d=0;
        if (cnt < 65)
        {
          byte_array[cnt] = (char)str.charAt(f);//pgm_read_byte_near(Courier_15 + d);
          Serial.println(byte_array[cnt]);
        }
        cnt++;
    }
   byte_array[strlen(byte_array)] = 0x0D;
   byte_array[strlen(byte_array)] = 0x0A;
   byte_array[strlen(byte_array)] = 0x1B;
   
   rcode1 = Printer->SendRawData(PRINTER_CLEAR_COMMAND_LEN, (uint8_t*)byte_array);

 if(rcode1)
 {
   Serial.println("rcode1 Failed");
   return;
 }
 else{
  Serial.println("Issued the clear command.");
  return ;
  }
 return;
} 
