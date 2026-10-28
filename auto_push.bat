@echo off
title Auto Push to GitHub
echo ===================================================
echo     MENGIRIM PEMBARUAN KE GITHUB (AUTO PUSH)
echo ===================================================
set "PATH=C:\Users\ThinkPad\AppData\Local\Programs\Git\cmd;C:\Users\ThinkPad\AppData\Local\Programs\Git\bin;C:\Users\ThinkPad\AppData\Local\Microsoft\WinGet\Packages\GitHub.cli_Microsoft.Winget.Source_8wekyb3d8bbwe\bin;%PATH%"
cd /d "C:\Monitoring suhu 26"

git add .
git commit -m "Update otomatis: %date% %time%"
git push origin main

echo.
echo ===================================================
echo   BERHASIL! Semua perubahan telah di-push ke GitHub
echo   https://github.com/akmalakhdrl/Monitoring-Suhu
echo ===================================================
timeout /t 5

