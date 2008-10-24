// exe_target.cpp -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "exe_target.h"
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
  // TODO factor a lot of this out, so it can be shared by LibTarget.

  context->Log(StringPrintf("dmb processing %s\n", name_.c_str()));

  Res res;
  res = Target::ProcessDependencies(context);
  if (!res.Ok()) {
    res.AppendDetail("\nwhile processing dependencies of " + name_);
    return res;
  }

  std::string out_dir = PathJoin(context->out_root(), CanonicalPathPart(name_));
  res = CreatePath(context->tree_root(), out_dir);
  if (!res.Ok()) {
    res.AppendDetail("\nwhile creating output path for " + name_);
    return res;
  }

  std::string abs_out_dir = PathJoin(context->tree_root(), out_dir);
  std::string target_out_path = PathJoin(out_dir, CanonicalFilePart(name_));

  std::string relative_path_to_tree_root = "../";
  const char* slash = strchr(out_dir.c_str(), '/');
  while (slash && *(slash + 1)) {
    relative_path_to_tree_root += "../";
    slash = strchr(slash + 1, '/');
  }

  // TODO: add lumping as an option
  bool lumping = false;
  std::string lump_file;

  if (lumping) {
    // Build lump file.
    lump_file = target_out_path + ".h";
    res = BuildLumpFile(lump_file, src());
    if (!res.Ok()) {
      res.AppendDetail("\nwhile lumping " + name_);
      return res;
    }
  }

  // Prepare template replacement vars.
  std::map<std::string, std::string> vars;
  std::string src_list;
  std::string obj_list;
  std::string inc_dirs_str;
  for (size_t i = 0; i < src().size(); i++) {
    src_list += " ";
    src_list += PathJoin(relative_path_to_tree_root, src()[i]);
    obj_list += " ";
    obj_list += StripExt(FilenameFilePart(src()[i])) + ".obj";
  }
  for (size_t i = 0; i < inc_dirs().size(); i++) {
    inc_dirs_str += " -I";
    inc_dirs_str += PathJoin(relative_path_to_tree_root, inc_dirs()[i]);
  }
  vars["src_list"] = src_list;
  vars["obj_list"] = obj_list;
  vars["basename"] = CanonicalFilePart(name_);
  vars["inc_dirs"] = inc_dirs_str;
  
  const Config* config = context->GetConfig();
  
  std::string cmd;
  res = FillTemplate(config->compile_template(), vars, &cmd);
  if (!res.Ok()) {
    res.AppendDetail("\nwhile preparing compiler command line for " + name_);
    return res;
  }

  res = RunCommand(abs_out_dir, cmd, config->compile_environment());
  if (!res.Ok()) {
    res.AppendDetail("\nwhile compiling " + name_);
    res.AppendDetail("\nin directory " + abs_out_dir);
    return res;
  }

  // Link.
  res = FillTemplate(config->link_template(), vars, &cmd);
  if (!res.Ok()) {
    res.AppendDetail("\nwhile preparing linker command line for " + name_);
    return res;
  }
  res = RunCommand(abs_out_dir, cmd, config->compile_environment());
  if (!res.Ok()) {
    res.AppendDetail("\nwhile linking " + name_);
    res.AppendDetail("\nin directory " + abs_out_dir);
    return res;
  }

  return Res(OK);
}
