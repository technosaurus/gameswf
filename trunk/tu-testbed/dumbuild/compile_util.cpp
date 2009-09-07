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

typedef std::multimap<const Target*, const Target*> EdgeMap;

static void InsertDependencies(const Context* context, const Target* t,
                               std::set<const Target*>* visited,
                               EdgeMap* deps_incoming) {
  if (visited->find(t) != visited->end()) {
    // We've already been here.  Don't recurse.
    return;
  }
  visited->insert(t);

  for (size_t i = 0; i < t->dep().size(); i++) {
    const string& dep = t->dep()[i];
    Target* this_dep = context->GetTarget(dep);
    assert(this_dep);
    deps_incoming->insert(std::make_pair(this_dep, t));

    // Recurse.
    InsertDependencies(context, this_dep, visited, deps_incoming);
  }
}

static void TopologicalSort(const Context* context, const Target* node,
                            EdgeMap* deps_incoming_edges,
                            vector<const Target*>* dep_list_out) {
  if (deps_incoming_edges->count(node) == 0) {
    // Add this node to the output, remove its edges from the graph,
    // and check its children.
    dep_list_out->push_back(node);
    // Remove all edges emanating from node.
    for (size_t i = 0; i < node->dep().size(); i++) {
      const string& dep = node->dep()[i];
      Target* this_dep = context->GetTarget(dep);
      assert(this_dep);
      std::pair<EdgeMap::iterator, EdgeMap::iterator> range =
        deps_incoming_edges->equal_range(this_dep);
      for (EdgeMap::iterator it = range.first; it != range.second; ) {
        EdgeMap::iterator next_it = it;
        ++next_it;
        if (it->second == node) {
          deps_incoming_edges->erase(it);
        }
        it = next_it;
      }
    }
    for (size_t i = 0; i < node->dep().size(); i++) {
      const string& dep = node->dep()[i];
      Target* this_dep = context->GetTarget(dep);
      assert(this_dep);
      TopologicalSort(context, this_dep, deps_incoming_edges, dep_list_out);
    }
  }
}

static void AppendIncDirs(const vector<string>& dirs,
                          const string& path_to_root,
                          string* out) {
  for (size_t i = 0; i < dirs.size(); i++) {
    *out += " -I";
    // TODO: handle absolute inc_dirs
    string inc_dir = PathJoin(path_to_root, dirs[i]);
    *out += inc_dir;
  }
}

Res PrepareCompileVars(const Target* t, const Context* context,
                       CompileInfo* ci, Hash* dep_hash) {
  const Config* config = context->GetConfig();

  bool build_all = context->rebuild_all();

  // Get our include dirs.
  string inc_dirs_str;
  AppendIncDirs(t->inc_dirs(), t->relative_path_to_tree_root(), &inc_dirs_str);

  // Get the include dirs from any external libs we depend on.
  for (size_t i = 0; i < t->dep().size(); i++) {
    const string& dep = t->dep()[i];
    const Target* this_dep = context->GetTarget(dep);
    assert(this_dep);
    AppendIncDirs(this_dep->dep_inc_dirs(), t->relative_path_to_tree_root(),
                  &inc_dirs_str);
  }

  Res res;
  
  string src_list;
  string obj_list;
  string lib_list;

  // Build the src_list and obj_list.
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

  // Update the dep hash based on our dependencies.
  for (size_t i = 0; i < t->dep().size(); i++) {
    const string& dep = t->dep()[i];
    const Target* this_dep = context->GetTarget(dep);
    assert(this_dep);
    *dep_hash << this_dep->dep_hash();
  }

  // Build a dependency graph of deps, and ultimately the lib_list.
  std::multimap<const Target*, const Target*> deps_incoming_edges;
  std::set<const Target*> visited;
  InsertDependencies(context, t, &visited, &deps_incoming_edges);

  vector<const Target*> lib_list_array;
  TopologicalSort(context, t, &deps_incoming_edges, &lib_list_array);
  if (deps_incoming_edges.size()) {
    // TODO make this a warning and use the deps in arbitrary order?
    // TODO flesh this out
    return Res(ERR_DEPENDENCY_CYCLE, "Cyclic dependency detected in deps of"
               "target " + t->name());
  }

  // Build the actual lib_list string.
  for (size_t i = 0; i < lib_list_array.size(); i++) {
    const Target* dep = lib_list_array[i];
    if (dep == t) {
      continue;
    }

    string linker_args = dep->GetLinkerArgs(context);
    if (linker_args.length() > 0) {
      lib_list += " ";
      lib_list += linker_args;
    }
  }

  // TODO: append external dependency libs to lib_list

  ci->vars_["src_list"] = src_list;
  ci->vars_["obj_list"] = obj_list;
  ci->vars_["lib_list"] = lib_list;
  ci->vars_["basename"] = FilenameFilePart(t->name());
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
