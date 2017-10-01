/************************************************************************//**
 * \file
 * \brief System state machine. Receives events from the cartridge and USB
 * interface, and performs the corresponding actions.
 *
 * \author Jesús Alonso (doragasu)
 * \date   2015
 ****************************************************************************/
#include "sys_fsm.h"
#include "flash.h"
#include "cart_if.h"
#include "util.h"
#include "bloader.h"
#include "slip.h"
#include "wifi-if.h"
#include <LUFA//Drivers/USB/USB.h>
#include <Descriptors.h>
#include <string.h>
#include <LUFA/Drivers/Board/LEDs.h>
#include <avr/cpufunc.h>

/// Command and data bytes for the ESP8266 SYNC command.
const char syncFrame[] = {
	0x00, 0x08, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x07, 0x07, 0x12, 0x20,
	0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 
	0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 
	0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
	0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55
};

/** \addtogroup sys_fsm PortDefs Port definitions for the programmer board.
 * \{ */
#define SF_GPIO_NUM_PORTS	6
#define SF_PORTA			0
#define SF_PORTB			1
#define SF_PORTC			2
#define SF_PORTD			3
#define SF_PORTE			4
#define SF_PORTF			5
/** \} */

/// Maximum length of the write flash payload (in words)
#define SF_MAX_WRITE_WLEN			((VENDOR_O_EPSIZE - 6)/2)
/// Maximum length of the read flash payload (in words)
#define SF_MAX_READ_WLEN			((VENDOR_I_EPSIZE - 6)/2)

/// Buffer for receiving data and sending replies
/// \note Compiler/linker is stupid, and if you place this buffer inside a
/// function body, you will not get an error, not even a single warning,
/// even though the buffer length is larger than the stack. Welcome to 2016!
static uint8_t buf[MAX(VENDOR_O_EPSIZE, VENDOR_I_EPSIZE)];

/************************************************************************//**
 * \brief Write a word using little endian order, guarantees proper
 * operation even when using unaligned addresses.
 *
 * \param[out] dest Destination to which write the word.
 * \param[in]  src  Source word
 ****************************************************************************/
inline void SfUnalignWordWrite(uint8_t dest[], uint16_t src) {
	dest[0] = src;
	dest[1] = src>>8;
}
// System FSM data
static SfInstance si;

/************************************************************************//**
 * \brief Module initialization. Must be called before using any other
 * function from this module.
 ****************************************************************************/
void SfInit(void) {
	// Init flash interface
	FlashInit();
	// Set default values
	memset(&si, 0, sizeof(SfInstance));
	// TODO: System timer initialization
	si.s = SF_IDLE;
}
//// Abort memory operations (if any)
//void SfMemAbort(void) {
//	// TODO
//}

/************************************************************************//**
 * \brief Receive a complete endpoint data frame.
 *
 * \param[out] data Array containing the received data.
 ****************************************************************************/
static inline void SfDataRecv(uint8_t data[]) {
	// We do not need to select endpoint, as it has been previously
	// selected to check if there is incoming data
	Endpoint_Read_Stream_LE(data, VENDOR_O_EPSIZE, NULL);
	Endpoint_ClearOUT();
}

/************************************************************************//**
 * \brief Send a complete endpoint data frame.
 *
 * \param[in] data Array with the data to send.
 * \param[in] len  Number of bytes of data to send.
 ****************************************************************************/
static inline void SfDataSend(uint8_t data[], uint16_t len) {
	memset(data+len, 0, VENDOR_I_EPSIZE-len);
	Endpoint_SelectEndpoint(VENDOR_IN_EPADDR);
	Endpoint_Write_Stream_LE(data, VENDOR_I_EPSIZE, NULL);
	Endpoint_ClearIN();
}

/************************************************************************//**
 * \brief Read/write to GPIO pins. Input/output parameters take a byte for
 * each port from PORTA to PORTF.
 *
 * \param[in] mask	Array with the pin mask. Pins set will be readed/written
 * \param[in] r_w	Read/write mask. '1' reads, '0' writes.
 * \param[in] value	Only used for writes. Holds the value to write.
 * \param[out] readed Values read for specified pins.
 *
 * \todo Write this function in a more elegant way!
 ****************************************************************************/
void SfGpioAction(uint8_t mask[], uint8_t r_w[], uint8_t value[], uint8_t readed[]) {
	uint8_t scratch;

	// First perform read on requested ports
	if ((scratch = mask[SF_PORTA] & r_w[SF_PORTA])) {
		// Configure pins for reading and obtain value
		DDRA &= ~scratch;	// Set pins as input
		PORTA |= scratch;	// Enable pullups
		readed[SF_PORTA] = PINA & scratch;	// Read data
	}
	if ((scratch = mask[SF_PORTB] & r_w[SF_PORTB])) {
		DDRB &= ~scratch;
		PORTB |= scratch;
		readed[SF_PORTB] = PINB & scratch;
	}
	if ((scratch = mask[SF_PORTC] & r_w[SF_PORTC])) {
		DDRC &= ~scratch;
		PORTC |= scratch;
		readed[SF_PORTC] = PINC & scratch;
	}
	if ((scratch = mask[SF_PORTD] & r_w[SF_PORTD])) {
		DDRD &= ~scratch;
		PORTD |= scratch;
		readed[SF_PORTD] = PIND & scratch;
	}
	if ((scratch = mask[SF_PORTE] & r_w[SF_PORTE])) {
		DDRE &= ~scratch;
		PORTE |= scratch;
		readed[SF_PORTE] = PINE & scratch;
	}
	if ((scratch = mask[SF_PORTF] & r_w[SF_PORTF])) {
		DDRF &= ~scratch;
		PORTF |= scratch;
		readed[SF_PORTF] = PINF & scratch;
	}
	// Write to requested pins
	if ((scratch = mask[SF_PORTA] & ~r_w[SF_PORTA])) {
		// Configure pins for writing and write value
		DDRA |= scratch;	// Set pins as outputs
		PORTA |= scratch & value[SF_PORTA];		// Write ones
		PORTA &= ~(scratch & ~value[SF_PORTA]);	// Write zeros
	}
	if ((scratch = mask[SF_PORTB] & ~r_w[SF_PORTB])) {
		DDRB |= scratch;
		PORTB |= scratch & value[SF_PORTB];
		PORTB &= ~(scratch & ~value[SF_PORTB]);
	}
	if ((scratch = mask[SF_PORTC] & ~r_w[SF_PORTC])) {
		DDRC |= scratch;
		PORTC |= scratch & value[SF_PORTC];
		PORTC &= ~(scratch & ~value[SF_PORTC]);
	}
	if ((scratch = mask[SF_PORTD] & ~r_w[SF_PORTD])) {
		DDRD |= scratch;
		PORTD |= scratch & value[SF_PORTD];
		PORTD &= ~(scratch & ~value[SF_PORTD]);
	}
	if ((scratch = mask[SF_PORTE] & ~r_w[SF_PORTE])) {
		DDRE |= scratch;
		PORTE |= scratch & value[SF_PORTE];
		PORTE &= ~(scratch & ~value[SF_PORTE]);
	}
	if ((scratch = mask[SF_PORTF] & ~r_w[SF_PORTF])) {
		DDRF |= scratch;
		PORTF |= scratch & value[SF_PORTF];
		PORTF &= ~(scratch & ~value[SF_PORTF]);
	}
}

/************************************************************************//**
 * \brief Process a WiFi module related command.
 *
 * \param[in] cmd Command directed to the WiFi chip.
 * \param[inout] data Array with the data containing the command request,
 *                    and the command reply once the function returns.
 ****************************************************************************/
uint16_t SfWiFiCmdProc(uint8_t event, uint8_t data[]) {
	uint16_t len;
	uint16_t sent;
	uint16_t step;
	uint8_t cmd;

	// Check we have a command request (because we received data).
	if (SF_EVT_DIN != event) return 0;

	UartFlush();
	// Check which command we have in.
	switch (MDMA_CMD(data)) {
		case MDMA_WIFI_CMD:			// Forward command to the WiFi module
			len = data[1];
			cmd = data[5];
			// Forward command to WiFi module and read response
			if (SlipFrameSendPoll(data + SF_WIFI_CMD_PAYLOAD_OFF, len,
					SF_WIFI_CMD_TOUT_CYCLES) != len) {
				data[0] = MDMA_ERR;
				data[1] = 1;
				return 2;
			}
			// Read module response
			for (step = 100; step; step--) {
				if (!SlipFrameRecvPoll(data, VENDOR_O_EPSIZE, &len,
							SF_WIFI_CMD_TOUT_CYCLES)) {
					if (1 == data[0] && data[1] == cmd) {
						data[0] = MDMA_OK;
						return len;
					}
				}
				USB_USBTask();
			}
			return 2;

		case MDMA_WIFI_CMD_LONG:	// Forward long command to WiFi module
			len = data[1] | (data[2]<<8);
			// Forward split long command to WiFi module and read response.
			// Data is received split in several bulk transfers until
			// completion
			SlipSplitFrameSendSof(SF_WIFI_CMD_TOUT_CYCLES);
			Endpoint_SelectEndpoint(VENDOR_OUT_EPADDR);
			for (sent = 0; sent < len; sent += step) {
				SfDataRecv(data);
				step = MIN(VENDOR_O_EPSIZE, len - sent);
				if (SlipSplitFrameAppendPoll(data, step, SF_WIFI_CMD_TOUT_CYCLES) !=
						step) {
					data[0] = MDMA_ERR;
					return 1;
				}
			}
			SlipSplitFrameSendEof(SF_WIFI_CMD_TOUT_CYCLES);
			// Completed, receive module response
			if (SlipFrameRecvPoll(data, VENDOR_O_EPSIZE, &len,
						SF_WIFI_CMD_TOUT_CYCLES)) {
				data[0] = MDMA_ERR;
				return 1;
			}
			return len;	// OK!

		case MDMA_WIFI_CTRL:		// WiFi module control using GPIO
			// Execute control action
			switch (data[1]) {
				case SF_WIFI_CTRL_RST:
					// Put module in reset
					WiFiReset();
					break;

				case SF_WIFI_CTRL_RUN:
					// Release reset
					WiFiStart();
					break;

				case SF_WIFI_CTRL_BLOAD:
					// Set bootloader mode
					WiFiPrgEnable();
					break;

				case SF_WIFI_CTRL_APP:
					// Set application mode
					WiFiPrgDisable();
					break;

				case SF_WIFI_CTRL_SYNC:
					// Send the SYNC frame and try reading the response
					// until success or too many attemps.
					for (step = data[2]; step; step--) {
						UartFlush();
						SlipFrameSendPoll((uint8_t*)syncFrame,
								sizeof(syncFrame),
								SF_WIFI_CMD_TOUT_CYCLES);
						while(!UartTxFifoEmpty());
						if (!SlipFrameRecvPoll(data, VENDOR_O_EPSIZE,
									&len, SF_WIFI_CMD_TOUT_CYCLES)) {
							// Check we received the sync response
							if (1 == data[0] && 8 == data[1]) {
								data[0] = MDMA_OK;
								return 1;
							}
						}
					}
					// Retries completed before sync correct
					data[0] = MDMA_ERR;
					break;

				default:
					// Unsupported!!!
					data[0] = MDMA_ERR;
					return 1;
			}
			data[0] = MDMA_OK;
			return 1;

		default:
			break;
	}
	// Unsupported!!!
	data[0] = MDMA_ERR;
	return 1;
}

/************************************************************************//**
 * \brief Processes a command, doing the requested action, and preparing the
 * reply to be sent.
 *
 * \param[inout] data Incoming data containing the command. On function return,
 *               it contains the reply to send to the command.
 * \return The number of bytes of the reply to be sent.
 ****************************************************************************/
uint16_t SfCmdProc(uint8_t data[]) {
	// Length of the reply that must be sent to host
	uint16_t repLen;
	uint16_t i;
	uint32_t addr;
	uint8_t port[SF_GPIO_NUM_PORTS];
	uint16_t length;
	uint8_t toWrite, written;
	uint16_t step;

	switch (MDMA_CMD(data)) {
		case MDMA_MANID_GET:	// Flash manufacturer ID
			data[0] = MDMA_OK;
			SfUnalignWordWrite(data+1, si.fc.manId);
			repLen = 3;
			break;

		case MDMA_DEVID_GET:	// Flash device ID
			data[0] = MDMA_OK;
			for (i = 0; i < 3; i++)
				SfUnalignWordWrite(data+1+(2*i), si.fc.devId[i]);
			repLen = 7;
			break;

		case MDMA_READ:			// Flash read
			// Save address and length
			addr = MDMA_ADDR(data);
			length = MDMA_LENGTH(data);
			// Send OK
			data[0] = MDMA_OK;
			SfDataSend(data, 1);
			// Data send loop
			while (length) {
				step = MIN(length, VENDOR_I_EPSIZE>>1);
				for (i = 0; i < step; i++, addr++)
					((uint16_t*)data)[i] = FlashRead(addr);
				length -= step;
				SfDataSend(data, step<<1);
			}
			repLen = 0;
			break;

		case MDMA_CART_ERASE:	// Complete flash erase
			data[0] = FlashChipErase()?MDMA_OK:MDMA_ERR;
			repLen = 1;
			break;

		case MDMA_SECT_ERASE:	// Complete flash sector erase
			data[0] = FlashSectErase(MDMA_DWORD_AT(data,1))?
				MDMA_OK:MDMA_ERR;
			repLen = 1;
			break;

		case MDMA_WRITE:		// Flash write
			// Save address and length
			addr = MDMA_ADDR(data);
			length = MDMA_LENGTH(data);
			// Send OK
			data[0] = MDMA_OK;
			SfDataSend(data, 1);
			// Data write loop
			Endpoint_SelectEndpoint(VENDOR_OUT_EPADDR);
			while (length)
			{
				// Read data
				SfDataRecv(data);
				// Data received on endpoint
				step = MIN(length, VENDOR_O_EPSIZE>>1);
				// Write data in blocks of max 16 words. The first
				// write takes care of avoiding crossing a 16-word
				// write-buffer boundary. Following writes are
				// guaranteed not to cross it.
				toWrite = MIN(step, 16 - (addr&0xF));
				i = FlashWriteBuf(addr, (uint16_t*)data, toWrite);
				if (i == toWrite) {
					addr += i;
					// First write is OK, write remaining data
					while (i < step) {
						toWrite = MIN(step - i, 16);
						written = FlashWriteBuf(addr, ((uint16_t*)data) + i,
								toWrite);
						i += written;
						addr += written;
						// Check for errors
						if (written != toWrite) break;
					}
				}
				length -= i;
			}
			repLen = 0;
			break;

		case MDMA_MAN_CTRL:		// Manual line control
			// Check magic bytes
			if ((data[1] == 0x19) && (data[2] == 0x85) &&
				(data[3] == 0xBA) && (data[4] == 0xDA) &&
				(data[5] == 0x55)) {
				SfGpioAction(&data[6], &data[12], &data[18], port);
				repLen = 1 + SF_GPIO_NUM_PORTS;
			} else {
				// Incorrect magic bytes, return error
				data[0] = MDMA_ERR;
				repLen = 1;
			}
			break;

		case MDMA_BUTTON_GET:	// Read button status
			// Return button status and clear button events
			repLen = 2;
			data[0] = MDMA_OK;
			data[1] = si.sw;
			si.sw &= ~SF_SW_EVENT;
			break;

		case MDMA_BOOTLOADER:	// Enter bootloader
			JumpToBootloader();
			// The function above does not return, but we assign a value
			// for the compiler not to complain.
			repLen = 0;

		// WiFi module related commands, processed in a separate function.
		case MDMA_WIFI_CMD:
		case MDMA_WIFI_CMD_LONG:
		case MDMA_WIFI_CTRL:
			repLen = SfWiFiCmdProc(SF_EVT_DIN, data);
			break;

		default:
			// Unsupported command, return error
			data[0] = MDMA_ERR;
			repLen = 1;
			break;
	}
	return repLen;
}

/************************************************************************//**
 * \brief Resets cartridge and starts timer to wait for chip ready.
 ****************************************************************************/
void SfCartInit(void) {
	// Hold reset during at least 500 ns (4 cycles@8MHz)
	CIF_CLR__RST;
	_NOP();_NOP();_NOP();_NOP();
	// Launch 1 ms timer to wait until chip is ready to accept commands
	Timer1Config(TimerMsToCount(1));
	Timer1Start();
	si.s = SF_CART_INIT;
	// Remove reset condition from flash chip
	CIF_SET__RST;
	// Wait 500 ns
	_NOP();_NOP();_NOP();_NOP();
	// Initialize UART
	UartInit();
}

/************************************************************************//**
 * \brief Puts cart pins at their default (idle bus) state.
 ****************************************************************************/
void SfCartRemove(void) {
	CIF_CLR__RST;
	CIF_SET__TIME;
	FlashIdle();
	si.s = SF_IDLE;
}

/************************************************************************//**
 * \brief Takes an incoming event and executes a cycle of the system FSM
 *
 * \param[in] evt Incoming event to be processed.
 *
 * \note Lots of states have been removed, might be needed if problems arise
 * because USB_USBTask() needs to be serviced more often than it is with
 * the current implementation.
 ****************************************************************************/
void SfFsmCycle(uint8_t evt) {
	// Holds reply length
	uint16_t repLen;

	// Process prioritary events (e.g. cart in/out) and events that can
	// generate more events (like data reception from host).
	// TODO: might be better removing cart events, and checking cart
	// status here.
	switch (evt) {
		case SF_EVT_TIMER:
			if (si.s == SF_STAB_WAIT) {
				LEDs_TurnOffLEDs(LEDS_LED2);
				// Check if cart is finally inserted and USB ready
				if (si.f.cart_in && si.f.usb_ready) SfCartInit();
				else SfCartRemove();
			} else if (si.s == SF_CART_INIT) {
				// Reset finished, cart should be ready to accept commands.
				// Obtain IDs.
				si.fc.manId = FlashGetManId();
				FlashGetDevId(si.fc.devId);
				// Finally jump to ready state
				si.s = SF_READY;
			} else if (SF_WIFI_MOD  == si.s) {
				// TODO: Call espcomm FSM or remove this block?
			}
			break;
		case SF_EVT_CIN:			// Cartridge inserted
			si.f.cart_in = TRUE;
			if (si.s == SF_IDLE) {
				si.s = SF_STAB_WAIT;
				// Launch 1 s debounce timer
				Timer1Config(TimerMsToCount(1000));
				Timer1Start();
				LEDs_TurnOnLEDs(LEDS_LED2);
			}
			break;
		case SF_EVT_COUT:			// Cartridge removed
			si.f.cart_in = FALSE;
			if (si.s != SF_STAB_WAIT) {
				// Remove cart and return to IDLE state
				SfCartRemove();
			}
			break;
		case SF_EVT_USB_ATT:		// USB attached and enumerated
			si.f.usb_ready = TRUE;
			// Check if cart is inserted and we are IDLE.
			if (si.f.cart_in && si.s == SF_IDLE) SfCartInit();
			break;
		case SF_EVT_USB_DET:		// USB detached
		case SF_EVT_USB_ERR:		// Error on USB interface
			si.f.usb_ready = FALSE; break;
			// Abort memory operations (if any) and go back to idle
			//SfMemAbort();
			SfCartRemove();
			break;
		case SF_EVT_DIN:
			// Get data from USB endpoint
			SfDataRecv(buf);
			// If status == SF_READY, parse command. Else reply with error.
			// There is an exception with the bootloader command, that must
			// be always honored
			if ((si.s == SF_READY) || (MDMA_CMD(buf) == MDMA_BOOTLOADER)) {
				repLen = SfCmdProc(buf);
			} else {
				buf[0] = MDMA_ERR;
				repLen = 1;
			}
			if (repLen) SfDataSend(buf, repLen);
			break;
		case SF_EVT_SW_PRESS:		// Button pressed event
			si.sw = SF_SW_EVENT | SF_SW_PRESSED;
			break;
		case SF_EVT_SW_REL:			// Button released event
			si.sw = SF_SW_EVENT;
			break;
	}
}

