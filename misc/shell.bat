@echo off

REM %windir%\system32\cmd.exe /k w:\shell.bat

call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
set path=w:\;w:\build\compressed_nanocube;"C:\Program Files (x86)\Git\bin";"C:\Program Files (x86)\Graphviz2.38\bin";%path%
