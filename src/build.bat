@echo off

set CFLAGS=/FC /Zi /EHa /TC
set CFLAGS=/Wall /WX /wd4214 /wd4820 /wd4127 /wd4255 /wd4201 /wd4668 /wd4710 /wd4711 /wd4706 /wd4996 /wd4189 /wd4100 /wd4702 /wd4464 %CFLAGS%
rem mongoose related flags
set CFLAGS=/wd4574 /wd4242 /wd4244 /wd4311 /wd4245 /wd4267 /wd4456 /wd4777 /wd4548 %CFLAGS%
set CFLAGS=/DMG_ENABLE_THREADS /DMG_DISABLE_HTTP_WEBSOCKET %CFLAGS%
set CFLAGS=/DNANOCUBE_HTTP %CFLAGS%

set DEBUG=/DEBUG /DCHECK_ASSERTIONS /Od %CFLAGS%
set RELEASE=/DEBUG /DCHECK_ASSERTIONS /O2 %CFLAGS%
rem /DLOG_INSERT_RECURSIVE

rem /TC     All sources are c regardless of extension
rem /EHa    Catches C++ exceptions
rem /FC     Display full path of source code files passed to cl.exe in diagnostic text.
rem /Od     Disables optimization
rem /O2     Creates fast code
rem /MTd    Creates a debug multithreaded executable file using LIBCMTD.lib
rem /Zi     Generates complete debugging information.
rem         implies /DEBUG

rem set OPTIONS=%DEBUG%
set OPTIONS=%RELEASE%

rem -wd1769
rem -wd4201 -wd4100 -wd4189 -wd4505 -wd4127
rem -DLOG_RECURSIVE_INSERT
rem force warnings as errors: Wall -WX

set BUILD=..\..\build\compressed_nanocube
set SRC=..\..\compressed_nanocube\src

rem LOG_INSERT_RECURSIVE CHECK_ASSERTIONS

IF NOT EXIST %BUILD% mkdir %BUILD%

pushd %BUILD%
del *.pdb > NUL 2> NUL

cl ^
%OPTIONS%  ^
%SRC%\nanocube_app.c ^
/Fm:nanocube_app.map ^
/LD ^
/link ^
/incremental:no ^
/opt:ref ^
/PDB:nanocube_app.pdb ^
/export:application_process_request

cl ^
%OPTIONS%  ^
%SRC%\win32_nanocube_app.c ^
%SRC%\..\thirdparty\mongoose\mongoose.c ^
/Fe:nanocube.exe ^
/Fm:win32_nanocube_app.map ^
/link ^
/PDB:nanocube.pdb ^
advapi32.lib

rem cl %OPTIONS% %SRC%\win32_nanocube_test.c /Fe:nctest /Fm:win32_nctest.map /link /pdb:nctest.pdb mongoose.lib

popd








rem mongoose.dll
rem cl %OPTIONS% %SRC%\nanocube_count.c -Fmnanocube_count.map -LD /link -incremental:no -opt:ref -PDB:nanocube_count_%random%.pdb -EXPORT:application_process_request
rem %CommonLinkerFlags%
rem cl %OPTIONS% %SRC%\nanocube_count.c -Fmnanocube_count.map -LD /link -incremental:no -opt:ref -PDB:nanocube_count.pdb -EXPORT:application_process_request
rem cl %MONGOOSE% %SRC%\..\thirdparty\mongoose\mongoose.c -Fmmongoose.map -LD /link -incremental:no -opt:ref -PDB:mongoose.pdb advapi32.lib
rem cl %OPTIONS% %SRC%\win32_nanocube_count.c -Fencc.exe -Fmwin32_nanocube_count.map /link %CommonLinkerFlags% -PDB:ncc.pdb
rem cl %OPTIONS% %SRC%\nanocube_draw.c -Fmnanocube_draw.map -LD /link -incremental:no -opt:ref -PDB:nanocube_draw_%random%.pdb -EXPORT:application_process_request
rem cl %OPTIONS% %SRC%\win32_nanocube_draw.c -Fmwin32_nanocube_draw.map /link %CommonLinkerFlags%
rem cl %OPTIONS% %SRC%\win32_test_parser.c
rem cl %OPTIONS% %SRC%\win32_test_measure.c
rem %SRC%\nanocube_alloc.cc %SRC%\nanocube_index.cc %SRC%\nanocube_count.cc %SRC%\nanocube_platform.cc
rem cl %OPTIONS% %SRC%\test.cc %SRC%\nanocube_alloc.cc %SRC%\nanocube_index.cc %SRC%\nanocube_platform.cc
rem cl %OPTIONS% /c %SRC%\nanocube_index.cc
rem cl /c %SRC%\nanocube_alloc.c -Fmnanocube_alloc.map
rem cl %OPTIONS% -Tc %SRC%\win32_nanocube_test.c
rem cl %OPTIONS% -Tc %SRC%\win32_nanocube_test.c
rem cl %OPTIONS% -Tc %SRC%\win32_nanocube_count.c
rem cl %OPTIONS% -Tc %SRC%\win32_nanocube_index.c
rem cl %OPTIONS% -Tc %SRC%\win32_nanocube_count.c
rem cl -c %OPTIONS% -Tc %SRC%\nanocube_index.c
rem cl %OPTIONS% %SRC%\win32_nanocube_test.cc
rem cl %OPTIONS% %SRC%\win32_nanocube_index.cc
rem cl %OPTIONS% %SRC%\win32_nanocube_count.cc
