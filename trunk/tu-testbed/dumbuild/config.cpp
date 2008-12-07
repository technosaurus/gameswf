// config.cpp -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "config.h"
#include "util.h"

Res Config::Init(const Context* context, const std::string& name,
                 const Json::Value& value) {
  Res res = Object::Init(context, name, value);
  if (!res.Ok()) {
    return res;
  }

  assert(value.isObject() || value.isArray());

  std::string null_char;
  null_char += '\0';

  if (value.isMember("compile_environment")) {
    res = ParseValueStringOrMap(value["compile_environment"], "=", null_char,
                                &compile_environment_);
    if (!res.Ok()) {
      res.AppendDetail("\nwhile initializing compile_environment of config: " +
                       name_);
      return res;
    }
    compile_environment_ += null_char;
  }

  if (value.isMember("compile_template")) {
    res = ParseValueStringOrMap(value["compile_template"], "=", ";",
                                &compile_template_);
    if (!res.Ok()) {
      res.AppendDetail("\nwhile initializing compile_template of config: " +
                       name_);
      return res;
    }
  }

  if (value.isMember("link_template")) {
    res = ParseValueStringOrMap(value["link_template"], "=", ";",
                                &link_template_);
    if (!res.Ok()) {
      res.AppendDetail("\nwhile initializing link_template of config: " +
                       name_);
      return res;
    }
  }

  if (value.isMember("lib_template")) {
    res = ParseValueStringOrMap(value["lib_template"], "=", ";",
                                &lib_template_);
    if (!res.Ok()) {
      res.AppendDetail("\nwhile initializing lib_template of config: " +
                       name_);
      return res;
    }
  }

  if (value.isMember("obj_extension")) {
    if (!value["obj_extension"].isString()) {
      return Res(ERR_PARSE, "obj_extension must be a string value"
                 "\nwhile initializing obj_extension of config: " + name_);
    }
    obj_extension_ = value["obj_extension"].asString();
  }

  if (value.isMember("lib_extension")) {
    if (!value["lib_extension"].isString()) {
      return Res(ERR_PARSE, "lib_extension must be a string value"
                 "\nwhile initializing lib_extension of config: " + name_);
    }
    lib_extension_ = value["lib_extension"].asString();
  }

  return Res(OK);
}
