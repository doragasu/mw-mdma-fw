/************************************************************************//**
 * \file
 * \brief This module allows to manage (mainly read and write) from flash
 * memory chips such as S29GL032.
 *
 * \author Jesús Alonso (doragasu)
 * \date   2015
 ****************************************************************************/

#include "flash.h"
#include "util.h"
#include <LUFA/Drivers/Board/LEDs.h>

/// Obtains a sector address from an address. Current chip does not actually
//require any calculations to get SA.
/// \TODO Support several flash chips
#define FLASH_SA_GET(addr)		(addr)

/************************************************************************//**
 * \brief Polls flash chip after a program operation, and returns when the
 * program operation ends, or when there is an error.
 *
 * \param[in] addr Address to which data has been written.
 * \param[in] data Data written to addr address.
 * \return 0 if OK, 1 if error during program operation.
 ****************************************************************************/
uint8_t FlashDataPoll(uint32_t addr, uint16_t data) {
	uint16_t read;

	// Poll while DQ7 != data(7) and DQ5 == 0 and DQ1 == 0
	do {
		read = FlashRead(addr);
	} while (((data ^ read) & 0x80) && ((read & 0x22) == 0));

	// DQ7 must be tested after another read, according to datasheet
//	if (((data ^ read) & 0x80) == 0) return 0;
	read = FlashRead(addr);
	if (((data ^ read) & 0x80) == 0) return 0;
	// Data has not been programmed, return with error. If DQ5 is set, also
	// issue a reset command to return to array read mode
	if (read & 0x20) {
		FlashReset();
	}
	// If DQ1 is set, issue the write-to-buffer-abort-reset command.
	if (read & 0x02) {
		FlashUnlock();
		FlashReset();
	}
	return 1;
}

/************************************************************************//**
 * \brief Polls flash chip after an erase operation, and returns when the
 * program operation ends, or when there is an error.
 *
 * \param[in] addr Address contained in the erased zone.
 * \return 1 if OK, 0 if error during program operation.
 ****************************************************************************/
uint8_t FlashErasePoll(uint32_t addr) {
	uint16_t read;

	// Wait until DQ7 or DQ5 are set
	do {
		read = FlashRead(addr);
	} while (!(read & 0xA0));


	// If DQ5 is set, an error has occurred. Also a reset command needs to
	// be sent to return to array read mode.
	if (!(read & 0x80)) return 0;

	FlashReset();
	return 1;
	//return (read & 0x80) != 0;
}

/**
 * Public functions
 */

/************************************************************************//**
 * \brief Module initialization. Configures the 68k bus.
 ****************************************************************************/
void FlashInit(void){
}


/************************************************************************//**
 * \brief Set flash ports to default (idle) values.
 ****************************************************************************/
void FlashIdle(void) {
	// Control signals
	CIF_SET__W;
	CIF_SET__OE;
	CIF_SET__CE;
	// Data ports (input with pullups enabled)
	CIF_DATAH_DDR  = CIF_DATAL_DDR = 0;
	CIF_DATAH_PORT = CIF_DATAL_PORT = 0xFF;
	// Addresses
	CIF_ADDRH_PORT  = CIF_ADDRL_PORT = 0xFF;
	CIF_ADDRU_PORT |= CIF_ADDRU_MASK;
}

/************************************************************************//**
 * \brief Writes the manufacturer ID query command to the flash chip.
 *
 * \return The manufacturer ID code.
 ****************************************************************************/
uint16_t FlashGetManId(void) {
	uint16_t retVal;

	// Obtain manufacturer ID and reset interface to return to array read.
	FlashAutoselect();
	retVal = FlashRead(FLASH_MANID_RD[0]);
	FlashReset();

	return retVal;
}

/************************************************************************//**
 * \brief Writes the device ID query command to the flash chip.
 *
 * \param[out] The device ID code, consisting of 3 words.
 ****************************************************************************/
void FlashGetDevId(uint16_t devId[3]) {
	// Obtain device ID and reset interface to return to array read.
	FlashAutoselect();
	devId[0] = FlashRead(FLASH_DEVID_RD[0]);
	devId[1] = FlashRead(FLASH_DEVID_RD[1]);
	devId[2] = FlashRead(FLASH_DEVID_RD[2]);
	FlashReset();
}

/************************************************************************//**
 * \brief Programs a word to the specified address.
 *
 * \param[in] addr The address to which data will be programmed.
 * \param[in] data Data to program to the specified address.
 *
 * \warning Doesn't poll until programming is complete
 ****************************************************************************/
void FlashProg(uint32_t addr, uint16_t data) {
	uint8_t i;

	// Unlock and write program command
	FlashUnlock();
	FLASH_WRITE_CMD(FLASH_PROG, i);
	// Write data
	FlashWrite(addr, data);
}

/************************************************************************//**
 * \brief Programs a buffer to the specified address range.
 *
 * \param[in] addr The address of the first word to be written
 * \param[in] data The data array to program to the specified address range.
 * \param[in] wLen The number of words to program, contained on data.
 * \return The number of words successfully programed.
 *
 * \note wLen must be less or equal than 16.
 * \note If addr-wLen defined range crosses a write-buffer boundary, all the
 *       requested words will not be written. To avoid this situation, it
 *       is advisable to write to addresses having the lower 4 bits (A1~A5)
 *       equal to 0.
 ****************************************************************************/
uint8_t FlashWriteBuf(uint32_t addr, uint16_t data[], uint8_t wLen) {
	// Sector address
	uint32_t sa;
	// Number of words to write
	uint8_t wc;
	// Index
	uint8_t i;

	// Check maximum write length
	if (wLen > 16) return 0;

	// Obtain the sector address
	sa = FLASH_SA_GET(addr);
	// Compute the number of words to write minus 1. Maximum number is 15,
	// but without crossing a write-buffer page
	wc = MAX(wLen, 16 - (addr & 0xF)) - 1;
	// Unlock and send Write to Buffer command
	FlashUnlock();
	FlashWrite(sa, FLASH_WR_BUF[0]);
	// Write word count - 1
	FlashWrite(sa, wc);

	// Write data to bufffer
	for (i = 0; i <= wc; i++, addr++) FlashWrite(addr, data[i]);
	// Write program buffer command
	FlashWrite(sa, FLASH_PRG_BUF[0]);
	// Poll until programming is complete
	if (FlashDataPoll(addr - 1, data[i - 1])) return 0;

	// Return number of elements (words) written
	return i;
}

/************************************************************************//**
 * Enables the "Unlock Bypass" status, allowing to issue several commands
 * (like the Unlock Bypass Programm) using less write cycles.
 ****************************************************************************/
void FlashUnlockBypass(void) {
	uint8_t i;

	FlashUnlock();
	FLASH_WRITE_CMD(FLASH_UL_BYP, i);
}

/************************************************************************//**
 * Ends the "Unlock Bypass" state, returning to default read mode.
 ****************************************************************************/
void FlashUnlockBypassReset(void) {
	// Write reset command. Addresses are don't care
	FlashWrite(0, FLASH_UL_BYP_RST[0]);
	FlashWrite(0, FLASH_UL_BYP_RST[1]);
}

/************************************************************************//**
 * Erases the complete flash chip.
 *
 * \return '0' the if erase operation completed successfully, '1' otherwise.
 ****************************************************************************/
uint8_t FlashChipErase(void) {
	uint8_t i;

	// Unlock and write chip erase sequence
	FlashUnlock();
	FLASH_WRITE_CMD(FLASH_CHIP_ERASE, i);
	// Poll until erase complete
	return FlashErasePoll(0);
}

/************************************************************************//**
 * Erases a complete flash sector, specified by addr parameter.
 *
 * \param[in] addr Address contained in the sector that will be erased.
 * \return '0' if the erase operation completed successfully, '1' otherwise.
 ****************************************************************************/
uint8_t FlashSectErase(uint32_t addr) {
	// Sector address
	uint32_t sa;
	// Index
	uint8_t i;

	// Obtain the sector address
	sa = FLASH_SA_GET(addr);
	// Unlock and write sector address erase sequence
	FlashUnlock();
	FLASH_WRITE_CMD(FLASH_SEC_ERASE, i);
	// Write sector address 
	FlashWrite(sa, FLASH_SEC_ERASE_WR[0]);
	// Wait until erase starts (polling DQ3)
	while (!(FlashRead(sa) & 0x08));
	// Poll until erase complete
	return FlashErasePoll(addr);
}

