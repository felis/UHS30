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
                case 0: printf("12 Mbps ");
                        break;
                case 1: printf("1.5 Mbps ");
                        break;
                case 2: printf("480 Mbps ");
                        break;
                default: printf("(undef) ");
        }
        if(n & USBHS_PORTSC_HSP) {
                printf("highspeed ");
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
        printf(",USBINT=%i", n & USBHS_USBSTS_UI ? 1 : 0);
        printf(",USBERRINT=%i", n & USBHS_USBSTS_UEI ? 1 : 0);
        printf(",PCHGINT=%i", n & USBHS_USBSTS_PCI ? 1 : 0);
        printf(",FRAMEINT=%i", n & USBHS_USBSTS_FRI ? 1 : 0);
        printf(",errINT=%i", n & USBHS_USBSTS_SEI ? 1 : 0);
        printf(",AAINT=%i", n & USBHS_USBSTS_AAI ? 1 : 0);
        printf(",RSTINT=%i", n & USBHS_USBSTS_URI ? 1 : 0);
        printf(",SOFINT=%i", n & USBHS_USBSTS_SRI ? 1 : 0);
        printf(",SUSP=%i", n & USBHS_USBSTS_HCH ? 1 : 0);
        printf(",RECL=%i", n & USBHS_USBSTS_RCL ? 1 : 0);
        printf(",PSRUN=%i", n & USBHS_USBSTS_PS ? 1 : 0);
        printf(",ASRUN=%i", n & USBHS_USBSTS_AS ? 1 : 0);
        printf(",NAKINT=%i", n & USBHS_USBSTS_NAKI ? 1 : 0);
        printf(",ASINT=%i", n & USBHS_USBSTS_UAI ? 1 : 0);
        printf(",PSINT=%i", n & USBHS_USBSTS_UPI ? 1 : 0);
        printf(",T0INT=%i", n & USBHS_USBSTS_TI0 ? 1 : 0);
        printf(",T1INT=%i", n & USBHS_USBSTS_TI1 ? 1 : 0);

        printf(",index=%lu\r\n", USBHS_FRINDEX); // periodic index

#endif
}


void UHS_NI UHS_KINETIS_EHCI::VBUS_changed(void) {
        /* modify USB task state because Vbus changed or unknown */
        uint8_t speed = 1;
        // printf("\r\n\r\n\r\n\r\nSTATE %2.2x -> ", usb_task_state);
        switch(vbusState) {
                case 1: // Low speed
                        speed = 0;
                        //intentional fall-through
                case 0: // Full speed
                        // Resets to the device cause an IRQ
                        ReleaseChildren();
                        timer_countdown = 0;
                        sof_countdown = 0;
                        usb_task_state = UHS_USB_HOST_STATE_DEBOUNCE;
                        break;
                case 2: // high speed
                        speed = 2;
                        ReleaseChildren();
                        timer_countdown = 0;
                        sof_countdown = 0;
                        usb_task_state = UHS_USB_HOST_STATE_DEBOUNCE;
                        break;
                default:
                        ReleaseChildren();
                        timer_countdown = 0;
                        sof_countdown = 0;
                        usb_task_state = UHS_USB_HOST_STATE_IDLE;
                        USB0_INTEN |= USB_INTEN_ATTACHEN;
                        break;
        }
        // printf("0x%2.2x\r\n\r\n\r\n\r\n", usb_task_state);
        usb_host_speed = speed;
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

        printf("ISRbottom, usb_task_state: %x \r\n", (uint8_t)usb_task_state);

        switch(usb_task_state) {
                case UHS_USB_HOST_STATE_INITIALIZE: // initial state
                        //printf("ISRbottom, UHS_USB_HOST_STATE_INITIALIZE\r\n");
                        // if an attach happens we will detect it in the isr
                        // update usb_task_state and check speed (so we replace busprobe and VBUS_changed funcs)
                        break;
                case UHS_USB_HOST_STATE_DEBOUNCE:
                        //printf("ISRbottom, UHS_USB_HOST_STATE_DEBOUNCE\r\n");
                        sof_countdown = UHS_HOST_DEBOUNCE_DELAY_MS;
                        usb_task_state = UHS_USB_HOST_STATE_DEBOUNCE_NOT_COMPLETE;
                        break;
                case UHS_USB_HOST_STATE_DEBOUNCE_NOT_COMPLETE://settle time for just attached device
                        //printf("ISRbottom, UHS_USB_HOST_STATE_DEBOUNCE_NOT_COMPLETE\r\n");
                        usb_task_state = UHS_USB_HOST_STATE_RESET_DEVICE;
                        break;

                case UHS_USB_HOST_STATE_RESET_DEVICE:
                        //printf("ISRbottom, UHS_USB_HOST_STATE_RESET_DEVICE\r\n");
                        busevent = true;
                        usb_task_state = UHS_USB_HOST_STATE_RESET_NOT_COMPLETE;
                        //issue bus reset
                        USB0_CTL |= USB_CTL_RESET;
                        timer_countdown = 20;
                        break;
                case UHS_USB_HOST_STATE_RESET_NOT_COMPLETE:
                        //printf("ISRbottom, UHS_USB_HOST_STATE_RESET_NOT_COMPLETE\r\n");
                        USB0_CTL &= ~USB_CTL_RESET; // stop bus reset
                        USB0_CTL |= USB_CTL_USBENSOFEN; // start SOF generation
                        usb_task_state = UHS_USB_HOST_STATE_WAIT_BUS_READY;
                        // We delay two extra ms to ensure that at least one SOF has been sent.
                        // This trick is performed by just moving to the next state.
                        break;
                case UHS_USB_HOST_STATE_WAIT_BUS_READY:
                        //printf("ISRbottom, UHS_USB_HOST_STATE_WAIT_BUS_READY\r\n");
                        usb_task_state = UHS_USB_HOST_STATE_CONFIGURING;
                        break; // don't fall through

                case UHS_USB_HOST_STATE_CONFIGURING:
                        HOST_DUBUG("ISRbottom, UHS_USB_HOST_STATE_CONFIGURING\r\n");
                        usb_task_state = UHS_USB_HOST_STATE_CHECK;
                        x = Configuring(0, 1, usb_host_speed);
                        if(usb_task_state == UHS_USB_HOST_STATE_CHECK) {
                                if(x) {
                                        if(x == hrJERR) {
                                                usb_task_state = UHS_USB_HOST_STATE_IDLE;
                                        } else if(x != UHS_HOST_ERROR_DEVICE_INIT_INCOMPLETE) {
                                                usb_error = x;
                                                usb_task_state = UHS_USB_HOST_STATE_ERROR;
                                        }
                                } else
                                        usb_task_state = UHS_USB_HOST_STATE_CONFIGURING_DONE;
                        }
                        break;
                case UHS_USB_HOST_STATE_CONFIGURING_DONE:
                        HOST_DUBUG("ISRbottom, UHS_USB_HOST_STATE_CONFIGURING_DONE\r\n");
                        usb_task_state = UHS_USB_HOST_STATE_RUNNING;
                        break;
                case UHS_USB_HOST_STATE_CHECK:
                        // Serial.println((uint32_t)__builtin_return_address(0),HEX);
                        break;
                case UHS_USB_HOST_STATE_RUNNING:
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
 *      2: If DMA bandwidth is not enough, hrNAK is returned.
 *              Drivers that have NAK processing know to retry.
 */


void UHS_NI UHS_KINETIS_EHCI::Task(void) {
}

void UHS_NI UHS_KINETIS_EHCI::ISRTask(void) {
        counted = false;
        // ACK all IRQ early
        uint32_t Ostat = USBHS_OTGSC; // OTG status
        USBHS_OTGSC &= Ostat;
        uint32_t Ustat = USBHS_USBSTS; // USB status
        USBHS_USBSTS &= Ustat;
        uint32_t Pstat = USBHS_PORTSC1; // Port status
        //USBHS_PORTSC1 &= Pstat;

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


        if(Ustat & USBHS_USBSTS_PCI) {
                // port change
#if LED_STATUS
                // toggle LEDs so a mere human can see them
                digitalWriteFast(31, CL1);
                digitalWriteFast(32, CL2);
                CL1 = !CL1;
                CL2 = !CL2;
#endif
                USBHS_USBSTS = USBHS_USBSTS_PCI;
                // record vbus state here
                // 0x0 = full, 0x1 = low, 0x2 = high, 0x3 = disconnected
                vbusState = ((Pstat & 0x04000000U) ? 1 : 0) | ((Pstat & 0x08000000U) ? 2 : 0);
#if defined(EHCI_TEST_DEV)
                printf("Vbus state changed to %1.1x\r\n", vbusState);
#endif
                condet = true;
        }

        if(Ustat & USBHS_USBSTS_SEI) {
        }
        if(Ustat & USBHS_USBSTS_FRI) {
                // nothing yet...
        }

        if((Ustat & USBHS_USBSTS_UEI) || (Ustat & USBHS_USBSTS_UI)) {
                // USB interrupt or USB error interrupt both end a transaction
                if(Ustat & USBHS_USBSTS_UEI) {
                // nothing yet :-)
                }
                if(Ustat & USBHS_USBSTS_UI) {
                // nothing yet :-)
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
#if defined(EHCI_TEST_DEV)
        printf("*Q = %p\r\n", &Q);
        printf("*Q.qh[0] = %p\r\n", &(Q.qh[0]));
        printf("*Q.qh[1] = %p\r\n\n", &(Q.qh[1]));
        printf("*Q.qtd[0] = %p\r\n", &(Q.qtd[0]));
        printf("*Q.qtd[1] = %p\r\n\n", &(Q.qtd[1]));
        printf("*Q.itd[0] = %p\r\n", &(Q.itd[0]));
        printf("*Q.itd[1] = %p\r\n\n", &(Q.itd[1]));
        printf("*Q.sitd[0] = %p\r\n", &(Q.sitd[0]));
        printf("*Q.sitd[1] = %p\r\n\n", &(Q.sitd[1]));
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
                Q.qh[i].horizontalLinkPointer = 2LU | (uint32_t)&(Q.qh[i + 1]);
        }
        Q.qh[UHS_KEHCI_MAX_QH - 1].horizontalLinkPointer = (uint32_t)3;

        // Init queue transfer descriptors
#if defined(EHCI_TEST_DEV)
        printf("\r\nInit QTD %u queue transfer descriptors...", UHS_KEHCI_MAX_QTD);
#endif
        for(unsigned int i = 0; i < UHS_KEHCI_MAX_QTD; i++) {
                Q.qtd[i].nextQtdPointer = (uint32_t)&(Q.qtd[i + 1]);
        }
        Q.qtd[UHS_KEHCI_MAX_QTD - 1].nextQtdPointer = (uint32_t)NULL;

        // Init isochronous transfer descriptors
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
#if defined(EHCI_TEST_DEV)
        printf("\r\n");
#endif
        uint32_t *framePointer = (uint32_t *)(&frame[0]);
        for(int i = 0; i < UHS_KEHCI_MAX_FRAMES; i++) {
                framePointer[i] = 1;
        }

#ifdef HAS_KINETIS_MPU
        MPU_RGDAAC0 |= 0x30000000;
#endif
        PORTE_PCR6 = PORT_PCR_MUX(1);
        GPIOE_PDDR |= (1 << 6);
        GPIOE_PSOR = (1 << 6); // turn on USB host power

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

        /*
                poop();
                do {
                        s = (USBHS_USBSTS & USBHS_USBSTS_AS) | (USBHS_USBCMD & USBHS_USBCMD_ASE);
                } while((s == USBHS_USBSTS_AS) || (s == USBHS_USBCMD_ASE));
                USBHS_PERIODICLISTBASE = (uint32_t)frame;
                USBHS_USBCMD |= USBHS_USBCMD_PSE;
         */

        /*
        do {
                s = (USBHS_USBSTS & USBHS_USBSTS_AS) | (USBHS_USBCMD & USBHS_USBCMD_ASE);
        } while((s == USBHS_USBSTS_AS) || (s == USBHS_USBCMD_ASE));
        USBHS_ASYNCLISTADDR = (uint32_t)Q.qh;
        USBHS_USBCMD |= USBHS_USBCMD_ASE;
        poop();
         */
        USBHS_PORTSC1 |= USBHS_PORTSC_PP;
#if defined(EHCI_TEST_DEV)
        poopOutStatus();
#endif

        USBHS_OTGSC =
                //        USBHS_OTGSC_BSEIE | // B session end
                //        USBHS_OTGSC_BSVIE | // B session valid
                //        USBHS_OTGSC_IDIE  | // USB ID
                        USBHS_OTGSC_MSS   | // 1mS timer
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
                //        USBHS_USBINTR_SRE  | // SOF
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

#endif	/* UHS_KINETIS_EHCI_INLINE_H */

