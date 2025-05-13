@echo off
IF NOT DEFINED DevEnvDir call shell_win64.bat

set PROJECT_DIR=%~dp0\..

IF %1==debug (
     set config_options=/Zi /Od
) ELSE set config_options=/Ox

set compiler_options=/I C:\src\include /GR- /EHa- /FC /MT /nologo /volatile:iso /W4 /wd4068 /wd4100 /wd4201 /wd4701 /wd4189 /wd4530 /wd4996 %config_options%
set linker_options=/link /opt:ref /incremental:no /LIBPATH:"C:\src\lib\libssh" "C:\src\lib\libssh\ssh.lib"

pushd %PROJECT_DIR%\export
cl /std:c++17 %PROJECT_DIR%\modules\ssh_native\main_wingui.cpp /Fe:"fac.exe" %compiler_options% %linker_options% 
del *.obj > NUL 2> NUL
popd
