// lib_target.cpp -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "lib_target.h"
#include "compile_util.h"
#include "config.h"
#include "os.h"
#include "util.h"

// Re-archive if:
// * any source file or dep was recompiled
// * lib_template changed

LibTarget::LibTarget() {
  type_ = "lib";
}

Res LibTarget::Init(const Context* context, const std::string& name,
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

  bool any_compile = false;
  CompileInfo ci;
  res = PrepareCompileVars(this, config, &ci);
  if (res.value() == ERR_DONT_REBUILD) {
    // We don't need to compile here.
    context->LogVerbose(StringPrintf("skipping compile %s\n", name_.c_str()));
  } else {
    // Need to compile some stuff.
    if (!res.Ok()) {
      return res;
    }

    any_compile = true;
    res = DoCompile(this, config, ci);
    if (!res.Ok()) {
      return res;
    }
  }

  if (any_compile /* TODO || LibTemplateChanged(this, config) */) {
    // Archive the objs to make the lib
    std::string cmd;
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

    // TODO: write hashes for the deps
    // TODO: write a hash for the lib_template
  }

  processed_ = true;
  return Res(OK);
}
