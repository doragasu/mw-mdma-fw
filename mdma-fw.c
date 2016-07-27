/** \file
 *
 *  \brief Main source file for the MegaDrive Memory Administration firmware.
 *  This firmware runs on the MeGaWiFi Programmer board by doragasu &
 *  1985alternativo.
 *
 *  \author doragasu
 *  \date   2015
 */

#define  INCLUDE_FROM_BULKVENDOR_C
#include "mdma-fw.h"
#include "cart_if.h"
#include "util.h"
#include "sys_fsm.h"
#include "bloader.h"
#include "timers.h"
#include "wifi-if.h"
#include "slip.h"	// Delete after tests!

// Define this to test BulkVendor echo
//#define _DEBUG_ECHO_TEST

/// \brief Returns cartridge status.
CartStatus CheckCartStatus(void) {
	// Status the last time this function was called.
	static uint8_t lastStatus;
	// Cartridge status
	CartStatus s;

	s.inserted = CIF__CIN_GET;
	if (s.inserted != lastStatus) {
		lastStatus = s.inserted;
		s.statChange = TRUE;
	} else {
		s.statChange = FALSE;
	}
	return s;
}


/** Main program entry point. This routine configures the hardware required by the application, then
 *  enters a loop to run the application tasks in sequence.
 */
#ifdef _DEBUG_ECHO_TEST
	uint8_t ReceivedData[VENDOR_IO_EPSIZE];
#endif //_DEBUG_ECHO_TEST
int main(void)
{
	uint8_t buttonStat, prevButtonStat;
#ifndef _DEBUG_ECHO_TEST
	CartStatus cs;
#endif //_DEBUG_ECHO_TEST

	// Init LUFA related stuff
	SetupHardware();
	// If button pressed, enter bootloader
	if (Buttons_GetStatus()) {
		JumpToBootloader();	
	}
	// Init Cartridge interface (and leave it in reset state)
	CifInit();
	// FIXME: REMOVE THIS LINE AFTER UART TEST
	CIF_SET__RST;
	// Init system state machine
	SfInit();
	// Initialize WiFi chip interface
	WiFiInit();

	// FIXME: REMOVE THESE LINES AFTER UART TEST
	// UART Scratchpad register test
//	UartWrite(UART_SPR, 0x55);
//	if (UartRead(UART_SPR) == 0x55) LEDs_TurnOnLEDs(LEDS_LED1);
//	UartWrite(UART_SPR, 0xAA);
//	if (UartRead(UART_SPR) == 0xAA) LEDs_TurnOnLEDs(LEDS_LED2);
//	WiFiSendTest();
//	WiFiEchoTest();
//	WiFiEchoServerTest();
//	const char testStr[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
//	while(1) {
//		if (SlipFrameSendPoll((unsigned char*)testStr, sizeof(testStr),
//				   	65535) != sizeof(testStr)) 
//			LEDs_TurnOnLEDs(LEDS_LED2);
//	}
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	prevButtonStat = Buttons_GetStatus();
	GlobalInterruptEnable();

#if !defined(_DEBUG_ECHO_TEST)
	// Generate an initial cart event
	cs = CheckCartStatus();
	if (cs.inserted) SfFsmCycle(SF_EVT_CIN);
	else SfFsmCycle(SF_EVT_COUT);
#endif //!defined(_DEBUG_ECHO_TEST)

	for (;;)
	{
		USB_USBTask();
		// If button changed status, send event
		if ((buttonStat = Buttons_GetStatus()) != prevButtonStat) {
			SfFsmCycle(buttonStat?SF_EVT_SW_PRESS:SF_EVT_SW_REL);
			prevButtonStat = buttonStat;
		}

#ifdef _DEBUG_ECHO_TEST	
		memset(ReceivedData, 0x00, sizeof(ReceivedData));
#else
		// Check if there has been a change on cart status
		cs = CheckCartStatus();
		if (cs.statChange) SfFsmCycle(cs.inserted?
				SF_EVT_CIN:SF_EVT_COUT);
		// Check if we must generate a time event
		if (SfEvtTimerNotify()) SfFsmCycle(SF_EVT_TIMER);
#endif //_DEBUG_ECHO_TEST
		Endpoint_SelectEndpoint(VENDOR_OUT_EPADDR);
		if (Endpoint_IsOUTReceived())
		{
#ifdef _DEBUG_ECHO_TEST
			Endpoint_Read_Stream_LE(ReceivedData, VENDOR_IO_EPSIZE, NULL);
			Endpoint_ClearOUT();

			Endpoint_SelectEndpoint(VENDOR_IN_EPADDR);
			Endpoint_Write_Stream_LE(ReceivedData, VENDOR_IO_EPSIZE, NULL);
			Endpoint_ClearIN();
#else
			LEDs_TurnOnLEDs(LEDMASK_USB_BUSY);
			SfFsmCycle(SF_EVT_DIN);
			LEDs_TurnOffLEDs(LEDMASK_USB_BUSY);
#endif // _DEBUG_ECHO_TEST
		}
	}
}

/** \brief Configures the board hardware and chip peripherals. */
void SetupHardware(void)
{
#if (ARCH == ARCH_AVR8)
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);
#elif (ARCH == ARCH_XMEGA)
	/* Start the PLL to multiply the 2MHz RC oscillator to 32MHz and switch the CPU core to run from it */
	XMEGACLK_StartPLL(CLOCK_SRC_INT_RC2MHZ, 2000000, F_CPU);
	XMEGACLK_SetCPUClockSource(CLOCK_SRC_PLL);

	/* Start the 32MHz internal RC oscillator and start the DFLL to increase it to 48MHz using the USB SOF as a reference */
	XMEGACLK_StartInternalOscillator(CLOCK_SRC_INT_RC32MHZ);
	XMEGACLK_StartDFLL(CLOCK_SRC_INT_RC32MHZ, DFLL_REF_INT_USBSOF, F_USB);

	PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
#endif

	/* Hardware Initialization */
	LEDs_Init();
	Buttons_Init();
	USB_Init();
}

/** \brief Event handler for the USB_Connect event. This indicates that the device is enumerating via the status LEDs. */
void EVENT_USB_Device_Connect(void)
{
	/* Indicate USB enumerating */
	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** \brief Event handler for the USB_Disconnect event. This indicates that the device is no longer connected to a host via
 *  the status LEDs.
 */
void EVENT_USB_Device_Disconnect(void)
{
	/* Indicate USB not ready */
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	SfFsmCycle(SF_EVT_USB_DET);
}

/** \brief Event handler for the USB_ConfigurationChanged event. This is fired when the host set the current configuration
 *  of the USB device after enumeration - the device endpoints are configured.
 */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool configSuccess = true;

	/* Setup Vendor Data Endpoints */
	configSuccess &= Endpoint_ConfigureEndpoint(VENDOR_IN_EPADDR,  EP_TYPE_BULK, VENDOR_I_EPSIZE, 2);
	configSuccess &= Endpoint_ConfigureEndpoint(VENDOR_OUT_EPADDR, EP_TYPE_BULK, VENDOR_O_EPSIZE, 2);

	// Set LEDs and generate FSM events according to result
	if (configSuccess) {
		LEDs_SetAllLEDs(LEDMASK_USB_READY);
		SfFsmCycle(SF_EVT_USB_ATT);
	} else {
		LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
		SfFsmCycle(SF_EVT_USB_ERR);
	}
}

/** Event handler for the USB_ControlRequest event. This is used to catch and process control requests sent to
 *  the device from the USB host before passing along unhandled control requests to the library for processing
 *  internally.
 */
void EVENT_USB_Device_ControlRequest(void)
{
	// Process vendor specific control requests here
}
