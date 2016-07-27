#include "bloader.h"
#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Drivers/Board/LEDs.h>

/// Disables USB and interrupts, and then enters bootloader
void JumpToBootloader(void) {
	LEDs_TurnOffLEDs(LEDS_ALL_LEDS);
	USB_Disable();
	cli();
	Delay_MS(2000);
	((void (*)(void))BOOTLOADER_START_ADDRESS)();
}


