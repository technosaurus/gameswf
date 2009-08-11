// context.h -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// In dumbuild, a Context holds all the info necessary to execute a
// build.  This includes all the relevant Targets, plus any additional
// config data, and the values of command-line arguments.

#ifndef CONTEXT_H_
#define CONTEXT_H_

#include "dmb_types.h"
#include "hash.h"
#include "res.h"
#include "util.h"

class Config;
class ObjectStore;
class Target;

// Context is the whole set of targets and metadata that defines the
// project's build information.
class Context {
 public:
  Context();
  ~Context();

  Res ProcessArgs(int argc, const char** argv);

  Res Init(const string& root_path, const string& canonical_currdir);

  // Absolute path of project root dir.
  const string& tree_root() const {
    return tree_root_;
  }

  // Output dir, relative to tree_root.
  const string& out_root() const {
    return out_root_;
  }

  // "vc8", "gcc", etc
  const string& config_name() const {
    return config_name_;
  }

  const map<string, Target*>& targets() const {
    return targets_;
  }

  bool rebuild_all() const {
    return rebuild_all_;
  }

  void set_rebuild_all(bool ra) {
    rebuild_all_ = ra;
  }

  Target* main_target() const {
    return main_target_;
  }

  // Access to command-line args.  The name should be the long name of
  // the argument (in case it has a short alias).
  bool HasArg(const char* argname) const;
  string GetArgValue(const char* argname) const;

  string AbsoluteFile(const string& canonical_path,
                           const string& filename);
  string AbsoluteFile(const char* canonical_path, const char* filename) {
    return AbsoluteFile(string(canonical_path), string(filename));
  }

  // Parses build config from the specified file.
  Res ReadObjects(const string& path, const string& filename);

  // Call this when you're done reading configs and are ready to
  // proceed with the build.
  void DoneReading();

  // Does the build.
  Res ProcessTargets() const;

  // Takes ownership of c.
  void AddConfig(const string& name, Config* c) {
    configs_.insert(make_pair(name, c));
    // TODO
  }

  // Takes ownership of t.
  void AddTarget(const string& name, Target* t) {
    targets_.insert(make_pair(name, t));
  }

  Target* GetTarget(const string& name) const {
    map<string, Target*>::const_iterator it = targets_.find(name);
    if (it == targets_.end()) {
      return NULL;
    }
    return it->second;
  }

  Target* GetOrLoadTarget(const string& name) {
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

  // Returns the named config, if any.  Returns NULL if we don't have
  // the named config.
  Config* GetNamedConfig(const char* name) const {
    map<string, Config*>::const_iterator it = configs_.find(name);
    if (it == configs_.end()) {
      return NULL;
    }
    return it->second;
  }

  Res ComputeOrGetFileContentHash(const string& filename,
                                  Hash* out) const;

  // Access to the persistent object store.
  ObjectStore* GetObjectStore() const {
    return object_store_;
  }

  // Access to the dep_hash cache.
  HashCache<Hash>* GetDepHashCache() const {
    return dep_hash_cache_;
  }

  void set_log_verbose(bool verbose) {
    log_verbose_ = verbose;
  }

  // TODO: add printf-style formatting.
  void Log(const string& msg) const;
  void LogVerbose(const string& msg) const;

 private:
  Res ParseValue(const string& path, const Json::Value& value);
  Res ParseGroup(const string& path, const Json::Value& value);

  string tree_root_;
  string config_name_;
  string out_root_;
  bool done_reading_;
  bool rebuild_all_;
  const Config* active_config_;
  map<string, Config*> configs_;
  map<string, Target*> targets_;
  map<string, string> args_;
  bool log_verbose_;
  string specified_target_;
  string main_target_name_;
  Target* main_target_;

  HashCache<string>* content_hash_cache_;
  HashCache<Hash>* dep_hash_cache_;
  ObjectStore* object_store_;
};

#endif  // CONTEXT_H_
