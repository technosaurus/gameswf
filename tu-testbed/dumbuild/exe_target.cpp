// exe_target.cpp -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "exe_target.h"
#include "compile_util.h"
#include "config.h"
#include "os.h"
#include "util.h"

ExeTarget::ExeTarget() {
  type_ = "exe";
}

Res ExeTarget::Init(const Context* context, const std::string& name,
                    const Json::Value& val) {
  Res res = Target::Init(context, name, val);
  if (res.Ok()) {
    // TODO
    // more exe-specific init???
  }

  return res;
}

Res ExeTarget::Resolve(Context* context) {
  context->LogVerbose(StringPrintf("ExeTarget Resolve: %s\n", name_.c_str()));
  // Default Resolve() is OK.
  return Target::Resolve(context);
}

Res BuildLumpFile(const std::string& lump_file,
                  const std::vector<std::string>& src) {
  // TODO
  assert(0);
  return Res(OK);
}

Res ExeTarget::Process(const Context* context) {
  if (processed()) {
    return Res(OK);
  }

  context->Log(StringPrintf("dmb processing exe %s\n", name_.c_str()));

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

  // Compile.
  CompileInfo ci;
  res = PrepareCompileVars(this, config, &ci);
  if (!res.Ok()) {
    return res;
  }
  res = DoCompile(this, config, ci);
  if (!res.Ok()) {
    return res;
  }

  // Link.
  context->Log(StringPrintf("Linking %s\n", name_.c_str()));
  std::string cmd;
  res = FillTemplate(config->link_template(), ci.vars_, &cmd);
  if (!res.Ok()) {
    res.AppendDetail("\nwhile preparing linker command line for " + name_);
    return res;
  }
  res = RunCommand(absolute_out_dir(), cmd, config->compile_environment());
  if (!res.Ok()) {
    res.AppendDetail("\nwhile linking " + name_);
    res.AppendDetail("\nin directory " + absolute_out_dir());
    return res;
  }

  processed_ = true;
  return Res(OK);
}
