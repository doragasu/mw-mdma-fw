/************************************************************************//**
 * \file
 * \brief System state machine. Receives events from the cartridge and USB
 * interface, and performs the corresponding actions.
 *
 * \author Jesús Alonso (doragasu)
 * \date   2015
 * \defgroup sys_fsm System finite state machine.
 * \{
 ****************************************************************************/
/*
 * FSM that controls the flasher. It manages the system, and processes
 * commands received via USB interface. Each time a command is received, the
 * first received byte contains the command code. The following bytes (if
 * any), contain the data needed to complete the command.
 *
 * SUPPORTED COMMANDS:
 * TODO: This information is outdated!
 * - MDMA_MANID_GET: Obtains flash chip manufacturer ID
 *   + Extra data fields: none.
 *   + Reply: OK + manufacturer ID (2 bytes).
 * - MDMA_DEVID_GET: Obtains flash chip device ID.
 *   + Extra data fields: none.
 *   + Reply: up to 6 bytes containing the device ID.
 * - MDMA_READ: Reads from flash.
 *   + Extra data fields:
 *     * Word address (4 bytes).
 *     * Word length (1 byte, maximum value is 16).
 *   + Reply: OK plus data:
 *     * Start Word address (4 bytes).
 *     * Word length (4 bytes).
 *     * Data (length according to length parameter).
 * - MDMA_CART_ERASE: Erases entire cartridge flash chip.
 *   + Extra data fields: none.
 *   + Reply: OK.
 * - MDMA_SECT_ERASE: Erases a sector of the flash chip.
 *   + Extra data fields: Address (4 bytes).
 *   + Reply: OK
 * - MDMA_WRITE: Writes data to flash.
 *   + Extra data fields:
 *     * Word address (4 bytes).
 *     * Word length (1 byte).
 *     * Data (length according to length parameter).
 *   + Reply: OK. On error, reply has two bytes: the error code and the
 *     number of written words.
 * - MDMA_MAN_CTRL: Manual lines control (dangerous!).
 *   + Extar data fields:
 *     * Hex data: 19 85 BA DA 55 (5 bytes).
 *     * Pin mask (from port A to F, 6 bytes).
 *     * Read/#Write (from port A to F, 6 bytes).
 *     * Output value from writes (from port A to F, 6 bytes).
 *   + Reply: OK plus data:
 *     * Data readed from requested pins (from port A to F, 6 bytes).
 *
 * \warning All words and dwords sent, must use LITTLE ENDIAN order, \b excepting the data to be flashed to the chip, that must be BIG ENDIAN.
 */

#ifndef _SYS_FSM_H_
#define _SYS_FSM_H_

#include <stdint.h>
#include "mdma-pr.h"
#include "timers.h"

/** \addtogroup sys_fsm SfEvents State machine events.
 * These are the events that the state machine understands and can process.
 * \{ */
#define SF_EVT_NONE		 0	///< No event (just cycle FSM)
#define SF_EVT_TIMER	 1	///< Timer event
#define SF_EVT_CIN		 2	///< Cartridge inserted
#define SF_EVT_COUT		 3	///< Cartridge removed
#define SF_EVT_USB_ATT	 4	///< USB attached and enumerated
#define SF_EVT_USB_DET	 5	///< USB detached
#define SF_EVT_USB_ERR	 6	///< Error on USB interface
#define SF_EVT_DIN		 7	///< Data reception from host
#define SF_EVT_DOUT		 8	///< Data sent to host
#define SF_EVT_SW_PRESS	 9	///< Button pressed
#define SF_EVT_SW_REL	10	///< Button released
/** \} */

/** \addtogroup sys_fsm SfWifiCtrlCode Control code for WiFi module operations.
 *  \{ */
typedef enum {
	SF_WIFI_CTRL_RST = 0,	///< Hold chip in reset state.
	SF_WIFI_CTRL_RUN,		///< Reset the chip.
	SF_WIFI_CTRL_BLOAD,		///< Enter bootloader mode.
	SF_WIFI_CTRL_APP,		///< Start application.
	SF_WIFI_CTRL_SYNC		///< Perform a SYNC attemp.
} SfWifiCtrlCode;
/** \} */

/// Offset for the data payloa of the WiFi command
#define SF_WIFI_CMD_PAYLOAD_OFF		4

/// Maximum number of poll cycles for the UART before timing out
#define SF_WIFI_TOUT_CYCLES_MAX		UINT16_MAX

/// Number of UART poll cycles for WiFi command operations.
#define SF_WIFI_CMD_TOUT_CYCLES		SF_WIFI_TOUT_CYCLES_MAX

/** \addtogroup sys_fsm SfSwData Pushbutton data interpretation masks.
 *  \{ */
#define SF_SW_PRESSED	0x01	///< If bit set, button is pressed.
#define SF_SW_EVENT		0x02	///< If bit set, a button event occurred.
/** \} */

/** \addtogroup sys_fsm SfStates Allowed state machine transitions
 * These are the state machines the system FSM can be in.
 * \{
 */
typedef enum {
	SF_IDLE,		///< Idle state, cartridge not inserted
	SF_STAB_WAIT,	///< Wait until cart/USB stabilizes
	SF_CART_INIT,	///< Initialize cartridge (obtain cart info)
	SF_READY,		///< System ready to parse host commands
	SF_MANID_GET,	///< Obtaining manufacturer ID
	SF_DEVID_GET,	///< Obtaining device ID
	SF_CART_READ,	///< Reading cartridge
	SF_CART_ERASE,	///< Erasing cartridge
	SF_SECT_ERASE,	///< Erasing sector
	SF_CART_PROG,	///< Programming cartridge
	SF_LINE_CTRL,	///< Manual line control
	SF_WIFI_MOD,	///< WiFi module command
	SF_ST_MAX
} SfStat;
/** \} */

/** \addtogroup sys_fsm SfFlags Auxiliar flags to define system status
 * \{
 */
typedef union {
	uint8_t all;				///< Access to all flags
	struct {
		uint8_t cart_in:1;		///< Cartridge inserted
		uint8_t usb_ready:1;	///< USB attached and ready
	};
} SfFlags;
/** \} */

/** \addtogroup sys_fsm SfFlashData Data describing the cartridge flash chip.
 * \{
 */
typedef struct {
	uint16_t	manId;		///< Chip manufacturer ID
	uint16_t	devId[3];	///< Chip device ID
} SfFlashData;
/** \} */

/** \addtogroup sys_fsm SfInstance Data about the currently running system instance.
 * \{
 */
typedef struct {
	SfStat s;		///< Current system status
	SfFlags f;		///< System status flags
	SfFlashData fc;	///< Flash chip data
	uint8_t sw;		///< Switch (pushbutton) status
} SfInstance;
/** \} */

/*
 * Public functions
 */

/************************************************************************//**
 * \brief Module initialization. Must be called before using any other function
 * from this module.
 ****************************************************************************/
void SfInit(void);

/************************************************************************//**
 * \brief Takes an incoming event and executes a cycle of the system FSM
 *
 * \param[in] evt Incoming event to be processed.
 *
 * \note Lots of states have been removed, might be needed if problems arise
 * because USB_USBTask() needs to be serviced more often than it is with
 * the current implementation.
 ****************************************************************************/
void SfFsmCycle(uint8_t evt);

/// Returns TRUE if a SF_EVT_TIMER event must be noticed to the FSM
#define SfEvtTimerNotify()	Timer1Ovfw()

#endif /*_SYS_FSM_H_*/

/** \} */

