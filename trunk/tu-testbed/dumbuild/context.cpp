// context.cpp -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include <stdio.h>
#include "context.h"
#include "config.h"
#include "dmb_types.h"
#include "hash.h"
#include "hash_util.h"
#include "object_store.h"
#include "os.h"
#include "path.h"
#include "target.h"
#include "test.h"

Context::Context() : done_reading_(false),
                     rebuild_all_(false),
                     active_config_(NULL),
                     log_verbose_(false),
                     main_target_(NULL),
                     content_hash_cache_(NULL),
                     dep_hash_cache_(NULL),
                     object_store_(NULL) {
  config_name_ = ":default";

  content_hash_cache_ = new HashCache<string>();
  dep_hash_cache_ = new HashCache<Hash>();
}

Context::~Context() {
  for (map<string, Target*>::iterator it = targets_.begin();
       it != targets_.end();
       ++it) {
    delete it->second;
  }
  for (map<string, Config*>::iterator it = configs_.begin();
       it != configs_.end();
       ++it) {
    delete it->second;
  }

  delete object_store_;
  delete content_hash_cache_;
  delete dep_hash_cache_;
}

// Command-line arg convention:
//
// * if it starts with one dash, the rest of the raw string is the
//   name, and any value is in the next arg.
//
// * if it starts with two dashes, the value (if any) is in the same
//   string, following an '=' character.
//
// Examples:
//
// -r                     --> argname is "r", has no value
// --rebuild              --> argname is "rebuild", has no value
// -c :vc8-debug          --> argname is "c", value is "vc8-debug"
// --c=:vc8-debug         --> argname is "c", value is "vc8-debug"
// -changedir /path/dir   --> argname is "changedir", value is "/path/dir"
// --changedir=/path/dir  --> argname is "changedir", value is "/path/dir"

// Given a raw cmd line arg string, extracts the name of the arg
// (filtering out the leading '-' characters and ignoring any value
// part after a '=' sign).
static void ParseArgName(const char* arg, string* argname) {
  if (arg[0] && arg[0] == '-') {
    if (arg[1] == '-') {
      // Two-dash format.
      const char* argstart = arg + 2;
      const char* argend = strchr(argstart, '=');
      if (argend) {
        *argname = string(argstart, argend - argstart);
      } else {
        *argname = argstart;
      }
    } else {
      // One-dash format.
      *argname = (arg + 1);
    }
  }
}

// Fills *argvalue with the value of the argument at argv[i].  Returns
// 0 if the arg value was found within argv[i], or 1 if the value was
// found in argv[i + 1] (and therefore argv[i + 1] should be skipped
// in further arg processing).
static int ParseArgValue(int i, int argc, const char** argv, string* argvalue) {
  const char* arg = argv[i];
  if (arg[0] && arg[0] == '-') {
    if (arg[1] == '-') {
      // Two-dash format.
      const char* argend = strchr(arg + 2, '=');
      if (argend) {
        *argvalue = argend + 1;
      }
      return 0;
    } else {
      // One-dash format.
      if (i + 1 < argc) {
        *argvalue = argv[i + 1];
        return 1;
      } else {
        // No value.
        return 0;
      }
    }
  }
  assert(0);
  return 0;
}

Res Context::ProcessArgs(int argc, const char** argv) {
  for (int i = 1; i < argc; i++) {
    const char* arg = argv[i];
    if (arg[0] == '-') {
      string argname;
      ParseArgName(arg, &argname);
      args_[argname] = "";

      // Parse the special value-less args.
      if (argname == "r" || argname == "rebuild") {
        // Rebuild all.
        set_rebuild_all(true);
      } else if (argname == "v" || argname == "verbose") {
        // Verbose.
        set_log_verbose(true);
      } else if (argname == "test") {
        // Run the self-tests.
        RunSelfTests();
        printf("Self tests OK\n");
        exit(0);
      } else {
        // Other args should have a value.
        string argvalue;
        i += ParseArgValue(i, argc, argv, &argvalue);
        args_[argname] = argvalue;
        if (argname == "C" || argname == "changedir") {
          // Change directory.
          if (!argvalue.length()) {
            return Res(ERR_COMMAND_LINE, "-changedir option requires a directory");
          }
          Res res = ChangeDir(argvalue.c_str());
          if (!res.Ok()) {
            res.AppendDetail("\nWhile processing argument -C");
            return res;
          }
        } else if (argname == "c" || argname == "config") {
          // Config.
          if (!argvalue.length()) {
            return Res(ERR_COMMAND_LINE, "-c option requires a config name");
          }
          config_name_ = argv[i];
        } else {
          // Non-builtin arg; maybe it is used by .dmb instructions.
          //
          // TODO(tulrich): figure out a way to check for expected
          // args, so we can give a useful error in case of arg
          // mispelling etc.
        }
      }
    }
  }

  return Res(OK);
}

Res Context::Init(const string& root_path, const string& canonical_currdir) {
  tree_root_ = root_path;
  out_root_ = PathJoin("dmb-out", CanonicalFilePart(config_name_));

  Res res = CreatePath(tree_root_, "dmb-out/ostore");
  if (!res.Ok()) {
    return res;
  }
  object_store_ = new ObjectStore((tree_root_ + "/dmb-out/ostore").c_str());

  res = ReadObjects("", AbsoluteFile("", "root.dmb"));
  if (!res.Ok()) {
    return res;
  }

  // TODO parse specified target
  string target_dir = "";
  main_target_name_ = ":default";

  res = ReadObjects(target_dir, AbsoluteFile(target_dir, "build.dmb"));
  if (!res.Ok()) {
    return res;
  }

  main_target_ = GetTarget(main_target_name_);
  if (!main_target_) {
    return Res(ERR_UNKNOWN_TARGET, main_target_name_);
  }

  DoneReading();
  if (!GetConfig()) {
    return Res(ERR, StringPrintf("config '%s' is not defined",
                                 config_name().c_str()));
  }

  return Res(OK);
}

Res Context::ReadObjects(const string& path, const string& filename) {
  assert(!done_reading_);

  // Slurp in the file.
  string file_data;
  FILE* fp = fopen(filename.c_str(), "r");
  if (!fp) {
    fprintf(stderr, "Can't open build file '%s'\n", filename.c_str());
    exit(1);
  }
  for (;;) {
    int c = fgetc(fp);
    if (c == EOF) {
      break;
    }
    file_data += c;
  }
  fclose(fp);

  // Parse.
  Json::Reader reader;
  Json::Value root;
  if (reader.parse(file_data, root, false)) {
    // Parse OK.
    // Iterate through the values and store the objects.
    Res result = ParseGroup(path, root);
    if (!result.Ok()) {
      return result;
    }
  } else {
    return Res(ERR_PARSE, "Parse error in file " + filename + "\n" +
               reader.getFormatedErrorMessages());
  }

  return Res(OK);
}

void Context::DoneReading() {
  done_reading_ = true;

  // Look up the active config.
  map<string, Config*>::const_iterator it =
    configs_.find(config_name_);
  if (it != configs_.end()) {
    active_config_ = it->second;
  }
}

Res Context::ProcessTargets() const {
  const map<string, Target*>& targs = targets();
  for (map<string, Target*>::const_iterator it = targs.begin();
       it != targs.end();
       ++it) {
    Target* t = it->second;
    if (t->resolved()) {
      Res res = t->Process(this);
      if (!res.Ok()) {
        return res;
      }
    }
  }
  return Res(OK);
}

Res Context::ParseValue(const string& path, const Json::Value& value) {
  if (!value.isObject()) {
    return Res(ERR_PARSE, "object is not a JSON object");
  }

  if (!value.isMember("name")) {
    return Res(ERR_PARSE, "object lacks a name");
  }

  // TODO store the current path with the object.
  Object* object = NULL;
  Res create_result = Object::Create(this, path, value, &object);
  if (create_result.Ok()) {
    assert(object);
    Target* target = object->CastToTarget();
    if (target) {
      AddTarget(target->name(), target);
    } else {
      Config* config = object->CastToConfig();
      if (config) {
        AddConfig(config->name(), config);
      } else {
        return Res(ERR, "Object is neither config nor target, name = " +
		   object->name());
      }
    }
  } else {
    delete object;
  }

  return create_result;
}

Res Context::ParseGroup(const string& path, const Json::Value& value) {
  if (!value.isObject() && !value.isArray()) {
    return Res(ERR_PARSE, "group is not an object or array");
  }

  // iterate
  for (Json::Value::const_iterator it = value.begin();
       it != value.end();
       ++it) {
    Res result = ParseValue(path, *it);
    if (!result.Ok()) {
      return result;
    }
  }

  return Res(OK);
}

bool Context::HasArg(const char* argname) const {
  return args_.find(argname) != args_.end();
}

string Context::GetArgValue(const char* argname) const {
  if (HasArg(argname)) {
    return args_.find(argname)->second;
  }
  assert(0);
  return "";
}

string Context::AbsoluteFile(const string& canonical_path,
                                  const string& filename) {
  return PathJoin(tree_root(), PathJoin(canonical_path, filename));
}

Res Context::ComputeOrGetFileContentHash(const string& filename,
                                         Hash* out) const {
  if (content_hash_cache_->Get(filename, out)) {
    return Res(OK);
  }
  out->Reset();
  Res res = out->AppendFile(filename);
  if (!res.Ok()) {
    return res;
  }

  content_hash_cache_->Insert(filename, *out);
  return Res(OK);
}

void Context::Log(const string& msg) const {
  fputs(msg.c_str(), stdout);
  fflush(stdout);
}

void Context::LogVerbose(const string& msg) const {
  if (log_verbose_) {
    Log(msg);
  }
}
