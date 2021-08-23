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

#if !defined(DEBUG_PRINTF_EXTRA_HUGE_USB_HUB)
#define DEBUG_PRINTF_EXTRA_HUGE_USB_HUB 0
#endif

#if DEBUG_PRINTF_EXTRA_HUGE
#define HUB_DEBUGx(...) printf(__VA_ARGS__)
#if DEBUG_PRINTF_EXTRA_HUGE_USB_HUB
#define HUB_DEBUG(...) printf(__VA_ARGS__)
#else
#define HUB_DEBUG(...) VOID0
#endif
#else
#define HUB_DEBUGx(...) VOID0
#define HUB_DEBUG(...) VOID0
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
        // check if ei->address is already in-use in another driver on this host
        HUB_DEBUGx("USBHub: checking numep %i, klass %2.2x, interface.klass %2.2x, protocol %2.2x/%2.2x-%2.2x?%2.2x\r\n", ei->interface.numep, ei->klass, ei->interface.klass, ei->interface.protocol, ei->protocol, ei->currentconfig, ei->address);
        for(uint8_t devConfigIndex = 0; devConfigIndex < UHS_HOST_MAX_INTERFACE_DRIVERS; devConfigIndex++) {
                if(!pUsb->devConfig[devConfigIndex]) continue;
                if(pUsb->devConfig[devConfigIndex]->bAddress == ei->address) return false;
        }
        return ((ei->interface.numep == 1) && ((ei->klass == UHS_USB_CLASS_HUB) && (ei->interface.klass == UHS_USB_CLASS_HUB)));
}

void UHS_NI UHS_USBHub::DriverDefaults(void) {
        bNbrPorts = 0;
        bAlternateSetting = 255;
        pUsb->DeviceDefaults(2, this);
        epInfo[0].maxPktSize = 8; // kludge
        epInfo[0].bmNakPower = UHS_USB_NAK_MAX_POWER;

        epInfo[1].epAddr = 1;
        epInfo[1].maxPktSize = 8; //kludge
        epInfo[1].bmNakPower = UHS_USB_NAK_NOWAIT;

        bResetInitiated = false;
}

uint8_t UHS_NI UHS_USBHub::SetInterface(ENUMERATION_INFO *ei) {
        HUB_DEBUGx("USBHub Accepting address assignment %2.2x\r\n", ei->address);
        bNumEP = 2;
        bAddress = ei->address;
        epInfo[0].epAddr = 0;
        epInfo[0].maxPktSize = ei->bMaxPacketSize0;
        epInfo[0].epAttribs = 0;
        epInfo[0].type = USB_TRANSFER_TYPE_CONTROL;

        epInfo[1].epAddr = 1;
        epInfo[1].maxPktSize = ei->interface.epInfo[0].wMaxPacketSize;
        epInfo[1].epAttribs = ei->interface.epInfo[0].bmAttributes;
        epInfo[1].bmNakPower = UHS_USB_NAK_NOWAIT;
        epInfo[1].type = ei->interface.epInfo[0].bmAttributes;

        bIface = ei->interface.bInterfaceNumber;
        bAlternateSetting = ei->interface.bAlternateSetting;

        return 0;
}

uint8_t UHS_NI UHS_USBHub::Finalize(void) {
        uint8_t rcode;
        uint8_t l;
        {
                uint8_t buf[4];
                UHS_HubDescriptor* nhd = reinterpret_cast<UHS_HubDescriptor*>(buf);
                // Get hub descriptor
                rcode = GetHubDescriptor(0, 3, buf);
                l = nhd->bDescLength;
        }
        uint8_t buf[l + 1];
        UHS_HubDescriptor* hd = reinterpret_cast<UHS_HubDescriptor*>(buf);
        if(rcode) goto Fail;
        rcode = GetHubDescriptor(0, l, buf);
        if(rcode) goto Fail;

        // Save number of ports for future use
        if(!UHS_SLEEP_MS(50)) goto Fail;
        bNbrPorts = hd->bNbrPorts;
        // Allowed to stall as per spec
        pUsb->ctrlReq(bAddress, mkSETUP_PKT16(USB_SETUP_RECIPIENT_INTERFACE, USB_REQUEST_SET_INTERFACE, bAlternateSetting, bIface, 0), 0, NULL);
        //if(rcode) goto Fail;

        // delay a bit...
        if(!UHS_SLEEP_MS(50)) goto Fail;

        rcode = pUsb->getDevStatus(bAddress, 2, buf);
        if(rcode) goto Fail;

        //rcode  = GetHubStatus(4, buf);
        //if(rcode) goto Fail;

        return 0;

Fail:
        Release();
        return rcode;
};

uint8_t UHS_NI UHS_USBHub::Start(void) {
        uint8_t rcode;
        //uint8_t buf[8];
        rcode = pUsb->setEpInfoEntry(bAddress, bIface, 2, epInfo);

        if(rcode)
                goto Fail;

        //rcode  = GetHubStatus(4, buf);

        //printf("\r\n\r\n\r\nPORT_POWER\r\n");
        for(uint8_t j = 1; j <= bNbrPorts; j++) {
                //printf("PORT_POWER to %i\r\n",j);
                //rcode =
                ClearPortFeature(UHS_HUB_FEATURE_PORT_POWER, j, 0); //HubPortPowerOn(j);
                //printf("Address 0x%2.2x Port %i rcode 0x%2.2x\r\n\r\n", bAddress, j, rcode);
        }
        if(!UHS_SLEEP_MS(200)) goto Fail;
        for(uint8_t j = 1; j <= bNbrPorts; j++) {
                //printf("PORT_POWER to %i\r\n",j);
                //rcode =
                SetPortFeature(UHS_HUB_FEATURE_PORT_POWER, j, 0); //HubPortPowerOn(j);
                //printf("Address 0x%2.2x Port %i rcode 0x%2.2x\r\n\r\n", bAddress, j, rcode);
        }
        //printf("PORT_POWER DONE\r\n\r\n\r\n");

        // delay a bit...
        if(!UHS_SLEEP_MS(200)) goto Fail;

        if(bAddress == 1) {
                if(pUsb) {
                        // Tell host a hub is present on root.
                        pUsb->IsHub(true);
                }
        }
        qNextPollTime = millis() + 1000;
        bPollEnable = true;
        return 0;
Fail:
        Release();
        return rcode;
}

void UHS_NI UHS_USBHub::Release(void) {
        pUsb->DisablePoll();
        UHS_DeviceAddress a;
        AddressPool *apool = pUsb->GetAddressPool();
        for(uint8_t j = 1; j <= bNbrPorts; j++) {
                a.devAddress = apool->FindPortChildAddress(bAddress, j);
                if(a.bmAddress != 0) pUsb->ReleaseDevice(a.devAddress);
        }
        if(bAddress == 1) {
                if(pUsb) {
                        // Tell host hub on root is gone.
                        pUsb->IsHub(false);
                }
        }
        DriverDefaults();
        bNbrPorts = 0;
        pUsb->EnablePoll();
        return;
}

void UHS_NI UHS_USBHub::CheckHubStatus(void) {
        uint8_t rcode;
        uint8_t buf[8];
        uint16_t read = 1;
        UHS_HubEvent evt;
        evt.bmEvent = 0;
        rcode = pUsb->inTransfer(bAddress, 1, &read, buf);
        if(rcode) {
                HUB_DEBUG("UHS_USBHub::CheckHubStatus %2.2x\r\n", rcode);
                return;
        }

        for(uint8_t port = 1, mask = 0x02; port < 8; mask <<= 1, port++) {
                if(buf[0] & mask) {
                        evt.bmEvent = 0;

                        HUB_DEBUG("UHS_USBHub::CheckHubStatus CHANGE on port %i\r\n", port);
                        rcode = GetPortStatus(port, 4, evt.evtBuff);
                        // HUB_DEBUG("UHS_USBHub::CheckHubStatus-GetPortStatus port %i %2.2x\r\n", port, rcode);
                        if(rcode) {
                                HUB_DEBUG("UHS_USBHub::CheckHubStatus-GetPortStatus port %i %2.2x\r\n", port, rcode);
                                continue;
                        }
                        HUB_DEBUG("UHS_USBHub::CheckHubStatus CHANGE SAYS %4.4x STATUS SAYS %4.4x\r\n", evt.bmChange, evt.bmStatus);
                        rcode = PortStatusChange(port, evt);
                        // HUB_DEBUG("UHS_USBHub::CheckHubStatus-PortStatusChange %8.8x port %i %2.2x\r\n", evt.bmEvent, port, rcode);
                        if(rcode == UHS_HUB_ERROR_PORT_HAS_BEEN_RESET) {
                                HUB_DEBUG("UHS_USBHub::CheckHubStatus-PortStatusChange RESET %8.8lx port %i %2.2x\r\n", evt.bmEvent, port, rcode);
                                return;
                        }
                        if(rcode)
                                HUB_DEBUG("UHS_USBHub::CheckHubStatus-PortStatusChange OTHER %8.8lx port %i %2.2x\r\n", evt.bmEvent, port, rcode);
                                return;
                }
        }

        for(uint8_t port = 1; port <= bNbrPorts; port++) {
                UHS_HubEvent evt;
                evt.bmEvent = 0;

                rcode = GetPortStatus(port, 4, evt.evtBuff);

                if(rcode) {
                        HUB_DEBUG("UHS_USBHub::CheckHubStatus port check failed for %i with %2.2x\r\n", port, rcode);
                        continue;
                }
                if((evt.bmStatus & UHS_HUB_bmPORT_STATE_CHECK_DISABLED) != UHS_HUB_bmPORT_STATE_DISABLED) {
                                HUB_DEBUG("UHS_USBHub::CheckHubStatus DISABLED %8.8lx port %i %2.2x\r\n", evt.bmEvent, port, rcode);
                        continue;
                }
                // Emulate connection event for the port
                HUB_DEBUG("UHS_USBHub::CheckHubStatus EMULATE\r\n");
                evt.bmChange |= UHS_HUB_bmPORT_STATUS_PORT_CONNECTION;

                rcode = PortStatusChange(port, evt);

                if(rcode == UHS_HUB_ERROR_PORT_HAS_BEEN_RESET) {
                                HUB_DEBUG("UHS_USBHub::CheckHubStatus-PortStatusChange RESET2 %8.8lx port %i %2.2x\r\n", evt.bmEvent, port, rcode);
                        return;
                }
                if(rcode) {
                        HUB_DEBUG("UHS_USBHub::CheckHubStatus-PortStatusChange OTHER2 %8.8lx port %i %2.2x\r\n", evt.bmEvent, port, rcode);
                        return;
                }
        } // for
        return;
}

void UHS_NI UHS_USBHub::Poll(void) {
        if(bPollEnable) {
                if(((long)(millis() - qNextPollTime) >= 0L)) {
                        CheckHubStatus();
                        //printf("Tick\r\n");
                        if(((long)(millis() - qNextPollTime) >= 0L)) {
                                qNextPollTime = millis() + 100;
                        } // else internal function set it.
                }
        }
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
                if(evt.bmEvent == UHS_HUB_bmPORT_EVENT_RESET_COMPLETE || evt.bmEvent == UHS_HUB_bmPORT_EVENT_LS_RESET_COMPLETE || evt.bmEvent == UHS_HUB_bmPORT_EVENT_HS_RESET_COMPLETE ) {
                        break;
                }
                if(!UHS_SLEEP_MS(100)) return; // simulate polling, bail out early if parent state changes
        }

        UHS_SLEEP_MS(20);
        ClearPortFeature(UHS_HUB_FEATURE_C_PORT_RESET, port, 0);
        ClearPortFeature(UHS_HUB_FEATURE_C_PORT_CONNECTION, port, 0);
        UHS_SLEEP_MS(20);
}

uint8_t UHS_NI UHS_USBHub::PortStatusChange(uint8_t port, UHS_HubEvent &evt) {
        UHS_DeviceAddress a;

        a.devAddress = pUsb->GetAddressPool()->FindPortChildAddress(bAddress, port);
        //a.bmHub = 0;
        // This isn't correct.
        //a.bmParent = bAddress;
        //a.bmAddress = port;

        switch(evt.bmEvent) {
                        // Device connected event
                case UHS_HUB_bmPORT_EVENT_CONNECT:
                case UHS_HUB_bmPORT_EVENT_LS_CONNECT:
                case UHS_HUB_bmPORT_EVENT_HS_CONNECT:
                        if(bResetInitiated) break;

                        ClearPortFeature(UHS_HUB_FEATURE_C_PORT_ENABLE, port, 0);
                        ClearPortFeature(UHS_HUB_FEATURE_C_PORT_CONNECTION, port, 0);
                        SetPortFeature(UHS_HUB_FEATURE_PORT_RESET, port, 0);
                        bResetInitiated = true;
                        qNextPollTime = millis() + 200;
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
                case UHS_HUB_bmPORT_EVENT_HS_RESET_COMPLETE:
                        if(bResetInitiated) {
                                ClearPortFeature(UHS_HUB_FEATURE_C_PORT_RESET, port, 0);
                                ClearPortFeature(UHS_HUB_FEATURE_C_PORT_CONNECTION, port, 0);

                                UHS_SLEEP_MS(200);

                                a.devAddress = bAddress;
                                //uint8_t speed=((evt.bmStatus & UHS_HUB_bmPORT_STATUS_PORT_LOW_SPEED) == UHS_HUB_bmPORT_STATUS_PORT_LOW_SPEED ? 0 : 1);
                                uint8_t speed=((evt.bmStatus & (UHS_HUB_bmPORT_STATUS_PORT_LOW_SPEED | UHS_HUB_bmPORT_STATUS_PORT_HIGH_SPEED)) >> 9);
                                switch(speed) {
                                        case 0: {
                                                speed = 1;
                                                break;
                                        }
                                        case 1: {
                                                speed = 0;
                                                break;
                                        }
                                        default:
                                                break;
                                }
                                HOST_DEBUGx("USBHub configure %2.2x %2.2x %2.2x\r\n", a.bmAddress, port, speed);
#if DEBUG_PRINTF_EXTRA_HUGE
                                uint8_t rcode =
#endif
                                        pUsb->Configuring(a.bmAddress, port, speed);
                                HOST_DEBUGx("Configuring returned %2.2x\r\n", rcode);
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
