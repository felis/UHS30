/* Copyright (C) 2011 Circuits At Home, LTD. All rights reserved.

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
/* USB functions */
#ifndef _UHS_host_h_
#define _UHS_host_h_

// WARNING: Do not change the order of includes, or stuff will break!
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#include <Wire.h>
#include <SPI.h>
// None of these should ever be directly included by a driver, or a user's sketch.
#include "UHS_macros.h"
#include "UHS_USB_IDs.h"
#include "UHS_settings.h"
#include "UHS_usb_ch9.h"
#include "UHS_UsbCore.h"
#include "UHS_address.h"
#include "UHS_usbhost.h"
#include "UHS_printhex.h"
#include "UHS_message.h"

// Load system components as required
#if defined(LOAD_USB_HOST_SYSTEM) && !defined(USB_HOST_SYSTEM_LOADED)
#include "UHS_util_INLINE.h"
#include "UHS_host_INLINE.h"
#include "../dyn_SWI/dyn_SWI.h"

#include "UHS_printf_HELPER.h"

#if defined(LOAD_USB_HOST_SHIELD)
#include "../USB_HOST_SHIELD/USB_HOST_SHIELD.h"
#endif

#if defined(LOAD_UHS_KINETIS_FS_HOST) && !defined(UHS_KINETIS_FS_HOST_LOADED)
#include "../UHS_KINETIS_FS_HOST/UHS_KINETIS_FS_HOST.h"
#endif

// Load USB drivers and multiplexers

#if defined(LOAD_UHS_HUB)
#include "../UHS_HUB/UHS_HUB.h"
#endif // HUB loaded

#if defined(LOAD_UHS_BULK_STORAGE)
#include "../UHS_BULK_STORAGE/UHS_BULK_STORAGE.h"
#endif

#if defined(LOAD_GENERIC_STORAGE)
#include "../UHS_FS/UHS_FS.h"
#endif
// Add BT and optionally HID if directed to do so
#if defined(LOAD_UHS_BT)
#include "../UHS_BT/UHS_BT.h"
#endif // BT and optionally HID loaded

// Add HID
#if defined(LOAD_UHS_HID)
#include "../UHS_HID/UHS_HID.h"
#endif // HID loaded

// Add CDC multiplexers (currently only ACM)
#if defined(LOAD_UHS_CDC_ACM) || defined(LOAD_UHS_CDC_ACM_FTDI) || defined(LOAD_UHS_CDC_ACM_PROLIFIC) || defined(LOAD_UHS_CDC_ACM_XR21B1411)
#include "../UHS_CDC/UHS_CDC.h"
#include "../UHS_CDC_ACM/UHS_CDC_ACM.h"
// Basic CDC_ACM loaded
#if defined(LOAD_UHS_CDC_ACM_FTDI)
#include "../UHS_CDC_ACM/UHS_CDC_ACM.h"
#endif // FTDI loaded
#if defined(LOAD_UHS_CDC_ACM_PROLIFIC)
#include "../UHS_CDC_ACM/UHS_CDC_ACM.h"
#endif // Prolific loaded
#if defined(LOAD_UHS_CDC_ACM_XR21B1411)
#include "../UHS_CDC_ACM/UHS_CDC_ACM.h"
#endif // XR21B1411 loaded

//#if defined()
#endif // CDC loaded

#endif // System code loaded

#endif // _UHS_host_h_
