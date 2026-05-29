@echo off
title Museo Robot - Exhibicion
color 0B
cls
echo.
echo  =====================================================
echo    PLATAFORMA ROBOTICA EDUCATIVA MODULAR
echo    Iniciando servidor local para exhibicion...
echo  =====================================================
echo.
echo  Se abrira automaticamente el navegador.
echo  Cierra esta ventana para detener el servidor.
echo.

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0servidor.ps1"

pause
