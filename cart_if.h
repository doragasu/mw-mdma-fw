/************************************************************************//**
 * \file
 * \brief Contains some definitions to use some special
 *  cart lines, not used by the Flash chip interface.
 *
 * \author Jesús Alonso (doragasu)
 * \date   2015
 * \defgroup cart_if Cartridge interface.
 * \{
 ****************************************************************************/

#ifndef _CIF_H_
#define _CIF_H_

#include <stdint.h>
#include <avr/io.h>
#include <avr/cpufunc.h>

/*
 * CUSTOMIZE THE FOLLOWING DEFINITIONS TO MATCH YOUR MICROCONTROLLER WIRING
 */
/** \addtogroup cart_if CifLines Pin letters and numbers used by each function.
 * Can be customized to match the microcontroller wiring.
 * \{
 */
/// Letter used for the address bus, upper (17~21) lines
#define CIF_ADDRU			E
/// Letter used for the address bus, high (9~16) lines lines
#define CIF_ADDRH			D
/// Letter used for the address bus, lower (1~8) lines
#define CIF_ADDRL			C
/// Mask used for the upper lines (because they do not use the complete port)
#define CIF_ADDRU_MASK	0x1F

/// Letter used for the active low write signal
#define CIF__W_LET		B
/// Pin number used for the active low write signal
#define CIF__W			0

/// Letter used for the active low assert signal
#define CIF__AS_LET		B
/// Pin number used for the active low assert signal
#define CIF__AS			2

/// Letter used for the active low output enable signal
#define CIF__OE_LET		E
/// Pin number used for the active low output enable signal
#define CIF__OE			6

/// Letter used for the active low chip enable signal
#define CIF__CE_LET		E
/// Pin number used for the active low chip enable signal
#define CIF__CE			7

/// Letter used for the data bus, high (8~15) lines.
#define CIF_DATAH			A
/// Letter used for the data bus, lower (0~7) lines.
#define CIF_DATAL			F

/// _TIME signal letter, used to select extra hardware (active low)
#define CIF__TIME_LET		B
/// _TIME signal number, used to select extra hardware (active low)
#define CIF__TIME			1
// Bus read assert letter (active low, not used by Flash chip)
#define CIF__AS_LET			B
// Bus read assert number (active low, not used by Flash chip)
#define CIF__AS				2
// Cart reset (active low) letter
#define CIF__RST_LET		B
// Cart reset (active low) number
#define CIF__RST			3
// Cartridge detect input (active low) letter
#define CIF__CIN_LET		B
// Cartridge detect input (active low) number
#define CIF__CIN			4

/*
 * DO NOT CUSTOMIZE ANYTHING BEYOND THIS POINT.
 */

/** \addtogroup cart_if BusCtrl Registers used for controlling the data buses.
 * \{
 */
// Helper macro to construct port registers, second level
#define CIF_REG2(letter, reg)	(reg ## letter)
/// Helper macro to construct port registers
#define CIF_REG(letter, reg) CIF_REG2(letter, reg)

/// Address port, upper bits
#define CIF_ADDRU_PORT	CIF_REG(CIF_ADDRU, PORT)
/// Address port, high bits
#define CIF_ADDRH_PORT	CIF_REG(CIF_ADDRH, PORT)
/// Address port, low bits
#define CIF_ADDRL_PORT	CIF_REG(CIF_ADDRL, PORT)

/// Address data directon register, upper bits
#define CIF_ADDRU_DDR		CIF_REG(CIF_ADDRU, DDR)
/// Address data directon register, high bits
#define CIF_ADDRH_DDR		CIF_REG(CIF_ADDRH, DDR)
/// Address data directon register, low bits
#define CIF_ADDRL_DDR		CIF_REG(CIF_ADDRL, DDR)

/// Address pin register, upper bits
#define CIF_ADDRU_PIN		CIF_REG(CIF_ADDRU, PIN)
/// Address pin register, high bits
#define CIF_ADDRH_PIN		CIF_REG(CIF_ADDRH, PIN)
/// Address pin register, low bits
#define CIF_ADDRL_PIN		CIF_REG(CIF_ADDRL, PIN)

/// Data port, high bits
#define CIF_DATAH_PORT	CIF_REG(CIF_DATAH, PORT)
/// Data port, low bits
#define CIF_DATAL_PORT	CIF_REG(CIF_DATAL, PORT)

/// Data data direction register, high bits
#define CIF_DATAH_DDR		CIF_REG(CIF_DATAH, DDR)
/// Data data direction register, low bits
#define CIF_DATAL_DDR		CIF_REG(CIF_DATAL, DDR)

/// Data pin register, high bits
#define CIF_DATAH_PIN		CIF_REG(CIF_DATAH, PIN)
/// Data pin register, low bits
#define CIF_DATAL_PIN		CIF_REG(CIF_DATAL, PIN)

/// Active low write signal port
#define CIF__W_PORT		CIF_REG(CIF__W_LET, PORT)
/// Active low assert signal port
#define CIF__AS_PORT		CIF_REG(CIF__AS_LET, PORT)
/// Active low output enable signal port
#define CIF__OE_PORT		CIF_REG(CIF__OE_LET, PORT)
/// Active low chip enable signal port
#define CIF__CE_PORT		CIF_REG(CIF__CE_LET, PORT)

/// Active low write signal data direction register
#define CIF__W_DDR		CIF_REG(CIF__W_LET, DDR)
/// Active low assert signal data direction register
#define CIF__AS_DDR		CIF_REG(CIF__AS_LET, DDR)
/// Active low output enable signal data direction register
#define CIF__OE_DDR		CIF_REG(CIF__OE_LET, DDR)
/// Active low chip enable signal data direction register
#define CIF__CE_DDR		CIF_REG(CIF__CE_LET, DDR)

/// Active low write signal pin register
#define CIF__W_PIN		CIF_REG(CIF__W_LET, PIN)
/// Active low assert signal pin register
#define CIF__AS_PIN		CIF_REG(CIF__AS_LET, PIN)
/// Active low output enable signal pin register
#define CIF__OE_PIN		CIF_REG(CIF__OE_LET, PIN)
/// Active low chip enable signal pin register
#define CIF__CE_PIN		CIF_REG(CIF__CE_LET, PIN)

/// _TIME PORT register
#define CIF__TIME_PORT			CIF_REG(CIF__TIME_LET, PORT)
/// _TIME DDR register
#define CIF__TIME_DDR			CIF_REG(CIF__TIME_LET, DDR)
/// _TIME PIN register
#define CIF__TIME_PIN			CIF_REG(CIF__TIME_LET, PIN)

/// _AS PORT register
#define CIF__AS_PORT			CIF_REG(CIF__AS_LET, PORT)
/// _AS DDR register
#define CIF__AS_DDR				CIF_REG(CIF__AS_LET, DDR)
/// _AS PIN register
#define CIF__AS_PIN				CIF_REG(CIF__AS_LET, PIN)

/// _RST PORT register
#define CIF__RST_PORT			CIF_REG(CIF__RST_LET, PORT)
/// _RST DDR register
#define CIF__RST_DDR			CIF_REG(CIF__RST_LET, DDR)
/// _RST PIN register
#define CIF__RST_PIN			CIF_REG(CIF__RST_LET, PIN)

/// _CIN PORT register
#define CIF__CIN_PORT			CIF_REG(CIF__CIN_LET, PORT)
/// _CIN DDR register
#define CIF__CIN_DDR			CIF_REG(CIF__CIN_LET, DDR)
/// _CIN PIn register
#define CIF__CIN_PIN			CIF_REG(CIF__CIN_LET, PIN)
/** \} */

/** \addtogroup cart_if BusSetClr Macros to set and clear bus lines.
 * \{
 */
/// Set (1) active low write signal.
#define CIF_SET__W		(CIF__W_PORT |=  (1<<CIF__W))
/// Clear (0) active low write signal.
#define CIF_CLR__W		(CIF__W_PORT &= ~(1<<CIF__W))
/// Set (1) active low output enable signal.
#define CIF_SET__OE		(CIF__OE_PORT |=  (1<<CIF__OE))
/// Clear (0) active low output enable signal.
#define CIF_CLR__OE		(CIF__OE_PORT &= ~(1<<CIF__OE))
/// Set (1) active low chip enable signal.
#define CIF_SET__CE		(CIF__CE_PORT |=  (1<<CIF__CE))
/// Clear (0) active low chip enable signal.
#define CIF_CLR__CE		(CIF__CE_PORT &= ~(1<<CIF__CE))

// The following macros can be used to set and clear _RST and _TIME,
// once the module has been initialised by calling CifInit().
/// Set (1) the active low reset signal.
#define CIF_SET__RST	do{CIF__RST_PORT |=  (1<<CIF__RST);}while(0)
/// Clear (0) the active low reset pin.
#define CIF_CLR__RST	do{CIF__RST_PORT &= ~(1<<CIF__RST);}while(0)

/// Set (1) the active low TIME signal.
#define CIF_SET__TIME	do{CIF__TIME_PORT |=  (1<<CIF__TIME);}while(0)
/// Clear (0) the active low TIME signal.
#define CIF_CLR__TIME	do{CIF__TIME_PORT &= ~(1<<CIF__TIME);}while(0)

/// Set (1) the active low AS signal.
#define CIF_SET__AS		do{CIF__AS_PORT |=  (1<<CIF__AS);}while(0)
/// Clear (0) the active low AS signal.
#define CIF_CLR__AS		do{CIF__AS_PORT &= ~(1<<CIF__AS);}while(0)

/// Returns TRUE if cart inserted, FALSE otherwise
#define CIF__CIN_GET	((CIF__CIN_PIN & (1<<CIF__CIN)) == 0)
/** \} */


/************************************************************************//**
 * \brier Initializes the module. Must be called before using any other
 * macro.
 ****************************************************************************/
static inline void CifInit(void) {
	// Ensure JTAG interface is disabled to allow using PF[4~7] GPIO pins
	MCUCR |= (1<<JTD);	// JTAG disable sequence requieres writing the
	MCUCR |= (1<<JTD);	// disable bit twice withing 4 clock cycles.
	
	// Configure control lines, and set outputs to inactive
	CIF__W_DDR  |= (1<<CIF__W);
	CIF__OE_DDR |= (1<<CIF__OE);
	CIF__CE_DDR |= (1<<CIF__CE);
	CIF__AS_DDR |= (1<<CIF__AS);
	CIF_SET__W;
	CIF_SET__OE;
	CIF_SET__CE;
	
	// Configure address registers as output, and set to 0x000000
	CIF_ADDRH_DDR   = CIF_ADDRL_DDR = 0xFF;
	CIF_ADDRU_DDR  |= CIF_ADDRU_MASK;
	CIF_ADDRH_PORT  = CIF_ADDRL_PORT = 0xFF;
	CIF_ADDRU_PORT |= CIF_ADDRU_MASK;

	// Configure data registers as input (with pull-ups enabled)
	CIF_DATAH_DDR  = CIF_DATAL_DDR  = 0;
	CIF_DATAH_PORT = CIF_DATAL_PORT = 0xFF;
	// Initialize _CIN as input, with active pullup
	CIF__CIN_DDR  &= ~(1<<CIF__CIN);
	CIF__CIN_PORT |= (1<<CIF__CIN);
	
	// Initialize _AS, _RST and _TIME as outputs, set _RST as active
	CIF_SET__TIME;
	CIF_CLR__RST;
	CIF__TIME_DDR |= 1<<CIF__TIME;
	CIF__RST_DDR  |= 1<<CIF__RST;
	CIF__AS_DDR   |= ~(1<<CIF__AS);
	CIF__AS_PORT  |= (1<<CIF__AS);
}

#endif /*_CIF_H_*/

/** \} */

