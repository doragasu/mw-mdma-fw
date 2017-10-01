/************************************************************************//**
 * \file
 *
 * \author Jesús Alonso (doragasu)
 * \date   2015
 * \defgroup mdma-pr Definitions related to the MDMA protocol implementation.
 * \{
 ****************************************************************************/

#ifndef _MDMA_PR_H_
#define _MDMA_PR_H_

/** \addtogroup mdma-pr MdmaCmds Commands and replies supported by the
 *  platform.
 * \{
 */
#define MDMA_OK				0	///< Used to report OK status during replies
#define MDMA_MANID_GET		1	///< Flash chip manufacturer ID request.
#define MDMA_DEVID_GET		2	///< Flash chip device ID request.
#define MDMA_READ			3	///< Flash data read command.
#define MDMA_CART_ERASE		4	///< Cartridge Flash erase command.
#define MDMA_SECT_ERASE		5	///< Flash sector erase command.
#define MDMA_WRITE			6	///< Flash write (program) command.
#define MDMA_MAN_CTRL		7	///< Manual GPIO pin control command.
#define MDMA_BOOTLOADER		8	///< Puts board in bootloader mode.
#define MDMA_BUTTON_GET		9	///< Gets pushbutton status.
#define MDMA_WIFI_CMD	   10	///< Command forwarded to the WiFi chip.
#define MDMA_WIFI_CMD_LONG 11	///< Long command forwarded to the WiFi chip.
#define MDMA_WIFI_CTRL	   12	///< WiFi chip control action (using GPIO).
#define MDMA_RANGE_ERASE   13	///< Erase a memory range of the flash chip
#define MDMA_ERR		  255	///< Used to report ERROR during replies.
/** \} */

/// Obtains a double word (uint32_t) from the specified variable and offset.
/// This macro is not optimal, but will always work, even if the address is
/// misaligned. Order is little endian
#define MDMA_DWORD_AT(var, pos)		(((uint32_t)((var)[(pos)+3])<<24) | \
									 ((uint32_t)((var)[(pos)+2])<<16) | \
									 ((uint32_t)((var)[(pos)+1])<<8)  | \
									 (var)[pos])

/// Obtains a 24-bit value (stored as an uint32_t) from the specified
/// variale and offset.
#define MDMA_3BYTES_AT(var, pos)	(((uint32_t)((var)[(pos)+2])<<16) | \
								 	 ((uint32_t)((var)[(pos)+1])<<8)  | \
								 	 (var)[pos])


/// Obtains a word (uint16_t) from the specified variable and offset.
#define MDMA_WORD_AT(var, pos)		(((uint16_t)((var)[(pos)+1])<<8)  | \
									 (var)[pos])

/// Address offset in command request
#define MDMA_ADDR_OFF		3
/// Length offset in command request
#define MDMA_LENGTH_OFF		1
/// Data offset in command request
#define MDMA_DATA_OFF		6

/// Obtains command from a data frame
#define MDMA_CMD(data)		(data[0])
/// Obtains address from a flash read, write or sector erase command
#define MDMA_ADDR(data)			MDMA_3BYTES_AT(data, MDMA_ADDR_OFF)
/// Obtains length from a data read or write command
#define MDMA_LENGTH(data)		MDMA_WORD_AT(data, MDMA_LENGTH_OFF)

#define MDMA_SET_ADDR(data, addr)	do {							\
		(data)[MDMA_ADDR_OFF] = (addr) & 0xFF; 						\
		(data)[MDMA_ADDR_OFF+1] = ((addr)>>8) & 0xFF;				\
		(data)[MDMA_ADDR_OFF+2] = ((addr)>>16) & 0xFF;}while(0)

#define MDMA_SET_LENGTH(data, length)	do {						\
		(data)[MDMA_LENGTH_OFF] = (length) & 0xFF;					\
		(data)[MDMA_LENGTH_OFF+1] = ((length)>>8) & 0xFF;}while(0)	\

#endif /*_MDMA_PR_H_*/

/** \} */

