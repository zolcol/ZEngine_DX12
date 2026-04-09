@echo off
setlocal enabledelayedexpansion
title Trinh quet code C++ toan bo thu muc

:: Chuyen den thu muc chua file bat
cd /d "%~dp0"

echo =====================================================
echo    DANG QUET TOAN BO FILE C++ TRONG THU MUC NAY
echo    (Bao gom tat ca cac thu muc con)
echo =====================================================
echo 1. Dem tat ca (Full)
echo 2. Dem tinh gon (Loai bo dong trong va comment //)
echo =====================================================

set /p choice="Nhap lua chon (1 hoac 2): "
set "totalLines=0"
set "fileCount=0"

if "%choice%"=="1" (
    :: Quet toan bo thu muc con voi /r
    for /r %%f in (*.cpp *.hpp *.h *.cxx *.cc) do (
        if exist "%%f" (
            set "fileLineCount=0"
            for /f %%a in ('type "%%f" ^| find /c /v ""') do set "fileLineCount=%%a"
            set /a totalLines+=fileLineCount
            set /a fileCount+=1
            echo [+] %%f: !fileLineCount! dong
        )
    )
) else if "%choice%"=="2" (
    for /r %%f in (*.cpp *.hpp *.h *.cxx *.cc) do (
        if exist "%%f" (
            set "fileLineCount=0"
            for /f "usebackq delims=" %%a in ("%%f") do (
                set "line=%%a"
                :: Loai bo khoang trang o dau dong de kiem tra comment
                for /f "tokens=* delims= " %%b in ("!line!") do set "trimmed=%%b"
                set "start=!trimmed:~0,2!"
                if not "!trimmed!"=="" if not "!start!"=="//" (
                    set /a fileLineCount+=1
                )
            )
            set /a totalLines+=fileLineCount
            set /a fileCount+=1
            echo [+] %%f: !fileLineCount! dong
        )
    )
) else (
    echo Lua chon sai.
    pause
    exit /b
)

echo.
echo =====================================================
echo KET QUA THONG KE:
echo - Tong so file tim thay: %fileCount%
echo - Tong so dong code:    %totalLines%
echo =====================================================
pause