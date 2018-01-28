# (WIP) Circle Clicker Keypad

A minimal, no-fuss keypad that is fully open-source.  
Mainly designed for playing [osu!](https://osu.ppy.sh/home)

### Hardware

Case is designed for easy 3D printing and is available at [Onshape](https://cad.onshape.com/documents/0c4176a79cdff478a6cae731/w/b5ecb7bfa3edbe6043146154/e/b18b9b10b9c10c07a1bcd025)*  
PCB is drawn in EAGLE. Main version is double-sided and not recommended for beginners (*Will be uploaded with V1)

### Firmware

Uses AVR microcontrollers with [LUFA library](https://github.com/abcminiuser/lufa) handling USB communication.  
Atmel Studio 7 project files included, but binaries are built via makefiles to keep references to LUFA minimal.
Main version uses AT90USB162. Experimental touch version uses Arduino Leonardo clone board (Atmega32U4) with no external components.

### Software

Currently none. Feel free to contribute by making parameters configurable over additional HID endpoints.

\* Login needed to customize and export Onshape documents, but STLs and CAD sources will be published via GitHub releases.  