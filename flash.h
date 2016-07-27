/************************************************************************//**
 * \file
 * \brief This module allows to manage (mainly read and write) from flash
 * memory chips such as S29GL032.
 *
 * \author Jesús Alonso (doragasu)
 * \date   2015
 * \defgroup flash Flash chip management.
 * \{
 ****************************************************************************/

#include <stdint.h>
#include <avr/io.h>
#include <avr/cpufunc.h>
#include "cart_if.h"

/** \addtogroup flash FlashCmdData Data used to perform different flash commands.
 * This data depends on the flash chip and on the mode (x8/x16) used.
 * \{
 */
typedef struct {
	uint16_t addr;	///< Flash address
	uint8_t data;	///< Flash data
} FlashCmd;
/** \} */


/* 
 * Command definitions. NOTE: Commands use only 12-bit addresses. higher bits
 * are don't care.
 */

/// Number of write cycles to reset Flash interface
#define FLASH_RESET_CYC 1
/// Reset command data
const static FlashCmd FLASH_RESET[FLASH_RESET_CYC] = {
	{0x555, 0x00F0}
};

/// Number of cycles to unlock
#define FLASH_UNLOCK_CYC 2
/// Unlock command addresses and data
const static FlashCmd FLASH_UNLOCK[FLASH_UNLOCK_CYC] = {
	{0x555, 0xAA}, {0x2AA, 0x55},
};

/*
 * Autoselect commands. Cycles beyong the third, are read ones, and must
 * present a valid address according to part datasheet. Before the autoselect
 * command, an unlock command must be issued.
 */
/// Number of cycles of the autoselect command
#define FLASH_AUTOSEL_CYC 1
/// Autosel command addresses and data
const static FlashCmd FLASH_AUTOSEL[FLASH_AUTOSEL_CYC] = {
	{0x555, 0x90},
};

/// Number of cycles of the manufacturer ID request command.
#define FLASH_MANID_CYC 1
/// Manufacturer ID request data. Read must be prefixed by FLASH_AUTOSEL.
const static uint16_t FLASH_MANID_RD[FLASH_MANID_CYC] = {
	0x000
};

/// Number of cycles of the device ID request command.
#define FLASH_DEVID_CYC 3
/// Device ID request data. Read must be prefixed by FLASH_AUTOSEL.
const static uint16_t FLASH_DEVID_RD[FLASH_DEVID_CYC] = {
	0x001, 0x00E, 0x00F
};

/// Number of cycles of the program command.
#define FLASH_PROG_CYC 1
/// Program. Must be prefixed by FLASH_UNLOCK, and followed by a write cycle.
const static FlashCmd FLASH_PROG[FLASH_PROG_CYC] = {
	{0x555, 0xA0}
};

/// Number of cycles of the write to buffer command.
#define FLASH_WR_BUF_CYC 1
/// Write to buffer. Must be prefixed with FLASH_UNLOCK, and followed with
/// buffer write sequence. On this cycle, address must be SA (see datasheet).
const static uint8_t FLASH_WR_BUF[FLASH_WR_BUF_CYC] = {0x25};

/// Number of cycles of the program buffer to flash command.
#define FLASH_PRG_BUF_CYC	1
/// Program buffer to flash data.
/// \note Address must be SA (see datasheet), but data is fixed.
const static uint8_t FLASH_PRG_BUF[FLASH_PRG_BUF_CYC] = {0x29};

/// Number of cycles of the unlock bypass command.
#define FLASH_UL_BYP_CYC 1
/// Unlock bypass command data. Must be prefixed with FLASH_UNLOCK.
const static FlashCmd FLASH_UL_BYP[FLASH_UL_BYP_CYC] = {
	{0x555, 0x20}
};

/// Number of cycles of the unlock bypass program command.
#define FLASH_UL_BYP_PROG_CYC 1
/// Unlock bypass program data. \note address is don't care. Must be followed
/// by a write cycle.
const static uint8_t FLASH_UL_BYP_PROG[FLASH_UL_BYP_PROG_CYC] = {0xA0};

/// Number of cycles of the unlock bypass reset command.
#define FLASH_UL_BYP_RST_CYC 2
/// Unlock bypass reset data. \note addresses are don't care
const static uint8_t FLASH_UL_BYP_RST[FLASH_UL_BYP_RST_CYC] = {0x90, 0x00};

/// Number of cycles of the chip erase command.
#define FLASH_CHIP_ERASE_CYC 4
/// Chip erase data. Must be prefixed with FLASH_UNLOCK.
const static FlashCmd FLASH_CHIP_ERASE[FLASH_CHIP_ERASE_CYC] = {
	{0x555, 0x80}, {0x555, 0xAA}, {0x2AA, 0x55}, {0x555, 0x10}
};

/// Number of cycles of the sector erase command.
#define FLASH_SEC_ERASE_CYC 3
/// Sector erase. Must be prefixed with FLASH_UNLOCK. Address on last cycle
/// must be SA (see datasheet).
const static FlashCmd FLASH_SEC_ERASE[FLASH_SEC_ERASE_CYC] = {
	{0x555, 0x80}, {0x555, 0xAA}, {0x2AA, 0x55}
};

/// Data to be written along with sector address after FLASH_SEC_ERASE
const static uint8_t FLASH_SEC_ERASE_WR[1] = {0x30};

/*
 * Public functions
 */

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************//**
 * \brief Module initialization. Configures the 68k bus.
 ****************************************************************************/
void FlashInit(void);

/************************************************************************//**
 * \brief Set flash ports to default (idle) values.
 ****************************************************************************/
void FlashIdle(void);

/************************************************************************//**
 * \brief Writes a word to specified address.
 *
 * \param[in] addr Address to which data will be written.
 * \param[in] data Data to write to addr address.
 *
 * \note Do not mistake this function with the program ones.
 ****************************************************************************/
inline void FlashWrite(uint32_t addr, uint16_t data) {
	// Put address on the bus
	CIF_ADDRL_PORT = addr;
	CIF_ADDRH_PORT = addr>>8;
	CIF_ADDRU_PORT = (CIF_ADDRU_PORT & (~CIF_ADDRU_MASK)) |
		((addr>>16) & CIF_ADDRU_MASK);
	// Write data to bus
	CIF_DATAL_PORT = data;
	CIF_DATAH_PORT = data>>8;
	CIF_DATAH_DDR = CIF_DATAL_DDR = 0xFF;
	// Select chip
	CIF_CLR__CE;
	// Signal _W
	CIF_CLR__W;

	// Disable _W
	CIF_SET__W;
	// Remove data from bus
	CIF_DATAH_DDR  = CIF_DATAL_DDR  = 0;
	CIF_DATAH_PORT = CIF_DATAL_PORT = 0xFF;
	// Deselect chip
	CIF_SET__CE;
}

/************************************************************************//**
 * \brief Reads a word from the specified address.
 *
 * \param[in] addr Address that will be read.
 *
 * \return Readed word.
 ****************************************************************************/
inline uint16_t FlashRead(uint32_t addr) {
	uint16_t data;

	// Put address on the bus
	CIF_ADDRL_PORT = addr;
	CIF_ADDRH_PORT = addr>>8;
	CIF_ADDRU_PORT = (CIF_ADDRU_PORT & (~CIF_ADDRU_MASK)) |
		((addr>>16) & CIF_ADDRU_MASK);
	//CIF_DATAH_PORT = CIF_DATAL_PORT = 0x00;
	// Enable chip outputs
	CIF_CLR__OE;
	// Select chip
	CIF_CLR__CE;
	// Read data
	_NOP();		// Insert NOPs to ensure the input sinchronizer gets the data
	_NOP();
	//while (PINB & 0x80);	// For debugging reads
	data = (((uint16_t)CIF_DATAH_PIN)<<8) | CIF_DATAL_PIN;
	//CIF_DATAH_PORT = CIF_DATAL_PORT = 0xFF;
	// Deselect chip
	CIF_SET__CE;
	// Disable chip outputs
	CIF_SET__OE;
	
	return data;
}

/************************************************************************//**
 * \brief Writes a command to the flash chip.
 * 
 * \param[in] The command data structure to write to the flash chip.
 ****************************************************************************/
static inline void FlashWriteCmd(FlashCmd cmd) {
	FlashWrite(cmd.addr, cmd.data);
}

// Helper macro for writing commands. Takes the command and an auxiliar
// variable used as an iterator.
#define FLASH_WRITE_CMD(cmd, iterator)	do {					\
	for (iterator = 0; iterator < cmd ## _CYC; iterator++) {	\
		FlashWrite(cmd[iterator].addr, cmd[iterator].data);		\
	}															\
} while (0);

/************************************************************************//**
 * \brief Writes the flash unlock command to the flash chip. This command
 * must be used as part of other larger commands.
 ****************************************************************************/
static inline void FlashUnlock(void) {
	uint8_t i;
	
	FLASH_WRITE_CMD(FLASH_UNLOCK, i);
}

/************************************************************************//**
 * \brief Writes the flash autoselect command to the flash chip. This command
 * must be used as part of other larger commands.
 ****************************************************************************/
static inline void FlashAutoselect(void) {
	uint8_t i;

	FlashUnlock();
	FLASH_WRITE_CMD(FLASH_AUTOSEL, i);
}

/************************************************************************//**
 * \brief Sends the reset command, to return to array read mode.
 ****************************************************************************/
static inline void FlashReset(void) {
	uint8_t i;
	FLASH_WRITE_CMD(FLASH_RESET, i);
}

/************************************************************************//**
 * \brief Writes the manufacturer ID query command to the flash chip.
 *
 * \return The manufacturer ID code.
 ****************************************************************************/
uint16_t FlashGetManId(void);

/************************************************************************//**
 * \brief Writes the device ID query command to the flash chip.
 *
 * \param[out] The device ID code, consisting of 3 words.
 ****************************************************************************/
void FlashGetDevId(uint16_t devId[3]);

/************************************************************************//**
 * \brief Programs a word to the specified address.
 *
 * \param[in] addr The address to which data will be programmed.
 * \param[in] data Data to program to the specified address.
 *
 * \warning Doesn't poll until programming is complete
 ****************************************************************************/
void FlashProg(uint32_t addr, uint16_t data);

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
uint8_t FlashWriteBuf(uint32_t addr, uint16_t data[], uint8_t wLen);

/************************************************************************//**
 * Enables the "Unlock Bypass" status, allowing to issue several commands
 * (like the Unlock Bypass Programm) using less write cycles.
 ****************************************************************************/
void FlashUnlockBypass(void);

// Warning: must be issued after a FlashUnlockBypass command
// Warning: doesn't poll until programming is complete
static inline void FlashUnlockProgram(uint32_t addr, uint8_t data) {
	// Write unlock bypass program command
	FlashWrite(addr, FLASH_UL_BYP_PROG[0]);
	// Write data
	FlashWrite(addr, data);
}


/************************************************************************//**
 * Ends the "Unlock Bypass" state, returning to default read mode.
 ****************************************************************************/
void FlashUnlockBypassReset(void);

/************************************************************************//**
 * Erases the complete flash chip.
 *
 * \return '0' the if erase operation completed successfully, '1' otherwise.
 ****************************************************************************/
uint8_t FlashChipErase(void);

/************************************************************************//**
 * Erases a complete flash sector, specified by addr parameter.
 *
 * \param[in] addr Address contained in the sector that will be erased.
 * \return '0' if the erase operation completed successfully, '1' otherwise.
 ****************************************************************************/
uint8_t FlashSectErase(uint32_t addr);

/************************************************************************//**
 * \brief Polls flash chip after a program operation, and returns when the
 * program operation ends, or when there is an error.
 *
 * \param[in] addr Address to which data has been written.
 * \param[in] data Data written to addr address.
 * \return 0 if OK, 1 if error during program operation.
 ****************************************************************************/
uint8_t FlashDataPoll(uint32_t addr, uint16_t data);

/************************************************************************//**
 * \brief Polls flash chip after an erase operation, and returns when the
 * program operation ends, or when there is an error.
 *
 * \param[in] addr Address contained in the erased zone.
 * \return 0 if OK, 1 if error during program operation.
 ****************************************************************************/
uint8_t FlashErasePoll(uint32_t addr);

#ifdef __cplusplus
}
#endif

/** \} */
