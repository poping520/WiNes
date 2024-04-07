cmake_minimum_required(VERSION 3.27)
project(WiNes C)

set(CMAKE_C_STANDARD 11)


add_executable(
        WiNes
        src/main.c
        src/common.h
        src/cpu.h
        src/cpu.c
        src/cpu_mem.c
        src/ppu.h
        src/ppu.c
        src/cartridge.c
        src/cartridge.h
        src/platform.c
        src/platform.h
        src/common.c
        src/mapper.c
        src/mapper.h
        src/wines.c
        src/mappers/mapper0_nrom.h
)


#set(SDL2_DIR ${CMAKE_CURRENT_LIST_DIR}/external/SDL2-2.30.1/cmake)
#find_package(SDL2 REQUIRED)
#
#include_directories(
#        ${SDL2_INCLUDE_DIRS}
#)
#
#set(SDL2_LIB_DIR ${CMAKE_CURRENT_LIST_DIR}/external/SDL2-2.30.1/lib)
#
## Support both 32 and 64 bit builds
#if (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
#    set(SDL2_LIBRARIES "${SDL2_LIB_DIR}/x64/SDL2.lib;${SDL2_LIB_DIR}/x64/SDL2main.lib")
#else ()
#    set(SDL2_LIBRARIES "${SDL2_LIB_DIR}/x86/SDL2.lib;${SDL2_LIB_DIR}/x86/SDL2main.lib")
#endif ()
#
#message(
#        ${SDL2_LIBRARIES}
#)
#
#target_link_libraries(WiNes ${SDL2_LIBRARIES})