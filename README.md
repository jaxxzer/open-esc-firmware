Welcome to gsc: the gangster esc!

The aim of this project is to produce an ESC that is very high performing, widely applicable, and attractive to manufacture (cost and size optimized).

GSC will run on these target microcontrollers:

- Giga Device GD32 F350 series (108 MHz M4F @ 0.50~1.00$ that's *gangster*!)
- STM32 F0 series (in development)
- STM32 F3 series (in development)
- STM32 G0 series (planned)
- STSPIN series (planned)
- Active Semi PAC5523 (planned)

[gsc-hardware](https://github.com/jaxxzer/gsc-hardware) is a repository of hardware designs supported by this firmware.

To add support for a new hardware, copy one of the exisiting folders in the [target](target) directory and change the pin defitions.

Current functionality:
- automatic input signal detection (pwm, oneshot125/42, multishot, dshot, proshot)
- audio
- open-loop sine-modulated pwm

Project roadmap:
- Closed-loop comparator based commutation
- Closed-loop adc based commutation
- Support for dshot commands
- Save/store settings
- Sensored Field Oriented Control
- Sensorless Field Oriented Control
- Support for common communication interfaces (uart, i2c, spi, can, usb)
- GUI configuration
- Bipolar pwm (active braking)
- Support for PAC5523
- Switch from stm32-lib to STM LL, libopencm3 or bare-metal

To build the main application for the selected [target board](src/target) (the default board is `gsc`):
```sh
mkdir -p build
cd build
cmake --configure -DTARGET_BOARD=wraith32 ..
make
```

To flash the program after building (with openocd and st-link programmer), use the `flash` make target:
```sh
make flash
```

This project's [`launch.json`](.vscode/launch.json) will allow you to debug the program in vscode with the cortex debug extension using an st-link programmer and openocd.

**Be Advised:** I am developing my understanding of motor control and embedded programming as I work on this project. I am not an expert (yet). Many cool open source projects already exist to control brushless motors. Some day, I might choose to retire this project in favor of developing further one or more of these:

- [f051bldc](https://github.com/conuthead/f051bldc) (I see you, @conuthead)
- [phobia](https://bitbucket.org/amaora/phobia) (stm32, foc)
- [blheli_s](https://github.com/bitdump/BLHeli/tree/master/BLHeli_S%20SiLabs) (assembly, avr, silabs)
- [tgy](https://github.com/sim-/tgy) (assembly, avr, silabs)
- [openmotordrive](https://github.com/OpenMotorDrive) (stm32)
- [vesc](https://github.com/vedderb/bldc) (stm32, foc)
- [odrive](https://github.com/madcowswe/ODrive) (stm32)
- [betaflight-esc](https://github.com/betaflight/betaflight-esc) (stm32, not mature)
- [sapog](https://github.com/PX4/sapog) (stm32)
