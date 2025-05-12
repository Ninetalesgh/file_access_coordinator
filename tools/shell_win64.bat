@echo off 

REM ------------------------------------------------------------------------------------------------
REM --- Add your vcvarsall.bat path to this list ---------------------------------------------------
REM ------------------------------------------------------------------------------------------------

set vcPath[0]="C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" 
set vcPath[1]="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
set vcPath[2]=.
set vcPath[3]=.
set vcPath[4]=.

REM ------------------------------------------------------------------------------------------------
REM ------------------------------------------------------------------------------------------------
REM ------------------------------------------------------------------------------------------------
REM ------------------------------------------------------------------------------------------------

setlocal EnableDelayedExpansion
set /a x=0
:loop 
if defined vcPath[%x%] ( 
  set /a "x+=1"
  if EXIST !vcPath[%x%]! (

    set callPath=!vcPath[%x%]!
    GOTO end
  )  
   GOTO :loop 
)

echo couldn't find vcvarsall.bat, please add your vcvarsall.bat path to the vcPath[] array in /misc/shell.bat

:end
(endlocal & rem call environment setup outside of local scope
call %callPath% x64
)