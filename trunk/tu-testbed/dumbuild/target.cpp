// target.cpp -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "target.h"
#include "os.h"
#include "util.h"

Target::Target()
    : resolved_(false), processed_(false), dep_hash_was_set_(false),
      resolve_recursion_(0) {
}

static Res ParseCanonicalPathList(const Target* t, const Json::Value& list,
                                   vector<string>* dirs) {
  if (!list.isObject() && !list.isArray()) {
    return Res(ERR_PARSE, "value is not object or array: " +
               list.toStyledString());
  }

  for (Json::Value::iterator it = list.begin();
       it != list.end();
       ++it) {
    if (!(*it).isString()) {
      return Res(ERR_PARSE, "list value is not a string: " +
                 (*it).toStyledString());
    }
    string dir_name = Canonicalize(t->base_dir(), (*it).asString());
    dirs->push_back(dir_name);
  }
  return Res(OK);
}

Res Target::Init(const Context* context,
                 const string& name,
                 const Json::Value& value) {
  Res res = Object::Init(context, name, value);
  if (!res.Ok()) {
    return res;
  }

  context->LogVerbose(StringPrintf("Target::Init(): %s\n", name_.c_str()));

  // Initialize dependencies.
  if (value.isMember("dep")) {
    Json::Value deplist = value["dep"];
    if (!deplist.isObject() && !deplist.isArray()) {
      return Res(ERR_PARSE,
                 name_ + ": dep value is not object or array: " +
                 deplist.toStyledString());
    }

    for (Json::Value::iterator it = deplist.begin();
         it != deplist.end();
         ++it) {
      if (!(*it).isString()) {
        return Res(ERR_PARSE, name_ + ": dep list value is not a string: " +
                   (*it).toStyledString());
      }
      string depname = Canonicalize(base_dir(), (*it).asString());

      dep_.push_back(depname);
    }
  }

  // Initialize source list.
  if (value.isMember("src")) {
    Json::Value srclist = value["src"];
    if (!srclist.isObject() && !srclist.isArray()) {
      return Res(ERR_PARSE,
                 name_ + ": src value is not object or array: " +
                 srclist.toStyledString());
    }

    for (Json::Value::iterator it = srclist.begin();
         it != srclist.end();
         ++it) {
      if (!(*it).isString()) {
        return Res(ERR_PARSE, name_ + ": src list value is not a string: " +
                   (*it).toStyledString());
      }
      string srcname = Canonicalize(base_dir(), (*it).asString());
      src_.push_back(srcname);
    }
  }

  // Initialize include dirs.
  if (value.isMember("inc_dirs")) {
    Json::Value inclist = value["inc_dirs"];
    res = ParseCanonicalPathList(this, inclist, &inc_dirs_);
    if (!res.Ok()) {
      res.AppendDetail("while parsing inc_dirs in " + name_);
      return res;
    }
  }
  // TODO: collect additional inc dirs from dependencies (?).

  // Initialize include dirs that should be passed on to dependents.
  if (value.isMember("dep_inc_dirs")) {
    Json::Value dep_inclist = value["dep_inc_dirs"];
    res = ParseCanonicalPathList(this, dep_inclist, &dep_inc_dirs_);
    if (!res.Ok()) {
      res.AppendDetail("while parsing dep_inc_dirs in " + name_);
      return res;
    }
  }

  if (value.isMember("linker_flags")) {
    Json::Value srclist = value["linker_flags"];
    res = ParseValueStringOrMap(context, NULL, srclist, "=", ";",
                                &linker_flags_);
    if (!res.Ok()) {
      res.AppendDetail("\nwhile evaluating linker_flags field of target " +
                       name_);
      return res;
    }
  }

  return Res(OK, "");
}

Res Target::Resolve(Context* context) {
  ScopedIncrement inc(&resolve_recursion_);

  if (resolve_recursion_ > 1) {
    // Cycle!  Stop the cycle and log the names of targets in the
    // dependency chain.
    return Res(ERR_DEPENDENCY_CYCLE, name());
  }

  if (!resolved()) {
    context->LogVerbose(StringPrintf("Target Resolve: %s\n", name_.c_str()));

    // Try to Resolve() all our dependencies.
    for (size_t i = 0; i < dep().size(); i++) {
      Target* dependency = NULL;
      Res res = context->GetOrLoadTarget(dep()[i], &dependency);
      if (!res.Ok()) {
        return res;
      }

      assert(dependency);
      res = dependency->Resolve(context);
      if (!res.Ok()) {
        if (res.value() == ERR_DEPENDENCY_CYCLE) {
          // Tack on our name to the error message, so the whole
          // cycle is logged for the user.
          res.AppendDetail("\nreferred to by " + name());
          return res;
        }
        return res;
      }
    }
  }

  resolved_ = true;
  return Res(OK);
}

Res Target::ProcessDependencies(const Context* context) {
  context->LogVerbose(StringPrintf("Target ProcessDependencies: %s\n",
                                   name_.c_str()));
  // TODO: once dmb does a topological sort of targets, this can just
  // be a series of asserts to make sure it worked right.
  
  // Try to Process() all our dependencies.
  for (size_t i = 0; i < dep().size(); i++) {
    Target* dependency = context->GetTarget(dep()[i]);
    assert(dependency);
    Res res = dependency->Process(context);
    if (!res.Ok()) {
      return res;
    }
    assert(dependency->processed());
  }

  return Res(OK);
}

Res Target::BuildOutDirAndSetupPaths(const Context* context) {
  string out_dir = PathJoin(context->out_root(), base_dir());
  Res res = CreatePath(context->tree_root(), out_dir);
  if (!res.Ok()) {
    res.AppendDetail("\nwhile creating output path for " + name_);
    return res;
  }

  absolute_out_dir_ = PathJoin(context->tree_root(), out_dir);

  relative_path_to_tree_root_ = "../";
  const char* slash = strchr(out_dir.c_str(), '/');
  while (slash && *(slash + 1)) {
    relative_path_to_tree_root_ += "../";
    slash = strchr(slash + 1, '/');
  }

  return res;
}

string Target::GetLinkerArgs(const Context* context) const {
  return "";
}
