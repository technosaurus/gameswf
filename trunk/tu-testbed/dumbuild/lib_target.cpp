// lib_target.cpp -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "lib_target.h"
#include "compile_util.h"
#include "config.h"
#include "hash_util.h"
#include "os.h"
#include "util.h"

LibTarget::LibTarget() {
  type_ = "lib";
}

Res LibTarget::Init(const Context* context, const string& name,
                    const Json::Value& val) {
  Res res = Target::Init(context, name, val);
  if (res.Ok()) {
    // TODO
    // more lib-specific init???
  }

  return res;
}

Res LibTarget::Resolve(Context* context) {
  context->LogVerbose(StringPrintf("LibTarget Resolve: %s\n", name_.c_str()));
  // Default Resolve() is OK.
  return Target::Resolve(context);
}

// TODO combine this with ExeTarget::Process which is very similar.
Res LibTarget::Process(const Context* context) {
  if (processed()) {
    return Res(OK);
  }

  context->Log(StringPrintf("dmb processing lib %s\n", name_.c_str()));

  Res res;
  res = Target::ProcessDependencies(context);
  if (!res.Ok()) {
    res.AppendDetail("\nwhile processing dependencies of " + name_);
    return res;
  }

  res = BuildOutDirAndSetupPaths(context);
  if (!res.Ok()) {
    return res;
  }

  const Config* config = context->GetConfig();

  // Dephash will include link args plus dephashes of all sources and
  // all dependencies.

  // Read existing build marker, if any.
  string output_fname = CanonicalFilePart(name_) + config->lib_extension();
  Hash previous_dep_hash;
  res = ReadFileHash(absolute_out_dir(), output_fname, &previous_dep_hash);
  // Ignore return value; we don't care!

  assert(dep_hash_was_set_ == false);
  dep_hash_.Reset();
  dep_hash_ << "lib_dep_hash" << name_ << config->lib_template();
  
  CompileInfo ci;
  res = PrepareCompileVars(this, context, &ci, &dep_hash_);
  if (!res.Ok()) {
    return res;
  }

  dep_hash_was_set_ = true;

  bool do_build = context->rebuild_all() || (previous_dep_hash != dep_hash_);
  if (do_build) {
    if (ci.src_list_.size()) {
      res = DoCompile(this, context, ci);
      if (!res.Ok()) {
        return res;
      }
    }
  } else {
    if (ci.src_list_.size()) {
      // This is sort of unexpected; let's print something.
      context->Log("warning: lib_target " + name_ + " has apparently "
                   "not changed, but some component obj files may "
                   "be missing!\n");
    }
  }

  if (do_build) {
    // Archive the objs to make the lib
    string cmd;
    res = FillTemplate(config->lib_template(), ci.vars_, &cmd);
    if (!res.Ok()) {
      res.AppendDetail("\nwhile preparing lib command line for " + name_);
      return res;
    }
    res = RunCommand(absolute_out_dir(), cmd, config->compile_environment());
    if (!res.Ok()) {
      res.AppendDetail("\nwhile making lib " + name());
      res.AppendDetail("\nin directory " + absolute_out_dir());
      return res;
    }

    // Write a build marker.
    res = WriteFileHash(absolute_out_dir(), output_fname, dep_hash_);
    if (!res.Ok()) {
      return res;
    }
  }

  processed_ = true;
  return Res(OK);
}
