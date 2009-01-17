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
class ContentHash;
class ContentHashCache;
class ObjectStore;
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

  bool rebuild_all() const {
    return rebuild_all_;
  }

  void set_rebuild_all(bool ra) {
    rebuild_all_ = ra;
  }

  std::string AbsoluteFile(const std::string& canonical_path,
                           const std::string& filename);
  std::string AbsoluteFile(const char* canonical_path, const char* filename) {
    return AbsoluteFile(std::string(canonical_path), std::string(filename));
  }

  // Parses build config from the specified file.
  Res ReadObjects(const std::string& path, const std::string& filename);

  // Call this when you're done reading configs and are ready to
  // proceed with the build.
  void DoneReading();

  // Does the build.
  Res ProcessTargets() const;

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

  // Returns the active config, if any.
  const Config* GetConfig() const {
    assert(done_reading_);
    return active_config_;
  }

  Res ComputeOrGetFileContentHash(const std::string& filename,
                                  ContentHash* out) const;

  // Access to the persistent object store.
  ObjectStore* GetObjectStore() const {
    return object_store_;
  }

  void set_log_verbose(bool verbose) {
    log_verbose_ = verbose;
  }

  // TODO: add printf-style formatting.
  void Log(const std::string& msg) const;
  void LogVerbose(const std::string& msg) const;

 private:
  Res ParseValue(const std::string& path, const Json::Value& value);
  Res ParseGroup(const std::string& path, const Json::Value& value);

  std::string tree_root_;
  std::string config_name_;
  std::string out_root_;
  bool done_reading_;
  bool rebuild_all_;
  const Config* active_config_;
  std::map<std::string, Config*> configs_;
  std::map<std::string, Target*> targets_;
  bool log_verbose_;

  ContentHashCache* content_hash_cache_;
  ObjectStore* object_store_;
};

#endif  // CONTEXT_H_
