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
//   no ':' char allowed in paths
//
// Canonical paths:
//   absolute path, nothing relative
//   doesn't end in '/'
//
// Canonical names:
//   "path:targetname"

#ifndef PATH_H_
#define PATH_H_

#include "dmb_types.h"
#include "res.h"

string GetPath(const string& filename);
string PathJoin(const string& a, const string& b);
bool HasParentDir(const string& path);
// Doesn't work correctly on paths that contain ".." elements.
string ParentDir(const string& path);
void SplitCanonicalName(const string& name, string* path_part,
                        string* name_part);
bool IsCanonicalPath(const string& path);
bool IsCanonicalNamePart(const string& path);
bool IsCanonicalName(const string& name);
Res CanonicalizeName(const string& base_path,
                     const string& relative_path,
                     string* out);
string CanonicalPathPart(const string& name);
string CanonicalFilePart(const string& name);
void SplitFileName(const string& name, string* path_part,
                   string* name_part);
string FilenamePathPart(const string& name);
string FilenameFilePart(const string& name);

#endif  // PATH_H_
