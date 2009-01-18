// path.h -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Paths:
//
//   "" == root
//   "subdir" == first level subdir
//   "subdir1/subdir2" == two levels deep
//   "../something" == relative path
//   "./something" == relative path
//   no ':' char allowed in paths
//
// Canonical paths:
//   absolute path, nothing relative
//   doesn't end in '/'
//
// Target specs:
//   "path:targetname"

#ifndef PATH_H_
#define PATH_H_

#include "dmb_types.h"
#include "res.h"

inline string GetPath(const string& filename) {
  // TODO
  return "./";
}

inline string PathJoin(const string& a, const string& b) {
  // TODO deal with cwd and "/" and ".." and so on; i.e. normalize the path.
  if (a.length() == 0 || a[a.length() - 1] == '/') {
    return a + b;
  } else {
    return a + '/' + b;
  }
}

inline bool HasParentDir(const string& path) {
  size_t last_slash = path.rfind('/');
  if (last_slash == string::npos || last_slash == path.length() - 1) {
    return false;
  }
  return true;
}

// Doesn't work correctly on paths that contain ".." elements.
inline string ParentDir(const string& path) {
  size_t last_slash = path.rfind('/');
  assert(last_slash < path.length() - 1);
  if (last_slash == 0) {
    assert(path.length() > 1);
    return "/";
  }
  return string(path, 0, last_slash);
}

inline void SplitCanonicalName(const string& name, string* path_part,
			       string* name_part) {
  size_t split_point = name.length();
  size_t last_colon = name.rfind(':');
  if (last_colon < name.length() - 1) {
    split_point = last_colon;
  }

  if (split_point < name.length() - 1) {
    if (path_part) {
      *path_part = string(name, 0, split_point);
    }
    if (name_part) {
      *name_part = string(name, split_point + 1, name.length());
    }
  } else {
    if (path_part) {
      *path_part = name;
    }
    if (name_part) {
      name_part->clear();
    }
  }
}

inline bool IsCanonicalPath(const string& path) {
  if (path == "") {  // root
    return true;
  }
  if (path[path.length() - 1] == '/') {
    return false;  // no trailing '/' allowed
  }
  // if (path contains ':') return false;
  // if (relative elements) return false;
  // assert(no relative elements);
  
  return true;
}

inline bool IsCanonicalNamePart(const string& path) {
  if (path.rfind('/') < path.length()
      || path.rfind(':') < path.length()) {
    return false;
  }
  return true;
}

inline bool IsCanonicalName(const string& name) {
  string path_part, name_part;
  SplitCanonicalName(name, &path_part, &name_part);
  return IsCanonicalPath(path_part) && IsCanonicalNamePart(name_part);
}

inline Res CanonicalizeName(const string& base_path,
                     const string& relative_path,
                     string* out) {
  assert(IsCanonicalPath(base_path));
  // TODO: deal with relative stuff in relative_path, etc.
  *out = base_path + ":" + relative_path;
  return Res(OK);
}

inline string CanonicalPathPart(const string& name) {
  string path_part;
  SplitCanonicalName(name, &path_part, NULL);
  return path_part;
}

inline string CanonicalFilePart(const string& name) {
  string file_part;
  SplitCanonicalName(name, NULL, &file_part);
  return file_part;
}

inline void SplitFileName(const string& name, string* path_part,
                          string* name_part) {
  size_t split_point = name.rfind('/');

  if (split_point < name.length() - 1) {
    if (path_part) {
      *path_part = string(name, 0, split_point);
    }
    if (name_part) {
      *name_part = string(name, split_point + 1, name.length());
    }
  } else {
    if (path_part) {
      path_part->clear();
    }
    if (name_part) {
      *name_part = name;
    }
  }
}

inline string FilenamePathPart(const string& name) {
  string path_part;
  SplitFileName(name, &path_part, NULL);
  return path_part;
}

inline string FilenameFilePart(const string& name) {
  string file_part;
  SplitFileName(name, NULL, &file_part);
  return file_part;
}

#endif  // PATH_H_
