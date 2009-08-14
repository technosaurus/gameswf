// path.h -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Paths:
//
//   "#" == project root
//   "#subdir" == first level subdir
//   "#subdir1/subdir2" == two levels deep
//   "../something" == relative path
//   "./something" == relative path
//   "something" == relative path
//
// Canonical paths:
//   absolute path, nothing relative
//   doesn't end in '/'
//
// Target names:
//   "path/targetname"
//   "targetname"           == targetname in the current dir
//   "./targetname"         == ditto
//   "../targetname"        == targetname in parent dir
//   "#path/to/targetname"  == targetname relative to root

#ifndef PATH_H_
#define PATH_H_

#include "dmb_types.h"
#include "res.h"

string GetPath(const string& filename);
string PathJoin(const string& a, const string& b);
bool HasParentDir(const string& path);
// Doesn't work correctly on paths that contain ".." elements.
string ParentDir(const string& path);

bool IsFileNamePart(const string& name);
void SplitFileName(const string& name, string* path_part,
                   string* name_part);
string FilenamePathPart(const string& name);
string FilenameFilePart(const string& name);

#endif  // PATH_H_
