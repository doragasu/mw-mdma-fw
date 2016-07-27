#include <stdint.h>
#include <avr/io.h>
#include "util.h" 	// Just for TRUE/FALSE defines


/************************************************************************//**
 * \brief Obtains the count for Timer to elapse required ms.
 *
 * \param[in]	ms Milliseconds to convert to Timer1 count.
 *
 * \note Uses a 1/1024 prescaler to compute value.
 * \warning Computation might silently overflow!
 ****************************************************************************/
#define TimerMsToCount(ms)	((uint16_t)((ms)*(F_CPU/1000)/1024))

/************************************************************************//**
 * \brief Configures Timer 0 to generate overflow once elapsed
 * count timer 0 cycles. Prescaler is hardcoded to clkio/1024.
 *
 * \param[in]	count	Number of timer cycles to count before overflow.
 ****************************************************************************/
void Timer1Config(uint16_t count);

/************************************************************************//**
 * \brief Starts timer 0.
 ****************************************************************************/
void Timer1Start(void);

/************************************************************************//**
 * \brief Stops timer 0.
 ****************************************************************************/
void Timer1Stop(void);

/************************************************************************//**
 * \brief Returns TRUE if Timer1 Has overflowed, FALSE otherwise.
 *
 * \return TRUE if Timer1 overflow condition occurred, false otherwise.
 ****************************************************************************/
uint8_t Timer1Ovfw(void);

