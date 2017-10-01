/************************************************************************//**
 * \file
 * \brief This module allows to control the 16c550 UART mounted in MeGaWiFi
 * cartridges.
 *
 * \author Jesús Alonso (doragasu)
 * \date   2016
 * \defgroup 16c550 16c550 chip management.
 * \{
 ****************************************************************************/

#ifndef _16C550_H_
#define _16C550_H_

#include <stdint.h>
#include "cart_if.h"

/// Clock applied to 16C550 chip (same as avr chip, 8 MHz)
//#define UART_CLK		14745600LU
#define UART_CLK		24000000LU

/// Desired baud rate.
/// \note 500 kbps tested under Linux with Minicom and works reliably.
/// ESP8266 is supposed to auto-baud, but it's untested with this speed.
//#define UART_BR			460800LU
//#define UART_BR			230400LU
//#define UART_BR			38400LU
//#define UART_BR			250000LU
//#define UART_BR			115200LU
#define UART_BR			500000LU
//#define UART_BR			750000LU
//#define UART_BR				1500000LU	// Fails with 24 MHz XTAL!

/// Lenght of the UART TX and RX FIFOs in characters
#define UART_FIFO_LENGTH	16

/// Division with one bit rounding, useful for divisor calculations.
#define DivWithRounding(dividend, divisor)	((((dividend)*2/(divisor))+1)/2)
/// Value to load on the UART divisor, high byte
#define UART_DLM_VAL	(DivWithRounding(UART_CLK, 16 * UART_BR)>>8)
//#define UART_DLM_VAL	((UART_CLK/16/UART_BR)>>8)
/// Value to load on the UART divisor, low byte
#define UART_DLL_VAL	(DivWithRounding(UART_CLK, 16 * UART_BR) & 0xFF)
//#define UART_DLL_VAL	((UART_CLK/16/UART_BR)&0xFF)

/// Base address for accessing the UART (A6 and A7 must be 1, all others
/// don't care as long as #TIME is also 0).
/// \note We are using WORD addresses
#define UART_BASE_ADDR	0x60

/** \addtogroup 16c550 uartRegs Addresses of the 16C550 UART registers
 *  \{
 */
/// Receiver holding register. Read only.
#define UART_RHR	(UART_BASE_ADDR + 0)
/// Transmit holding register. Write only.
#define UART_THR	(UART_BASE_ADDR + 0)
/// Interrupt enable register.
#define UART_IER	(UART_BASE_ADDR + 1)
/// FIFO control register. Write only.
#define UART_FCR	(UART_BASE_ADDR + 2)
/// Interrupt status register. Read only.
#define UART_ISR	(UART_BASE_ADDR + 2)
/// Line control register.
#define UART_LCR	(UART_BASE_ADDR + 3)
/// Modem control register.
#define UART_MCR	(UART_BASE_ADDR + 4)
/// Line status register. Read only.
#define UART_LSR	(UART_BASE_ADDR + 5)
/// Modem status register. Read only.
#define UART_MSR	(UART_BASE_ADDR + 6)
/// Scratchpad register.
#define UART_SPR	(UART_BASE_ADDR + 7)
/// Divisor latch LSB. Acessed only when LCR[7] = 1.
#define UART_DLL	(UART_BASE_ADDR + 0)
/// Divisor latch MSB. Acessed only when LCR[7] = 1.
#define UART_DLM	(UART_BASE_ADDR + 1)
/** \} */

/************************************************************************//**
 * \brief Writes a word to the Uart (in the #TIME range).
 *
 * \param[in] addr Address to which data will be written. Only the lower 8
 *            bits are used.
 * \param[in] data Data to write to addr address (8 bits width).
 ****************************************************************************/
static inline void UartWrite(uint8_t addr, uint8_t data) {
	// Generate address strobe and put address on the bus
	CIF_CLR__AS;
	CIF_ADDRL_PORT = addr;
	CIF_SET__AS;
	// Select chip
	CIF_CLR__TIME;
	// Signal _W
	CIF_CLR__W;
	// Write data to bus
	CIF_DATAL_PORT = data;
	CIF_DATAL_DDR = 0xFF;
	
	// Disable _W
	CIF_SET__W;
	// Deselect chip
	CIF_SET__TIME;
	// Remove data from bus
	CIF_DATAL_DDR  = 0;
	CIF_DATAL_PORT = 0xFF;
}

/************************************************************************//**
 * \brief Reads a word from the Uart (in the #TIME range).
 *
 * \param[in] addr Address that will be read.
 *
 * \return Readed word.
 ****************************************************************************/
static inline uint8_t UartRead(uint8_t addr) {
	uint8_t data;

	// Generate address strobe and put address on the bus
	CIF_CLR__AS;
	CIF_ADDRL_PORT = addr;
	CIF_SET__AS;
	// Select chip
	CIF_CLR__TIME;
	// Enable chip outputs
	CIF_CLR__OE;
	// Read data
	_NOP();		// Insert NOPs to ensure the input sinchronizer gets the data
	_NOP();		// TODO: a single NOP should be enough.
	data = CIF_DATAL_PIN;
	// Disable chip outputs
	CIF_SET__OE;
	// Deselect chip
	CIF_SET__TIME;
	
	return data;
}

/************************************************************************//**
 * \brief Initializes the driver. The baud rate is set to UART_BR, and the
 *        UART FIFOs are enabled. This function must be called before using
 *        any other API call.
 ****************************************************************************/
void UartInit(void);

/************************************************************************//**
 * \brief Checks if UART transmit register/FIFO is ready. In FIFO mode, up to
 *        16 characters can be loaded each time transmitter is ready.
 *
 * \return TRUE if transmitter is ready, FALSE otherwise.
 ****************************************************************************/
#define UartTxReady()	(UartRead(UART_LSR) & 0x20)

/************************************************************************//**
 * \brief Checks if UART receive register/FIFO has data available.
 *
 * \return TRUE if at least 1 byte is available, FALSE otherwise.
 ****************************************************************************/
#define UartRxReady()	(UartRead(UART_LSR) & 0x01)

/************************************************************************//**
 * \brief Sends a character. Please make sure there is room in the transmit
 *        register/FIFO by calling UartRxReady() before using this function.
 *
 * \return Received character.
 ****************************************************************************/
#define UartPutchar(c)	do{UartWrite(UART_THR, c);}while(0)

/************************************************************************//**
 * \brief Returns a received character. Please make sure data is available by
 *        calling UartRxReady() before using this function.
 *
 * \return Received character.
 ****************************************************************************/
#define UartGetchar()	UartRead(UART_RHR)


/************************************************************************//**
 * \brief Set bits specified by mask on specified register.
 *
 * \param[in] reg  Register that will be modified.
 * \param[in] mask Bit mask. Bits at '1' will be set on reg.
 ****************************************************************************/
#define UartSet(reg, mask)	do{UartWrite(reg, UartRead(reg)| mask);}while(0)

/************************************************************************//**
 * \brief Clears bits specified by mask on specified register.
 *
 * \param[in] reg  Register that will be modified.
 * \param[in] mask Bit mask. Bits at '1' will be clear on reg.
 ****************************************************************************/
#define UartClr(reg, mask)	do{UartWrite(reg, UartRead(reg)&~mask);}while(0)

/************************************************************************//**
 * \brief Sets (output high) #DTR UART pin.
 ****************************************************************************/
#define UartSetDtr()	do{UartClr(UART_MCR, 0x01);}while(0)

/************************************************************************//**
 * \brief Clears (output low) #DTR UART pin.
 ****************************************************************************/
#define UartClrDtr()	do{UartSet(UART_MCR, 0x01);}while(0)

/************************************************************************//**
 * \brief Sets (output high) #CLR UART pin.
 ****************************************************************************/
#define UartSetRts()	do{UartClr(UART_MCR, 0x02);}while(0)

/************************************************************************//**
 * \brief Clears (output low) #RTS UART pin.
 ****************************************************************************/
#define UartClrRts()	do{UartSet(UART_MCR, 0x02);}while(0)

/************************************************************************//**
 * \brief Sets (output high) #OUT1 UART pin.
 ****************************************************************************/
#define UartSetOut1()	do{UartClr(UART_MCR, 0x04);}while(0)

/************************************************************************//**
 * \brief Clears (output low) #OUT1 UART pin.
 ****************************************************************************/
#define UartClrOut1()	do{UartSet(UART_MCR, 0x04);}while(0)

/************************************************************************//**
 * \brief Sets (output high) #OUT2 UART pin.
 ****************************************************************************/
#define UartSetOut2()	do{UartClr(UART_MCR, 0x08);}while(0)

/************************************************************************//**
 * \brief Clears (output low) #OUT2 UART pin.
 ****************************************************************************/
#define UartClrOut2()	do{UartSet(UART_MCR, 0x08);}while(0)

/************************************************************************//**
 * \brief Obtains the #CTS (active low) pin status.
 *
 * \return 0 if pin active, non-zero otherwise.
 ****************************************************************************/
#define UartCtsGet()	(UartRead(UART_LCR) & 0x10)

/************************************************************************//**
 * \brief Obtains the #DSR (active low) pin status.
 *
 * \return 0 if pin active, non-zero otherwise.
 ****************************************************************************/
#define UartDsrGet()	(UartRead(UART_LCR) & 0x20)

/************************************************************************//**
 * \brief Returns the TX FIFO status (empty/not empty).
 *
 * \return 0 if FIFO is not empty, non-zero if FIFO is empty.
 ****************************************************************************/
#define UartTxFifoEmpty()	(UartRead(UART_LSR) & 0x20)

/************************************************************************//**
 * \brief Returns the RX FIFO status (Data available/data not available).
 *
 * \return 0 if there's no data in the FIFO, non-zero if FIFO has data.
 ****************************************************************************/
#define UartRxFifoData()	(UartRead(UART_LSR) & 0x01)

/************************************************************************//**
 * \brief Resets UART FIFOs, removing pending characters to send/receive.
 ****************************************************************************/
#define UartFlush()			do{UartWrite(UART_FCR, 0x03);}while(0)

#endif //_16C550_H_

/** \} */

