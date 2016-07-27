/************************************************************************//**
 * \file
 * \brief This module allows to control the WiFi chip mounted on MeGaWiFi
 * cartridges.
 *
 * \author Jesús Alonso (doragasu)
 * \date   2016
 * \defgroup wifi-if On-cart WiFi chip management.
 * \{
 ****************************************************************************/


#ifndef _WIFI_IF_H_
#define _WIFI_IF_H_

#include "16c550.h"

/*
 * Test functions. To be deleted when hardware tests finish.
 */
void WiFiSendTest(void);
void WiFiEchoTest(void);
void WiFiEchoServerTest(void);

/************************************************************************//**
 * \brief Initializes WiFi interface. Must be called once before any other
 *        function in the module, and after calling CifInit().
 ****************************************************************************/
void WiFiInit(void);

/************************************************************************//**
 * \brief Sets the WiFi module in RESET state.
 ****************************************************************************/
#define WiFiReset()			do{UartClrOut1();}while(0)

/************************************************************************//**
 * \brief Releases WiFi module from RESET state.
 ****************************************************************************/
#define WiFiStart()			do{UartSetOut1();}while(0)

/************************************************************************//**
 * \brief Releases WiFi module from power down state.
 ****************************************************************************/
#define WiFiPwrUp()			do{UartSetDtr();}while(0)

/************************************************************************//**
 * \brief Sets WiFi module in power down state.
 ****************************************************************************/
#define WiFiPwrDown()		do{UartClrDtr();}while(0)

/************************************************************************//**
 * \brief Enables (low level) the UART Request To Send pin.
 ****************************************************************************/
#define WiFiRtsEnable()		do{UartClrRts();}while(0)

/************************************************************************//**
 * \brief Disables (high level) the UART Request To Send pin.
 ****************************************************************************/
#define WiFiRtsDisable()	do{UartSetRts();}while(0)

/************************************************************************//**
 * \brief Enables (low level) the WiFi Program pin.
 ****************************************************************************/
#define WiFiPrgEnable()		do{UartClrOut2();}while(0)

/************************************************************************//**
 * \brief Disables (high level) the WiFi Program pin.
 ****************************************************************************/
#define WiFiPrgDisable()	do{UartSetOut2();}while(0)

/************************************************************************//**
 * \brief Obtains the #CTS (active low) pin status.
 *
 * \return 0 if pin active, non-zero otherwise.
 ****************************************************************************/
#define WiFiCtsGet()		do{UartCtsGet();}while(0)

/************************************************************************//**
 * \brief Obtains the #DATA (active low) pin status.
 *
 * \return 0 if pin active, non-zero otherwise.
 ****************************************************************************/
#define WiFiDataGet()		do{UartDsrGet();}while(0)

/************************************************************************//**
 * \brief Sends an array of characters using polling method.
 *
 * \param[in] data Data buffer to send.
 * \param[in] dataLen Number of bytes to send.
 *
 * \return The number of bytes sent.
 * \warning This function blocks, eating all the CPU until buffer is sent.
 ****************************************************************************/
uint16_t WiFiPollSend(uint8_t data[], uint16_t dataLen);

/************************************************************************//**
 * \brief Receives an array of characters using polling method.
 *
 * \param[in] data Data buffer holding the received characters.
 * \param[in] dataLen Number of bytes to read.
 *
 * \return The number of bytes received.
 * \warning This function blocks, eating the CPU until buffer is received.
 ****************************************************************************/
uint16_t WiFiPollRecv(uint8_t data[], uint16_t dataLen);

#endif /*_WIFI_IF_H_*/

/** \} */

