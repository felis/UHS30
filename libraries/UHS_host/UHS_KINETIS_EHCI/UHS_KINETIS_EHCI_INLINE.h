/*
 * File:   UHS_KINETIS_EHCI_INLINE.h
 * Author: root
 *
 * Created on July 31, 2016, 1:01 AM
 *
 * This is a simplification & small subset of EHCI adapted for UHS.  If you read the
 * EHCI spec, normally a driver would dynamically allocate one QH structure for each
 * endpoint in every device.  The many QH structures would be placed into a circular
 * linked list for the asynchronous schedule or an inverted binary tree for the
 * periodic schedule.  For actual data transfer, qTD structures would be dynamically
 * allocated and hot-inserted to the linked list from the QH representing the
 * desired endpoint.  Additional list structures outside the scope of EHCI would
 * normally be used to manage qTD structures after EHCI completes their work
 * requirements.  As USB devices & hubs connect and disconnect, special rules need
 * to be followed to dynamically change the QH lists while EHCI is actively using
 * them.  The normal EHCI usage model is very complex!
 *
 * UHS uses a much simpler model, where SetAddress() is first called to configure
 * the hardware to communicate with one endpoint on a particular device.  Then the
 * functions below are called to perform the USB communication.  To meet this usage
 * model, a single QH structure is connected to EHCI's asynchronous schedule.  The
 * periodic schedule is not used or required.  When SetAddress() requests a different
 * device or endpoint, this one QH is reconfigured.
 *
 * Data transfer is accomplished using two qTD structures.  qHalt is reserved for
 * placing the QH into its halted state.  When no data transfer is needed, the
 * QH.nextQtdPointer field must always have the address of qHalt.  This is how
 * the QH "knows" no more data transfers are to be attempted.  When data transfer
 * is needed, the main qTD structure is initialized with the details and a pointer
 * to the actual buffer which sources or sinks USB data.  This main qTD structure
 * must have its nextQtdPointer field set to the address of qHalt *before* it is
 * used.  Then to cause EHCI to perform the data transfer, the QH.nextQtdPointer
 * is written to the address of the main qTD.  When the transfer completes, EHCI
 * automatically updates the QH.nextQtdPointer field to the qTD.nextQtdPointer.
 * The QH returns to its halted state because the main qTD points to the qHalt structure.
 */


#if defined(UHS_KINETIS_EHCI_H) && !defined(UHS_KINETIS_EHCI_LOADED)

#define UHS_KINETIS_EHCI_LOADED

#if !defined(DEBUG_PRINTF_EXTRA_HUGE_USB_EHCI)
#define DEBUG_PRINTF_EXTRA_HUGE_USB_EHCI 0
#endif

#if DEBUG_PRINTF_EXTRA_HUGE
#if DEBUG_PRINTF_EXTRA_HUGE_USB_EHCI
#define UHS_EHCI_DEBUG(...) printf(__VA_ARGS__)
#else
#define UHS_EHCI_DEBUG(...) VOID0
#endif
#else
#define UHS_EHCI_DEBUG(...) VOID0
#endif

static UHS_KINETIS_EHCI *_UHS_KINETIS_EHCI_THIS_;

static void UHS_NI call_ISR_kinetis_EHCI(void) {
        _UHS_KINETIS_EHCI_THIS_->ISRTask();
}

void UHS_NI UHS_KINETIS_EHCI::busprobe(void) {
        uint8_t speed = 15;


        switch(vbusState) {
                case 0: // Full speed
                        speed = 1;
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
        HOST_DEBUG("USB host speed now %1.1x\r\n", speed);
        usb_host_speed = speed;
        if(speed == 2) {
                UHS_KIO_SETBIT_ATOMIC(USBPHY_CTRL, USBPHY_CTRL_ENHOSTDISCONDETECT);
        } else {
                UHS_KIO_CLRBIT_ATOMIC(USBPHY_CTRL, USBPHY_CTRL_ENHOSTDISCONDETECT);
        }
}

void UHS_NI UHS_KINETIS_EHCI::VBUS_changed(void) {
        /* modify USB task state because Vbus changed or unknown */
        HOST_DEBUG("\r\n\r\n\r\n\r\nSTATE %2.2x -> ", usb_task_state);
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
        HOST_DEBUG("0x%2.2x\r\n\r\n\r\n\r\n", usb_task_state);
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
        HOST_DEBUG("ISRbottom, usb_task_state: 0x%0X \r\n", (uint8_t)usb_task_state);

        switch(usb_task_state) {
                case UHS_USB_HOST_STATE_INITIALIZE: /* 0x10 */ // initial state
                        HOST_DEBUG("ISRbottom, UHS_USB_HOST_STATE_INITIALIZE\r\n");
                        // if an attach happens we will detect it in the isr
                        // update usb_task_state and check speed (so we replace busprobe and VBUS_changed methods)
                        break;
                case UHS_USB_HOST_STATE_DEBOUNCE: /* 0x01 */
                        HOST_DEBUG("ISRbottom, UHS_USB_HOST_STATE_DEBOUNCE\r\n");
                        sof_countdown = UHS_HOST_DEBOUNCE_DELAY_MS;
                        usb_task_state = UHS_USB_HOST_STATE_DEBOUNCE_NOT_COMPLETE;
                        break;
                case UHS_USB_HOST_STATE_DEBOUNCE_NOT_COMPLETE: /* 0x02 */
                        //settle time for just attached device
                        HOST_DEBUG("ISRbottom, UHS_USB_HOST_STATE_DEBOUNCE_NOT_COMPLETE\r\n");
                        usb_task_state = UHS_USB_HOST_STATE_RESET_DEVICE;
                        break;

                case UHS_USB_HOST_STATE_RESET_DEVICE: /* 0x0A */
                        HOST_DEBUG("ISRbottom, UHS_USB_HOST_STATE_RESET_DEVICE\r\n");
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
                        HOST_DEBUG("ISRbottom, UHS_USB_HOST_STATE_RESET_NOT_COMPLETE\r\n");
                        if(!busevent) usb_task_state = UHS_USB_HOST_STATE_WAIT_BUS_READY;
                        // We delay two extra ms to ensure that at least one SOF has been sent.
                        // This trick is performed by just moving to the next state.
                        break;
                case UHS_USB_HOST_STATE_WAIT_BUS_READY: /* 0x05 */
                        noInterrupts();
                        doingreset = false;
                        DDSB();
                        interrupts();
                        HOST_DEBUG("ISRbottom, UHS_USB_HOST_STATE_WAIT_BUS_READY\r\n");
                        usb_task_state = UHS_USB_HOST_STATE_CONFIGURING;
                        break; // don't fall through

                case UHS_USB_HOST_STATE_CONFIGURING: /* 0x0C */
                        HOST_DEBUG("ISRbottom, UHS_USB_HOST_STATE_CONFIGURING\r\n");
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
                        HOST_DEBUG("ISRbottom, UHS_USB_HOST_STATE_CONFIGURING_DONE\r\n");
                        usb_task_state = UHS_USB_HOST_STATE_RUNNING;
                        break;
                case UHS_USB_HOST_STATE_CHECK: /* 0x0E */
                        break;

                case UHS_USB_HOST_STATE_ERROR: /* 0xF0 */
                        HOST_DEBUG("ISRbottom, UHS_USB_HOST_STATE_ERROR\r\n");
                        //while(1);
                        break;

                case UHS_USB_HOST_STATE_RUNNING: /* 0x60 */
                        HOST_DEBUG("ISRbottom, UHS_USB_HOST_STATE_RUNNING\r\n");
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
                HOST_DEBUG("PCI Vbus state changed to %1.1x\r\n", vbusState);
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
#if defined(EHCI_TEST_DEV)
                        HOST_DEBUG("USBHS_USBSTS_UI\r\n");
#endif
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
                if(nak_countdown) {
                        nak_countdown--;
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
        DDSB();
        if(!timer_countdown && !nak_countdown &&!sof_countdown && !counted && !usb_task_polling_disabled) {
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
        qHalt.transferResults.token = 0x40;
        int count = 0;
#if defined(__MK66FX1M0__)
        MPU_RGDAAC0 |= 0x30000000;
        PORTE_PCR6 = PORT_PCR_MUX(1);
        GPIOE_PDDR |= (1 << 6);

        DDSB();
        vbusPower(1, vbus_off);
        DDSB();

        MCG_C1 |= MCG_C1_IRCLKEN; // enable MCGIRCLK 32kHz
        OSC0_CR |= OSC_ERCLKEN;
        SIM_SOPT2 |= SIM_SOPT2_USBREGEN; // turn on USB regulator
        SIM_SOPT2 &= ~SIM_SOPT2_USBSLSRC; // use IRC for slow clock
        HOST_DEBUG("power up EHCI PHY\r\n");
        SIM_USBPHYCTL |= SIM_USBPHYCTL_USBDISILIM; // disable USB current limit
        SIM_SCGC3 |= SIM_SCGC3_USBHSDCD | SIM_SCGC3_USBHSPHY | SIM_SCGC3_USBHS;
        USBHSDCD_CLOCK = 33 << 2;
        HOST_DEBUG("init EHCI PHY & PLL\r\n");

        // init process: page 1681-1682
        USBPHY_CTRL_CLR = (USBPHY_CTRL_SFTRST | USBPHY_CTRL_CLKGATE); // // CTRL pg 1698
        USBPHY_TRIM_OVERRIDE_EN_SET = 1;
        USBPHY_PLL_SIC = USBPHY_PLL_SIC_PLL_POWER | USBPHY_PLL_SIC_PLL_ENABLE |
                USBPHY_PLL_SIC_PLL_DIV_SEL(1) | USBPHY_PLL_SIC_PLL_EN_USB_CLKS;
        // wait for the PLL to lock
        while((USBPHY_PLL_SIC & USBPHY_PLL_SIC_PLL_LOCK) == 0) {
                count++;
        }
        HOST_DEBUG("PLL locked, waited %i\r\n", count);

#elif defined(__IMXRT1052__) || defined(__IMXRT1062__)
#ifdef ARDUINO_TEENSY41
        IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_40 = 5;
        IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_40 = 0x0008; // slow speed, weak 150 ohm drive
        GPIO8_GDIR |= 1 << 26;
        GPIO8_DR_CLEAR = 1 << 26;
#endif
        while(1) {
                uint32_t n = CCM_ANALOG_PLL_USB2;
                if(n & CCM_ANALOG_PLL_USB2_DIV_SELECT) {
                        CCM_ANALOG_PLL_USB2_CLR = 0xC000; // get out of 528 MHz mode
                        CCM_ANALOG_PLL_USB2_SET = CCM_ANALOG_PLL_USB2_BYPASS;
                        CCM_ANALOG_PLL_USB2_CLR = CCM_ANALOG_PLL_USB2_POWER |
                                CCM_ANALOG_PLL_USB2_DIV_SELECT |
                                CCM_ANALOG_PLL_USB2_ENABLE |
                                CCM_ANALOG_PLL_USB2_EN_USB_CLKS;
                        continue;
                }
                if(!(n & CCM_ANALOG_PLL_USB2_ENABLE)) {
                        CCM_ANALOG_PLL_USB2_SET = CCM_ANALOG_PLL_USB2_ENABLE; // enable
                        continue;
                }
                if(!(n & CCM_ANALOG_PLL_USB2_POWER)) {
                        CCM_ANALOG_PLL_USB2_SET = CCM_ANALOG_PLL_USB2_POWER; // power up
                        continue;
                }
                if(!(n & CCM_ANALOG_PLL_USB2_LOCK)) {
                        continue; // wait for lock
                }
                if(n & CCM_ANALOG_PLL_USB2_BYPASS) {
                        CCM_ANALOG_PLL_USB2_CLR = CCM_ANALOG_PLL_USB2_BYPASS; // turn off bypass
                        continue;
                }
                if(!(n & CCM_ANALOG_PLL_USB2_EN_USB_CLKS)) {
                        CCM_ANALOG_PLL_USB2_SET = CCM_ANALOG_PLL_USB2_EN_USB_CLKS; // enable
                        continue;
                }
                break; // USB2 PLL up and running
        }
        // turn on USB clocks (should already be on)
        CCM_CCGR6 |= CCM_CCGR6_USBOH3(CCM_CCGR_ON);
        // turn on USB2 PHY
        USBPHY_CTRL_CLR = USBPHY_CTRL_SFTRST | USBPHY_CTRL_CLKGATE;
#endif

        /*
         * To enable full USB support on the PHY, bits ENUTMILEVEL3 and ENUTMILEVEL2, in the
         * USBPHY_CTRL register, must be set. UTMI+ Level 2 adds support for directly connected low-speed
         * devices. This is needed when the controller operates in host mode for operation with low-speed devices
         * such as USB mice. UTMI+ Level 3 adds support for directly connected full-speed hubs that need to support
         * low-speed devices
         */

        USBPHY_CTRL_SET = USBPHY_CTRL_ENUTMILEVEL2 | USBPHY_CTRL_ENUTMILEVEL3;
        // turn on power to PHY
        USBPHY_PWD = 0;
        delay(10);
        HOST_DEBUG("begin ehci reset\r\n");

        USBHS_USBCMD |= USBHS_USBCMD_RST;
        count = 0;
        while(USBHS_USBCMD & USBHS_USBCMD_RST) {
                count++;
        }
        HOST_DEBUG("reset waited %i\r\n", count);

        // turn on the USBHS controller
        USBHS_USBMODE = USBHS_USBMODE_CM(3); // host mode
        USBHS_FRINDEX = 0;
        USBHS_USBINTR = 0;
        USBHS_USBCMD = USBHS_USBCMD_ITC(0) | USBHS_USBCMD_RS | USBHS_USBCMD_ASP(3) |
                USBHS_USBCMD_FS2 | USBHS_USBCMD_FS(0); // periodic table is 64 pointers

        // async list
        USBHS_ASYNCLISTADDR = 0;

        USBHS_PORTSC1 |= USBHS_PORTSC_PP;

        USBHS_OTGSC = 0 |
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

        // Delay a minimum of 1 second to ensure any capacitors are drained.
        // 1 second is required to make sure we do not smoke a Microdrive!
        if(mseconds != INT16_MIN) {
                if(mseconds < 1000) mseconds = 1000;
                delay(mseconds); // We can't depend on SOF timer here.
        }
        vbusPower(1, vbus_on);

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

void UHS_KINETIS_EHCI::init_qTD(uint32_t len, uint32_t data01) {
        qTD.nextQtdPointer = (uint32_t) & qHalt;
        qTD.alternateNextQtdPointer = (uint32_t) & qHalt;
        //if(data01) data01 = 0x80000000;
        //qTD.transferResults.token = data01 | (len << 16) | (irq ? 0x8000 : 0) | (pid << 8) | 0x80;
        uint32_t addr = (uint32_t) & data_buf;
        qTD.bufferPointers[0] = addr;
        addr &= 0xFFFFF000;
        qTD.bufferPointers[1] = addr + 0x1000;
        qTD.bufferPointers[2] = addr + 0x2000;
        qTD.bufferPointers[3] = addr + 0x3000;
        qTD.bufferPointers[4] = addr + 0x4000;

        qTD.transferResults.token = 0x80; // status is set, the remaining is cleared to zero
        qTD.transferResults.toggle = data01;
        qTD.transferResults.length = len;
        //qTD.transferResults.ioc = (irq ? 1 : 0);
}

uint8_t UHS_NI UHS_KINETIS_EHCI::SetAddress(uint8_t addr, uint8_t ep, UHS_EpInfo **ppep, uint16_t & nak_limit) {
        HOST_DEBUG("SetAddress, addr=%d, ep=%x\r\n", addr, ep);

        UHS_Device *p = addrPool.GetUsbDevicePtr(addr);
        if(!p) return UHS_HOST_ERROR_NO_ADDRESS_IN_POOL;

        if(!p->epinfo) return UHS_HOST_ERROR_NULL_EPINFO;
        *ppep = getEpInfoEntry(addr, ep);
        if(!*ppep) return UHS_HOST_ERROR_NO_ENDPOINT_IN_TABLE;
        nak_limit = (*ppep)->bmNakPower;
        if(nak_limit == 1) {
                nak_limit = 2;
        }

        USBHS_USBCMD &= ~USBHS_USBCMD_ASE;
        while((USBHS_USBSTS & USBHS_USBSTS_AS)); // wait for async schedule disable

        //uint32_t type = (*ppep)->type;
        uint32_t speed;

        if(p->speed == 0) {
                speed = 1; // 1.5 Mbit/sec
        } else if(p->speed == 1) {
                speed = 0; // 12 Mbit/sec
        } else speed = p->speed; // 480 Mbit/sec

        uint32_t c = 0;
        uint32_t maxlen = (*ppep)->maxPktSize;
        uint32_t hub_addr = p->parent.bmAddress;
        uint32_t hub_port = p->port;

        if(speed < 2) {
                c = 1; // not high speed
        }
        qHalt.nextQtdPointer = 1;
        qHalt.alternateNextQtdPointer = 1;
        qHalt.transferResults.token = 0x40;
        memset(&QH, 0, sizeof (QH));
        QH.horizontalLinkPointer = (uint32_t) & QH | 2;
        QH.nextQtdPointer = (uint32_t) & qHalt;
        QH.alternateNextQtdPointer = (uint32_t) & qHalt;
        if((*ppep)->bmNeedPing) {
                QH.transferOverlayResults[0] = 1;
                (*ppep)->bmNeedPing = 0;
        }
        // NAK limit,
        // LS/FS control ep flag,
        // max packet length,
        // head of reclamation list,
        // Data Toggle control always set as 1, as we are in control,
        // speed,
        // endpoint,
        // Inactivate on Next Transaction (Periodic only, unused, set to zero),
        // device address
        QH.staticEndpointStates[0] = QH_capabilities1(15, c, maxlen, 1, 1, speed, ep, 0, addr);
        //printf("SETUP C bit %2.2x EP %2.2x SPEED %2.2x\r\n", (size_t)(((QH.staticEndpointStates[0])& 0x8000000u) >> 27), (size_t)ep, (size_t)speed);
        //fflush(stdout);
        // high_bw_mult
        // hub_port_number ( >1 <8 ), or 1 if root
        // hub_address, zero is root
        // split_completion_mask (always 0)
        // interrupt_schedule_mask (always 0)
        QH.staticEndpointStates[1] = QH_capabilities2(1, hub_port, hub_addr, 0, 0);
        USBHS_ASYNCLISTADDR = (uint32_t) & QH;
        USBHS_USBCMD |= USBHS_USBCMD_ASE;

        return UHS_HOST_ERROR_NONE;
}

uint8_t UHS_NI UHS_KINETIS_EHCI::dispatchPkt(uint8_t token, UHS_EpInfo *pep, uint16_t nak_limit) {
        uint16_t nc = 0;
        //printf("DISPATCH! %i\r\n", nak_limit);
        noInterrupts();
        if(nak_limit == 1) nak_limit = 2; // allow for transmission if we are between...
        nak_countdown = nak_limit;
        // set token and final endpoint here
        qTD.transferResults.PID = token;
        QH.nextQtdPointer = (uint32_t) & qTD;
        QH.transferOverlayResults[0] &= 1;
        newError = false;
        isrHappened = false;
        DDSB();
        interrupts();
        while(!condet) {
                uint32_t status = qTD.transferResults.token;
                // Wait for a state change.
                //if(newError) {
                // we need to get the actual translated error
                //noInterrupts();
                //nak_countdown = 0;
                //interrupts();
                //UHS_EHCI_DEBUG("XXXXXXXX newError trap - status said %2.2x\r\n", (unsigned int)(status & 0xffu));
                //}

                if(!(status & 0x80) || newError) { // halted
                        if((status & 0x01u) && (QH.staticEndpointStates[0] & (1 << 13))) {
                                // 480 Mbit OUT endpoint responded with NYET token.
                                // NYET means the OUT transfer was successful, but
                                // next time we need to begin the PING protocol.
                                // USB 2.0 spec: section 8.5.1, pages 217-220
                                pep->bmNeedPing = 1;
                                status &= 0xfeu;
                        }
                        if(!(status & 0xffu)) {
                                // no longer active, not halted, no errors... so ok
                                noInterrupts();
                                nak_countdown = 0;
                                interrupts();
                                return UHS_HOST_ERROR_NONE;
                        }
                        HOST_DEBUGx("dispatchPkt status code %2.2x\r\n", (uint8_t)(status & 0xffu));

                        if(status & 0x10u) {
                                noInterrupts();
                                nak_countdown = 0;
                                interrupts();
                                return UHS_HOST_ERROR_BABBLE; // Babble Detected
                        }
                        // Important??
                        // Never seen these, as we only send one packet at a time.
                        // if(status & 0x02u) return; // split state...
                        // Missed Micro-Frame. The host controller detected that a host-induced
                        // hold-off caused the host controller to miss a required complete-split transaction.
                        // if(status & 0x04u) return; // Missed Micro-Frame...

                        //if(status & 0x02u) { // QTD_STS_STS
                        //        UHS_EHCI_DEBUG("XXXXXXXX split state?\r\n");
                        //}

                        if((status & 0x04u) && (((status) >> 8) & 0x03u)) { // QTD_STS_MMF and PID_CODE_IN
                                /* EHCI Specification, Table 4-13.
                                 * When MMF is active and PID Code is IN, queue is halted.
                                 * ...but what is the error code :-)
                                 */
                                noInterrupts();
                                nak_countdown = 0;
                                interrupts();
                                UHS_EHCI_DEBUG("XXXXXXXX Missed Micro-Frame?\r\n");
                                return UHS_HOST_ERROR_PROTOCOL;
                        }
                        // CERR nonzero + halt --> stall
                        if((((status) >> 8) & 0x03u)) {
                                noInterrupts();
                                nak_countdown = 0;
                                interrupts();
                                HOST_DEBUGx("dispatchPkt STALL, CERR nonzero\r\n");
                                return UHS_HOST_ERROR_STALL;
                        }
                        if(status & 0x20u) { // QTD_STS_DBE
                                noInterrupts();
                                nak_countdown = 0;
                                interrupts();
                                HOST_DEBUGx("dispatchPkt DMA Error\r\n");
                                return UHS_HOST_ERROR_DMA; // Data Buffer Error (could not read or write)
                        }
                        if(status & 0x48u) { // Halted or Transaction Error
                                // Needs to return one of:
                                // STALL
                                // if error counter reached 0:
                                // UHS_HOST_ERROR_TIMEOUT
                                // UHS_HOST_ERROR_CRC
                                // UHS_HOST_ERROR_TOGERR
                                // UHS_HOST_ERROR_WRONGPID
                                // Are these available??
                                //
                                // bit 6 0x40 Serious error halted
                                // bit 4 0x08 Transaction Error
                                //
                                //
                                noInterrupts();
                                nak_countdown = 0;
                                interrupts();
                                //if(status & 0x40u) { // Seems to be correct for a stall.
                                //        HOST_DEBUGx("dispatchPkt STALL\r\n");
                                //        return UHS_HOST_ERROR_STALL;
                                //}
                                // so what exactly is the error anyway??
                                // Transaction Error doesn't classify the fucking thing!
                                HOST_DEBUGx("dispatchPkt - UHS_HOST_ERROR_PROTOCOL\r\n");
                                return UHS_HOST_ERROR_PROTOCOL;
                        }
                        noInterrupts();
                        nak_countdown = 0;
                        interrupts();
                        USBHS_USBCMD &= ~USBHS_USBCMD_ASE;
                        while((USBHS_USBSTS & USBHS_USBSTS_AS)); // wait for async schedule disable
                        UHS_EHCI_DEBUG("XXXXXXXX Default trap - UHS_HOST_ERROR_PROTOCOL\r\n");
                        return UHS_HOST_ERROR_PROTOCOL;
                }
                noInterrupts();
                nc = nak_countdown;
                interrupts();
                if(nc == 0 && nak_limit) {
                        // cancel transfer.
                        USBHS_USBCMD &= ~USBHS_USBCMD_ASE;
                        while((USBHS_USBSTS & USBHS_USBSTS_AS)); // wait for async schedule disable

                        UHS_EHCI_DEBUG("XXXXXXXX NAK out NAKing\r\n");
                        return UHS_HOST_ERROR_NAK;
                }
        }
        if(condet) {
                noInterrupts();
                nak_countdown = 0;
                interrupts();
                USBHS_USBCMD &= ~USBHS_USBCMD_ASE;
                while((USBHS_USBSTS & USBHS_USBSTS_AS)); // wait for async schedule disable
                return UHS_HOST_ERROR_UNPLUGGED;
        }
        noInterrupts();
        nak_countdown = 0;
        interrupts();
        USBHS_USBCMD &= ~USBHS_USBCMD_ASE;
        while((USBHS_USBSTS & USBHS_USBSTS_AS)); // wait for async schedule disable
        return UHS_HOST_ERROR_TIMEOUT;
}

uint8_t UHS_NI UHS_KINETIS_EHCI::OutTransfer(UHS_EpInfo *pep, uint16_t nak_limit, uint16_t nbytes, uint8_t *data) {
        HOST_DEBUG("OutTransfer %d, NAKS:%d toggle %d\r\n", nbytes, nak_limit, pep->bmSndToggle);
        uint8_t* p_buffer = data; // local copy
        uint16_t bytes;
        uint16_t maxpktsize = pep->maxPktSize;
        uint8_t rcode = UHS_HOST_ERROR_NONE;
        nak_limit = (0x0001UL << ((nak_limit > UHS_USB_NAK_MAX_POWER) ? UHS_USB_NAK_MAX_POWER : nak_limit));
        nak_limit--;
        while(nbytes && !rcode) {
                bytes = nbytes;
                if(bytes > maxpktsize) bytes = maxpktsize;
                memcpy(data_buf, p_buffer, bytes); // copy packet into buffer
                init_qTD(bytes, pep->bmSndToggle);
                rcode = dispatchPkt(UHS_KINETIS_EHCI_TOKEN_DATA_OUT, pep, nak_limit);
                uint32_t status = qTD.transferResults.token;
                pep->bmSndToggle = status >> 31;
                nbytes -= bytes;
                p_buffer += bytes;
        }
        HOST_DEBUG("OutTransfer done.\r\n");

        return rcode;
}

uint8_t UHS_NI UHS_KINETIS_EHCI::InTransfer(UHS_EpInfo *pep, uint16_t nak_limit, uint16_t *nbytesptr, uint8_t *data) {
        HOST_DEBUG("InTransfer %d NAKS: %d\r\n", *nbytesptr, nak_limit);

        nak_limit = (0x0001UL << ((nak_limit > UHS_USB_NAK_MAX_POWER) ? UHS_USB_NAK_MAX_POWER : nak_limit));
        nak_limit--;
        if(nak_limit == 1)nak_limit = 2;
        uint8_t rcode = 0;
        uint16_t pktsize;
        uint16_t maxpktsize = pep->maxPktSize;
        uint32_t datalen;
        uint16_t nbytes = *nbytesptr;
        uint8_t* p_buffer = data;
        *nbytesptr = 0;
        while(nbytes && !rcode) {
                datalen = (nbytes > maxpktsize) ? maxpktsize : nbytes;
                init_qTD(datalen, pep->bmRcvToggle);
                rcode = dispatchPkt(UHS_KINETIS_EHCI_TOKEN_DATA_IN, pep, nak_limit);
                uint32_t status = qTD.transferResults.token;
                // This field is decremented by the number of bytes actually moved
                // during the transaction, only on the successful completion of the
                // transaction.
                pktsize = datalen - ((status >> 16) & 0x7FFF);
                memcpy(p_buffer, data_buf, pktsize); // copy packet into buffer
                pep->bmRcvToggle = status >> 31;

                p_buffer += pktsize;
                nbytes -= pktsize;
                *nbytesptr += pktsize;
                HOST_DEBUG("InTransfer Got %d Bytes\r\n", *nbytesptr);
                if(pktsize < datalen) break; // short packet.
        }
        HOST_DEBUG("InTransfer done.\r\n");
        return rcode;
}

UHS_EpInfo * UHS_NI UHS_KINETIS_EHCI::ctrlReqOpen(uint8_t addr, uint64_t Request, uint8_t *dataptr) {
        UHS_EHCI_DEBUG("ctrlReqOpen Request and 0x80 %2.2x\r\n", uint8_t(((Request) & 0x80U)&0xffU));

        UHS_EpInfo *pep = NULL;
        uint16_t nak_limit;
        uint32_t datalen = 8;
        uint8_t rcode = SetAddress(addr, 0, &pep, nak_limit);
        nak_limit = (0x0001UL << ((nak_limit > UHS_USB_NAK_MAX_POWER) ? UHS_USB_NAK_MAX_POWER : nak_limit));
        nak_limit--;
        UHS_EHCI_DEBUG("ctrlReqOpen nak_limit is %i, rcode from SetAddress %i\r\n", nak_limit, rcode);
        if(!rcode) {
                //static uint8_t setupbuf[8];
                //memcpy(setupbuf, &Request, 8);
                memcpy(data_buf, &Request, datalen); // copy packet into buffer
                init_qTD(datalen, 0); // setup always uses DATA0
                rcode = dispatchPkt(UHS_KINETIS_EHCI_TOKEN_SETUP, pep, nak_limit); // setup always uses DATA0
                if(!rcode) {
                        if(dataptr != NULL) {
                                if(((Request) /* bmReqType */ & 0x80) == 0x80) {
                                        pep->bmRcvToggle = 1; //bmRCVTOG1;
                                } else {
                                        pep->bmSndToggle = 1; //bmSNDTOG1;
                                }
                        }
                } else {
                        HOST_DEBUGx("ctrlReqOpen failed dispatchPkt with %2.2x\r\n", rcode);
                        pep = NULL;
                }
        }
        return pep;
}

uint8_t UHS_NI UHS_KINETIS_EHCI::ctrlReqRead(UHS_EpInfo *pep, uint16_t *left, uint16_t *read, uint16_t nbytes, uint8_t * dataptr) {
        HOST_DEBUG("*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*& ctrlReqRead left: %i, nbytes: %i, dataptr: %lx\r\n", *left, nbytes, (uint32_t)dataptr);
        uint8_t rcode = 0;

        if(*left) {
                *read = nbytes;
                rcode = InTransfer(pep, 0, read, dataptr);
                if(rcode) {
                        HOST_DEBUG("ctrlReqRead ERROR: %2.2x\r\n", rcode);
                } else {
                        *left -= *read;
                }
        }
        HOST_DEBUG("ctrlReqRead left: %i, read %i\r\n", *left, *read);
        return rcode;
}

uint8_t UHS_NI UHS_KINETIS_EHCI::ctrlReqClose(UHS_EpInfo *pep, uint8_t bmReqType, uint16_t left, uint16_t nbytes, uint8_t *dataptr) {
        uint8_t rcode = 0;
        //printf("Close\r\n");
        if(((bmReqType & 0x80) == 0x80) && pep && left && dataptr) {
                HOST_DEBUG("ctrlReqClose Sinking %i\r\n", left);
                while(left) {
                        uint16_t read = nbytes;
                        rcode = InTransfer(pep, 0, &read, dataptr);
                        if(rcode) break;
                        left -= read;
                        if(read < nbytes) break;
                }

        }
        if(!rcode) {
                if(((bmReqType & 0x80) == 0x80)) {
                        init_qTD(0, 1); // make sure we use DATA1 for status phase
                        rcode = dispatchPkt(UHS_KINETIS_EHCI_TOKEN_DATA_OUT, pep, 2000);
                } else {
                        init_qTD(0, 1); // make sure we use DATA1 for status phase
                        rcode = dispatchPkt(UHS_KINETIS_EHCI_TOKEN_DATA_IN, pep, 2000);
                }
        }

        return rcode;
}

#endif /* UHS_KINETIS_EHCI_INLINE_H */
