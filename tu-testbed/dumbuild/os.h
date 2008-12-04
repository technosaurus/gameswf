// os.h -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some routines that interact with the operating system.

#ifndef OS_H_
#define OS_H_

#include <string>
#include <vector>

#include "res.h"

// Ensure that the given sub-directory path exists below the given
// absolute root.
Res CreatePath(const std::string& root, const std::string& sub_path);

// Execute a sub-process.  dir gives the current directory of the
// subprocess; cmd_line gives the command line with arguments.
//
// environment is an optional set of environment variable strings to
// give to the new process.  Pass an empty string to inherit this
// process' environment.  The environment is encoded as a set of
// null-terminated strings inside the std::string, the whole thing
// terminated with an extra '\0'.  Each string is of the form
// "VARNAME=VALUE" (sans quotes).
//
// On success, Res.Ok() is true.  On failure, the return value
// contains error details.
Res RunCommand(const std::string& dir,
               const std::string& cmd_line,
               const std::string& environment);

std::string GetCurrentDir();
Res ChangeDir(const char* newdir);

#endif  // OS_H_
