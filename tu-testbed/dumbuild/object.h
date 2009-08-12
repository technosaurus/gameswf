// object.h -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Base class for dumbuild Config and Target types.  An Object has a
// name, a type, and other type-specific attributes.
//

#ifndef OBJECT_H_
#define OBJECT_H_

#include <assert.h>
#include <json/json.h>

#include "context.h"
#include "dmb_types.h"
#include "path.h"
#include "res.h"

class Config;
class Target;

class Object {
 public:
  // Factory.
  static Res Create(const Context* context,
		    const string& path,
		    const Json::Value& value,
		    Object** object);

  Object() {
  }

  virtual ~Object() {
  }
  
  virtual Res Init(const Context* context,
		   const string& name,
		   const Json::Value& value);

  const string& name() const {
    return name_;
  }

  const string& type() const {
    return type_;
  }

  virtual Target* CastToTarget() {
    return NULL;
  }

  virtual Config* CastToConfig() {
    return NULL;
  }

 protected:
  string name_, type_;
};

#endif  // TARGET_H_
