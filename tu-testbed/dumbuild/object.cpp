// object.cpp -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "object.h"
#include "config.h"
#include "exe_target.h"
#include "generic_target.h"
#include "lib_target.h"
#include "target.h"

// Factory.
Res Object::Create(const Context* context, const string& path,
                   const Json::Value& value, Object** object) {
  *object = NULL;

  string name = PathJoin(path, value["name"].asString());

  if (!value.isMember("type")) {
    return Res(ERR_PARSE, "object '" + name + "' has no type");
  }
  if (!value["type"].isString()) {
    return Res(ERR_PARSE, "object type is not a string");
  }

  const string& type = value["type"].asString();
  if (type == "exe") {
    *object = new ExeTarget();
  } else if (type == "lib") {
    *object = new LibTarget();
  } else if (type == "generic") {
    *object = new GenericTarget();
  } else if (type == "config") {
    *object = new Config();
  } else {
    return Res(ERR_PARSE, "unknown type: " + type);
  }

  assert(object);
  return (*object)->Init(context, name, value);
}

Res Object::Init(const Context* context, const string& name,
                 const Json::Value& value) {
  name_ = name;
  context->LogVerbose(StringPrintf("Object::Init(): %s\n", name_.c_str()));
  if (!IsFileNamePart(name_)) {
    return Res(ERR_PARSE, "Error: object name '" + name_ +
               "' is not valid due to containing a path part.");
  }

  return Res(OK);
}
