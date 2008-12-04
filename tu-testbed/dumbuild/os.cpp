// os.cpp -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "os.h"
#include "util.h"

#ifdef _WIN32

#include <windows.h>
#include <direct.h>

Res CreatePath(const std::string& root, const std::string& sub_path) {
  std::string current = root;
  const char* prev = sub_path.c_str();
  for (;;) {
    const char* next_slash = strchr(prev, '/');
    if (next_slash) {
      current += "/";
      current += std::string(prev, next_slash - prev);
    } else {
      current += "/";
      current += prev;
    }
    if (!CreateDirectory(current.c_str(), NULL)) {
      DWORD last_error = GetLastError();
      if (last_error != ERROR_ALREADY_EXISTS) {
        // TODO add last_error to res detail.
        return Res(ERR, "CreatePath " + sub_path + " failed.");
      }
    }

    if (!next_slash) {
      break;
    }
    prev = next_slash + 1;
  }

  return Res(OK);
}

Res RunCommand(const std::string& dir, const std::string& cmd_line,
               const std::string& environment) {
  PROCESS_INFORMATION proc_info;
  memset(&proc_info, 0, sizeof(proc_info));

  STARTUPINFO startup_info;
  memset(&startup_info, 0, sizeof(startup_info));
  startup_info.cb = sizeof(startup_info);
  startup_info.dwFlags = STARTF_USESTDHANDLES;
  startup_info.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
  startup_info.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
  startup_info.hStdError = GetStdHandle(STD_ERROR_HANDLE);

  BOOL retval = CreateProcess(
          NULL,
          (LPSTR) cmd_line.c_str(),
          NULL, NULL,
          TRUE,  /* inherit handles */
          0,
          environment.length() ? (LPVOID) environment.c_str() : NULL,
          dir.c_str(),
          &startup_info,
          &proc_info);
  if (!retval) {
    return Res(ERR_SUBCOMMAND_FAILED, "Failed to invoke " + cmd_line);
  }

  DWORD wait_res = WaitForSingleObject(proc_info.hProcess, INFINITE);
  if (wait_res != WAIT_OBJECT_0) {
    return Res(ERR, "RunCommand: WaitForSingleObject failed.");
  }

  DWORD exit_code = 0;
  if (!GetExitCodeProcess(proc_info.hProcess, &exit_code)) {
    return Res(ERR, "RunCommand: GetExitCodeProcess failed.");
  }
  if (exit_code != 0) {
    // TODO: add the exit code to the error detail.
    return Res(ERR_SUBCOMMAND_FAILED,
               StringPrintf("RunCommand returned non-zero exit status 0x%X:"
                            "\n>>%s", exit_code, cmd_line.c_str()));
  }

  return Res(OK);
}

#else // not _WIN32

#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>

Res CreatePath(const std::string& root, const std::string& sub_path) {
  std::string current = root;
  const char* prev = sub_path.c_str();
  for (;;) {
    const char* next_slash = strchr(prev, '/');
    if (next_slash) {
      current += "/";
      current += std::string(prev, next_slash - prev);
    } else {
      current += "/";
      current += prev;
    }
    if (mkdir(current.c_str(), 0755) != 0) {
      if (errno != EEXIST) {
        // TODO add last_error to res detail.
        return Res(ERR, StringPrintf("CreatePath '%s' failed, errno = %d", errno));
      }
    }

    if (!next_slash) {
      break;
    }
    prev = next_slash + 1;
  }

  return Res(OK);
}

Res RunCommand(const std::string& dir, const std::string& cmd_line,
               const std::string& environment) {
  // Split command line on spaces, ignoring any quoting.
  //
  // TODO(tulrich): might be good to support quoting someday, to allow
  // spaces inside individual args.
  std::vector<std::string> args;
  {
    const char* p = cmd_line.c_str();
    for (;;) {
      while (*p == ' ') p++;  // skip leading spaces.
      const char* n = strchr(p, ' ');
      assert(n != p);
      if (!n) {
        // last arg.
        if (*p) {
          args.push_back(p);
        }
        break;
      } else {
        args.push_back(std::string(p, n - p));
      }
      p = n + 1;
    }
  }

  // Set up program, argv and envp.
  const char* program = NULL;
  std::vector<const char*> argv;
  std::vector<const char*> envp;
  for (size_t i = 0; i < args.size(); i++) {
    argv.push_back(args[i].c_str());
  }
  argv.push_back(NULL);

  if (argv.size()) {
    program = argv[0];
  }

  {
    const char* p = environment.c_str();
    const char* e = environment.c_str() + environment.size();
    while (*p && p < e) {
      envp.push_back(p);
      p += strlen(p) + 1;
    }
    envp.push_back(NULL);
  }
  char* const *argv_p = (char* const*) &argv[0];
  char* const *envp_p = (char* const*) &envp[0];

  // Fork.
  pid_t pid = fork();
  if (pid == 0) {
    // Child process.
    if (chdir(dir.c_str()) == 0) {
      execve(program, argv_p, envp_p);
    }
    _exit(1);
  }

  // Parent process.
  if (pid == -1) {
    return Res(ERR_SUBCOMMAND_FAILED,
               StringPrintf("RunCommand failed to fork, errno = %d\n>>",
                            errno, cmd_line.c_str()));
  }
  int status = 0;
  if (waitpid(pid, &status, 0) != pid) {
    return Res(ERR_SUBCOMMAND_FAILED,
               StringPrintf("RunCommand failed to get status, "
                            "errno = %d, cmd =\n>>%s",
                            errno, cmd_line.c_str()));
  }
  if (status != 0) {
    return Res(ERR_SUBCOMMAND_FAILED,
               StringPrintf("RunCommand returned non-zero exit status "
                            "0x%X:\n>>%s", status, cmd_line.c_str()));
  }

  return Res(OK);
}

#endif  // not _WIN32


#ifdef _WIN32
#define GETCWD _getcwd
#define CHDIR _chdir
#else
#define GETCWD getcwd
#define CHDIR chdir
#endif

std::string GetCurrentDir() {
  // Allocate a big return buffer for getcwd.
  const int MAX_CURRDIR_SIZE = 2000;
  std::string currdir;
  currdir.resize(MAX_CURRDIR_SIZE);
  if (!GETCWD(&currdir[0], currdir.size())) {
    fprintf(stderr, "Internal error: getcwd() larger than %d\n",
            MAX_CURRDIR_SIZE);
    exit(1);
  }
  // Trim to the correct size.
  size_t sz = strlen(currdir.c_str());
  currdir.resize(sz);

#ifdef _WIN32
  // Change backslashes to forward slashes.
  for (size_t pos = currdir.find('\\');
       pos != std::string::npos;
       pos = currdir.find('\\', pos)) {
    currdir[pos] = '/';
  }
#endif  // _WIN32

  return currdir;
}

Res ChangeDir(const char* newdir) {
  if (CHDIR(newdir) == 0) {
    return Res(OK);
  }

  return Res(ERR, StringPrintf("chdir(\"%s\") failed", newdir));
}
