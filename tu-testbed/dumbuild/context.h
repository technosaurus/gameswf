// context.h -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// In dumbuild, a Context holds all the info necessary to execute a
// build.  This includes all the relevant Targets, plus any additional
// config data.

#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <map>
#include <string>

#include "res.h"
#include "util.h"

class Config;
class Target;

// Context is the whole set of targets and metadata that defines the
// project's build information.
class Context {
 public:
  Context();
  ~Context();

  Res ProcessArgs(int argc, const char** argv);

  Res Init(const std::string& root_path);

  // Absolute path of project root dir.
  const std::string& tree_root() const {
    return tree_root_;
  }

  // Output dir, relative to tree_root.
  const std::string& out_root() const {
    return out_root_;
  }

  // "vc8", "gcc", etc
  const std::string& config_name() const {
    return config_name_;
  }

  const std::map<std::string, Target*>& targets() const {
    return targets_;
  }

  std::string AbsoluteFile(const std::string& canonical_path,
                           const std::string& filename);
  std::string AbsoluteFile(const char* canonical_path, const char* filename) {
    return AbsoluteFile(std::string(canonical_path), std::string(filename));
  }

  // Takes ownership of c.
  void AddConfig(const std::string& name, Config* c) {
    configs_.insert(make_pair(name, c));
    // TODO
  }

  // Takes ownership of t.
  void AddTarget(const std::string& name, Target* t) {
    targets_.insert(make_pair(name, t));
  }

  Target* GetTarget(const std::string& name) const {
    std::map<std::string, Target*>::const_iterator it = targets_.find(name);
    if (it == targets_.end()) {
      return NULL;
    }
    return it->second;
  }

  Target* GetOrLoadTarget(const std::string& name) {
    Target* target = GetTarget(name);
    if (target) {
      return target;
    }

    // Maybe we need to load.
    // TODO: look for build file in path.  Did we already load it?  If
    // not, load it, and look for target within it.
    Log(StringPrintf("TODO: can't load %s\n", name.c_str()));
    return NULL;
  }

  const Config* GetConfig() const {
    std::map<std::string, Config*>::const_iterator it =
      configs_.find(config_name_);
    if (it == configs_.end()) {
      return NULL;
    }
    return it->second;
  }

  void SetLogVerbose(bool verbose) {
    log_verbose_ = verbose;
  }

  void Log(const std::string& msg) const;
  void LogVerbose(const std::string& msg) const;

 private:
  std::string tree_root_;
  std::string config_name_;
  std::string out_root_;
  std::map<std::string, Config*> configs_;
  std::map<std::string, Target*> targets_;
  bool log_verbose_;
};

#endif  // CONTEXT_H_
