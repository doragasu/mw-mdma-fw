/************************************************************************//**
 * \file
 * \brief SLIP framing handling.
 *
 * \author Jesús Alonso (doragasu)
 * \date   2016
 ****************************************************************************/

#include "slip.h"
#include "16c550.h"
#include "util.h"
#include <string.h>

/** \addtogroup slip SlipStat States for handling the SLIP transmissions.
 *  \{ */
typedef enum {
	SLIP_ST_SOF = 0,	///< Start of frame.
	SLIP_ST_DATA,		///< Data stage.
	SLIP_ST_SOF_ESC,	///< Escaping SOF symbol.
	SLIP_ST_ESC_ESC,	///< Escaping ESC symbol.
	SLIP_ST_EOF,		///< EOF.
	SLIP_ST_DONE,		///< Transfer complete.
	SLIP_ST_NUM			///< Number of states.
} SlipStat;
/** \} */

/** \addtogroup slip SlipBuf Payload of the SLIP frame.
 *  \{ */
typedef struct {
	uint16_t length;	///< Payload length.
	uint16_t pos;		///< Data reception position.
	uint8_t *data;		///< Pointer to data buffer.
} SlipBuf;
/** \} */

/** \addtogroup slip SlipData Static data of a SLIP instance.
 *  \{ */
typedef struct {
	SlipBuf txb;		///< Transmission buffer.
	SlipBuf rxb;		///< Reception buffer.
	SlipStat txs;		///< Transmission state.
	SlipStat rxs;		///< Reception state.
	uint8_t sendEof;	///< If TRUE, EOF will be sent to end frame.
} SlipData;
/** \} */

/// Module static data
static SlipData d;

/************************************************************************//**
 * \brief Module initialization. Must be called once before using any other
 *        function in this module.
 *
 * \note  This function does not initialize UART, because UART must be
 *        initialized each time a cart is attached or reset.
 ****************************************************************************/
void SlipInit(void) {
	memset(&d, 0, sizeof(d));
}

/************************************************************************//**
 * \brief Continues the transmission of a data frame using SLIP protocol.
 *
 * \param[in] toutCount Maximum loop count until timeout condition.
 *
 * \return The number of bytes sent. Should equal len, unless a timeout
 *         condition has occurred. If timeout occurs, transmission can be
 *         continued with SllpFrameSendCont(), or restarted by calling this
 *         function again.
 ****************************************************************************/
uint16_t SlipFrameSendCont(uint16_t toutCount) {
	// Number of UART check loops until a timeout condition occurs
	uint16_t loopCount;
	// Number of characters available in FIFO until filled
	uint8_t fifoRoom;
	// Character to be sent
	uint8_t c;

	while (TRUE) {
		// Wait until FIFO empty or timeout
		for (loopCount = toutCount; !UartTxFifoEmpty() && loopCount;
				loopCount--);
		if (!loopCount) return d.txb.pos;

		// TX up to UART_FIFO_LENGTH characters (to fill FIFO)
		fifoRoom = UART_FIFO_LENGTH;
		while (fifoRoom--) {
			// Send data depending on status
			switch (d.txs) {
				case SLIP_ST_SOF:		// TX SOF character
					c = SLIP_SOF;
					d.txs = SLIP_ST_DATA;
					break;

				case SLIP_ST_DATA:		// TX data payload
					// If we want to TX SOR or ESC characters, escape them
					if (SLIP_SOF == d.txb.data[d.txb.pos]) {
						c = SLIP_ESC;
						d.txs = SLIP_ST_SOF_ESC;
					} else if (SLIP_ESC == d.txb.data[d.txb.pos]) {
						c = SLIP_ESC;
						d.txs = SLIP_ST_ESC_ESC;
					} else {
						// TX character and check if we completed the frame
						c = d.txb.data[d.txb.pos++];
						if (d.txb.pos >= d.txb.length)
							// If not a split frame, send EOF
							d.txs = d.sendEof?SLIP_ST_EOF:SLIP_ST_DONE;
					}
					break;

				case SLIP_ST_SOF_ESC:	// TX escaped SOF character
					c = SLIP_SOF_ESC;
					d.txb.pos++;
					if (d.txb.pos >= d.txb.length)
						d.txs = d.sendEof?SLIP_ST_EOF:SLIP_ST_DONE;
					else d.txs = SLIP_ST_DATA;
					break;
			
				case SLIP_ST_ESC_ESC:	// TX escaped ESC character
					c = SLIP_ESC_ESC;
					d.txb.pos++;
					if (d.txb.pos >= d.txb.length)
						d.txs = d.sendEof?SLIP_ST_EOF:SLIP_ST_DONE;
					else d.txs = SLIP_ST_DATA;
					break;

				case SLIP_ST_EOF:		// TX EOF
					c = SLIP_SOF;
					d.txs = SLIP_ST_DONE;
					break;

				case SLIP_ST_DONE:		// TX complete.
					return d.txb.pos;

				default:
					// We should never reach here!
					return d.txb.pos;
			} // switch (d.txs)
			UartPutchar(c);				// Send the prepared character
		} // while (fifoRoom--)
	} // while (TRUE)
}



/************************************************************************//**
 * \brief Sends the SOF character, marking the start of a split frame send.
 *
 * \param[in] toutCount Maximum loop count until timeout condition.
 *
 * \return 0 if SOF was successfully sent, 1 otherwise.
 ****************************************************************************/
uint8_t SlipSplitFrameSendSof(uint16_t toutCount) {
	while(!UartTxFifoEmpty() && toutCount--);
	if (!toutCount) return 1;
	UartPutchar(SLIP_SOF);
	return 0;
}

/************************************************************************//**
 * \brief Sends data through the data link, using SLIP protocol. When using
 *        this function, SOF and EOF characters are not automatically sent,
 *        so SlipSplitFrameSendSof() must be called before calling this
 *        function. Then this function can be called N times until all the
 *        data in the frame is sent. Finally EOF must be sent by calling
 *        SlipSplitFrameSendEof().
 *
 * \param[in] data      Buffer with the data to send.
 * \param[in] len       Number of bytes to send.
 * \param[in] toutCount Maximum loop count until timeout condition.
 *
 * \return The number of bytes sent. Should equal len, unless a timeout
 *         condition has occurred. If timeout occurs, transmission can be
 *         continued with SllpFrameSendCont(), or restarted by calling this
 *         function again.
 ****************************************************************************/
uint16_t SlipSplitFrameAppendPoll(uint8_t *data, uint16_t len,
		uint16_t toutCount) {
	d.txb.data = data;
	d.txb.length = len;
	d.txb.pos = 0;
	d.sendEof = FALSE;
	d.txs = SLIP_ST_DATA;

	// Receive the new frame
	return SlipFrameSendCont(toutCount);
}

/************************************************************************//**
 * \brief Sends a data frame using SLIP protocol.
 *
 * \param[in] data      Buffer with the data to send.
 * \param[in] len       Number of bytes to send.
 * \param[in] toutCount Maximum loop count until timeout condition.
 *
 * \return The number of bytes sent. Should equal len, unless a timeout
 *         condition has occurred. If timeout occurs, transmission can be
 *         continued with SllpFrameSendCont(), or restarted by calling this
 *         function again.
 * \warning If there was a half sent frame, calling this function aborts the
 *         half sent frame and starts sending the new one.
 ****************************************************************************/
uint16_t SlipFrameSendPoll(uint8_t *data, uint16_t len, uint16_t toutCount) {
	// Prepare to send frame and don't look back (if there was a half sent
	// frame, it will be lost).
	d.txb.data = data;
	d.txb.length = len;
	d.txb.pos = 0;
	d.txs = SLIP_ST_SOF;
	d.sendEof = TRUE;

	// Receive the new frame
	return SlipFrameSendCont(toutCount);
}

/************************************************************************//**
 * \brief Continues receiving a data frame using SLIP protocol.
 *
 * \param[out] length   Length of the received frame.
 * \param[in] toutCount Maximum loop count until timeout condition.
 *
 * \return 0 if a complete frame was received, 1 if a timeout occurred,
 *         2 if reception was aborted because buffer was filled before
 *         receiving the EOF, or greater if other reception error occurred.
 ****************************************************************************/
uint16_t SlipFrameRecvCont(uint16_t *length, uint16_t toutCount) {
	// Number of UART check loops until a timeout condition occurs
	uint16_t loopCount;
	// Character to be sent
	uint8_t c;

	while (TRUE) {
		// Wait until there is data on the UART or timeout
		for (loopCount = toutCount; !UartRxFifoData() && loopCount;
				loopCount--);
		if (!loopCount) return 1;
		// Receive data depending on state
		c = UartGetchar();
		switch (d.rxs) {
			case SLIP_ST_SOF:			// Wait until SOF received
				// Silently discard data until SOF received
				if (SLIP_SOF == c) d.rxs = SLIP_ST_DATA;
				break;

			case SLIP_ST_DATA:			// Receive data payload
				// Check special case: if we receive an EOF, and pos == 0,
				// it means that this is indeed a SOF, and previous SOF
				// was instead an EOF. Otherwise, it's an EOF.
				if (SLIP_SOF == c && d.rxb.pos) {
					*length = d.rxb.pos;
					return 0;
				}
				// Check for ESC character
				else if (SLIP_ESC == c) d.rxs = SLIP_ST_ESC_ESC;
				else if (SLIP_SOF != c) {
					// Check if buffer is full
					if (d.rxb.pos >= d.rxb.length) {
						*length = d.rxb.pos;
						return 2;
					}
					// There's room in the buffer, copy character
					d.rxb.data[d.rxb.pos++] = c;
				}
				break;

			case SLIP_ST_ESC_ESC:		// Receive escaped character
				if (SLIP_SOF_ESC == c) c = SLIP_SOF;
				else if (SLIP_ST_ESC_ESC == c) c = SLIP_ESC;
				else {
					// An error has occurred, an escape character should
					// be followed by SOF or ESC escape codes only.
					*length = d.rxb.pos;
					return 3;
				}
				// Check if buffer is full
				if (d.rxb.pos >= d.rxb.length) {
					*length = d.rxb.pos;
					return 2;
				}
				// There's room in the buffer, copy character
				d.rxb.data[d.rxb.pos++] = c;
				break;

			default:
				// Should never reach here!
				*length = d.rxb.pos;
				return 3;
		}
	} // while (loopCount)
}

/************************************************************************//**
 * \brief Receives a data frame using SLIP protocol.
 *
 * \param[in] data      Buffer that will hold the received frame.
 * \param[in] max       Lenght of data buffer (maximum bytes to receive).
 * \param[out] length   Length of the received frame.
 * \param[in] toutCount Maximum loop count until timeout condition.
 *
 * \return 0 if a complete frame was received, 1 if a timeout occurred,
 *         2 if reception was aborted because buffer was filled before
 *         receiving the EOF, or greater if other reception error occurred.
 ****************************************************************************/
uint16_t SlipFrameRecvPoll(uint8_t *data, uint16_t max, uint16_t *length,
		uint16_t toutCount) {

	// Prepare to receive frame and don't look back (if there was a half
	// received frame, it will be lost).
	d.rxb.data = data;
	d.rxb.length = max;
	d.rxb.pos = 0;
	d.rxs = SLIP_ST_SOF;

	return SlipFrameRecvCont(length, toutCount);
}

