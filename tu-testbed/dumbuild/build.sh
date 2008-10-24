#!/bin/bash

# Simple build script to bootstrap a build of dmb.exe

# Windows:
# TODO if [ platform == 'windows' ] then ; blah blah

mkdir -p dmb_out/bootstrap
cd dmb_out/bootstrap

# Make lib_json.lib
cl -c -GX ../../jsoncpp/src/lib_json/*.cpp -I../../jsoncpp/include
lib /OUT:lib_json.lib json_reader.obj json_value.obj json_writer.obj

# Make dmb.exe
cl -Fedmb.exe \
  ../../config.cpp \
  ../../context.cpp \
  ../../dumbuild.cpp \
  ../../exe_target.cpp \
  ../../object.cpp \
  ../../os.cpp \
  ../../res.cpp \
  ../../target.cpp \
  ../../util.cpp \
  -Zi -GX -I../../jsoncpp/include -link lib_json.lib -subsystem:console

cd ../..
