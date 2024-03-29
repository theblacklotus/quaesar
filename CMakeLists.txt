# Minimum CMake version requirement
cmake_minimum_required(VERSION 3.10)

option(ENABLE_CODE_GENERATION "Enable code generation" OFF)

# Project name and languages used
project(quaesar LANGUAGES CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Zlib and disable examples

set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/zlib)
add_subdirectory(external/ADFlib)

if (APPLE OR LINUX OR UNIX)
    find_package(SDL2 REQUIRED)
endif()

if (WIN32)
  add_definitions(-D_HAS_STD_BYTE=0)
endif()

include_directories(uae_src/include uae_src src .)

if (ENABLE_CODE_GENERATION)
    add_executable(build68k uae_src/build68k.cpp)
    add_executable(gencpu uae_src/cpudefs.cpp uae_src/gencpu.cpp uae_src/missing.cpp uae_src/readcpu.cpp src/unicode.cpp)
    add_executable(gencomp uae_src/cpudefs.cpp uae_src/jit/gencomp.cpp uae_src/missing.cpp uae_src/readcpu.cpp src/unicode.cpp)
    add_executable(genblitter uae_src/blitops.cpp uae_src/genblitter.cpp)

    target_compile_definitions(build68k PRIVATE FSUAE)

    if (NOT WIN32)
        target_compile_definitions(gencomp PRIVATE FSUAE)
        target_compile_definitions(gencpu PRIVATE FSUAE)
    endif()

    # Set the output path for the generated code
    set(BUILD68K_OUTPUT ../uae_src/cpudefs.cpp)
    set(GENCPU_OUTPUT ../uae_src/cputbl.h)
    set(GENCOMP_OUTPUT ../uae_jit/comptbl.h)

    add_custom_command(
        OUTPUT ${BUILD68K_OUTPUT}
        COMMAND build68k < table68k > ${BUILD68K_OUTPUT}
        WORKING_DIRECTORY ../uae_src 
        DEPENDS build68k
        COMMENT "Generating 68k cpu definitions"
    )

    add_custom_command(
        OUTPUT ${GENCOMP_OUTPUT}
        COMMAND gencomp
        WORKING_DIRECTORY ../uae_src
        DEPENDS build68k
        COMMENT "Generating jit/comptbl.h"
    )

    add_custom_command(
        OUTPUT ${GENCPU_OUTPUT}
        COMMAND gencpu
        WORKING_DIRECTORY ../uae_src 
        DEPENDS gencpu
        COMMENT "Generating CPU code"
    )

    function(gen_blitter output letter)
        add_custom_command(
            OUTPUT ${output}
            COMMAND genblitter ${letter} > ${output}
            DEPENDS genblitter
            COMMENT "Generating blitter code for ${letter}"
        )
    endfunction()

    gen_blitter(../uae_src/blit.h i)
    gen_blitter(../uae_src/blitfunc.cpp f)
    gen_blitter(../uae_src/blitfunc.h h)
    gen_blitter(../uae_src/blittable.cpp t)
endif()

add_executable(quaesar 
    uae_src/aros.rom.cpp
    uae_src/akiko.cpp
    uae_src/amax.cpp
    uae_src/ar.cpp
    uae_src/audio.cpp
    uae_src/autoconf.cpp
    uae_src/blitfunc.cpp
    uae_src/blittable.cpp
    uae_src/blitter.cpp
    uae_src/calc.cpp
    uae_src/cd32_fmv_genlock.cpp
    uae_src/cdrom.cpp
    uae_src/cdtvcr.cpp
    uae_src/cfgfile.cpp
    uae_src/cia.cpp
    uae_src/consolehook.cpp
    uae_src/cpudefs.cpp
    uae_src/cpuemu_0.cpp
    uae_src/cpuemu_11.cpp
    uae_src/cpuemu_13.cpp
    uae_src/cpuemu_20.cpp
    uae_src/cpuemu_21.cpp
    uae_src/cpuemu_22.cpp
    uae_src/cpuemu_23.cpp
    uae_src/cpuemu_24.cpp
    uae_src/cpuemu_31.cpp
    uae_src/cpuemu_32.cpp
    uae_src/cpuemu_33.cpp
    uae_src/cpuemu_34.cpp
    uae_src/cpuemu_35.cpp
    uae_src/cpuemu_40.cpp
    uae_src/cpuemu_50.cpp
    uae_src/cpummu.cpp
    uae_src/cpummu30.cpp
    uae_src/cpustbl.cpp
    uae_src/crc32.cpp
    uae_src/custom.cpp
    uae_src/debug.cpp
    uae_src/debugmem.cpp
    uae_src/def_icons.cpp
    uae_src/devices.cpp
    uae_src/disasm.cpp
    uae_src/disk.cpp
    uae_src/diskutil.cpp
    uae_src/dlopen.cpp
    uae_src/dongle.cpp
    uae_src/drawing.cpp
    uae_src/driveclick.cpp
    uae_src/enforcer.cpp
    uae_src/ethernet.cpp
    uae_src/events.cpp
    uae_src/expansion.cpp
    uae_src/fdi2raw.cpp
    uae_src/filesys.cpp
    uae_src/flashrom.cpp
    uae_src/fpp.cpp
    uae_src/fpp_native.cpp
    uae_src/fpp_softfloat.cpp
    uae_src/fsdb.cpp
    uae_src/gayle.cpp
    uae_src/hardfile.cpp
    uae_src/hrtmon.rom.cpp
    uae_src/ide.cpp
    uae_src/idecontrollers.cpp
    uae_src/identify.cpp
    uae_src/ini.cpp
    uae_src/inputdevice.cpp
    uae_src/inputrecord.cpp
    uae_src/isofs.cpp
    uae_src/keybuf.cpp
    uae_src/logging.cpp
    uae_src/main.cpp
    uae_src/memory.cpp
    uae_src/missing.cpp
    uae_src/native2amiga.cpp
    uae_src/newcpu.cpp
    uae_src/newcpu_common.cpp
    uae_src/readcpu.cpp
    uae_src/rommgr.cpp
    uae_src/rtc.cpp
    uae_src/sana2.cpp
    uae_src/savestate.cpp
    uae_src/scp.cpp
    uae_src/scsi.cpp
    uae_src/scsiemul.cpp
    uae_src/scsitape.cpp
    uae_src/serial.cpp
    uae_src/sndboard.cpp
    uae_src/statusline.cpp
    uae_src/tabletlibrary.cpp
    uae_src/test_card.cpp
    uae_src/tinyxml2.cpp
    uae_src/traps.cpp
    uae_src/uaeexe.cpp
    uae_src/uaelib.cpp
    uae_src/uaenative.cpp
    uae_src/uaeresource.cpp
    uae_src/uaeserial.cpp
    uae_src/blkdev.cpp
    uae_src/gfxutil.cpp
    uae_src/zfile.cpp
    uae_src/zfile_archive.cpp
    uae_src/vm.cpp
    uae_src/softfloat/softfloat.cpp
    uae_src/softfloat/softfloat_fpsp.cpp
    uae_src/softfloat/softfloat_decimal.cpp
    uae_src/cputbl.h 
    uae_src/jit/comptbl.h
    uae_src/blit.h
    src/gfx.cpp
    src/gui.cpp
    src/mman.cpp
    src/machdep/m68k.cpp
    src/thread.cpp
    src/unicode.cpp
    src/file_system.cpp
    src/hardfile_host.cpp
    src/filepaths.cpp
    src/sounddep/sound.cpp
    src/time.cpp
    src/dummy.cpp
    src/input.cpp
    src/quaesar.cpp
    src/adf.cpp
)

if (APPLE OR LINUX OR UNIX)
	target_compile_options(quaesar PRIVATE -DUAE=1 -D_cdecl= -DFILEFLAG_WRITE=1 -DOS_NAME=\"linux\")
	target_compile_options(quaesar PRIVATE -DUSHORT=uint16_t -DHWND=uint32_t -DHRESULT=uint32_t -DWPARAM=uint16_t -DLPARAM=uint32_t)
    target_compile_definitions(quaesar PRIVATE FSUAE)
    target_include_directories(quaesar PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(quaesar PRIVATE ${SDL2_LIBRARIES})
endif()

if (WIN32)
    target_include_directories(quaesar PRIVATE "${CMAKE_SOURCE_DIR}/external/sdl2/include")
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
       target_link_libraries(quaesar PRIVATE "${CMAKE_SOURCE_DIR}/external/sdl2/x64/sdl2.lib")
    else()
       target_link_libraries(quaesar PRIVATE "${CMAKE_SOURCE_DIR}/external/sdl2/win32/sdl2.lib")
    endif()
    target_link_libraries(quaesar PRIVATE Ws2_32.lib Winmm.lib Version.lib Imm32.lib Setupapi.lib)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT quaesar)
endif()

if (APPLE)
    target_compile_options(quaesar PRIVATE -DFILEFLAG_DIR=1 -DFILEFLAG_DIR=1 -D_ftelli64=ftell -D_fseeki64=fseek -D_fseeko64=fseeko -Dfseeko64=fseeko -Dftello64=ftell)
endif()

if (LINUX OR UNIX)
    target_compile_options(quaesar PRIVATE -DFILEFLAG_DIR=1 -D_ftelli64=ftello64 -D_fseeki64=fseeko64)
    target_link_libraries(quaesar PRIVATE dl)
endif()

target_include_directories(quaesar PRIVATE "${CMAKE_SOURCE_DIR}/external/ADFlib/src")
target_link_libraries(quaesar PRIVATE ${SDL2_LIBRARIES} zlibstatic adf)
