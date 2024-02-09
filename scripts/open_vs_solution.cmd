@echo off

setlocal

set "OUTPUT_DIR=..\output"

# check if build folder exists (i.e we are running from the root dir)
if exists "build" (
    set "OUTPUT_DIR=output"
)

# check if output folder exists, if not create it
if not exists %OUTPUT_DIR % (
    mkdir %OUTPUT_DIR %
)

cd %OUTPUT_DIR%
..\bin\windows\bin\cmake .. && start quaesar.sln 

