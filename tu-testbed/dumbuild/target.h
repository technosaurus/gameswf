// target.h -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Base class for dumbuild Target type.  A Target has a canonical
// name, a type, and dependencies.  Subclasses define different
// behavior.
//
// There are basically three stages of Target processing:
//
// 1. Init() -- construct the Target based on its .dmb declaration data.
//
// 2. Resolve() -- make sure dependencies are known/loaded and resolved.
//
// 3. Process() -- make sure dependencies are processed, then do the
//    build action for this target.

#ifndef TARGET_H_
#define TARGET_H_

#include <assert.h>
#include <json/json.h>
#include <map>
#include <string>

#include "context.h"
#include "object.h"
#include "path.h"
#include "res.h"

class Target : public Object {
 public:
  Target() : resolved_(false), processed_(false), resolve_recursion_(0) {
  }
  
  virtual Res Init(const Context* context,
		   const std::string& name,
		   const Json::Value& value);

  virtual Target* CastToTarget() {
    return this;
  }

  bool resolved() const {
    return resolved_;
  }

  bool processed() const {
    return resolved_;
  }

  // List of src files.
  const std::vector<std::string>& src() const {
    return src_;
  }
	
  // List of other targets we depend on.
  const std::vector<std::string>& dep() const {
    return dep_;
  }

  // List of include dirs.
  const std::vector<std::string>& inc_dirs() const {
    return inc_dirs_;
  }

  virtual Res Resolve(Context* context);

  // Helper: does standard processing of dependencies.
  // Returns Res(OK) on success.
  Res ProcessDependencies(const Context* context);

  // Build the target.
  virtual Res Process(const Context* context) = 0;

 protected:
  bool resolved_, processed_;
  int resolve_recursion_;
  std::vector<std::string> src_, dep_, inc_dirs_;
};

#endif  // TARGET_H_
