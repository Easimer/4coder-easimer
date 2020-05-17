@echo off

copy 4coder_base_commands.cpp

call ..\bin\buildsuper_x64.bat 4coder_easimer.cpp

move /Y custom_4coder.* ..\..\