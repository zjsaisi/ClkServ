@ECHO OFF
REM ****************************************************************************
REM **    IXXAT Automation GmbH
REM ****************************************************************************
REM **
REM **       File: GeneSIS_compl.bat
REM **    Summary: This batch file has to be called after each modification of the target.h
REM **                      file in the IEEE1588 V2 stack.  It generates a configuartion wizard file and
REM **                      after that it generates the SIS source code files out of it.
REM ** 
REM **     Author:  Werner Abt
REM **
REM ****************************************************************************

REM first clean old files
del *.ncb
del SISerror.c
del SISmain.c
del SISmbox.c
del SISmpool.c
del SISevent.c
del SIShdls.h
del *.obj
del *.opt
del *.plg
del *.scc
del SIScfg.wzd

REM set path to SIStool folder here
SET SISTOOL_DIR=..\..\..\..\SIStool
REM set path to target file folder
SET TARGET_DIR=..\..\Proj

REM call GenSISwzd.py in the SIStool folder to create the SIScfg.wzd file
%SISTOOL_DIR%\minipy.exe %SISTOOL_DIR%\GenSISwzd.py %TARGET_DIR%\target.h

rem then generate the new SIS-files out of the SIScfg.wzd
%SISTOOL_DIR%\minipy.exe %SISTOOL_DIR%\geneSIS.py 
