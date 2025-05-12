@echo off
IF NOT DEFINED DevEnvDir call shell_win64.bat

set PROJECT_DIR=%~dp0\..

set compiler_options=/I C:\src\include /GR- /EHa- /FC /MT /nologo /volatile:iso /W4 /wd4068 /wd4100 /wd4201 /wd4701 /wd4189 /wd4530 /wd4996 /Ox
set linker_options=/link /opt:ref /incremental:no /LIBPATH:"C:\src\lib\libssh" "C:\src\lib\libssh\ssh.lib"

pushd %PROJECT_DIR%\export
cl %PROJECT_DIR%\modules\ssh_native\main.cpp /Fe:"fac_cli.exe" %compiler_options% %linker_options% 
popd
