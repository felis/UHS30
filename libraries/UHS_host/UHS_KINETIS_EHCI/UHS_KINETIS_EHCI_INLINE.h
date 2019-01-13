/*
 * File:   UHS_KINETIS_EHCI_INLINE.h
 * Author: root
 *
 * Created on July 31, 2016, 1:01 AM
 */

// TO-DO: TX/RX packets.

#if defined(UHS_KINETIS_EHCI_H) && !defined(UHS_KINETIS_EHCI_LOADED)

#define UHS_KINETIS_EHCI_LOADED

static UHS_KINETIS_EHCI *_UHS_KINETIS_EHCI_THIS_;

static void UHS_NI call_ISR_kinetis_EHCI(void) {
        _UHS_KINETIS_EHCI_THIS_->ISRTask();
}

void UHS_NI UHS_KINETIS_EHCI::poopOutStatus() {
#if defined(EHCI_TEST_DEV)
        uint32_t n;

        n = USBHS_PORTSC1;
        if(n & USBHS_PORTSC_PR) {
                printf("reset ");
        }
        if(n & USBHS_PORTSC_PP) {
                printf("on ");
        } else {
                printf("off ");
        }
        if(n & USBHS_PORTSC_PHCD) {
                printf("phyoff ");
        }
        if(n & USBHS_PORTSC_PE) {
                if(n & USBHS_PORTSC_SUSP) {
                        printf("suspend ");
                } else {
                        printf("enable ");
                }
        } else {
                printf("disable ");
        }
        printf("speed=");
        switch(((n >> 26) & 3)) {
                case 0: printf("12 Mbps FS ");
                        break;
                case 1: printf("1.5 Mbps LS ");
                        break;
                case 2: printf("480 Mbps HS ");
                        break;
                default: printf("(undefined) ");
        }
        if(n & USBHS_PORTSC_HSP) {
                printf("high-speed ");
        }
        if(n & USBHS_PORTSC_OCA) {
                printf("overcurrent ");
        }
        if(n & USBHS_PORTSC_CCS) {
                printf("connected ");
        } else {
                printf("not-connected ");
        }

        printf(" run=%i", (USBHS_USBCMD & 1) ? 1 : 0); // running mode

        // print info about the EHCI status
        n = USBHS_USBSTS;
        printf(",SUSP=%i", n & USBHS_USBSTS_HCH ? 1 : 0);
        printf(",RECL=%i", n & USBHS_USBSTS_RCL ? 1 : 0);
        printf(",PSRUN=%i", n & USBHS_USBSTS_PS ? 1 : 0);
        printf(",ASRUN=%i", n & USBHS_USBSTS_AS ? 1 : 0);
        printf(",USBINT=%i", n & USBHS_USBSTS_UI ? 1 : 0);
        printf(",USBERRINT=%i", n & USBHS_USBSTS_UEI ? 1 : 0);
        printf(",PCHGINT=%i", n & USBHS_USBSTS_PCI ? 1 : 0);
        printf(",FRAMEINT=%i", n & USBHS_USBSTS_FRI ? 1 : 0);
        printf(",errINT=%i", n & USBHS_USBSTS_SEI ? 1 : 0);
        printf(",AAINT=%i", n & USBHS_USBSTS_AAI ? 1 : 0);
        printf(",RSTINT=%i", n & USBHS_USBSTS_URI ? 1 : 0);
        printf(",SOFINT=%i", n & USBHS_USBSTS_SRI ? 1 : 0);
        printf(",NAKINT=%i", n & USBHS_USBSTS_NAKI ? 1 : 0);
        printf(",ASINT=%i", n & USBHS_USBSTS_UAI ? 1 : 0);
        printf(",PSINT=%i", n & USBHS_USBSTS_UPI ? 1 : 0);
        printf(",T0INT=%i", n & USBHS_USBSTS_TI0 ? 1 : 0);
        printf(",T1INT=%i", n & USBHS_USBSTS_TI1 ? 1 : 0);

        printf(",index=%lu\r\n", USBHS_FRINDEX); // periodic index

#endif
}

/*
 * This will be part of packet dispatch.
        do {
                s = (USBHS_USBSTS & USBHS_USBSTS_AS) | (USBHS_USBCMD & USBHS_USBCMD_ASE);
        } while((s == USBHS_USBSTS_AS) || (s == USBHS_USBCMD_ASE));
        USBHS_USBCMD |= USBHS_USBCMD_ASE;
        while (!(USBHS_USBSTS & USBHS_USBSTS_AS)); // spin
 */


void UHS_NI UHS_KINETIS_EHCI::busprobe(void) {
        uint8_t speed = 1;


        switch(vbusState) {
                case 0: // Full speed
                        break;
                case 1: // Low speed
                        speed = 0;
                        break;
                case 2: // high speed
                        speed = 2;
                        break;
                default:
                        // no speed.. hrmmm :-)
                        speed = 15;
                        break;
        }
        HOST_DUBUG("USB host speed now %1.1x\r\n", speed);
        usb_host_speed = speed;
        if(speed == 2) {
                UHS_KIO_SETBIT_ATOMIC(USBPHY_CTRL, USBPHY_CTRL_ENHOSTDISCONDETECT);
                //USBPHY_CTRL |= USBPHY_CTRL_ENHOSTDISCONDETECT;
        } else {
                UHS_KIO_CLRBIT_ATOMIC(USBPHY_CTRL, USBPHY_CTRL_ENHOSTDISCONDETECT);
                //USBPHY_CTRL &= ~USBPHY_CTRL_ENHOSTDISCONDETECT;
        }
}

void UHS_NI UHS_KINETIS_EHCI::VBUS_changed(void) {
        /* modify USB task state because Vbus changed or unknown */
        // printf("\r\n\r\n\r\n\r\nSTATE %2.2x -> ", usb_task_state);
        ReleaseChildren();
        timer_countdown = 0;
        sof_countdown = 0;
        switch(vbusState) {
                case 0: // Full speed
                case 1: // Low speed
                case 2: // high speed
                        usb_task_state = UHS_USB_HOST_STATE_DEBOUNCE;
                        break;
                default:
                        usb_task_state = UHS_USB_HOST_STATE_IDLE;
                        break;
        }
        // printf("0x%2.2x\r\n\r\n\r\n\r\n", usb_task_state);
        return;
};

/**
 * Bottom half of the ISR task
 */
void UHS_NI UHS_KINETIS_EHCI::ISRbottom(void) {
        uint8_t x;
#if LED_STATUS
        digitalWriteFast(34, HIGH);
#endif
        if(condet) {
                VBUS_changed();
                noInterrupts();
                condet = false;
                interrupts();
        }

        HOST_DUBUG("ISRbottom, usb_task_state: 0x%0X \r\n", (uint8_t)usb_task_state);

        switch(usb_task_state) {
                case UHS_USB_HOST_STATE_INITIALIZE: /* 0x10 */ // initial state
                        //printf("ISRbottom, UHS_USB_HOST_STATE_INITIALIZE\r\n");
                        // if an attach happens we will detect it in the isr
                        // update usb_task_state and check speed (so we replace busprobe and VBUS_changed methods)
                        break;
                case UHS_USB_HOST_STATE_DEBOUNCE: /* 0x01 */
                        //printf("ISRbottom, UHS_USB_HOST_STATE_DEBOUNCE\r\n");
                        sof_countdown = UHS_HOST_DEBOUNCE_DELAY_MS;
                        usb_task_state = UHS_USB_HOST_STATE_DEBOUNCE_NOT_COMPLETE;
                        break;
                case UHS_USB_HOST_STATE_DEBOUNCE_NOT_COMPLETE: /* 0x02 */
                        //settle time for just attached device
                        //printf("ISRbottom, UHS_USB_HOST_STATE_DEBOUNCE_NOT_COMPLETE\r\n");
                        usb_task_state = UHS_USB_HOST_STATE_RESET_DEVICE;
                        break;

                case UHS_USB_HOST_STATE_RESET_DEVICE: /* 0x0A */
                        //printf("ISRbottom, UHS_USB_HOST_STATE_RESET_DEVICE\r\n");
                        noInterrupts();
                        busevent = true;
                        doingreset = true;
                        usb_task_state = UHS_USB_HOST_STATE_RESET_NOT_COMPLETE;
                        //issue bus reset
                        UHS_KIO_SETBIT_ATOMIC(USBHS_PORTSC1, USBHS_PORTSC_PR);
                        //USBHS_PORTSC1 |= USBHS_PORTSC_PR;
                        interrupts();
                        //timer_countdown = 20;
                        break;
                case UHS_USB_HOST_STATE_RESET_NOT_COMPLETE: /* 0x03 */
                        //printf("ISRbottom, UHS_USB_HOST_STATE_RESET_NOT_COMPLETE\r\n");
                        if(!busevent) usb_task_state = UHS_USB_HOST_STATE_WAIT_BUS_READY;
                        // We delay two extra ms to ensure that at least one SOF has been sent.
                        // This trick is performed by just moving to the next state.
                        break;
                case UHS_USB_HOST_STATE_WAIT_BUS_READY: /* 0x05 */
                        noInterrupts();
                        doingreset = false;
                        DDSB();
                        interrupts();
                        //printf("ISRbottom, UHS_USB_HOST_STATE_WAIT_BUS_READY\r\n");
                        usb_task_state = UHS_USB_HOST_STATE_CONFIGURING;
                        break; // don't fall through

                case UHS_USB_HOST_STATE_CONFIGURING: /* 0x0C */
                        HOST_DUBUG("ISRbottom, UHS_USB_HOST_STATE_CONFIGURING\r\n");
                        usb_task_state = UHS_USB_HOST_STATE_CHECK;
                        x = Configuring(0, 1, usb_host_speed);
                        if(usb_task_state == UHS_USB_HOST_STATE_CHECK) {
                                if(x) {
                                        if(x == UHS_HOST_ERROR_JERR) {
                                                usb_task_state = UHS_USB_HOST_STATE_IDLE;
                                        } else if(x != UHS_HOST_ERROR_DEVICE_INIT_INCOMPLETE) {
                                                usb_error = x;
                                                usb_task_state = UHS_USB_HOST_STATE_ERROR;
                                        }
                                } else
                                        usb_task_state = UHS_USB_HOST_STATE_CONFIGURING_DONE;
                        }
                        break;
                case UHS_USB_HOST_STATE_CONFIGURING_DONE: /* 0x0D */
                        HOST_DUBUG("ISRbottom, UHS_USB_HOST_STATE_CONFIGURING_DONE\r\n");
                        usb_task_state = UHS_USB_HOST_STATE_RUNNING;
                        break;
                case UHS_USB_HOST_STATE_CHECK: /* 0x0E */
                        // Serial.println((uint32_t)__builtin_return_address(0),HEX);
                        break;
                case UHS_USB_HOST_STATE_ERROR: /* 0xF0 */
                        HOST_DUBUG("ISRbottom, error state, die here\r\n");
                        while(1);
                        break;
                case UHS_USB_HOST_STATE_RUNNING: /* 0x60 */
                        //printf("ISRbottom, UHS_USB_HOST_STATE_RUNNING\r\n");
                        Poll_Others();
                        for(x = 0; (usb_task_state == UHS_USB_HOST_STATE_RUNNING) && (x < UHS_HOST_MAX_INTERFACE_DRIVERS); x++) {
                                if(devConfig[x]) {
                                        if(devConfig[x]->bPollEnable) devConfig[x]->Poll();
                                }
                        }
                        // fall thru
                default:
                        //printf("ISRbottom, default\r\n");
                        // Do nothing
                        break;
        } // switch( usb_task_state )
        if(condet) {
                VBUS_changed();
                noInterrupts();
                condet = false;
                interrupts();
        }
        usb_task_polling_disabled--;
#if LED_STATUS
        digitalWriteFast(34, LOW);
#endif
}

/**
 * Top half of the ISR. This is what services the hardware, and calls the bottom half as-needed.
 *
 * SPECIAL NOTES:
 *      1: After an error, set isrError to zero.
 *
 *      2: If DMA bandwidth is not enough, UHS_HOST_ERROR_NAK is returned.
 *              Drivers that have NAK processing know to retry.
 */


void UHS_NI UHS_KINETIS_EHCI::Task(void) {
}

void UHS_NI UHS_KINETIS_EHCI::ISRTask(void) {
        counted = false;
        uint32_t Ostat = USBHS_OTGSC; // OTG status
        uint32_t Ustat = USBHS_USBSTS /*& USBHS_USBINTR */; // USB status
        uint32_t Pstat = USBHS_PORTSC1; // Port status
        USBHS_OTGSC &= Ostat;
        USBHS_USBSTS &= Ustat;

        if(Ustat & USBHS_USBSTS_PCI) {
                // port change
                vbusState = ((Pstat & 0x04000000U) ? 1 : 0) | ((Pstat & 0x08000000U) ? 2 : 0);
#if defined(EHCI_TEST_DEV)
                HOST_DUBUG("PCI Vbus state changed to %1.1x\r\n", vbusState);
#endif
                busprobe();
                if(!doingreset) condet = true;
                busevent = false;
#if LED_STATUS
                // toggle LEDs so a mere human can see them
                digitalWriteFast(31, CL1);
                digitalWriteFast(32, CL2);
                CL1 = !CL1;
                CL2 = !CL2;
#endif
        }

        if(Ustat & USBHS_USBSTS_SEI) {
        }
        if(Ustat & USBHS_USBSTS_FRI) {
                // nothing yet...
        }

        if((Ustat & USBHS_USBSTS_UEI) || (Ustat & USBHS_USBSTS_UI)) {
                // USB interrupt or USB error interrupt both end a transaction
                if(Ustat & USBHS_USBSTS_UEI) {
                        newError = true;
                        // set isrError to one of the named errors...
                }
                if(Ustat & USBHS_USBSTS_UI) {
                        isrHappened = true;
                        // May, or may not be needed...
                }
        }

        if(Ostat & USBHS_OTGSC_MSS) {
#if LED_STATUS
                CL3 = !CL3;
                digitalWriteFast(33, CL3);
#endif
                if(timer_countdown) {
                        timer_countdown--;
                        counted = true;
                }
        }

        if(Ustat & USBHS_USBINTR_SRE) {
                if(sof_countdown) {
                        sof_countdown--;
                        counted = true;
                }
        }

#if LED_STATUS
        // shit out speed status on LEDs
        // Indicate speed on 2,3
        digitalWriteFast(2, (Pstat & 0x04000000U) ? 1 : 0);
        digitalWriteFast(3, (Pstat & 0x08000000U) ? 1 : 0);
        // connected on pin 4
        digitalWriteFast(4, (Pstat & USBHS_PORTSC_CCS) ? 1 : 0);
#endif
        //USBHS_PORTSC1 &= Pstat;

        DDSB();
        if(!timer_countdown && !sof_countdown && !counted && !usb_task_polling_disabled) {
                usb_task_polling_disabled++;

                // ARM uses SWI for bottom half
                // NVIC disallows reentrant IRQ
                exec_SWI(this);
        }
}

/**
 * Initialize USB hardware, turn on VBUS
 *
 * @param mseconds Delay energizing VBUS after mseconds, A value of INT16_MIN means no delay.
 * @return 0 on success, -1 on error
 */
int16_t UHS_NI UHS_KINETIS_EHCI::Init(int16_t mseconds) {
#if LED_STATUS
        // These are updated in the ISR and used to see if the code is working correctly.

        // Speed
        pinMode(2, OUTPUT);
        pinMode(3, OUTPUT);

        // Connected
        pinMode(4, OUTPUT);

        // Port change detect, Initial state is both off
        pinMode(31, OUTPUT);
        pinMode(32, OUTPUT);

        // 1millisec IRQ
        pinMode(33, OUTPUT);

        // in bottom half
        pinMode(34, OUTPUT);

        CL1 = false;
        CL2 = true;
        CL3 = false;
#endif
        Init_dyn_SWI();
        _UHS_KINETIS_EHCI_THIS_ = this;

        memset(&qHalt, 0, sizeof (qHalt));
        qHalt.transferResults = 0x40;

#if defined(UHS_FUTURE)
#if defined(EHCI_TEST_DEV)
        printf("*Q = %p\r\n", &Q);
        printf("*Q.qh = %p\r\n", (Q.qh));
        printf("*Q.qh[0] = %p\r\n", &(Q.qh[0]));
        printf("*Q.qh[1] = %p\r\n\n", &(Q.qh[1]));
        printf("*Q.qtd[0] = %p\r\n", &(Q.qtd[0]));
        printf("*Q.qtd[1] = %p\r\n\n", &(Q.qtd[1]));
#if defined(UHS_FUTURE)
        printf("*Q.itd[0] = %p\r\n", &(Q.itd[0]));
        printf("*Q.itd[1] = %p\r\n\n", &(Q.itd[1]));
        printf("*Q.sitd[0] = %p\r\n", &(Q.sitd[0]));
        printf("*Q.sitd[1] = %p\r\n\n", &(Q.sitd[1]));
#endif
#endif
        // Zero entire Q structure.
        uint8_t *bz = (uint8_t *)(&Q);
#if defined(EHCI_TEST_DEV)
        printf("Structure size = 0x%x ...", sizeof (Qs_t));
#endif
        memset(bz, 0, sizeof (Qs_t));
#if defined(EHCI_TEST_DEV)
        printf(" Zeroed");
        // Init queue heads
        printf("\r\nInit QH %u queue heads...", UHS_KEHCI_MAX_QH);
#endif
        for(unsigned int i = 0; i < UHS_KEHCI_MAX_QH; i++) {
                Q.qh[i].horizontalLinkPointer = 1;
                //2LU | (uint32_t)&(Q.qh[i + 1]);
                Q.qh[i].currentQtdPointer = 1;
                Q.qh[i].nextQtdPointer = 1;
                Q.qh[i].alternateNextQtdPointer = 1;

        }
        Q.qh[UHS_KEHCI_MAX_QH - 1].horizontalLinkPointer = (uint32_t)3;

        // Init queue transfer descriptors
#if defined(EHCI_TEST_DEV)
        printf("\r\nInit QTD %u queue transfer descriptors...", UHS_KEHCI_MAX_QTD);
#endif
        for(unsigned int i = 0; i < UHS_KEHCI_MAX_QTD; i++) {
                Q.qtd[i].nextQtdPointer = 1;
                // (uint32_t)&(Q.qtd[i + 1]);
                Q.qtd[i].alternateNextQtdPointer = 1;
                //Q.qtd[i].bufferPointers;
                //Q.qtd[i].transferResults;
        }
        Q.qtd[UHS_KEHCI_MAX_QTD - 1].nextQtdPointer = (uint32_t)NULL;

        // Init isochronous transfer descriptors
#if defined(UHS_FUTURE)
#if defined(EHCI_TEST_DEV)
        printf("\r\nInit ITD %u isochronous transfer descriptors...", UHS_KEHCI_MAX_ITD);
#endif
        for(unsigned int i = 0; i < UHS_KEHCI_MAX_ITD; i++) {
                Q.itd[i].nextLinkPointer = (uint32_t)&(Q.itd[i + 1]);
        }
        Q.itd[UHS_KEHCI_MAX_ITD - 1].nextLinkPointer = (uint32_t)NULL;

        // Init split transaction isochronous transfer descriptors
#if defined(EHCI_TEST_DEV)
        printf("\r\nInit SITD %u split transaction isochronous transfer descriptors...", UHS_KEHCI_MAX_SITD);
#endif
        for(unsigned int i = 0; i < UHS_KEHCI_MAX_SITD; i++) {
                Q.sitd[i].nextLinkPointer = (uint32_t)&(Q.sitd[i + 1]);
        }
        Q.sitd[UHS_KEHCI_MAX_SITD - 1].nextLinkPointer = (uint32_t)NULL;
#endif
#endif // UHS_FUTURE



#if defined(EHCI_TEST_DEV)
        printf("\r\n");
#endif

#if defined(UHS_FUTURE)
        for(int i = 0; i < UHS_KEHCI_MAX_FRAMES; i++) {
                frame[i] = 1;
        }
#endif

#ifdef HAS_KINETIS_MPU
        MPU_RGDAAC0 |= 0x30000000;
#endif
        PORTE_PCR6 = PORT_PCR_MUX(1);
        GPIOE_PDDR |= (1 << 6);

        vbusPower(vbus_off);
        // Delay a minimum of 1 second to ensure any capacitors are drained.
        // 1 second is required to make sure we do not smoke a Microdrive!
        if(mseconds != INT16_MIN) {
                if(mseconds < 1000) mseconds = 1000;
                delay(mseconds); // We can't depend on SOF timer here.
        }
        vbusPower(vbus_on);


        MCG_C1 |= MCG_C1_IRCLKEN; // enable MCGIRCLK 32kHz
        OSC0_CR |= OSC_ERCLKEN;
        SIM_SOPT2 |= SIM_SOPT2_USBREGEN; // turn on USB regulator
        SIM_SOPT2 &= ~SIM_SOPT2_USBSLSRC; // use IRC for slow clock
#if defined(EHCI_TEST_DEV)
        printf("power up USBHS PHY\r\n");
#endif
        SIM_USBPHYCTL |= SIM_USBPHYCTL_USBDISILIM; // disable USB current limit
        //SIM_USBPHYCTL = SIM_USBPHYCTL_USBDISILIM | SIM_USBPHYCTL_USB3VOUTTRG(6); // pg 237
        SIM_SCGC3 |= SIM_SCGC3_USBHSDCD | SIM_SCGC3_USBHSPHY | SIM_SCGC3_USBHS;
        USBHSDCD_CLOCK = 33 << 2;
#if defined(EHCI_TEST_DEV)
        printf("init USBHS PHY & PLL\r\n");
#endif
        // init process: page 1681-1682
        USBPHY_CTRL_CLR = (USBPHY_CTRL_SFTRST | USBPHY_CTRL_CLKGATE); // // CTRL pg 1698
        USBPHY_TRIM_OVERRIDE_EN_SET = 1;
        USBPHY_PLL_SIC = USBPHY_PLL_SIC_PLL_POWER | USBPHY_PLL_SIC_PLL_ENABLE |
                USBPHY_PLL_SIC_PLL_DIV_SEL(1) | USBPHY_PLL_SIC_PLL_EN_USB_CLKS;
        // wait for the PLL to lock
        int count = 0;
        while((USBPHY_PLL_SIC & USBPHY_PLL_SIC_PLL_LOCK) == 0) {
                count++;
        }
#if defined(EHCI_TEST_DEV)
        printf("PLL locked, waited %i\r\n", count);
#endif
        // turn on power to PHY
        USBPHY_PWD = 0;
        delay(10);
#if defined(EHCI_TEST_DEV)
        printf("begin ehci reset\r\n");
#endif
        USBHS_USBCMD |= USBHS_USBCMD_RST;
        count = 0;
        while(USBHS_USBCMD & USBHS_USBCMD_RST) {
                count++;
        }
#if defined(EHCI_TEST_DEV)
        printf("reset waited %i\r\n", count);
#endif
        // turn on the USBHS controller
        USBHS_USBMODE = /* USBHS_USBMODE_TXHSD(5) |*/ USBHS_USBMODE_CM(3); // host mode
        USBHS_FRINDEX = 0;
        USBHS_USBINTR = 0;
#if defined(EHCI_TEST_DEV)
        poopOutStatus();
#endif
        USBHS_USBCMD = USBHS_USBCMD_ITC(0) | USBHS_USBCMD_RS | USBHS_USBCMD_ASP(3) |
                USBHS_USBCMD_FS2 | USBHS_USBCMD_FS(0); // periodic table is 64 pointers

#if defined(UHS_FUTURE)
        uint32_t s;
        // periodic
        do {
                s = (USBHS_USBSTS & USBHS_USBSTS_PS) | (USBHS_USBCMD & USBHS_USBCMD_PSE);
        } while((s == USBHS_USBSTS_PS) || (s == USBHS_USBCMD_PSE));
        USBHS_PERIODICLISTBASE = (uint32_t)frame;
        USBHS_USBCMD |= USBHS_USBCMD_PSE;
#endif
        // async list
        //USBHS_ASYNCLISTADDR = (uint32_t)(Q.qh);
        USBHS_ASYNCLISTADDR = 0;

        USBHS_PORTSC1 |= USBHS_PORTSC_PP;

#if defined(EHCI_TEST_DEV)
        poopOutStatus();
#endif

        USBHS_OTGSC =
                //        USBHS_OTGSC_BSEIE | // B session end
                //        USBHS_OTGSC_BSVIE | // B session valid
                //        USBHS_OTGSC_IDIE  | // USB ID
                USBHS_OTGSC_MSS | // 1mS timer
                //        USBHS_OTGSC_BSEIS | // B session end
                //        USBHS_OTGSC_BSVIS | // B session valid
                //        USBHS_OTGSC_ASVIS | // A session valid
                //        USBHS_OTGSC_IDIS  | // A VBUS valid
                //        USBHS_OTGSC_HABA  | // Hardware Assist B-Disconnect to A-connect
                //        USBHS_OTGSC_IDPU  | // ID pullup
                //        USBHS_OTGSC_DP    | // Data pule IRQ enable
                //        USBHS_OTGSC_HAAR  | // Hardware Assist Auto-Reset

                //        USBHS_OTGSC_ASVIE | // A session valid
                //        USBHS_OTGSC_AVVIE | // A VBUS valid
                USBHS_OTGSC_MSE // 1mS timer
                ;
        // enable interrupts
        USBHS_USBINTR =
                //        USBHS_USBINTR_TIE1 | // GP timer 1
                //        USBHS_USBINTR_TIE0 | // GP timer 0
                //        USBHS_USBINTR_UPIE | // Host Periodic
                //        USBHS_USBINTR_UAIE | // Host Asynchronous
                //        USBHS_USBINTR_NAKE | // NAK
                //        USBHS_USBINTR_SLE  | // Sleep
                USBHS_USBINTR_SRE | // SOF
                //        USBHS_USBINTR_URE  | // Reset
                //        USBHS_USBINTR_AAE  | // Async advance
                USBHS_USBINTR_SEE | // System Error
                USBHS_USBINTR_FRE | // Frame list rollover
                USBHS_USBINTR_PCE | // Port change detect
                USBHS_USBINTR_UEE | // Error
                USBHS_USBINTR_UE // Enable
                ;

        // switch isr for USB
        noInterrupts();
        NVIC_DISABLE_IRQ(IRQ_USBHS);
        NVIC_SET_PRIORITY(IRQ_USBHS, 112);
        _VectorsRam[IRQ_USBHS + 16] = call_ISR_kinetis_EHCI;
        DDSB();
        NVIC_ENABLE_IRQ(IRQ_USBHS);
        interrupts();

        return 0;
}

static uint32_t QH_capabilities1(uint32_t nak_count_reload, uint32_t control_endpoint_flag,
        uint32_t max_packet_length, uint32_t head_of_list, uint32_t data_toggle_control,
        uint32_t speed, uint32_t endpoint_number, uint32_t inactivate, uint32_t address) {
        return ( (nak_count_reload << 28) | (control_endpoint_flag << 27) |
                (max_packet_length << 16) | (head_of_list << 15) |
                (data_toggle_control << 14) | (speed << 12) | (endpoint_number << 8) |
                (inactivate << 7) | (address << 0));
}

static uint32_t QH_capabilities2(uint32_t high_bw_mult, uint32_t hub_port_number,
        uint32_t hub_address, uint32_t split_completion_mask, uint32_t interrupt_schedule_mask) {
        return ( (high_bw_mult << 30) | (hub_port_number << 23) | (hub_address << 16) |
                (split_completion_mask << 8) | (interrupt_schedule_mask << 0));
}

// Fill in the qTD fields (token & data)
//   t       the Transfer qTD to initialize
//   buf     data to transfer
//   len     length of data  (up to 16K for any address, or up to 20K if 4K aligned)
//   pid     type of packet: 0=OUT, 1=IN, 2=SETUP
//   data01  value of DATA0/DATA1 toggle on 1st packet
//   irq     whether to generate an interrupt when transfer complete
//

void UHS_KINETIS_EHCI::init_qTD(void *buf, uint32_t len, uint32_t pid, uint32_t data01, bool irq) {
        qTD.nextQtdPointer = (uint32_t) & qHalt;
        qTD.alternateNextQtdPointer = (uint32_t) & qHalt;
        if(data01) data01 = 0x80000000;
        qTD.transferResults = data01 | (len << 16) | (irq ? 0x8000 : 0) | (pid << 8) | 0x80;
        uint32_t addr = (uint32_t)buf;
        qTD.bufferPointers[0] = addr;
        addr &= 0xFFFFF000;
        qTD.bufferPointers[1] = addr + 0x1000;
        qTD.bufferPointers[2] = addr + 0x2000;
        qTD.bufferPointers[3] = addr + 0x3000;
        qTD.bufferPointers[4] = addr + 0x4000;
}

/*
This is a simplification & small subset of EHCI adapted for UHS.  If you read the
EHCI spec, normally a driver would dynamically allocate one QH structure for each
endpoint in every device.  The many QH structures would be placed into a circular
linked list for the asynchronous schedule or an inverted binary tree for the
periodic schedule.  For actual data transfer, qTD structures would be dynamically
allocated and hot-inserted to the linked list from the QH representing the
desired endpoint.  Additional list structures outside the scope of EHCI would
normally be used to manage qTD structures after EHCI completes their work
requirements.  As USB devices & hubs connect and disconnect, special rules need
to be followed to dynamically change the QH lists while EHCI is actively using
them.  The normal EHCI usage model is very complex!

UHS uses a much simpler model, where SetAddress() is first called to configure
the hardware to communicate with one endpoint on a particular device.  Then the
functions below are called to perform the USB communication.  To meet this usage
model, a single QH structure is connected to EHCI's asynchronous schedule.  The
periodic schedule is not used.  When SetAddress() requests a different device or
endpoint, this one QH is reconfigured.

Data transfer is accomplished using two qTD structures.  qHalt is reserved for
placing the QH into its halted state.  When no data transfer is needed, the
QH.nextQtdPointer field must always have the address of qHalt.  This is how
the QH "knows" no more data transfers are to be attempted.  When data transfer
is needed, the main qTD structure is initialized with the details and a pointer
to the actual buffer which sources or sinks USB data.  This main qTD structure
must have its nextQtdPointer field set to the address of qHalt *before* it is
used.  Then to cause EHCI to perform the data transfer, the QH.nextQtdPointer
is written to the address of the main qTD.  When the transfer completes, EHCI
automatically updates the QH.nextQtdPointer field to the qTD.nextQtdPointer.
The QH returns to its halted state because the main qTD points to the qHalt
structure.
 */

uint8_t UHS_NI UHS_KINETIS_EHCI::SetAddress(uint8_t addr, uint8_t ep, UHS_EpInfo **ppep, uint16_t & nak_limit) {
        HOST_DUBUG("SetAddress, addr=%d, ep=%x\r\n", addr, ep);

        UHS_Device *p = addrPool.GetUsbDevicePtr(addr);
        if(!p) return UHS_HOST_ERROR_NO_ADDRESS_IN_POOL;

        if(!p->epinfo) return UHS_HOST_ERROR_NULL_EPINFO;
        *ppep = getEpInfoEntry(addr, ep);
        if(!*ppep) return UHS_HOST_ERROR_NO_ENDPOINT_IN_TABLE;
        nak_limit = (0x0001UL << (((*ppep)->bmNakPower > UHS_USB_NAK_MAX_POWER) ? UHS_USB_NAK_MAX_POWER : (*ppep)->bmNakPower));
        nak_limit--;

        USBHS_USBCMD &= ~USBHS_USBCMD_ASE;
        while((USBHS_USBSTS & USBHS_USBSTS_AS)); // wait for async schedule disable

        uint32_t type = 0; // 0=control, 2=bulk, 3=interrupt
        if((ep & 0x7F) > 0) {
                // From Andrew:
                //   There are currently only bulk packets.
                //   Interrupt variety can be added to the structure, but this won't be used yet.
                type = 2; // assume bulk ep != 0
        }
        uint32_t speed;
        if(p->speed == 2) {
                speed = 2; // 480 Mbit/sec
        } else if(p->speed == 1) {
                speed = 0; // 12 Mbit/sec
        } else {
                speed = 1; // 1.5 Mbit/sec
        }
        uint32_t c = 0;

        // TODO: May be a problem with maxPktSize.  When enumerating a device with
        // less than 64, the enumeration process correctly reads and learns the
        // endpoint 0 max packet size, using it to read the device descriptor.
        // But then after a bus reset and setting the address, reading the
        // config descriptor seems to give maxPktSize=64, rather than the correct
        // maxPktSize detected as the first step.
        // Paul: Never seen this happen, save for a few broken devices.

        uint32_t maxlen = (*ppep)->maxPktSize;
        HOST_DUBUG("SetAddress, speed=%ld, maxlen=%ld\r\n", speed, maxlen);
        // maxlen = 16; // uncomment for testing with device having ep0 maxlen=16

        // TODO, bmParent & bmAddress do not seem to always work
        // Paul: these are not what you think they are, that's why :-)

        uint32_t hub_addr = p->address.bmParent;
        uint32_t hub_port = 1; // hub ports start at zero
        if(hub_addr) {
                // get port number from hub, somehow... perhaps needs to be a new field... argh
                // Paul:
                // bmParent (if non-zero) == the parent hub, zero == the host controller
                // bmAddress == the actual address we want to talk to, however
                // hubs are stupid devices normally.
                //
                // Since EHCI is twiddling TT's we'll have to note, or ask external hubs what port.
                // This isn't implemented yet, so for now we are stuck and can't use hubs...
                // We will be able to decode port from address, though
                // bool is_a_hub = (bmUSB_DEV_ADDR_HUB & addr) == bmUSB_DEV_ADDR_HUB;
                // uint8_t port = (bmUSB_DEV_ADDR_ADDRESS & addr);
                
        }
        printf("SetAddress, parent=%lu, parent_port=%lu\r\n", hub_addr, hub_port);

        if(type == 0 && speed != 2) {
                c = 1;
        }
        qHalt.nextQtdPointer = 1;
        qHalt.alternateNextQtdPointer = 1;
        qHalt.transferResults = 0x40;
        memset(&QH, 0, sizeof (QH));
        QH.horizontalLinkPointer = (uint32_t) & QH | 2;
        QH.staticEndpointStates[0] = QH_capabilities1(15, c, maxlen, 1, 1, speed, ep, 0, addr);
        QH.staticEndpointStates[1] = QH_capabilities2(1, hub_port, hub_addr, 0, 0);
        QH.nextQtdPointer = (uint32_t) & qHalt;
        QH.alternateNextQtdPointer = (uint32_t) & qHalt;
#if 0
        // this can stop at specific locations in the enumeration process
        // helpful if things go awry, spewing endless data that scrolls
        // the important stuff off the window.
        static int diecount = 0;
        if(++diecount > 5) {
                printf("DIE HERE\r\n");
                while(1);
        }
#endif
        USBHS_ASYNCLISTADDR = (uint32_t) & QH;
        USBHS_USBCMD |= USBHS_USBCMD_ASE;

        return UHS_HOST_ERROR_NONE;
}

uint8_t UHS_NI UHS_KINETIS_EHCI::dispatchPkt(uint8_t token, uint8_t ep, uint16_t nak_limit) {
        QH.transferOverlayResults[0] = 0;
        QH.nextQtdPointer = (uint32_t) & qTD;

        // Nope... We signal really bad shit from ISR.
        //uint32_t usec_timeout = 1200; // TODO: use data length & speed
        //elapsedMicros usec=0;
        newError = false;
        isrHappened = false;

        while(!condet) {
                // Paul: Better to watch this from the ISR.
                // Wait for a state change.
                // See UHS_KINETIS_FS_HOST_INLINE.h
                uint32_t status = qTD.transferResults;
                //HOST_DUBUG("dispatchPkt %lx\r\n", status);
                if(!(status & 0x80)) {
                        if(!(status & 0xffu)) {
                                // no longer active, not halted, no errors... so ok
                                return UHS_HOST_ERROR_NONE;
                        }
                        // one of many possible errors
                        // TODO: do we need to clear halt condition or
                        // do anything else special here to deal with errors?
                        // no. NAK != real error though...
                        // Paul: 2 options- Translate in the ISR, or translate here.
                        //       It is easier to translate in the ISR.
                        //       See UHS_KINETIS_FS_HOST_INLINE.h
                        return UHS_HOST_ERROR_NAK;
                }
                if(newError) {
                        // we need to get the actual translated error, for now, we'll say NAK here too
                        return UHS_HOST_ERROR_NAK;
                }
                //if(isrHappened) {
                // ditto...
                //        return UHS_HOST_ERROR_NAK;
                //}
        }
        if(condet) return UHS_HOST_ERROR_UNPLUGGED;
        return UHS_HOST_ERROR_TIMEOUT;
}

uint8_t UHS_NI UHS_KINETIS_EHCI::InTransfer(UHS_EpInfo *pep, uint16_t nak_limit, uint16_t *nbytesptr, uint8_t *data) {
        HOST_DUBUG("InTransfer %d\r\n", *nbytesptr);
        init_qTD(data, *nbytesptr, 1, pep->bmRcvToggle, false);
        uint8_t rcode = dispatchPkt(0, 0, nak_limit);
        uint32_t status = qTD.transferResults;
        *nbytesptr -= (status >> 16) & 0x7FFF;
        pep->bmRcvToggle = status >> 31;
        return rcode;
}

UHS_EpInfo * UHS_NI UHS_KINETIS_EHCI::ctrlReqOpen(uint8_t addr, uint64_t Request, uint8_t *dataptr) {
        HOST_DUBUG("ctrlReqOpen\r\n");

        UHS_EpInfo *pep = NULL;
        uint16_t nak_limit = 0;

        uint8_t rcode = SetAddress(addr, 0, &pep, nak_limit);
        if(!rcode) {
                static uint8_t setupbuf[8];
                memcpy(setupbuf, &Request, 8);
                init_qTD(setupbuf, 8, 2, 0, false);
                rcode = dispatchPkt(0, 0, nak_limit);
                if(!rcode) {
                        if(dataptr != NULL) {
                                if(((Request) /* bmReqType */ & 0x80) == 0x80) {
                                        pep->bmRcvToggle = 1; //bmRCVTOG1;
                                } else {
                                        pep->bmSndToggle = 1; //bmSNDTOG1;
                                }
                        }
                } else {
                        pep = NULL;
                }
        }
        return pep;
}

uint8_t UHS_NI UHS_KINETIS_EHCI::ctrlReqRead(UHS_EpInfo *pep, uint16_t *left, uint16_t *read, uint16_t nbytes, uint8_t * dataptr) {
        printf("ctrlReqRead left: %i, nbytes: %i, dataptr: %lx\r\n",
                *left, nbytes, (uint32_t)dataptr);
#if 1
        uint8_t rcode = 0;
        if(*left) {
                *read = nbytes;
                rcode = InTransfer(pep, 0, read, dataptr);
                if(rcode) {
                        HOST_DUBUG("ctrlReqRead ERROR: %2.2x, left: %i, read %i\r\n", rcode, *left, *read);
                } else {
                        *left -= *read;
                        HOST_DUBUG("ctrlReqRead left: %i, read %i\r\n", *left, *read);
                }
        }
        return rcode;
#else
        // Paul... wtf??
        if(*left > 0) {
                uint16_t n = *left;
                uint8_t *ptr = dataptr + nbytes - n; // really?!
                uint8_t rcode = InTransfer(pep, 0, &n, ptr);
                if(rcode == 0) {
                        *left -= n;
                        *read = n;
                } else {
                        *read = 0;
                }
                return rcode;
        } else {
                *read = 0;
                return 0;
        }
#endif
}

uint8_t UHS_NI UHS_KINETIS_EHCI::ctrlReqClose(UHS_EpInfo *pep, uint8_t bmReqType, uint16_t left, uint16_t nbytes, uint8_t *dataptr) {
        uint8_t rcode;

        if(((bmReqType & 0x80) == 0x80) && pep && left && dataptr) {
                HOST_DUBUG("ctrlReqClose Sinking %i\r\n", left);
                // TODO: is this needed?
                // Paul: Yes! otherwise USB will stall and die. -- AJK
                while(left) {
                        uint16_t read = nbytes;
                        rcode = InTransfer(pep, 0, &read, dataptr);
                        if(rcode) break;
                        left -= read;
                        if(read < nbytes) break;
                }

        }

        if(((bmReqType & 0x80) == 0x80)) {
                init_qTD(NULL, 0, 0, 1, false);
                rcode = dispatchPkt(0, 0, 0);
        } else {
                init_qTD(NULL, 0, 1, 1, false);
                rcode = dispatchPkt(0, 0, 0);
        }
        return rcode;
}







#endif	/* UHS_KINETIS_EHCI_INLINE_H */

