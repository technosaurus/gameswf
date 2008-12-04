// config.h -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// The Config type holds compiler command-line templates and flags,
// etc.

#ifndef CONFIG_H_
#define CONFIG_H_

#include <map>
#include <string>
#include "object.h"

class Config : public Object {
 public:
  virtual Res Init(const Context* context,
		   const std::string& name,
		   const Json::Value& value);

  virtual Config* CastToConfig() {
    return this;
  }

  const std::string& compile_environment() const {
    return compile_environment_;
  }
  const std::string& compile_template() const {
    return compile_template_;
  }
  const std::string& link_template() const {
    return link_template_;
  }
  const std::string& obj_extension() const {
    return obj_extension_;
  }

  // TODO
 private:
  std::string compile_environment_;
  std::string compile_template_;
  std::string link_template_;
  std::string obj_extension_;
};

#endif  // CONFIG_H_
