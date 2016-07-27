/** \file
 *  \author doragasu
 *  \date   2015
 *	\defgroup mdma-fw MegaDrive Memory Administration Firmware
 *  \{ */

#ifndef _BULK_VENDOR_H_
#define _BULK_VENDOR_H_

	/* Includes: */
		#include <avr/io.h>
		#include <avr/wdt.h>
		#include <avr/power.h>
		#include <avr/interrupt.h>

		#include "Descriptors.h"

		#include <LUFA/Drivers/USB/USB.h>
		#include <LUFA/Drivers/Board/LEDs.h>
		#include <LUFA/Platform/Platform.h>
		#include <LUFA/Drivers/Board/Buttons.h>

/** \addtogroup mdma-fw CartStatus Cartridge status information
 * Holds information about if the cartridge is or not inserted, and if
 * there has been a cartridge status change since last time.
 * \{
 */
typedef union {
	uint8_t all;				///< Access to all fields simultaneously
	struct {
		uint8_t statChange:1;	///< Status changed if TRUE
		uint8_t inserted:1;		///< Cartridge inserted if TRUE
	};
} CartStatus;
/** \} */

	/* Macros: */
		/** LED mask for the library LED driver, to indicate that the USB interface is not ready. */
		#define LEDMASK_USB_NOTREADY      LEDS_NO_LEDS

		/** LED mask for the library LED driver, to indicate that the USB interface is enumerating. */
		#define LEDMASK_USB_ENUMERATING   (LEDS_LED1 | LEDS_LED2)

		/** LED mask for the library LED driver, to indicate that the USB interface is ready. */
		#define LEDMASK_USB_READY         LEDS_LED1

		/** LED mask for the library LED driver, to indicate that an error has occurred in the USB interface. */
		#define LEDMASK_USB_ERROR         LEDS_LED2

		/** LED mask for the library LED driver, to indicate that the USB interface is busy. */
		#define LEDMASK_USB_BUSY          LEDS_LED2

	/* Function Prototypes: */
		void SetupHardware(void);

		void EVENT_USB_Device_Connect(void);
		void EVENT_USB_Device_Disconnect(void);
		void EVENT_USB_Device_ConfigurationChanged(void);
		void EVENT_USB_Device_ControlRequest(void);

#endif

/** \} */

