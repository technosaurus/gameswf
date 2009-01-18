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
                       CompileInfo* ci) {
  const Config* config = context->GetConfig();

  bool build_all = context->rebuild_all();
  // TODO build_all = build_all || CompileTemplateChanged(t, config);

  string inc_dirs_str;
  for (size_t i = 0; i < t->inc_dirs().size(); i++) {
    inc_dirs_str += " -I";
    // TODO: handle absolute inc_dirs
    string inc_dir = PathJoin(t->relative_path_to_tree_root(),
				   t->inc_dirs()[i]);
    inc_dirs_str += inc_dir;
  }

  // TODO: more checks for build_all
  // build_all = build_all || IncludeDirsStringChanged(t, inc_dirs_str());
  // build_all = build_all || HeaderFilesChanged(t, header_dirs());

  Res res;
  
  string src_list;
  string obj_list;
  string lib_list;
  for (size_t i = 0; i < t->src().size(); i++) {
    string src_path = PathJoin(t->relative_path_to_tree_root(),
                                    t->src()[i]);
    Hash src_hash;

    // See if we should compile this file.
    bool do_compile = true;
    if (!build_all) {
      Hash previous_hash;
      res = ReadFileHash(t->absolute_out_dir(), src_path, &previous_hash);
      if (res.Ok()) {
        res = context->ComputeOrGetFileContentHash(
            PathJoin(t->absolute_out_dir(), src_path), &src_hash);
        if (!res.Ok()) {
          return res;
        }
        //ComputeFileHash(t->absolute_out_dir(), src_path, &src_hash);
        if (previous_hash == src_hash) {
          // File has not changed.
          //
          // What about dependencies?
          if (AnyIncludesChanged(t, context, inc_dirs_str, src_path,
                                 src_hash)) {
            // Need to compile.
          } else {
            // Skip this one.
            do_compile = false;
          }
        }
        // else this file has changed, so we have to compile the file.
      }
      // else no existing hash, so we have to compile the file.
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
  bool deps_changed = false;
  for (size_t i = 0; i < t->dep().size(); i++) {
    Target* this_dep = context->GetTarget(t->dep()[i]);
    assert(this_dep);
    deps_changed = deps_changed || this_dep->did_rebuild();

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

  if (!build_all && src_list.size() == 0) {
    if (!deps_changed) {
      return Res(ERR_DONT_REBUILD);
    } else {
      return Res(ERR_LINK_ONLY);
    }
  }

  return Res(OK);
}

Res DoCompile(const Target* t, const Context* context, const CompileInfo& ci) {
  if (ci.src_list_.size() == 0) {
    // Nothing to compile.
    return Res(OK);
  }

  const Config* config = context->GetConfig();

  string cmd;
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

  const string inc_dirs_str = ci.vars_.find("inc_dirs")->second;

  // Write hashes for the just-compiled sources.
  Hash src_hash;
  for (int i = 0; i < ci.src_list_.size(); i++) {
    const string& src_path = ci.src_list_[i];
    Res res = context->ComputeOrGetFileContentHash(
        PathJoin(t->absolute_out_dir(), src_path), &src_hash);
    if (!res.Ok()) {
      return res;
    }
    res = WriteFileHash(t->absolute_out_dir(), src_path, src_hash);
    if (!res.Ok()) {
      return res;
    }

    // TODO: Write deps files, recursively, for the just-compiled
    // sources.
    res = GenerateDepsFile(t, context, inc_dirs_str,
                           PathJoin(t->absolute_out_dir(), src_path),
                           src_hash);
    if (!res.Ok()) {
      return res;
    }
  }

  // TODO: write a hash for the inc_dirs?  I don't think it's
  // necessary since it's mixed into the deps id for each source file.

  // TODO: write a hash for the compile_template +
  // compile_environment?  Should mix that stuff into deps id instead,
  // right?  No, probably not, though it would do the job.

  // TODO: write a hash for the link template and deps list?  Yes.

  return res;
}
