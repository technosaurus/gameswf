// compile_util.cpp -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "compile_util.h"
#include "config.h"
#include "hash_util.h"
#include "os.h"
#include "target.h"
#include "util.h"

// build all, if any of these changed since last successful compile:
// * header files in header_dirs (including local dir)
// * compile template
// * compile environment
// * inc_dirs
//
// Compile a source file if:
// * build_all is true
// * source changed since last successfully compiled
//
// Re-link/re-archive if any of these changed since last link/archive:
// * any source file compiled
// * any deps compiled

Res PrepareCompileVars(const Target* t, const Config* config,
                       CompileInfo* ci) {
  bool build_all = false;  // TODO CompileTemplateChanged(t, config);

  std::string inc_dirs_str;
  for (size_t i = 0; i < t->inc_dirs().size(); i++) {
    inc_dirs_str += " -I";
    inc_dirs_str += PathJoin(t->relative_path_to_tree_root(), t->inc_dirs()[i]);
  }

  // TODO: more checks for build_all
  // build_all = build_all || IncludeDirsStringChanged(t, inc_dirs_str());
  // build_all = build_all || HeaderFilesChanged(t, header_dirs());

  std::string src_list;
  std::string obj_list;
  std::string lib_list;
  for (size_t i = 0; i < t->src().size(); i++) {
    std::string src_path = PathJoin(t->relative_path_to_tree_root(),
                                    t->src()[i]);
    ContentHash src_hash;

    // See if we should compile this file.
    bool do_compile = true;
    if (!build_all) {
      ContentHash previous_hash;
      ReadFileHash(t->absolute_out_dir(), src_path, &previous_hash);
      if (!previous_hash.IsZero()) {
        ComputeFileHash(t->absolute_out_dir(), src_path, &src_hash);
        if (previous_hash == src_hash) {
          // Skip this one.
          do_compile = false;
        }
        // else this file has changed
      }
    }

    if (do_compile) {
      ci->src_list_.push_back(std::make_pair(src_path, src_hash));
      src_list += " ";
      src_list += src_path;
    }

    obj_list += " ";
    obj_list += StripExt(FilenameFilePart(t->src()[i])) +
                config->obj_extension();
  }
  bool deps_changed = false;
  for (size_t i = 0; i < t->dep().size(); i++) {
    // TODO:
    // deps_changed = deps_changed || this dep was recompiled;
    deps_changed = true;
    
    lib_list += " ";
    const std::string& dep = t->dep()[i];
    lib_list += PathJoin(PathJoin(t->absolute_out_dir(),
                                  CanonicalPathPart(dep)),
                         CanonicalFilePart(dep)) +
                config->lib_extension();
  }

  ci->vars_["src_list"] = src_list;
  ci->vars_["obj_list"] = obj_list;
  ci->vars_["lib_list"] = lib_list;
  ci->vars_["basename"] = CanonicalFilePart(t->name());
  ci->vars_["inc_dirs"] = inc_dirs_str;

  if (!build_all && src_list.size() == 0 && !deps_changed) {
    return Res(ERR_DONT_REBUILD);
  }

  return Res(OK);
}

Res DoCompile(const Target* t, const Config* config, const CompileInfo& ci) {
  if (ci.src_list_.size() == 0) {
    // Nothing to compile.
    return Res(OK);
  }

  std::string cmd;
  Res res = FillTemplate(config->compile_template(), ci.vars_, &cmd);
  if (!res.Ok()) {
    res.AppendDetail("\nwhile preparing compiler command line for " +
                     t->name());
    return res;
  }

  res = RunCommand(t->absolute_out_dir(), cmd, config->compile_environment());
  if (!res.Ok()) {
    res.AppendDetail("\nwhile compiling " + t->name());
    res.AppendDetail("\nin directory " + t->absolute_out_dir());
    return res;
  }

  // Write hashes for the just-compiled sources.
  for (int i = 0; i < ci.src_list_.size(); i++) {
    ComputeIfNecessaryAndWriteFileHash(t->absolute_out_dir(),
                                       ci.src_list_[i].first,
                                       ci.src_list_[i].second);
  }

  // TODO: write a hash for the inc_dirs?
  // TODO: write a hash for the compile_template + compile_environment
  // TODO: write hashes for the header_dirs

  return res;
}
