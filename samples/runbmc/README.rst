# RunBMC - Day 1: Multi-threaded BMC Architecture

## Overview
This is Day 1 of the RunBMC learning project - a Baseboard Management Controller
implementation on Zephyr RTOS targeting QEMU RISC-V32/64.

## Features (Day 1)
- Multi-threaded architecture (4 threads)
- Power management thread
- Sensor monitoring thread
- Telemetry collection thread
- Interactive shell with custom BMC commands
- Thread synchronization using mutexes

## Building
west build -p always -b qemu_riscv32 samples/runbmc

## Running
west build -t run