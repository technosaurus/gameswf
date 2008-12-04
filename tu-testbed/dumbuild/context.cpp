// context.cpp -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include <vector>
#include "context.h"
#include "config.h"
#include "os.h"
#include "path.h"
#include "target.h"

Context::Context() : log_verbose_(false) {
#ifdef _WIN32
  config_name_ = ":vc8-debug";
#else
  config_name_ = ":gcc-debug";
#endif
}

Context::~Context() {
  for (std::map<std::string, Target*>::iterator it = targets_.begin();
       it != targets_.end();
       ++it) {
    delete it->second;
  }
  for (std::map<std::string, Config*>::iterator it = configs_.begin();
       it != configs_.end();
       ++it) {
    delete it->second;
  }
}

Res Context::ProcessArgs(int argc, const char** argv) {
  for (int i = 1; i < argc; i++) {
    const char* arg = argv[i];
    if (arg[0] == '-') {
      switch (arg[1]) {
        case '-':
          // long-form arguments.
        default:
          return Res(ERR_COMMAND_LINE, StringPrintf("Unknown arg %s", arg));
        case 'C': {
          // Change directory.
          i++;
          if (i >= argc) {
            return Res(ERR_COMMAND_LINE, "-C option requires a directory");
          }
          Res res = ChangeDir(argv[i]);
          if (!res.Ok()) {
            res.AppendDetail("\nWhile processing argument -C");
            return res;
          }
          break;
        }
        case 'c':
          // Config.
          i++;
          if (i >= argc) {
            return Res(ERR_COMMAND_LINE, "-c option requires a config name");
          }
          config_name_ = argv[i];
          break;
      }
    }
  }

  return Res(OK);
}

Res Context::Init(const std::string& root_path) {
  tree_root_ = root_path;
  out_root_ = PathJoin("dmb_out", CanonicalFilePart(config_name_));

  return Res(OK);
}

std::string Context::AbsoluteFile(const std::string& canonical_path,
                                  const std::string& filename) {
  return PathJoin(tree_root(), PathJoin(canonical_path, filename));
}

void Context::Log(const std::string& msg) const {
  fputs(msg.c_str(), stdout);
  fflush(stdout);
}

void Context::LogVerbose(const std::string& msg) const {
  if (log_verbose_) {
    Log(msg);
  }
}
