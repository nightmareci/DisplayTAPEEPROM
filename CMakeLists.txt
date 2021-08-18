cmake_minimum_required(VERSION 3.10)
project(DisplayTAPEEPROM
	DESCRIPTION "Tool that prints out all data in TAP's EEPROM"
	LANGUAGES C
)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(EXE "display-tap-eeprom")
add_executable(${EXE})
target_sources(${EXE} PRIVATE display-tap-eeprom.c)
