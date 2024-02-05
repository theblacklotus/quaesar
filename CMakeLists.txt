# Minimum CMake version requirement
cmake_minimum_required(VERSION 3.10)

# Project name and languages used
project(quaesar LANGUAGES CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SHARED_DEFINES FSUAE) 

# Zlib and disable examples

set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/zlib)

find_package(SDL2 REQUIRED)

# Put this global as all code shareds them
include_directories(include od-cross .)

# Code generators 
# add_executable(genblitter src/genblitter.cpp src/missing.cpp od-cross/unicode.cpp)
add_executable(build68k src/build68k.cpp)
add_executable(gencpu src/cpudefs.cpp src/gencpu.cpp src/missing.cpp src/readcpu.cpp od-cross/unicode.cpp)
add_executable(gencomp src/cpudefs.cpp jit/gencomp.cpp src/missing.cpp src/readcpu.cpp od-cross/unicode.cpp)
add_executable(genblitter src/blitops.cpp src/genblitter.cpp)

target_compile_definitions(build68k PRIVATE ${SHARED_DEFINES})
target_compile_definitions(gencomp PRIVATE ${SHARED_DEFINES})
target_compile_definitions(gencpu PRIVATE ${SHARED_DEFINES})

# Set the output path for the generated code
set(BUILD68K_OUTPUT ../src/cpudefs.cpp)
set(GENCPU_OUTPUT ../src/cputbl.h)
set(GENCOMP_OUTPUT ../jit/comptbl.h)

add_custom_command(
    OUTPUT ${BUILD68K_OUTPUT}
    COMMAND build68k < table68k > ${BUILD68K_OUTPUT}
    WORKING_DIRECTORY ../src 
    DEPENDS build68k
    COMMENT "Generating 68k cpu definitions"
)

add_custom_command(
     OUTPUT ${GENCOMP_OUTPUT}
     COMMAND gencomp
     WORKING_DIRECTORY ../
     DEPENDS build68k
     COMMENT "Generating jit/comptbl.h"
)

add_custom_command(
    OUTPUT ${GENCPU_OUTPUT}
    COMMAND gencpu
    WORKING_DIRECTORY ../src 
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

gen_blitter(../src/blit.h i)
gen_blitter(../src/blitfunc.cpp f)
gen_blitter(../src/blitfunc.h h)
gen_blitter(../src/blittable.cpp t)

add_executable(quaesar 
    src/aros.rom.cpp
    src/akiko.cpp
    src/amax.cpp
    src/ar.cpp
    src/audio.cpp
    src/autoconf.cpp
    src/blitfunc.cpp
    src/blittable.cpp
    src/blitter.cpp
    src/calc.cpp
    src/cd32_fmv_genlock.cpp
    src/cdrom.cpp
    src/cdtvcr.cpp
    src/cfgfile.cpp
    src/cia.cpp
    src/consolehook.cpp
    src/cpudefs.cpp
    src/cpuemu_0.cpp
    src/cpuemu_11.cpp
    src/cpuemu_13.cpp
    src/cpuemu_20.cpp
    src/cpuemu_21.cpp
    src/cpuemu_22.cpp
    src/cpuemu_23.cpp
    src/cpuemu_24.cpp
    src/cpuemu_31.cpp
    src/cpuemu_32.cpp
    src/cpuemu_33.cpp
    src/cpuemu_34.cpp
    src/cpuemu_35.cpp
    src/cpuemu_40.cpp
    src/cpuemu_50.cpp
    src/cpummu.cpp
    src/cpummu30.cpp
    src/cpustbl.cpp
    src/crc32.cpp
    src/custom.cpp
    src/debug.cpp
    src/debugmem.cpp
    src/def_icons.cpp
    src/devices.cpp
    src/disasm.cpp
    src/disk.cpp
    src/diskutil.cpp
    src/dlopen.cpp
    src/dongle.cpp
    src/drawing.cpp
    src/driveclick.cpp
    src/enforcer.cpp
    src/ethernet.cpp
    src/events.cpp
    src/expansion.cpp
    src/fdi2raw.cpp
    src/filesys.cpp
    src/flashrom.cpp
    src/fpp.cpp
    src/fpp_native.cpp
    src/fpp_softfloat.cpp
    src/fsdb.cpp
    src/gayle.cpp
    src/hardfile.cpp
    src/hrtmon.rom.cpp
    src/ide.cpp
    src/idecontrollers.cpp
    src/identify.cpp
    src/ini.cpp
    src/inputdevice.cpp
    src/inputrecord.cpp
    src/isofs.cpp
    src/keybuf.cpp
    src/logging.cpp
    src/main.cpp
    src/memory.cpp
    src/missing.cpp
    src/native2amiga.cpp
    src/newcpu.cpp
    src/newcpu_common.cpp
    src/readcpu.cpp
    src/rommgr.cpp
    src/rtc.cpp
    src/sana2.cpp
    src/savestate.cpp
    src/scp.cpp
    src/scsi.cpp
    src/scsiemul.cpp
    src/scsitape.cpp
    src/sndboard.cpp
    src/statusline.cpp
    src/tabletlibrary.cpp
    src/test_card.cpp
    src/tinyxml2.cpp
    src/traps.cpp
    src/uaeexe.cpp
    src/uaelib.cpp
    src/uaenative.cpp
    src/uaeresource.cpp
    src/uaeserial.cpp
    src/quaesar.cpp
    src/blkdev.cpp
    od-cross/gfx.cpp
    od-cross/gui.cpp
    od-cross/mman.cpp
    od-cross/machdep/m68k.cpp
    od-cross/thread.cpp
    od-cross/unicode.cpp
    od-cross/file_system.cpp
    od-cross/hardfile_host.cpp
    od-cross/filepaths.cpp
    od-cross/sounddep/sound.cpp
    od-cross/time.cpp
    od-cross/dummy.cpp
    od-cross/input.cpp
    src/zfile.cpp
    src/zfile_archive.cpp
    src/vm.cpp
    softfloat/softfloat.cpp
    softfloat/softfloat_fpsp.cpp
    softfloat/softfloat_decimal.cpp
    src/cputbl.h 
    jit/comptbl.h
    src/blit.h
)

target_compile_definitions(quaesar PRIVATE ${SHARED_DEFINES})
target_compile_options(quaesar PRIVATE -DUAE=1)
target_compile_options(quaesar PRIVATE -D_cdecl= -DFILEFLAG_WRITE=1 -DOS_NAME=\"linux\")
target_compile_options(quaesar PRIVATE -DUSHORT=uint16_t -DHWND=uint32_t -DHRESULT=uint32_t -DWPARAM=uint16_t -DLPARAM=uint32_t)
target_include_directories(quaesar PRIVATE ${SDL2_INCLUDE_DIRS} "src/external/zlib")
target_link_libraries(quaesar PRIVATE ${SDL2_LIBRARIES} zlibstatic)
target_compile_options(quaesar PRIVATE -DFILEFLAG_DIR=1 -D_ftelli64=ftello64 -D_fseeki64=fseeko64)



