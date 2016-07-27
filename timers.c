#include <avr/interrupt.h>
#include "timers.h"

/// Computed count for Timer 0 to overflow
static uint16_t t1load;

/************************************************************************//**
 * \brief Configures Timer 0 to generate overflow interrupt once elapsed
 * count timer 0 cycles. Prescaler is hardcoded to clkio/1024.
 *
 * \param[in]	count	Number of timer cycles to count before interrupting.
 ****************************************************************************/
void Timer1Config(uint16_t count) {
	TCCR1B = 0x00;		// Ensure timer is stopped
	// Compute count
	t1load = (0xFFFFU - count) + 1;
}

/************************************************************************//**
 * \brief Starts timer 0.
 ****************************************************************************/
void Timer1Start(void) {
	TCCR1B = 0x00;		// Stop timer
	TIFR1 |= (1<<TOV1);	// Clear overflow interrupt flag
	// Load computed count value, high byte first
	TCNT1H = t1load>>8;
	TCNT1L = t1load & 0xFF;
	TCCR1B = 0x05;		// Start timer, prescaler: 1/1024
}

/************************************************************************//**
 * \brief Stops timer 0.
 ****************************************************************************/
void Timer1Stop(void) {
	TCCR1B = 0x00;
}


/************************************************************************//**
 * \brief Returns TRUE if Timer1 Has overflowed, FALSE otherwise.
 *
 * \return TRUE if Timer1 overflow condition occurred, false otherwise.
 ****************************************************************************/
uint8_t Timer1Ovfw(void) {
	if (TIFR1 & (1<<TOV1)) {
		// Stop timer, clear interrupt flag and return
		Timer1Stop();
		TIFR1 |= (1<<TOV1);
		return TRUE;
	} else return FALSE;
}

