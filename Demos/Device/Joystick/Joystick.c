/*
             LUFA Library
     Copyright (C) Dean Camera, 2009.
              
  dean [at] fourwalledcubicle [dot] com
      www.fourwalledcubicle.com
*/

/*
  Copyright 2009  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, and distribute this software
  and its documentation for any purpose and without fee is hereby
  granted, provided that the above copyright notice appear in all
  copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Main source file for the Joystick demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#include "Joystick.h"

/** LUFA HID Class driver interface configuration and state information. This structure is
 *  passed to all HID Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_HID_t Joystick_HID_Interface =
	{
		.InterfaceNumber         = 0,

		.ReportINEndpointNumber  = JOYSTICK_EPNUM,
		.ReportINEndpointSize    = JOYSTICK_EPSIZE,
		
		.ReportINBufferSize      = sizeof(USB_JoystickReport_Data_t),

		.UsingReportProtocol     = true,
	};

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();
	
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	
	for (;;)
	{
		USB_HID_USBTask(&Joystick_HID_Interface);
		USB_USBTask();
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
	Joystick_Init();
	LEDs_Init();
	Buttons_Init();
	USB_Init();

	/* Millisecond timer initialization, with output compare interrupt enabled for the idle timing */
	OCR0A  = ((F_CPU / 64) / 1000);
	TCCR0A = (1 << WGM01);
	TCCR0B = ((1 << CS01) | (1 << CS00));
	TIMSK0 = (1 << OCIE0A);
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Connect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Disconnect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_ConfigurationChanged(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_READY);

	if (!(USB_HID_ConfigureEndpoints(&Joystick_HID_Interface)))
	  LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Unhandled Control Packet event. */
void EVENT_USB_UnhandledControlPacket(void)
{
	USB_HID_ProcessControlPacket(&Joystick_HID_Interface);
}

/** ISR to keep track of each millisecond interrupt, for determining the HID class idle period remaining when set. */
ISR(TIMER0_COMPA_vect, ISR_BLOCK)
{
	if (Joystick_HID_Interface.IdleMSRemaining)
	  Joystick_HID_Interface.IdleMSRemaining--;
}

/** HID class driver callback function for the creation of HID reports to the host.
 *
 *  \param HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param ReportData  Pointer to a buffer where the created report should be stored
 *
 *  \return Number of bytes written in the report (or zero if no report is to be sent
 */
uint16_t CALLBACK_USB_HID_CreateNextHIDReport(USB_ClassInfo_HID_t* HIDInterfaceInfo, uint8_t* ReportID, void* ReportData)
{
	USB_JoystickReport_Data_t* JoystickReport = (USB_JoystickReport_Data_t*)ReportData;
	
	uint8_t JoyStatus_LCL    = Joystick_GetStatus();
	uint8_t ButtonStatus_LCL = Buttons_GetStatus();

	if (JoyStatus_LCL & JOY_UP)
	  JoystickReport->Y = -100;
	else if (JoyStatus_LCL & JOY_DOWN)
	  JoystickReport->Y =  100;

	if (JoyStatus_LCL & JOY_RIGHT)
	  JoystickReport->X =  100;
	else if (JoyStatus_LCL & JOY_LEFT)
	  JoystickReport->X = -100;

	if (JoyStatus_LCL & JOY_PRESS)
	  JoystickReport->Button  = (1 << 1);
	  
	if (ButtonStatus_LCL & BUTTONS_BUTTON1)
	  JoystickReport->Button |= (1 << 0);
	  
	return sizeof(USB_JoystickReport_Data_t);
}

/** HID class driver callback function for the processing of HID reports from the host.
 *
 *  \param HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param ReportData  Pointer to a buffer where the created report has been stored
 *  \param ReportSize  Size in bytes of the received HID report
 */
void CALLBACK_USB_HID_ProcessReceivedHIDReport(USB_ClassInfo_HID_t* HIDInterfaceInfo, uint8_t ReportID,
                                               void* ReportData, uint16_t ReportSize)
{
	// Unused (but mandatory for the HID class driver) in this demo, since there are no Host->Device reports
}
