# mw-mdma-fw
Firmare for the MegaWiFi Programmer. Firmware produced by these sources must be programmed to the microcontroller inside a MegaWiFi Programmer for it to work properly. It is prepared to be used in conjunction with `mw-mdma-bl` bootloader (a LUFA DFU based bootloader) to be able to boot from the bootloader, and enter bootloader mode for firmware download.

# Building
You will need `avr-gcc`, `avr-binutils` and `avr-libc` to be able to build the sources. You will also need a recent [LUFA library](http://www.fourwalledcubicle.com/LUFA.php) installation. Edit `makefile` file, and change the `LUFA_PATH` definition to match the path where you have installed LUFA library:

```
LUFA_PATH   ?= $(HOME)/src/avr/lufa/lufa-latest/LUFA
```

Then cd to the path where the sources of this repository are located and simply run:

```
make
```

To burn the new firmware you can use your favorite tool. If the microcontroller has a DFU bootloader burned, you can enter DFU mode (e.g. keeping the pushbutton on the programmer pressed while plugging the USB cable) and burn the firmware by running:
```
make dfu
```

Once flashed, you can use [mw-mdma-cli](https://github.com/doragasu/mw-mdma-cli) tool to to talk to the MegaWiFi Programmer.

# Authors
This program has been written by doragasu. It uses the wonderful LUFA library by Dean Camera.

# Contributions
Contributions are welcome. If you find a bug please open an issue, and if you have implemented a cool feature/improvement, please send a pull request.

# License
This program is provided with NO WARRANTY, under the [GPLv3 license](https://www.gnu.org/licenses/gpl-3.0.html).

