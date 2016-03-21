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
#if !defined(__CDCPROLIFIC_H__)
#define __CDCPROLIFIC_H__

#define                      PROLIFIC_REV_H (0x0202U)
#define                      PROLIFIC_REV_X (0x0300U)
#define              PROLIFIC_REV_HX_CHIP_D (0x0400U)
#define                      PROLIFIC_REV_1 (0x0001U)

#define                   PROLIFIC_kXOnChar '\x11'
#define                  PROLIFIC_kXOffChar '\x13'

#define              PROLIFIC_SPECIAL_SHIFT (0x05U)
#define               PROLIFIC_SPECIAL_MASK ((1<<PROLIFIC_SPECIAL_SHIFT) - 1)
#define                  PROLIFIC_STATE_ALL ( PD_RS232_S_MASK | PD_S_MASK )
#define               PROLIFIC_FLOW_RX_AUTO ( PD_RS232_A_RFR | PD_RS232_A_DTR | PD_RS232_A_RXO )
#define               PROLIFIC_FLOW_TX_AUTO ( PD_RS232_A_CTS | PD_RS232_A_DSR | PD_RS232_A_TXO | PD_RS232_A_DCD )
#define                PROLIFIC_CAN_BE_AUTO ( FLOW_RX_AUTO | FLOW_TX_AUTO )
#define                 PROLIFIC_CAN_NOTIFY ( PD_RS232_N_MASK )
#define              PROLIFIC_EXTERNAL_MASK ( PD_S_MASK | (PD_RS232_S_MASK & ~PD_RS232_S_LOOP) )
#define             PROLIFIC_INTERNAL_DELAY ( PD_RS232_S_LOOP )
#define               PROLIFIC_DEFAULT_AUTO ( PD_RS232_A_DTR | PD_RS232_A_RFR | PD_RS232_A_CTS | PD_RS232_A_DSR )
#define             PROLIFIC_DEFAULT_NOTIFY (0x00U)
#define              PROLIFIC_DEFAULT_STATE ( PD_S_TX_ENABLE | PD_S_RX_ENABLE | PD_RS232_A_TXO | PD_RS232_A_RXO )

#define              PROLIFIC_CONTINUE_SEND (0x01U)
#define                 PROLIFIC_PAUSE_SEND (0x02U)

#define                PROLIFIC_kRxAutoFlow ((UInt32)( PD_RS232_A_RFR | PD_RS232_A_DTR | PD_RS232_A_RXO ))
#define                PROLIFIC_kTxAutoFlow ((UInt32)( PD_RS232_A_CTS | PD_RS232_A_DSR | PD_RS232_A_TXO | PD_RS232_A_DCD ))
#define         PROLIFIC_kControl_StateMask ((UInt32)( PD_RS232_S_CTS | PD_RS232_S_DSR | PD_RS232_S_CAR | PD_RS232_S_RI  ))
#define              PROLIFIC_kRxQueueState ((UInt32)( PD_S_RXQ_EMPTY | PD_S_RXQ_LOW_WATER | PD_S_RXQ_HIGH_WATER | PD_S_RXQ_FULL ))
#define              PROLIFIC_kTxQueueState ((UInt32)( PD_S_TXQ_EMPTY | PD_S_TXQ_LOW_WATER | PD_S_TXQ_HIGH_WATER | PD_S_TXQ_FULL ))

#define               PROLIFIC_kCONTROL_DTR (0x01U)
#define               PROLIFIC_kCONTROL_RTS (0x02U)

#define        PROLIFIC_kStateTransientMask (0x74U)
#define                PROLIFIC_kBreakError (0x04U)
#define                PROLIFIC_kFrameError (0x10U)
#define               PROLIFIC_kParityError (0x20U)
#define              PROLIFIC_kOverrunError (0x40U)

#define                       PROLIFIC_kCTS (0x80U)
#define                       PROLIFIC_kDSR (0x02U)
#define                        PROLIFIC_kRI (0x08U)
#define                       PROLIFIC_kDCD (0x01U)
#define           PROLIFIC_kHandshakeInMask ((UInt32)( PD_RS232_S_CTS | PD_RS232_S_DSR | PD_RS232_S_CAR | PD_RS232_S_RI  ))

#define  PROLIFIC_VENDOR_WRITE_REQUEST_TYPE (0x40U)
#define       PROLIFIC_VENDOR_WRITE_REQUEST (0x01U)

#define   PROLIFIC_VENDOR_READ_REQUEST_TYPE (0xc0U)
#define        PROLIFIC_VENDOR_READ_REQUEST (0x01U)

// Device Configuration Registers (DCR0, DCR1, DCR2)
#define                   PROLIFIC_SET_DCR0 (0x00U)
#define                   PROLIFIC_GET_DCR0 (0x80U)
#define                  PROLIFIC_DCR0_INIT (0x01U)
#define                PROLIFIC_DCR0_INIT_H (0x41U)
#define                PROLIFIC_DCR0_INIT_X (0x61U)

#define                   PROLIFIC_SET_DCR1 (0x01U)
#define                   PROLIFIC_GET_DCR1 (0x81U)
#define                PROLIFIC_DCR1_INIT_H (0x80U)
#define                PROLIFIC_DCR1_INIT_X (0x00U)

#define                   PROLIFIC_SET_DCR2 (0x02U)
#define                   PROLIFIC_GET_DCR2 (0x82U)
#define                PROLIFIC_DCR2_INIT_H (0x24U)
#define                PROLIFIC_DCR2_INIT_X (0x44U)

// On-chip Data Buffers:
#define PROLIFIC_RESET_DOWNSTREAM_DATA_PIPE (0x08U)
#define   PROLIFIC_RESET_UPSTREAM_DATA_PIPE (0x09U)

enum PROLIFIC_tXO_State {
        PROLIFIC_kXOnSent = -2,
        PROLIFIC_kXOffSent = -1,
        PROLIFIC_kXO_Idle = 0,
        PROLIFIC_kXOffNeeded = 1,
        PROLIFIC_kXOnNeeded = 2
};

enum PROLIFIC_pl2303_type {
        PROLIFIC_unknown,
        PROLIFIC_type_1, /* don't know the difference between type 0 and */
        PROLIFIC_rev_X, /* type 1, until someone from prolific tells us... */
        PROLIFIC_rev_HX, /* HX version of the pl2303 chip */
        PROLIFIC_rev_H
};

#endif // __CDCPROLIFIC_H__
