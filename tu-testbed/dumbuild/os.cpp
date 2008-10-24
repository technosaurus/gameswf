// os.cpp -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "os.h"
#include "util.h"

#ifdef _WIN32

#include <windows.h>

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

Res RunCommand(const std::string& dir, const std::vector<std::string>& argv,
               const std::string& environment) {
  if (argv.size() < 1) {
    return Res(ERR_SUBCOMMAND_FAILED, "Empty argv passed to RunCommand()");
  }

  // Build a appropriately-quoted command-line.
  std::string cmd_line;
  for (size_t i = 0; i < argv.size(); i++) {
    bool need_quote = (strchr(argv[i].c_str(), ' ') != NULL);
    if (need_quote) {
      // TODO: properly deal with existing quotes
      cmd_line += '\"';
      cmd_line += argv[i];
      cmd_line += '\"';
    } else {
      cmd_line += argv[i];
    }
    cmd_line += ' ';
  }

  return RunCommand(dir, cmd_line, environment);
}

#else // not _WIN32

// TODO: posix implementation using vfork etc.

#endif  // not _WIN32
