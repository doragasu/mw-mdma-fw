/************************************************************************//**
 * \file
 * \brief SLIP framing handling.
 *
 * \author Jesús Alonso (doragasu)
 * \date   2016
 * \defgroup slip SLIP framing handling.
 * \{
 ****************************************************************************/

#ifndef _SLIP_H_
#define _SLIP_H_

#include <stdint.h>

/** \addtogroup slip SlipChars SLIP special characters.
 *  \{ */
/// Start of frame (SOF)
#define SLIP_SOF		0xC0
/// Escape character (ESC)
#define SLIP_ESC		0xDB
/// Code for escaped SOF
#define SLIP_SOF_ESC	0xDC
/// Code for escaped ESC
#define SLIP_ESC_ESC	0xDD
/** \} */

/************************************************************************//**
 * \brief Module initialization. Must be called once before using any other
 *        function in this module.
 *
 * \note  This function does not initialize UART, because UART must be
 *        initialized each time a cart is attached or reset.
 ****************************************************************************/
void SlipInit(void);

/************************************************************************//**
 * \brief Sends a data frame using SLIP protocol.
 *
 * \param[in] data      Buffer with the data to send.
 * \param[in] len       Number of bytes to send.
 * \param[in] toutCount Maximum loop count without being able to send a
 *                      character.
 *
 * \return The number of bytes sent. Should equal len, unless a timeout
 *         condition has occurred. If timeout occurs, transmission can be
 *         continued with SllpFrameSendCont(), or restarted by calling this
 *         function again.
 * \warning If there was a half sent frame, calling this function aborts the
 *         half sent frame and starts sending the new one.
 ****************************************************************************/
uint16_t SlipFrameSendPoll(uint8_t *data, uint16_t len, uint16_t toutCount);

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
uint16_t SlipFrameSendCont(uint16_t toutCount);

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
		uint16_t toutCount);

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
uint16_t SlipFrameRecvCont(uint16_t *length, uint16_t toutCount);

/************************************************************************//**
 * \brief Sends the SOF character, marking the start of a split frame send.
 *
 * \param[in] toutCount Maximum loop count until timeout condition.
 *
 * \return 0 if SOF was successfully sent, 1 otherwise.
 ****************************************************************************/
uint8_t SlipSplitFrameSendSof(uint16_t toutCount);

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
		uint16_t toutCount);

/************************************************************************//**
 * \brief Sends the EOF character, marking the end of a split frame send.
 *
 * \param[in] toutCount Maximum loop count until timeout condition.
 *
 * \return 0 if EOF was successfully sent, 1 otherwise.
 ****************************************************************************/
#define SlipSplitFrameSendEof(toutCount) SlipSplitFrameSendSof(toutCount)

#endif /*_SLIP_H_*/

/** \} */

