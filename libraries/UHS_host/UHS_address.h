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

#if !defined(_UHS_host_h_) || defined(__ADDRESS_H__)
#error "Never include UHS_address.h directly; include UHS_Usb.h instead"
#else
#define __ADDRESS_H__



/* NAK powers. To save space in endpoint data structure, amount of retries before giving up and returning 0x4 is stored in */
/* bmNakPower as a power of 2. The actual nak_limit is then calculated as nak_limit = ( 2^bmNakPower - 1) */
#define UHS_USB_NAK_MAX_POWER               14      // NAK binary order maximum value
#define UHS_USB_NAK_DEFAULT                 13      // default 16K-1 NAKs before giving up
#define UHS_USB_NAK_NOWAIT                  1       // Single NAK stops transfer
#define UHS_USB_NAK_NONAK                   0       // Do not count NAKs, stop retrying after USB Timeout. Try not to use this.

#define bmUSB_DEV_ADDR_PORT             0x07
#define bmUSB_DEV_ADDR_PARENT           0x78
#define bmUSB_DEV_ADDR_HUB              0x40

// TODO: embed parent?
struct UHS_EpInfo {
        uint8_t epAddr; // Endpoint address
        uint8_t bIface;
        uint16_t maxPktSize; // Maximum packet size

        union {
                uint8_t epAttribs;

                struct {
                        uint8_t bmSndToggle : 1; // Send toggle, when zero bmSNDTOG0, bmSNDTOG1 otherwise
                        uint8_t bmRcvToggle : 1; // Send toggle, when zero bmRCVTOG0, bmRCVTOG1 otherwise
                        uint8_t bmNeedPing : 1; // 1 == ping protocol needed for next out packet
                        uint8_t bmNakPower : 5; // Binary order for NAK_LIMIT value
                } __attribute__((packed));
        };
} __attribute__((packed));

struct UHS_DeviceAddress {

        union {

                struct {
                        uint8_t bmAddress : 7; // address
                        uint8_t bmReserved : 1; // reserved, must be zero
                } __attribute__((packed));
                uint8_t devAddress;
        };
} __attribute__((packed));

struct UHS_Device {
        volatile UHS_EpInfo *epinfo[UHS_HOST_MAX_INTERFACE_DRIVERS]; // endpoint info pointer
        UHS_DeviceAddress address;
        UHS_DeviceAddress parent;
        uint8_t port;
        uint8_t epcount; // number of endpoints
        uint8_t speed; // indicates device speed
} __attribute__((packed));

typedef void (*UsbDeviceHandleFunc)(UHS_Device *pdev);

class AddressPool {
        UHS_EpInfo dev0ep; //Endpoint data structure used during enumeration for uninitialized device

        UHS_Device thePool[UHS_HOST_MAX_INTERFACE_DRIVERS];

        // Initializes address pool entry

        void UHS_NI InitEntry(uint8_t index) {
                thePool[index].address.devAddress = 0;
                thePool[index].epcount = 1;
                thePool[index].speed = 0;
                thePool[index].parent.devAddress = 0;
                thePool[index].port = 0;
                for(uint8_t i = 0; i < UHS_HOST_MAX_INTERFACE_DRIVERS; i++) {
                        thePool[index].epinfo[i] = &dev0ep;
                }
        };

        // find a new lowest unused address number.
        uint8_t UHS_NI FindNewAddress() {
                printf("Find address...\r\n");
                uint8_t rv = 0;
                bool got = false;
                // scan low to high
                for(uint8_t i = 1; i < 127; i++) {
                        for(uint8_t j = 1; j < UHS_HOST_MAX_INTERFACE_DRIVERS; j++) {
                                if(thePool[i].address.devAddress == i) {
                                        printf("Address %i used\r\n", i);
                                        got = true;
                                        break;
                                }
                        }
                        if(!got) {
                                printf("Address %i NOT used\r\n", i);
                                rv=i;
                                break;
                        }
                        got = false;
                }
                printf("Returning %i as free\r\n", rv);
                return rv;
        }

        // Returns thePool index for a given address
        uint8_t UHS_NI FindAddressIndex(uint8_t address = 0) {
                for(uint8_t i = 1; i < UHS_HOST_MAX_INTERFACE_DRIVERS; i++) {
                        if(thePool[i].address.devAddress == address)
                                return i;
                }
                return 0;
        };

        // Returns thePool child index for a given parent
        uint8_t UHS_NI FindChildIndex(UHS_DeviceAddress addr, uint8_t start = 1) {
                for(uint8_t i = (start < 1 || start >= UHS_HOST_MAX_INTERFACE_DRIVERS) ? 1 : start; i < UHS_HOST_MAX_INTERFACE_DRIVERS; i++) {
                        //if(thePool[i].address.bmParent == addr.bmAddress)
                        //        return i;
                        if(thePool[i].parent.bmAddress == addr.bmAddress) {
                                return i;
                        }
                }
                return 0;
        };

        // Frees address entry specified by index parameter
        void UHS_NI FreeAddressByIndex(uint8_t index) {
                // Zero field is reserved and should not be affected
                if(index == 0)
                        return;

                UHS_DeviceAddress uda = thePool[index].address;

                // If a hub was switched off all port addresses should be freed
                for(uint8_t i=1; i < UHS_HOST_MAX_INTERFACE_DRIVERS; i++) {
                        uint8_t q=FindChildIndex(uda,i);
                        if(q != 0) {
                                FreeAddressByIndex(q);
                        }
                }

                InitEntry(index);
        }

        void InitAllAddresses(void) {
                for(uint8_t i = 1; i < UHS_HOST_MAX_INTERFACE_DRIVERS; i++) InitEntry(i);
        };
public:

        AddressPool() {
                // Zero address is reserved
                InitEntry(0);

                thePool[0].epinfo[0] = &dev0ep;
                dev0ep.epAddr = 0;
#if UHS_DEVICE_WINDOWS_USB_SPEC_VIOLATION_DESCRIPTOR_DEVICE
                dev0ep.maxPktSize = 0x40; //starting at 0x40 and work down
#else
                dev0ep.maxPktSize = 0x08;
#endif
                dev0ep.epAttribs = 0; //set DATA0/1 toggles to 0
                dev0ep.bmNakPower = UHS_USB_NAK_MAX_POWER;
                InitAllAddresses();
        };

        // Returns a pointer to a specified address entry

        UHS_Device* UHS_NI GetUsbDevicePtr(uint8_t addr) {
                if(!addr)
                        return thePool;

                uint8_t index = FindAddressIndex(addr);

                return (!index) ? NULL : &thePool[index];
        };


        // Allocates new address
        // No longer depends on parent silliness.
        uint8_t UHS_NI AllocAddress(uint8_t parent, uint8_t port) {
                /* if (parent != 0 && port == 0)
                        USB_HOST_SERIAL.println("PRT:0"); */
                //UHS_DeviceAddress _parent;
                //_parent.devAddress = parent;
                if(port > 7)
                        return 0;

                // finds first empty address entry starting from one
                uint8_t index = FindAddressIndex(0);

                if(!index) {
                        // if empty entry is not found
                        return 0;
                }

                UHS_DeviceAddress addr;
                addr.devAddress = FindNewAddress();
                if(addr.devAddress == 0) {
                        return 0;
                }
                thePool[index].address = addr;
                thePool[index].port = port;
                thePool[index].parent.devAddress = parent;
#if DEBUG_PRINTF_EXTRA_HUGE
#if defined(UHS_DEBUG_USB_ADDRESS)
                //printf("Address: %x (%x.%x.%x)\r\n", addr.devAddress, addr.bmHub, addr.bmParent, addr.bmAddress);
                printf_P(PSTR("Address: %x\r\n"), addr.devAddress);
#endif
#endif
                return thePool[index].address.devAddress;
        };

        void UHS_NI FreeAddress(uint8_t addr) {
                if(addr >0) {
                        uint8_t index = FindAddressIndex(addr);
                        FreeAddressByIndex(index);
                }
        };

        uint8_t UHS_NI FindPortChildAddress(uint8_t addr, uint8_t port) {
                for(uint8_t i = 1; i < UHS_HOST_MAX_INTERFACE_DRIVERS; i++) {
                        if((thePool[i].parent.bmAddress == addr) && (thePool[i].port == port)) {
                                return thePool[i].address.devAddress;
                        }
                }
                return 0;
        };
};

#endif // __ADDRESS_H__
