@echo off
IF NOT DEFINED DevEnvDir call shell_win64.bat

set PROJECT_DIR=%~dp0\..

IF "%1"=="debug" (
     set config_options=/Zi /Od /DFAC_DEBUG
     set out_name="fac_console_debug.exe"
) ELSE IF "%1"=="simple" (
     set config_options=/Ox /DFAC_SIMPLE_GUI
     set out_name="fac.exe"
) ELSE IF "%1"=="simple_debug" (
     set config_options=/Zi /Od /DFAC_DEBUG /DFAC_SIMPLE_GUI
     set out_name="fac.exe"
     set out_name="fac_debug.exe"
) ELSE (
     set config_options=/Ox
     set out_name="fac_console.exe"
)

set compiler_options=/I C:\src\include /GR- /EHa- /FC /MT /nologo /volatile:iso /W4 /wd4068 /wd4100 /wd4201 /wd4701 /wd4189 /wd4530 /wd4996 /wd4067 %config_options%
set linker_options=/link /opt:ref /incremental:no /LIBPATH:"C:\src\lib\libssh" "C:\src\lib\libssh\ssh.lib"

pushd %PROJECT_DIR%\export
cl /std:c++17 %PROJECT_DIR%\modules\ssh_native\main_wingui.cpp /Fe:%out_name% %compiler_options% %linker_options% 
del *.obj > NUL 2> NUL
popd
