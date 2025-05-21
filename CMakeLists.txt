# Minimum CMake version requirement
cmake_minimum_required(VERSION 3.10)

#set(CMAKE_GENERATOR_TOOLSET "ClangCl") # test compilation on clang-c

option(ENABLE_CODE_GENERATION "Enable code generation" OFF)

# Project name and languages used
project(quaesar LANGUAGES CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Zlib and disable examples

set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CAPSTONE_M68K_SUPPORT ON CACHE BOOL "" FORCE)

add_subdirectory(external/zlib)
add_subdirectory(external/ADFlib)
add_subdirectory(external/dear_imgui)
add_subdirectory(external/capstone)
add_subdirectory(external/EASTL)

if (APPLE OR LINUX OR UNIX)
    find_package(SDL2 REQUIRED)
endif()

if(NOT DEFINED SDL2_INCLUDE_DIRS)
    set(SDL2_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/external/sdl2/include")
endif()


if (WIN32)
  add_definitions(-D_HAS_STD_BYTE=0)
  if(MSVC) # Enable Edit and Continue  for Debug builds
    string(REGEX REPLACE "/Z[iI7]" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /ZI")
    add_link_options("/INCREMENTAL")
    add_compile_options("/MP") # multiprocessor build
	
	if(NOT CMAKE_GENERATOR_PLATFORM)
		set(CMAKE_GENERATOR_PLATFORM "Win32") # add Win32 platform to solution
	endif()
  endif()
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

set(UAE_SOURCE_LIST
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
)
add_executable(quaesar ${UAE_SOURCE_LIST})
if (WIN32)
    add_dependencies(quaesar quaesar-clang-format)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${UAE_SOURCE_LIST})

# Recursively glob for all .cpp files in the src directory
file(GLOB_RECURSE CROSS_FILES "src/*.cpp")
file(GLOB_RECURSE QUAE_HEADERS "src/*.h")
target_sources(quaesar PRIVATE ${CROSS_FILES} ${QUAE_HEADERS})

# Create the source groups for source tree with root at CMAKE_CURRENT_SOURCE_DIR.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${QUAE_HEADERS} ${CROSS_FILES} ${SDL2_INCLUDE_DIRS})

target_include_directories(quaesar PRIVATE ${SDL2_INCLUDE_DIRS})

if (APPLE OR LINUX OR UNIX)
	target_compile_options(quaesar PRIVATE -DUAE=1 -D_cdecl= -DOS_NAME=\"linux\")
    target_compile_definitions(quaesar PRIVATE FSUAE)
    target_link_libraries(quaesar PRIVATE ${SDL2_LIBRARIES})
endif()

if (WIN32)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
       target_link_libraries(quaesar PRIVATE "${CMAKE_SOURCE_DIR}/external/sdl2/x64/sdl2.lib")
       target_link_libraries(quaesar PRIVATE "${CMAKE_SOURCE_DIR}/external/sdl2/x64/SDL2main.lib")
    else()
       target_link_libraries(quaesar PRIVATE "${CMAKE_SOURCE_DIR}/external/sdl2/win32/sdl2.lib")
	   target_link_libraries(quaesar PRIVATE "${CMAKE_SOURCE_DIR}/external/sdl2/win32/SDL2main.lib")
    endif()
    target_link_libraries(quaesar PRIVATE Ws2_32.lib Winmm.lib Version.lib Imm32.lib Setupapi.lib)
	set_target_properties(quaesar PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT quaesar) # makes startup project
endif()

if (APPLE)
    target_compile_options(quaesar PRIVATE -Wno-macro-redefined -Wno-deprecated-declarations)
endif()

if (LINUX OR UNIX)
    target_link_libraries(quaesar PRIVATE dl)
endif()

target_include_directories(quaesar PRIVATE "${CMAKE_SOURCE_DIR}/external/ADFlib/src")
target_include_directories(quaesar PRIVATE "${CMAKE_SOURCE_DIR}/external")
target_link_libraries(quaesar PRIVATE ${SDL2_LIBRARIES} zlibstatic adf imgui capstone EASTL)


if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/imgui.ini")
    file(COPY_FILE "${CMAKE_CURRENT_LIST_DIR}/bin/install/default_layout.ini" "${CMAKE_CURRENT_BINARY_DIR}/imgui.ini")
endif()


# Format source code with clang-format
if(WIN32)
    set(CLANG_FORMAT_EXE "${CMAKE_SOURCE_DIR}/bin/windows/bin/clang-format.exe")
else()
    find_program(CLANG_FORMAT_EXE NAMES clang-format)
endif()

if(CLANG_FORMAT_EXE)
    file(GLOB_RECURSE QUAE_FORMAT_SRC_FILES
        "${CMAKE_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_SOURCE_DIR}/src/*.h"
    )

    # Create a stamp file to track formatting
    set(CLANG_FORMAT_STAMP "${CMAKE_BINARY_DIR}/.clang_format.stamp")

    add_custom_command(
        OUTPUT ${CLANG_FORMAT_STAMP}
        COMMAND ${CMAKE_COMMAND} -E echo "Format sources with clang-format: ${CLANG_FORMAT_EXE}"
        COMMAND ${CLANG_FORMAT_EXE} -i --style=file ${QUAE_FORMAT_SRC_FILES}
        COMMAND ${CMAKE_COMMAND} -E touch ${CLANG_FORMAT_STAMP}
        DEPENDS ${QUAE_FORMAT_SRC_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-format on changed files"
    )

    add_custom_target(
        quaesar-clang-format
        DEPENDS ${CLANG_FORMAT_STAMP}
    )
endif()
