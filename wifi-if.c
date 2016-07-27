/************************************************************************//**
 * \file
 * \brief This module allows to control the WiFi chip mounted on MeGaWiFi
 * cartridges.
 *
 * \author Jesús Alonso (doragasu)
 * \date   2016
 ****************************************************************************/
#include "wifi-if.h"
#include "slip.h"
#include "util.h"
#include <LUFA/Drivers/Board/LEDs.h>

#define TEST_CHR	"0123456789ABCDEF"

/************************************************************************//**
 * \brief Initializes WiFi interface. Must be called once before any other
 *        function in the module, and after calling CifInit().
 ****************************************************************************/
void WiFiInit(void) {
	// Initializa UART
	SlipInit();
	// Set the module in RESET, and prepare for starting bootloader.
	WiFiReset();
	WiFiPwrUp();
	WiFiPrgEnable();
}

/************************************************************************//**
 * \brief Sends an array of characters using polling method.
 *
 * \param[in] data Data buffer to send.
 * \param[in] dataLen Number of bytes to send.
 *
 * \return The number of bytes sent.
 * \warning This function blocks, eating all the CPU until buffer is sent.
 ****************************************************************************/
uint16_t WiFiPollSend(uint8_t data[], uint16_t dataLen) {
	uint16_t i;
	uint8_t j;

	for (i = 0; i < dataLen;) {
		// Wait until FIFO empty, and send in bursts of up to 16 characters.
		while (!UartTxFifoEmpty());
		for (j = 0; j < 16 && i < dataLen; i++, j++) {
			UartPutchar(data[i]);
		}
	}

	return i;
}

/************************************************************************//**
 * \brief Receives an array of characters using polling method.
 *
 * \param[in] data Data buffer holding the received characters.
 * \param[in] dataLen Number of bytes to read.
 *
 * \return The number of bytes received.
 * \warning This function blocks, eating the CPU until buffer is received.
 ****************************************************************************/
uint16_t WiFiPollRecv(uint8_t data[], uint16_t dataLen) {
	uint16_t i;

	for (i = 0; i < dataLen; i++) {
		// Wait until there's data on FIFO
		while (!UartRxFifoData());
		data[i]  = UartGetchar();
	}

	return i;
}

void WiFiSendTest(void) {
	// Uart Write Test
	while (TRUE) {
		// Wait until FIFO empty
		while (!UartTxFifoEmpty());
		// FIFO empty, load 16 characters
		int i;
		for (i = 0; i < 16; i++) UartPutchar(TEST_CHR[i]);
	}
}

void WiFiEchoTest(void) {
	uint8_t c = 0;

	while (TRUE) {
		// Wait until FIFO empty and write char
		while (!UartTxFifoEmpty());
		UartPutchar(c);
		// Waitn until char is echoed and compare with sent
		while (!UartRxFifoData());
		if (c != UartGetchar()) {
			// Received character does not match!
			LEDs_TurnOffLEDs(LEDS_LED2);
			LEDs_ToggleLEDs(LEDS_LED1);
		}
		c++;
	}
}

void WiFiEchoServerTest(void) {
	while (TRUE) {
		while (!UartRxFifoData());
		UartPutchar(UartGetchar());
	}
}
