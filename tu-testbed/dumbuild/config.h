// config.h -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// The Config type holds compiler command-line templates and flags,
// etc.

#ifndef CONFIG_H_
#define CONFIG_H_

#include "dmb_types.h"
#include "object.h"

class Config : public Object {
 public:
  virtual Res Init(const Context* context,
		   const string& name,
		   const Json::Value& value);

  virtual Config* CastToConfig() {
    return this;
  }

  const string& compile_environment() const {
    return compile_environment_;
  }
  const string& compile_template() const {
    return compile_template_;
  }
  const string& link_template() const {
    return link_template_;
  }
  const string& lib_template() const {
    return lib_template_;
  }
  const string& obj_extension() const {
    return obj_extension_;
  }
  const string& lib_extension() const {
    return lib_extension_;
  }

  // TODO
 private:
  string compile_environment_;
  string compile_template_;
  string link_template_;
  string lib_template_;
  string obj_extension_;
  string lib_extension_;
};

#endif  // CONFIG_H_
