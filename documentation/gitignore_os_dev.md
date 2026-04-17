# OS Development Gitignore Guide

For your OS development project, you should ignore build artifacts, temporary logs, and intermediate object files.

## High Priority Patterns
- `*.o`: Intermediate object files.
- `*.elf`: Compiled kernel binaries.
- `*.iso`: Bootable disk images.
- `iso_root/`: Temporary directory used to assemble the ISO.
- `*.log`: QEMU/Serial output logs.
- `dump.txt`: Debug dumps.

## Project Specifics
- `DOOM1.WAD`: Typically game assets are not tracked if they are large or non-free.
- `sboot/`: If this is a downloaded dependency, you might want to ignore it or use a submodule.

## Root vs Subdirectory
Currently, your `.git` folder is inside `/gridz`. This means files in the root (like your main `Makefile`, `linker.ld`, and `DOOM1.WAD`) are NOT being tracked. 
It is recommended to move `.git` to the project root.
