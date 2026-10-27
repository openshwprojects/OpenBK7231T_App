@echo off
pushd "%~dp0\.."
call build_scripts\build_bl602.bat dev_bl602 build
popd
