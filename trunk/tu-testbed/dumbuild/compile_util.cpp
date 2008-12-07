// compile_util.cpp -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "compile_util.h"
#include "config.h"
#include "os.h"
#include "target.h"
#include "util.h"

// relative_path_to_tree_root()
// abs_out_dir


Res PrepareCompileVars(const Target* t, const Config* config,
		       std::map<std::string, std::string>* vars) {
  std::string src_list;
  std::string obj_list;
  std::string lib_list;
  std::string inc_dirs_str;
  for (size_t i = 0; i < t->src().size(); i++) {
    src_list += " ";
    src_list += PathJoin(t->relative_path_to_tree_root(), t->src()[i]);
    obj_list += " ";
    obj_list += StripExt(FilenameFilePart(t->src()[i])) +
                config->obj_extension();
  }
  for (size_t i = 0; i < t->dep().size(); i++) {
    lib_list += " ";
    const std::string& dep = t->dep()[i];
    lib_list += PathJoin(PathJoin(t->absolute_out_dir(),
                                  CanonicalPathPart(dep)),
                         CanonicalFilePart(dep)) +
                config->lib_extension();
  }
  for (size_t i = 0; i < t->inc_dirs().size(); i++) {
    inc_dirs_str += " -I";
    inc_dirs_str += PathJoin(t->relative_path_to_tree_root(), t->inc_dirs()[i]);
  }
  (*vars)["src_list"] = src_list;
  (*vars)["obj_list"] = obj_list;
  (*vars)["lib_list"] = lib_list;
  (*vars)["basename"] = CanonicalFilePart(t->name());
  (*vars)["inc_dirs"] = inc_dirs_str;

  return Res(OK);
}

Res DoCompile(const Target* t, const Config* config,
	      const std::map<std::string, std::string>& vars) {
  std::string cmd;
  Res res = FillTemplate(config->compile_template(), vars, &cmd);
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

  return res;
}
