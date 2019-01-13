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

// uncomment to get 'printf' console debugging. NOT FOR UNO!
// #define DEBUG_PRINTF_EXTRA_HUGE_USB_HUB

#if DEBUG_PRINTF_EXTRA_HUGE
#ifdef DEBUG_PRINTF_EXTRA_HUGE_USB_HUB
#define HUB_DUBUG(...) printf(__VA_ARGS__)
#else
#define HUB_DUBUG(...) VOID0
#endif
#else
#define HUB_DUBUG(...) VOID0
#endif


#if defined(LOAD_UHS_HUB) && defined(__UHS_HUB_H__) && !defined(UHS_HUB_LOADED)
#define UHS_HUB_LOADED

UHS_NI UHS_USBHub::UHS_USBHub(UHS_USB_HOST_BASE *p) {
        pUsb = p;
        if(pUsb) {
                DriverDefaults();
                pUsb->RegisterDeviceClass(this);
        }
}

bool UHS_NI UHS_USBHub::OKtoEnumerate(ENUMERATION_INFO *ei) {
        HUB_DUBUG("USBHub: checking numep %i, klass %2.2x, interface.klass %2.2x\r\n", ei->interface.numep, ei->klass, ei->interface.klass);
        return ((ei->interface.numep == 1) && ((ei->klass == UHS_USB_CLASS_HUB) || (ei->interface.klass == UHS_USB_CLASS_HUB)));
}

void UHS_NI UHS_USBHub::DriverDefaults(void) {
        bNbrPorts = 0;
        pUsb->DeviceDefaults(2, this);
        epInfo[0].maxPktSize = 8;
        epInfo[0].bmNakPower = UHS_USB_NAK_MAX_POWER;

        epInfo[1].epAddr = 1;
        epInfo[1].maxPktSize = 8; //kludge
        epInfo[1].bmNakPower = UHS_USB_NAK_NOWAIT;

        bResetInitiated = false;
}

uint8_t UHS_NI UHS_USBHub::SetInterface(ENUMERATION_INFO *ei) {
        //DriverDefaults();
        HUB_DUBUG("USBHub Accepting address assignment %2.2x\r\n", ei->address);
        bNumEP = 2;
        bAddress = ei->address;
        epInfo[0].epAddr = 0;
        epInfo[0].maxPktSize = ei->bMaxPacketSize0;
        epInfo[0].epAttribs = 0;
        epInfo[1].epAddr = 1;
        epInfo[1].maxPktSize = ei->interface.epInfo[0].wMaxPacketSize;
        epInfo[1].epAttribs = ei->interface.epInfo[0].bmAttributes;
        epInfo[1].bmNakPower = UHS_USB_NAK_NOWAIT;
        bIface = ei->interface.bInterfaceNumber;
        return 0;
}

uint8_t UHS_NI UHS_USBHub::Finalize(void) {
        uint8_t rcode;
        uint8_t buf[32];
        UHS_HubDescriptor* hd = reinterpret_cast<UHS_HubDescriptor*>(buf);
        // Get hub descriptor
        rcode = GetHubDescriptor(0, 8, buf);

        if(rcode)
                goto Fail;

        // Save number of ports for future use
        bNbrPorts = hd->bNbrPorts;
        return 0;

Fail:
        Release();
        return rcode;
};

uint8_t UHS_NI UHS_USBHub::Start(void) {
        uint8_t rcode;

        rcode = pUsb->setEpInfoEntry(bAddress, bIface, 2, epInfo);

        if(rcode)
                goto Fail;

        for(uint8_t j = 1; j <= bNbrPorts; j++) {
                //rcode =
                SetPortFeature(UHS_HUB_FEATURE_PORT_POWER, j, 0); //HubPortPowerOn(j);
                //printf("Address 0x%2.2x Port %i rcode 0x%2.2x\r\n", bAddress, j, rcode);
        }

        qNextPollTime = millis() + 100;
        bPollEnable = true;
        return 0;
Fail:
        Release();
        return rcode;
}

void UHS_NI UHS_USBHub::Release(void) {
        UHS_DeviceAddress a;
        a.devAddress = 0;
        a.bmHub = 0;
        a.bmParent = bAddress;
        for(uint8_t j = 1; j <= bNbrPorts; j++) {
                a.bmAddress = j;
                pUsb->ReleaseDevice(a.devAddress);
        }
        DriverDefaults();
        bNbrPorts = 0;
        return;
}

void UHS_NI UHS_USBHub::Poll(void) {
        if(bPollEnable) {
                if(((long)(millis() - qNextPollTime) >= 0L)) {
                        qNextPollTime = millis() + 100;
                        CheckHubStatus();
                        // BUG! if(((long)(millis() - qNextPollTime) >= 0L)) qNextPollTime = millis() + 100;
                }
        }
}

void UHS_NI UHS_USBHub::CheckHubStatus(void) {
        uint8_t rcode;
        uint8_t buf[8];
        uint16_t read = 1;

        rcode = pUsb->inTransfer(bAddress, 1, &read, buf);

        if(rcode) {
                HUB_DUBUG("UHS_USBHub::CheckHubStatus %2.2x\r\n", rcode);
                return;
        }

        for(uint8_t port = 1, mask = 0x02; port < 8; mask <<= 1, port++) {
                if(buf[0] & mask) {
                        UHS_HubEvent evt;
                        evt.bmEvent = 0;

                        rcode = GetPortStatus(port, 4, evt.evtBuff);

                        if(rcode)
                                continue;

                        rcode = PortStatusChange(port, evt);

                        if(rcode == UHS_HUB_ERROR_PORT_HAS_BEEN_RESET)
                                return;

                        if(rcode)
                                return;
                }
        }

        for(uint8_t port = 1; port <= bNbrPorts; port++) {
                UHS_HubEvent evt;
                evt.bmEvent = 0;

                rcode = GetPortStatus(port, 4, evt.evtBuff);

                if(rcode)
                        continue;

                if((evt.bmStatus & UHS_HUB_bmPORT_STATE_CHECK_DISABLED) != UHS_HUB_bmPORT_STATE_DISABLED)
                        continue;

                // Emulate connection event for the port
                evt.bmChange |= UHS_HUB_bmPORT_STATUS_PORT_CONNECTION;

                rcode = PortStatusChange(port, evt);

                if(rcode == UHS_HUB_ERROR_PORT_HAS_BEEN_RESET)
                        return;

                if(rcode)
                        return;
        } // for
        return;
}

void UHS_NI UHS_USBHub::ResetHubPort(uint8_t port) {
        UHS_HubEvent evt;
        evt.bmEvent = 0;
        uint8_t rcode;

        ClearPortFeature(UHS_HUB_FEATURE_C_PORT_ENABLE, port, 0);
        ClearPortFeature(UHS_HUB_FEATURE_C_PORT_CONNECTION, port, 0);
        SetPortFeature(UHS_HUB_FEATURE_PORT_RESET, port, 0);


        for(int i = 0; i < 3; i++) {
                rcode = GetPortStatus(port, 4, evt.evtBuff);
                if(rcode) return; // Some kind of error, bail.
                if(evt.bmEvent == UHS_HUB_bmPORT_EVENT_RESET_COMPLETE || evt.bmEvent == UHS_HUB_bmPORT_EVENT_LS_RESET_COMPLETE) {
                        break;
                }
                if(!UHS_SLEEP_MS(100)) return; // simulate polling, bail out early if parent state changes
        }
        ClearPortFeature(UHS_HUB_FEATURE_C_PORT_RESET, port, 0);
        ClearPortFeature(UHS_HUB_FEATURE_C_PORT_CONNECTION, port, 0);
        UHS_SLEEP_MS(20);
}

uint8_t UHS_NI UHS_USBHub::PortStatusChange(uint8_t port, UHS_HubEvent &evt) {
        UHS_DeviceAddress a;
        a.devAddress = 0;
        a.bmHub = 0;
        // This isn't correct.
        a.bmParent = bAddress;
        a.bmAddress = port;

        switch(evt.bmEvent) {
                        // Device connected event
                case UHS_HUB_bmPORT_EVENT_CONNECT:
                case UHS_HUB_bmPORT_EVENT_LS_CONNECT:
                        if(bResetInitiated) break;

                        ClearPortFeature(UHS_HUB_FEATURE_C_PORT_ENABLE, port, 0);
                        ClearPortFeature(UHS_HUB_FEATURE_C_PORT_CONNECTION, port, 0);
                        SetPortFeature(UHS_HUB_FEATURE_PORT_RESET, port, 0);
                        bResetInitiated = true;
                        qNextPollTime = millis() + 20;
                        return UHS_HUB_ERROR_PORT_HAS_BEEN_RESET;

                        // Device disconnected event
                case UHS_HUB_bmPORT_EVENT_DISCONNECT:
                        ClearPortFeature(UHS_HUB_FEATURE_C_PORT_ENABLE, port, 0);
                        ClearPortFeature(UHS_HUB_FEATURE_C_PORT_CONNECTION, port, 0);
                        bResetInitiated = false;

                        pUsb->ReleaseDevice(a.devAddress);
                        break;

                        // Reset complete event
                case UHS_HUB_bmPORT_EVENT_RESET_COMPLETE:
                case UHS_HUB_bmPORT_EVENT_LS_RESET_COMPLETE:
                        if(bResetInitiated) {
                                ClearPortFeature(UHS_HUB_FEATURE_C_PORT_RESET, port, 0);
                                ClearPortFeature(UHS_HUB_FEATURE_C_PORT_CONNECTION, port, 0);

                                UHS_SLEEP_MS(200);

                                a.devAddress = bAddress;
                                HUB_DUBUG("USBHub configure %2.2x %2.2x %2.2x\r\n", a.bmAddress, port, ((evt.bmStatus & UHS_HUB_bmPORT_STATUS_PORT_LOW_SPEED)==UHS_HUB_bmPORT_STATUS_PORT_LOW_SPEED?0:1));
                                pUsb->Configuring(a.bmAddress, port, ((evt.bmStatus & UHS_HUB_bmPORT_STATUS_PORT_LOW_SPEED)==UHS_HUB_bmPORT_STATUS_PORT_LOW_SPEED?0:1));
                                bResetInitiated = false;
                        }
                        break;

        } // switch (evt.bmEvent)
        return 0;
}

void UHS_NI PrintHubPortStatus(UHS_USBHub *hubptr, NOTUSED(uint8_t addr), uint8_t port, bool print_changes) {
        uint8_t rcode = 0;
        UHS_HubEvent evt;
        hubptr->pUsb->DisablePoll();

        rcode = hubptr->GetPortStatus(port, 4, evt.evtBuff);

        if(!rcode) {
                USB_HOST_SERIAL.print("\r\nPort ");
                USB_HOST_SERIAL.println(port, DEC);

                USB_HOST_SERIAL.println("Status");
                USB_HOST_SERIAL.print("CONNECTION:\t");
                USB_HOST_SERIAL.println((evt.bmStatus & UHS_HUB_bmPORT_STATUS_PORT_CONNECTION) > 0, DEC);
                USB_HOST_SERIAL.print("ENABLE:\t\t");
                USB_HOST_SERIAL.println((evt.bmStatus & UHS_HUB_bmPORT_STATUS_PORT_ENABLE) > 0, DEC);
                USB_HOST_SERIAL.print("SUSPEND:\t");
                USB_HOST_SERIAL.println((evt.bmStatus & UHS_HUB_bmPORT_STATUS_PORT_SUSPEND) > 0, DEC);
                USB_HOST_SERIAL.print("OVER_CURRENT:\t");
                USB_HOST_SERIAL.println((evt.bmStatus & UHS_HUB_bmPORT_STATUS_PORT_OVER_CURRENT) > 0, DEC);
                USB_HOST_SERIAL.print("RESET:\t\t");
                USB_HOST_SERIAL.println((evt.bmStatus & UHS_HUB_bmPORT_STATUS_PORT_RESET) > 0, DEC);
                USB_HOST_SERIAL.print("POWER:\t\t");
                USB_HOST_SERIAL.println((evt.bmStatus & UHS_HUB_bmPORT_STATUS_PORT_POWER) > 0, DEC);
                USB_HOST_SERIAL.print("LOW_SPEED:\t");
                USB_HOST_SERIAL.println((evt.bmStatus & UHS_HUB_bmPORT_STATUS_PORT_LOW_SPEED) > 0, DEC);
                USB_HOST_SERIAL.print("HIGH_SPEED:\t");
                USB_HOST_SERIAL.println((evt.bmStatus & UHS_HUB_bmPORT_STATUS_PORT_HIGH_SPEED) > 0, DEC);
                USB_HOST_SERIAL.print("TEST:\t\t");
                USB_HOST_SERIAL.println((evt.bmStatus & UHS_HUB_bmPORT_STATUS_PORT_TEST) > 0, DEC);
                USB_HOST_SERIAL.print("INDICATOR:\t");
                USB_HOST_SERIAL.println((evt.bmStatus & UHS_HUB_bmPORT_STATUS_PORT_INDICATOR) > 0, DEC);

                if(print_changes) {

                        USB_HOST_SERIAL.println("\r\nChange");
                        USB_HOST_SERIAL.print("CONNECTION:\t");
                        USB_HOST_SERIAL.println((evt.bmChange & UHS_HUB_bmPORT_STATUS_PORT_CONNECTION) > 0, DEC);
                        USB_HOST_SERIAL.print("ENABLE:\t\t");
                        USB_HOST_SERIAL.println((evt.bmChange & UHS_HUB_bmPORT_STATUS_PORT_ENABLE) > 0, DEC);
                        USB_HOST_SERIAL.print("SUSPEND:\t");
                        USB_HOST_SERIAL.println((evt.bmChange & UHS_HUB_bmPORT_STATUS_PORT_SUSPEND) > 0, DEC);
                        USB_HOST_SERIAL.print("OVER_CURRENT:\t");
                        USB_HOST_SERIAL.println((evt.bmChange & UHS_HUB_bmPORT_STATUS_PORT_OVER_CURRENT) > 0, DEC);
                        USB_HOST_SERIAL.print("RESET:\t\t");
                        USB_HOST_SERIAL.println((evt.bmChange & UHS_HUB_bmPORT_STATUS_PORT_RESET) > 0, DEC);
                }
        } else {
                USB_HOST_SERIAL.println("ERROR!");
        }

        hubptr->pUsb->EnablePoll();

}
#else
#error "Never include UHS_HUB_INLINE.h, include UHS_host.h instead"
#endif
