# RunBMC Firmware Framework Build System
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 RunBMC Project

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(runbmc_framework VERSION 0.2.0)

message(STATUS "========================================")
message(STATUS "RunBMC Firmware Framework v${PROJECT_VERSION}")
message(STATUS "Target Board: ${BOARD}")
message(STATUS "========================================")

# Framework version definitions
zephyr_compile_definitions(
  RUNBMC_VERSION_MAJOR=0
  RUNBMC_VERSION_MINOR=2
  RUNBMC_VERSION_PATCH=0
)

# Framework include directories
target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Core framework files (always compiled)
target_sources(app PRIVATE
  src/main.c
  src/core/bmc_core.c
)

# BMC Core Services (conditional compilation)
if(CONFIG_RUNBMC_POWER_MANAGEMENT)
  message(STATUS "  [x] Power Management Service")
  target_sources(app PRIVATE src/core/power.c)
else()
  message(STATUS "  [ ] Power Management Service")
endif()

if(CONFIG_RUNBMC_SENSOR_FRAMEWORK)
  message(STATUS "  [x] Sensor Framework")
  target_sources(app PRIVATE src/core/sensors.c)
else()
  message(STATUS "  [ ] Sensor Framework")
endif()

if(CONFIG_RUNBMC_TELEMETRY)
  message(STATUS "  [x] Telemetry Engine")
  target_sources(app PRIVATE src/core/telemetry.c)
else()
  message(STATUS "  [ ] Telemetry Engine")
endif()

if(CONFIG_RUNBMC_THERMAL_MANAGEMENT)
  message(STATUS "  [x] Thermal Management")
  target_sources(app PRIVATE src/core/thermal.c)
else()
  message(STATUS "  [ ] Thermal Management")
endif()

# Shell interface (if enabled)
if(CONFIG_SHELL)
  message(STATUS "  [x] Shell/CLI Interface")
  target_sources(app PRIVATE src/shell/shell_bmc.c)
endif()

# Platform abstraction layer
if(BOARD MATCHES "qemu_riscv.*")
  message(STATUS "  [x] QEMU RISC-V Platform HAL")
  target_sources(app PRIVATE src/platform/qemu_riscv.c)
elseif(BOARD STREQUAL "stm32f4_disco")
  message(STATUS "  [x] STM32F4 Discovery Platform HAL")
  target_sources(app PRIVATE src/platform/stm32f4_disco.c)
else()
  message(WARNING "  [!] Unknown platform, using generic HAL")
  target_sources(app PRIVATE src/platform/generic.c)
endif()

# Protocol support (future - Day 2+)
if(CONFIG_RUNBMC_PLDM)
  message(STATUS "  [x] PLDM Protocol (placeholder)")
  # Future: target_sources(app PRIVATE src/protocols/pldm/pldm_base.c)
else()
  message(STATUS "  [ ] PLDM Protocol")
endif()

if(CONFIG_RUNBMC_MCTP)
  message(STATUS "  [x] MCTP Transport (placeholder)")
  # Future: target_sources(app PRIVATE src/protocols/mctp/mctp_core.c)
else()
  message(STATUS "  [ ] MCTP Transport")
endif()

if(CONFIG_RUNBMC_FRU)
  message(STATUS "  [x] FRU Management (placeholder)")
  # Future: target_sources(app PRIVATE src/services/fru_manager.c)
else()
  message(STATUS "  [ ] FRU Management")
endif()

message(STATUS "========================================")