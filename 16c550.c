/************************************************************************//**
 * \file
 * \brief This module allows to control the 16c550 UART mounted in MeGaWiFi
 * cartridges.
 *
 * \author Jesús Alonso (doragasu)
 * \date   2016
 ****************************************************************************/

#include "16c550.h"

/************************************************************************//**
 * \brief Initializes the driver. The baud rate is set to UART_BR, and the
 *        UART FIFOs are enabled. This function must be called before using
 *        any other API call.
 ****************************************************************************/
void UartInit(void) {
	// Set line to BR,8N1. LCR[7] must be set to access DLX registers
	UartWrite(UART_LCR, 0x83);
	UartWrite(UART_DLM, UART_DLM_VAL);
	UartWrite(UART_DLL, UART_DLL_VAL);
	UartWrite(UART_LCR, 0x03);

	// Enable FIFOs
	UartWrite(UART_FCR, 0x01);
	// Reset FIFOs
	UartWrite(UART_FCR, 0x07);

	// Ready to go! Interrupt and DMA modes were not configured since the
	// Megadrive console lacks interrupt/DMA control pins on cart connector.
}

