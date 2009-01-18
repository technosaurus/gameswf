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

#include "context.h"
#include "dmb_types.h"
#include "object.h"
#include "path.h"
#include "res.h"

class Target : public Object {
 public:
  Target();
  
  virtual Res Init(const Context* context,
		   const string& name,
		   const Json::Value& value);

  virtual Target* CastToTarget() {
    return this;
  }

  bool resolved() const {
    return resolved_;
  }

  bool processed() const {
    return processed_;
  }

  // List of src files.
  const vector<string>& src() const {
    return src_;
  }
	
  // List of other targets we depend on.
  const vector<string>& dep() const {
    return dep_;
  }

  // List of include dirs.
  const vector<string>& inc_dirs() const {
    return inc_dirs_;
  }

  // Relative path from the target's compile output directory back up
  // to the tree root.
  const string& relative_path_to_tree_root() const {
    return relative_path_to_tree_root_;
  }

  // Absolute path to the target's compile output directory.
  const string& absolute_out_dir() const {
    return absolute_out_dir_;
  }

  virtual Res Resolve(Context* context);

  // Helper: does standard processing of dependencies.
  // Returns Res(OK) on success.
  Res ProcessDependencies(const Context* context);

  // Helper: does standard setup of paths & makes the output
  // directory.
  Res BuildOutDirAndSetupPaths(const Context* context);

  // Build the target.
  virtual Res Process(const Context* context) = 0;

  // Link dependencies will need to re-link.
  bool did_rebuild() const {
    return did_rebuild_;
  }

 protected:
  bool resolved_, processed_;
  bool did_rebuild_;
  int resolve_recursion_;
  vector<string> src_, dep_, inc_dirs_;
  string relative_path_to_tree_root_;
  string absolute_out_dir_;
};

#endif  // TARGET_H_
