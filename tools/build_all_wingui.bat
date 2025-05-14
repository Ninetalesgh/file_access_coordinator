@echo off

call %~dp0\build_wingui.bat debug
call %~dp0\build_wingui.bat simple_debug
call %~dp0\build_wingui.bat simple
call %~dp0\build_wingui.bat console