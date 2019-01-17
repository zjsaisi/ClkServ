@ECHO OFF
REM ****************************************************************************
REM **    IXXAT Automation GmbH
REM ****************************************************************************
REM **
REM **       File: GeneSIS_step2.bat
REM **    Summary: This batch file has to be called after each modification of the SIS 
REM **                      configuration file SIScfg.wzd. It generates the SIS source code files
REM **                      appropriate to the configurations stored in the SIScfg.wzd file.
REM ** 
REM **     Author:  Werner Abt
REM **
REM ****************************************************************************


REM set path to SIStool folder here
SET SISTOOL_DIR=..\..\..\..\SIStool

rem then generate the new SIS-files out of the SIScfg.wzd
%SISTOOL_DIR%\minipy.exe %SISTOOL_DIR%\geneSIS.py 
