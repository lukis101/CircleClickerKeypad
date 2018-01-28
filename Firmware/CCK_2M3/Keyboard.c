/*
             LUFA Library
     Copyright (C) Dean Camera, 2018.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2018  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
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
 *  Main source file adapted from the Keyboard demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#define BOARDVER 2

#include "Keyboard.h"

#define bitSet(value, bit) ((value) |= (1 << (bit)))
#define bitClear(value, bit) ((value) &= ~(1 << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

static uint8_t sw_states[5];

/** Buffer to hold the previously generated Keyboard HID report, for comparison purposes inside the HID class driver. */
static uint8_t PrevKeyboardHIDReportBuffer[sizeof(USB_KeyboardReport_Data_t)];

/** LUFA HID Class driver interface configuration and state information. This structure is
 *  passed to all HID Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_HID_Device_t Keyboard_HID_Interface =
	{
		.Config =
			{
				.InterfaceNumber              = INTERFACE_ID_Keyboard,
				.ReportINEndpoint             =
					{
						.Address              = KEYBOARD_EPADDR,
						.Size                 = KEYBOARD_EPSIZE,
						.Banks                = 1,
					},
				.PrevReportINBuffer           = PrevKeyboardHIDReportBuffer,
				.PrevReportINBufferSize       = sizeof(PrevKeyboardHIDReportBuffer),
			},
	};


/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();
	GlobalInterruptEnable();

	for (;;)
	{
	#if (BOARDVER == 1)
		bitWrite( PORTC, 5, (sw_states[0])^sw_states[2]); // LED1
		bitWrite( PORTD, 0, (sw_states[1])^sw_states[2]); // LED2
	#else
		bitWrite( PORTC, 5, (sw_states[0] || sw_states[3])^sw_states[2]); // LED1
		bitWrite( PORTC, 6, (sw_states[1] || sw_states[4])^sw_states[2]); // LED2
	#endif

		HID_Device_USBTask(&Keyboard_HID_Interface);
		USB_USBTask();
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware()
{
	/* ARCH_AVR8 */
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
#if (BOARDVER == 1)
	DDRC  |= _BV(5); // LED1
	DDRD  |= _BV(0); // LED2
	PORTC |= _BV(7); // SW1 pullup
	PORTD |= _BV(2); // SW2 pullup
	PORTD |= _BV(1); // Side SW pullup
	sw_states[1] = (PIND & _BV(2)) == 0;
	sw_states[0] = (PINC & _BV(7)) == 0;
	sw_states[2] = (PIND & _BV(1))  == 0;
#else
	DDRC  |= _BV(5); // LED1
	DDRC  |= _BV(6); // LED2
	PORTC |= _BV(7); // SW1 pullup
	PORTD |= _BV(3); // SW2 pullup
	PORTD |= _BV(6); // Side SW pullup
	PORTD |= _BV(0); // Top left SW pullup
	PORTD |= _BV(1); // Top right SW pullup
	_delay_us(10);
	sw_states[0] = (PINC & _BV(7)) == 0;
	sw_states[1] = (PIND & _BV(3)) == 0;
	sw_states[2] = (PIND & _BV(6))  == 0;
	sw_states[3] = (PIND & _BV(0))  == 0;
	sw_states[4] = (PIND & _BV(1))  == 0;
#endif

	// Interrupts on any edge
#if (BOARDVER == 1)
	EICRA |= (0<<ISC21)|(1<<ISC20) | (0<<ISC11)|(1<<ISC10);
	EICRB |= (0<<ISC41)|(1<<ISC40);
	EIMSK |= (1<<INT4) | (1<<INT2) | (1<<INT1); // Enable
#else
	EICRA |= (0<<ISC31)|(1<<ISC30) | (0<<ISC11)|(1<<ISC10) | (0<<ISC01)|(1<<ISC00);
	EICRB |= (0<<ISC61)|(1<<ISC60) | (0<<ISC41)|(1<<ISC40);
	EIMSK |= (1<<INT6) | (1<<INT4) | (1<<INT3) | (1<<INT1) | (1<<INT0); // Enable
#endif

	USB_Init();
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
	//LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
	//LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= HID_Device_ConfigureEndpoints(&Keyboard_HID_Interface);

	USB_Device_EnableSOFEvents();

	//LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	HID_Device_ProcessControlRequest(&Keyboard_HID_Interface);
}

/** Event handler for the USB device Start Of Frame event. */
void EVENT_USB_Device_StartOfFrame(void)
{
	HID_Device_MillisecondElapsed(&Keyboard_HID_Interface);
}

/** HID class driver callback function for the creation of HID reports to the host.
 *
 *  \param[in]     HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in,out] ReportID    Report ID requested by the host if non-zero, otherwise callback should set to the generated report ID
 *  \param[in]     ReportType  Type of the report to create, either HID_REPORT_ITEM_In or HID_REPORT_ITEM_Feature
 *  \param[out]    ReportData  Pointer to a buffer where the created report should be stored
 *  \param[out]    ReportSize  Number of bytes written in the report (or zero if no report is to be sent)
 *
 *  \return Boolean \c true to force the sending of the report, \c false to let the library determine if it needs to be sent
 */
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                         uint8_t* const ReportID,
                                         const uint8_t ReportType,
                                         void* ReportData,
                                         uint16_t* const ReportSize)
{
	USB_KeyboardReport_Data_t* KeyboardReport = (USB_KeyboardReport_Data_t*)ReportData;

	uint8_t UsedKeyCodes = 0;

	if ( sw_states[0] )
		KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_Z;
	if ( sw_states[1] )
		KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_X;
	if ( sw_states[2] )
		KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_SPACE;
	if ( sw_states[3] )
		KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_ESCAPE;
	if ( sw_states[4] )
		KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_TAB;

	*ReportSize = sizeof(USB_KeyboardReport_Data_t);
	return false;
}

/** HID class driver callback function for the processing of HID reports from the host.
 *
 *  \param[in] HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in] ReportID    Report ID of the received report from the host
 *  \param[in] ReportType  The type of report that the host has sent, either HID_REPORT_ITEM_Out or HID_REPORT_ITEM_Feature
 *  \param[in] ReportData  Pointer to a buffer where the received report has been stored
 *  \param[in] ReportSize  Size in bytes of the received HID report
 */
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                          const uint8_t ReportID,
                                          const uint8_t ReportType,
                                          const void* ReportData,
                                          const uint16_t ReportSize)
{
	/*uint8_t  LEDMask   = LEDS_NO_LEDS;
	uint8_t* LEDReport = (uint8_t*)ReportData;

	if (*LEDReport & HID_KEYBOARD_LED_NUMLOCK)
	  LEDMask |= LEDS_LED1;

	if (*LEDReport & HID_KEYBOARD_LED_CAPSLOCK)
	  LEDMask |= LEDS_LED3;

	if (*LEDReport & HID_KEYBOARD_LED_SCROLLLOCK)
	  LEDMask |= LEDS_LED4;

	LEDs_SetAllLEDs(LEDMask);*/
}

/* Switch interrupts */
#if (BOARDVER == 1)
ISR(INT4_vect) // Left mech
{
	sw_states[0] = (PINC & _BV(7)) == 0;
}
ISR(INT2_vect) // Right mech
{
	sw_states[1] = (PIND & _BV(2)) == 0;
}
ISR(INT1_vect) // Side sw
{
	sw_states[2] = (PIND & _BV(1))  == 0;
}

#else

ISR(INT4_vect) // Left mech
{
	sw_states[0] = (PINC & _BV(7)) == 0;
}
ISR(INT3_vect) // Right mech
{
	sw_states[1] = (PIND & _BV(3)) == 0;
}
ISR(INT6_vect) // Side sw
{
	sw_states[2] = (PIND & _BV(6))  == 0;
}
ISR(INT0_vect) // Top left
{
	sw_states[3] = (PIND & _BV(0))  == 0;
}
ISR(INT1_vect) // Top right
{
	sw_states[4] = (PIND & _BV(1))  == 0;
}
#endif