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

// Varies by environment
#define THRESHOLD    155
// Amount of samples to average
#define ADCBUFF_SIZE  128

#include "Keyboard.h"

#define bitSet(value, bit) ((value) |= (1 << (bit)))
#define bitClear(value, bit) ((value) &= ~(1 << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

static uint8_t btns[2][ADCBUFF_SIZE];
static uint8_t buffidx = 0;
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

static void TouchRead(void)
{
	const uint8_t MUX_CFG = 0b01100000;
	const uint8_t MUX_GND = 0b01111111;
	const uint8_t MUX_CHAN0 = 0b00000111;
	const uint8_t MUX_CHAN1 = 0b00000110;
	#define ADCPORT PORTF
	#define ADCPIN_0 7
	#define ADCPIN_1 6
	
	bitSet(ADCPORT, ADCPIN_0); // charge ADC0
	ADMUX = MUX_GND; // discharge internal capacitor
	_delay_us(10);
	bitClear(ADCPORT, ADCPIN_0); // tristate ADC0
	ADMUX = MUX_CFG | MUX_CHAN0; // connect internal cap to ADC0
	bitSet(ADCSRA, ADSC);// start the conversion
	while((ADCSRA & (1 << ADSC))); // Wait for conversion finish
	btns[0][buffidx] = ADCH;

	bitSet(ADCPORT, ADCPIN_1); // charge ADC1
	ADMUX = MUX_GND; // discharge internal capacitor
	_delay_us(10);
	bitClear(ADCPORT, ADCPIN_1); // tristate ADC1
	ADMUX = MUX_CFG | MUX_CHAN1; // connect internal cap to ADC1
	bitSet(ADCSRA, ADSC);// start the conversion
	while((ADCSRA & (1 << ADSC))); // Wait for conversion finish
	btns[1][buffidx] = ADCH;

	buffidx++;
	if (buffidx >= ADCBUFF_SIZE)
	buffidx = 0;
}

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();
	GlobalInterruptEnable();

	for (;;)
	{
		TouchRead();

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
	ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
	ADCSRB = 0;

	DDRB |= (1<<0); // RX LED
	DDRD |= (1<<5); // TX LED

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
	
	uint16_t btnsum1=0, btnsum2=0;
	for (uint8_t i=0; i<ADCBUFF_SIZE; i++)
	{
		btnsum1 += btns[0][i];
		btnsum2 += btns[1][i];
	}
	uint8_t btnavg1 = (btnsum1 / ADCBUFF_SIZE);// > THRESHOLD;
	uint8_t btnavg2 = (btnsum2 / ADCBUFF_SIZE);// > THRESHOLD;
	sw_states[0] = btnavg1 > THRESHOLD;
	sw_states[1] = btnavg2 > THRESHOLD;

	bitWrite(PORTB, 0, !sw_states[0]);
	bitWrite(PORTD, 5, !sw_states[1]);

	if ( sw_states[0] )
	KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_Z;
	if ( sw_states[1] )
	KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_X;

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
