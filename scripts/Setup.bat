@echo off

pushd ..
call Walnut\vendor\bin\premake\Windows\premake5.exe vs2022
popd
pause