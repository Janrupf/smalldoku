# Smalldoku

Ever wanted to run Sudoku without an Operating System? No? That's great, because now you can! Smalldoku is a standalone,
dependency free Sudoku library and application running directly on top of your firmware (as long as it is UEFI that is).

## Why would you do that?

Just because...why the hell not. Despite being challenged to create this, it gave me a lot of room to play with UEFI and
in general more or less bare metal development. It is a sometimes fun, sometimes frustrating learning experience to work
with UEFI and build applications on top of it

## Project structure

- `cmake` - Additional CMake modules
- `core-ui` - OS independent user interface implementation for Smalldoku, renders the UI
- `core` - OS independent logic library for Smalldoku, contains mostly basic Sudoku logic
- `linux-ui` X11 frontend, used for testing when you don't want to spin up an UEFI environment
- `uefi` - UEFI frontend, UEFI application which powers Smalldoku without an OS

## Not so frequently asked questions

**Q:** Is this useful in any way?

**A:** Absolutely not... HOWEVER, it can probably be taken as a reference on how to work with UEFI.

**Q:** How I can try this?

**A:** I might provide pre-built binaries at some point. For now, you can build it yourself,
       it's a standard CMake build. Installing it might be a pain depending on your firmware, so for now I recommend 
       running it either inside the EDK2 UEFI Emulator or QEMU with OVMF. That being said, it does run on real hardware,
       so if you feel comfortable messing around with your UEFI boot entries, go ahead!

**Q:** I want to add a feature/fix a bug/something, do you accept PR's?

**A:** If you have a cool idea or found a bug feel free to create a PR!