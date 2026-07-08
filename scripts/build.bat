@echo off
setlocal
if "%BUILD_DIR%"=="" set BUILD_DIR=build
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release
cmake -S . -B %BUILD_DIR% -DCMAKE_BUILD_TYPE=%BUILD_TYPE% || exit /b 1
cmake --build %BUILD_DIR% --config %BUILD_TYPE% -j || exit /b 1
echo Built: %BUILD_DIR%\%BUILD_TYPE%\jaishi.exe
endlocal
