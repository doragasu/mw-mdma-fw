// TODO: Error if MCU is not a 64 kiB one, or move this definition to makefile
#define BOOTLOADER_START_ADDRESS	((64U - 4U) * 1024U)

/// \brief Jumps to bootloader, to allow upgrading the firmware.
/// \todo  Use the watchdog/magic key combination to ensure all the modules
/// of the MCU are properly reset before jumping to bootloader.
void JumpToBootloader(void);

