// dumbuild.cpp -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Dead simple dumb C++ build tool.  Is it useful?
//
// Basic idea: take all the C++ files, concatenate them together (via
// #includes), and run that through the compiler.  The goal is fast
// compilation, and no need to figure out which cpp files depend on
// which headers (so as to pick which cpp files to compile).  Instead,
// everything gets compiled all the time.
//
// Build files are declared using JSON syntax.
//
// Possible (future) niceties:
//
// * have explicitly-declared library-level dependency analysis.  So
//   you declare the libraries and binaries, listing their cpp (and
//   probably also .h) files, and that is the unit of dependency
//   analysis.  If none of the source files or deps change, the
//   library or binary doesn't get rebuilt.

// TODO:
// * config system
// * lib_target
// * no-op check for lib_target and exe_target (relink, recompile+relink)

#include <assert.h>
#include <json/json.h>
#include <stdio.h>
#include <string>
#ifdef _WIN32
#include <direct.h>
#else  // not _WIN32
#include <unistd.h>
#endif  // not _WIN32

#include "config.h"
#include "context.h"
#include "os.h"
#include "res.h"
#include "target.h"
#include "util.h"

void PrintUsage() {
  printf("dmb [options] <target>\n"
         "\n"
         "options include:\n"
         "\n"
         "  -c      clean\n"
         "  -n      don't actually execute the commands\n"
         "  -v      verbose mode (show what commands would be executed)\n"
         );
}


/* example:

   [
   { "top": ".." },
  
   { "name": "myprog",
   "type": "exe",
   "src": [
   "file1.cpp",
   "file2.cpp",
   ],
   "dep": [
   "prebuild_myprog",
   ],
   },

   { "name": "mylib",
   "type": "lib",
   "src": [
   "libsrc1.cpp",
   "libsrc2.cpp",
   ],
   "dep": [
   "/proj/subdir/somelib",
   ],
   },
   ]

*/

Res ParseValue(const std::string& path, const Json::Value& value,
	       Context* out) {
  if (!value.isObject()) {
    return Res(ERR_PARSE, "object is not a JSON object");
  }

  if (!value.isMember("name")) {
    return Res(ERR_PARSE, "object lacks a name");
  }

  // TODO store the current path with the object.
  Object* object = NULL;
  Res create_result = Object::Create(out, path, value, &object);
  if (create_result.Ok()) {
    assert(object);
    Target* target = object->CastToTarget();
    if (target) {
      out->AddTarget(target->name(), target);
    } else {
      Config* config = object->CastToConfig();
      if (config) {
        out->AddConfig(config->name(), config);
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

Res ParseGroup(const std::string& path, const Json::Value& value,
	       Context* out) {
  if (!value.isObject() && !value.isArray()) {
    return Res(ERR_PARSE, "group is not an object or array");
  }

  // iterate
  for (Json::Value::const_iterator it = value.begin();
       it != value.end();
       ++it) {
    Res result = ParseValue(path, *it, out);
    if (!result.Ok()) {
      return result;
    }
  }

  return Res(OK);
}

Res ReadObjects(const std::string& path, const std::string& filename,
		Context* out) {
  assert(out);

  // Slurp in the file.
  std::string file_data;
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
    Res result = ParseGroup(path, root, out);
    if (!result.Ok()) {
      return result;
    }
  } else {
    return Res(ERR_PARSE, "Parse error in file " + filename + "\n" +
               reader.getFormatedErrorMessages());
  }

  return Res(OK);
}

Res ProcessTargets(const Context* context) {
  const std::map<std::string, Target*>& targets = context->targets();
  for (std::map<std::string, Target*>::const_iterator it = targets.begin();
       it != targets.end();
       ++it) {
    Target* t = it->second;
    if (t->resolved()) {
      Res res = t->Process(context);
      if (!res.Ok()) {
        return res;
      }
    }
  }
  return Res(OK);
}

void ExitIfError(const Res& res) {
  if (res.Ok()) {
    return;
  }

  fprintf(stderr, "dmb error\n");
  fprintf(stderr, "%s: %s\n", res.ValueString(), res.detail());

  exit(1);
}

// On success, currdir_relative_to_root will be a canonical path.
Res FindRoot(std::string* absolute_root,
             std::string* currdir_relative_to_root) {
  std::string currdir = GetCurrentDirectory();

  // Look for root dir.  Root dir is marked by the presence of a
  // "root.dmb" file.
  std::string root = currdir;
  for (;;) {
    if (FileExists(PathJoin(root, "root.dmb"))) {
      break;
    }
    if (HasParentDir(root)) {
      root = ParentDir(root);
    } else {
      return Res(ERR, "Can't find root.dmb in ancestor directories");
    }
  }
  *absolute_root = root;

  const char* remainder = currdir.c_str() + root.length();
  if (remainder[0] == '/') {
    remainder++;
  }
  *currdir_relative_to_root = remainder;
  assert(currdir_relative_to_root->size() == 0 ||
         (*currdir_relative_to_root)[currdir_relative_to_root->length() - 1]
         != '/');

  return Res(OK);
}

int main(int argc, const char** argv) {
  Context context;
  Res res;

  res = context.ProcessArgs(argc, argv);
  ExitIfError(res);

  std::string absolute_root;
  std::string canonical_currdir;
  res = FindRoot(&absolute_root, &canonical_currdir);
  ExitIfError(res);

  context.LogVerbose(StringPrintf("absolute_root = %s\n",
				  absolute_root.c_str()));
  context.LogVerbose(StringPrintf("canonical_currdir = %s\n",
				  canonical_currdir.c_str()));

  res = context.Init(absolute_root);
  ExitIfError(res);

  res = ReadObjects("", context.AbsoluteFile("", "root.dmb"), &context);
  ExitIfError(res);

  res = ReadObjects(canonical_currdir,
		    context.AbsoluteFile(canonical_currdir, "build.dmb"),
		    &context);
  ExitIfError(res);

  if (!context.GetConfig()) {
    ExitIfError(Res(ERR, StringPrintf("config '%s' is not defined",
                                      context.config_name().c_str())));
  }

  // TODO: allow command-line to specify the desired target.
  std::string target_name = canonical_currdir + ":default";
  Target* target = context.GetTarget(target_name);
  if (!target) {
    ExitIfError(Res(ERR_UNKNOWN_TARGET, target_name));
  }
  assert(target);

  res = target->Resolve(&context);
  ExitIfError(res);

  //ResolveReferences(target_list);
  //SortTargets(target_list);  // topological sort
  res = ProcessTargets(&context);
  ExitIfError(res);

  context.Log("dmb OK\n");

  return 0;
}
