@echo off
rem Simple Windows batch file to bootstrap a build of dmb.exe

rem It assumes you have MSVC set up in your path (i.e. with PATH, LIB
rem and INCLUDE set appropriately).
rem TODO: add some heuristics for finding the compiler automatically.

setlocal
pushd %~dp0

mkdir dmb-out
mkdir dmb-out\bootstrap
cd dmb-out\bootstrap

rem Make lib_json.lib
cl -nologo -c -GX ../../jsoncpp/src/lib_json/*.cpp -I../../jsoncpp/include
lib -nologo -OUT:lib_json.lib json_reader.obj json_value.obj json_writer.obj

rem Make dmb.exe
set srcs=^
 ../../compile_util.cpp ^
 ../../config.cpp ^
 ../../context.cpp ^
 ../../dumbuild.cpp ^
 ../../exe_target.cpp ^
 ../../file_deps.cpp ^
 ../../hash.cpp ^
 ../../hash_util.cpp ^
 ../../lib_target.cpp ^
 ../../object.cpp ^
 ../../object_store.cpp ^
 ../../os.cpp ^
 ../../res.cpp ^
 ../../sha1.cpp ^
 ../../target.cpp ^
 ../../util.cpp 
cl -nologo %srcs% -Fedmb.exe -Zi -GX -I../../jsoncpp/include -link lib_json.lib -subsystem:console

popd
