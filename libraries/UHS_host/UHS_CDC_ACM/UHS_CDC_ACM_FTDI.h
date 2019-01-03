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
#if !defined(UHS_CDC_ACM_FTDI_LOADED)
#define UHS_CDC_ACM_FTDI_LOADED

#define                         FT232AM (0x0200U)
#define                         FT232BM (0x0400U)
#define                         FT2232C (0x0500U)
#define                          FT232R (0x0600U)
#define                         FT2232H (0x0700U)
#define                         FT4232H (0x0800U)
#define                          FT232H (0x0900U)
#define                          FT230X (0x1000U)


// Commands
#define                  FTDI_SIO_RESET (0x00U) // Reset the port
#define             FTDI_SIO_MODEM_CTRL (0x01U) // Set the modem control register
#define          FTDI_SIO_SET_FLOW_CTRL (0x02U) // Set flow control register
#define          FTDI_SIO_SET_BAUD_RATE (0x03U) // Set baud rate
#define               FTDI_SIO_SET_DATA (0x04U) // Set the data characteristics of the port
#define       FTDI_SIO_GET_MODEM_STATUS (0x05U) // Get the current value of modem status register
#define         FTDI_SIO_SET_EVENT_CHAR (0x06U) // Set the event character
#define         FTDI_SIO_SET_ERROR_CHAR (0x07U) // Set the error character
#define      FTDI_SIO_SET_LATENCY_TIMER (0x09U) // Set the latency timer
#define      FTDI_SIO_GET_LATENCY_TIMER (0x0AU) // Get the latency timer

#define              FTDI_SIO_RESET_SIO (0x00U)
#define         FTDI_SIO_RESET_PURGE_RX (0x01U)
#define         FTDI_SIO_RESET_PURGE_TX (0x02U)

#define      FTDI_SIO_DISABLE_FLOW_CTRL (0x00U)
#define             FTDI_SIO_RTS_CTS_HS (0x01U)
#define             FTDI_SIO_DTR_DSR_HS (0x02U)
#define            FTDI_SIO_XON_XOFF_HS (0x04U)

// status 0
#define               FTDI_SIO_CTS_MASK (0x10U)
#define               FTDI_SIO_DSR_MASK (0x20U)
#define                FTDI_SIO_RI_MASK (0x40U)
#define              FTDI_SIO_RLSD_MASK (0x80U)

// status 1
#define                     FTDI_SIO_DR (0x01U) // Data Ready
#define                     FTDI_SIO_OE (0x02U) // Overrun Error
#define                     FTDI_SIO_PE (0x04U) // Parity Error
#define                     FTDI_SIO_FE (0x08U) // Framing Error
#define                     FTDI_SIO_BI (0x10U) // Break Interrupt
#define                   FTDI_SIO_THRE (0x20U) // Transmitter Holding Register Empty
#define                   FTDI_SIO_TEMT (0x40U) // Transmitter Empty
#define                   FTDI_SIO_FIFO (0x80U) // Error in RX FIFO


// keep for notes.
//#define FTDI_SIO_SET_DATA_PARITY_NONE   (0x0U << 8 )
//#define FTDI_SIO_SET_DATA_PARITY_ODD    (0x1U << 8 )
//#define FTDI_SIO_SET_DATA_PARITY_EVEN   (0x2U << 8 )
//#define FTDI_SIO_SET_DATA_PARITY_MARK   (0x3U << 8 )
//#define FTDI_SIO_SET_DATA_PARITY_SPACE  (0x4U << 8 )
//#define FTDI_SIO_SET_DATA_STOP_BITS_1   (0x0U << 11)
//#define FTDI_SIO_SET_DATA_STOP_BITS_15  (0x1U << 11)
//#define FTDI_SIO_SET_DATA_STOP_BITS_2   (0x2U << 11)
//#define FTDI_SIO_SET_BREAK              (0x1U << 14)

#endif // __UHS_CDC_ACM_FTDI_H__
