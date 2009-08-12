// compile_util.cpp -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Dependency checking and code to launch the compiler are in here.

#include "compile_util.h"
#include "config.h"
#include "file_deps.h"
#include "hash_util.h"
#include "object_store.h"
#include "os.h"
#include "target.h"
#include "util.h"

Res PrepareCompileVars(const Target* t, const Context* context,
                       CompileInfo* ci, Hash* dep_hash) {
  const Config* config = context->GetConfig();

  // Pull all config vars into ci, so they can be used in template
  // replacement.
  config->InsertVarsIntoMap(&ci->vars_);

  bool build_all = context->rebuild_all();

  string inc_dirs_str;
  for (size_t i = 0; i < t->inc_dirs().size(); i++) {
    inc_dirs_str += " -I";
    // TODO: handle absolute inc_dirs
    string inc_dir = PathJoin(t->relative_path_to_tree_root(),
				   t->inc_dirs()[i]);
    inc_dirs_str += inc_dir;
  }

  Res res;
  
  string src_list;
  string obj_list;
  string lib_list;
  for (size_t i = 0; i < t->src().size(); i++) {
    string src_path = PathJoin(t->relative_path_to_tree_root(),
                                    t->src()[i]);
    Hash current_hash;
    res = AccumulateObjFileDepHash(
        t, context, PathJoin(t->absolute_out_dir(), src_path),
        inc_dirs_str, &current_hash);
    if (!res.Ok()) {
      return res;
    }

    // Append this obj's dephash into the target dep_hash.
    *dep_hash << current_hash;

    // See if we should compile this file.
    bool do_compile = true;
    if (!build_all) {
      Hash previous_hash;
      res = ReadFileHash(t->absolute_out_dir(), src_path, &previous_hash);
      if (res.Ok()) {
        if (previous_hash == current_hash) {
          // Flags/environment, file, and dependencies have not
          // changed.
          do_compile = false;
          context->LogVerbose("file hash unchanged: " + src_path + "\n");
        } else {
          // This file has changed, so we have to compile the file.
          context->LogVerbose("file hash changed:   " + src_path + "\n");
        }
      } else {
        // No existing hash, so we have to compile the file.
        context->LogVerbose(StringPrintf("no file hash, %s: %s\n",
                                         res.ValueString(), res.detail()));
      }
    }

    if (do_compile) {
      ci->src_list_.push_back(src_path);
      src_list += " ";
      src_list += src_path;
    }

    obj_list += " ";
    obj_list += StripExt(FilenameFilePart(t->src()[i])) +
                config->obj_extension();
  }

  // Check deps.
  for (size_t i = 0; i < t->dep().size(); i++) {
    Target* this_dep = context->GetTarget(t->dep()[i]);
    assert(this_dep);
    *dep_hash << this_dep->dep_hash();

    lib_list += " ";
    const string& dep = t->dep()[i];
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

  return Res(OK);
}

Res DoCompile(const Target* t, const Context* context, const CompileInfo& ci) {
  if (ci.src_list_.size() == 0) {
    // Nothing to compile.
    return Res(OK);
  }

  const Config* config = context->GetConfig();

  string cmd;
  Res res = FillTemplate(config->prefilled_compile_template(), ci.vars_, false,
			 &cmd);
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

  const string inc_dirs_str = ci.vars_.find("inc_dirs")->second;

  // Write build markers for the just-compiled sources.
  Hash obj_dep_hash;
  for (size_t i = 0; i < ci.src_list_.size(); i++) {
    const string& src_path = ci.src_list_[i];
    obj_dep_hash.Reset();
    Res res = AccumulateObjFileDepHash(
        t, context, PathJoin(t->absolute_out_dir(), src_path), inc_dirs_str,
        &obj_dep_hash);
    if (!res.Ok()) {
      return res;
    }
    res = WriteFileHash(t->absolute_out_dir(), src_path, obj_dep_hash);
    if (!res.Ok()) {
      return res;
    }
  }

  return res;
}
