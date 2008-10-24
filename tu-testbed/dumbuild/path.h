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

#include <string>

#include "res.h"

inline std::string GetPath(const std::string& filename) {
  // TODO
  return "./";
}

inline std::string PathJoin(const std::string& a, const std::string& b) {
  // TODO deal with cwd and "/" and ".." and so on
  if (a.length() == 0 || a[a.length() - 1] == '/') {
    return a + b;
  } else {
    return a + '/' + b;
  }
}

inline bool HasParentDir(const std::string& path) {
  size_t last_slash = path.rfind('/');
  if (last_slash == std::string::npos || last_slash == path.length() - 1) {
    return false;
  }
  return true;
}

// Doesn't work correctly on paths that contain ".." elements.
inline std::string ParentDir(const std::string& path) {
  int last_slash = path.rfind('/');
  assert(last_slash < path.length() - 1);
  if (last_slash == 0) {
    assert(path.length() > 1);
    return "/";
  }
  return std::string(path, 0, last_slash);
}

inline void SplitCanonicalName(const std::string& name, std::string* path_part,
			       std::string* name_part) {
  int split_point = name.length();
  int last_colon = name.rfind(':');
  if (last_colon < name.length() - 1) {
    split_point = last_colon;
  }

  if (split_point < name.length() - 1) {
    if (path_part) {
      *path_part = std::string(name, 0, split_point);
    }
    if (name_part) {
      *name_part = std::string(name, split_point + 1, name.length());
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

inline bool IsCanonicalPath(const std::string& path) {
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

inline bool IsCanonicalNamePart(const std::string& path) {
  if (path.rfind('/') < path.length()
      || path.rfind(':') < path.length()) {
    return false;
  }
  return true;
}

inline bool IsCanonicalName(const std::string& name) {
  std::string path_part, name_part;
  SplitCanonicalName(name, &path_part, &name_part);
  return IsCanonicalPath(path_part) && IsCanonicalNamePart(name_part);
}

inline Res CanonicalizeName(const std::string& base_path,
                     const std::string& relative_path,
                     std::string* out) {
  assert(IsCanonicalPath(base_path));
  // TODO: deal with relative stuff in relative_path, etc.
  *out = base_path + ":" + relative_path;
  return Res(OK);
}

inline std::string CanonicalPathPart(const std::string& name) {
  std::string path_part;
  SplitCanonicalName(name, &path_part, NULL);
  return path_part;
}

inline std::string CanonicalFilePart(const std::string& name) {
  std::string file_part;
  SplitCanonicalName(name, NULL, &file_part);
  return file_part;
}

inline void SplitFileName(const std::string& name, std::string* path_part,
                          std::string* name_part) {
  int split_point = name.rfind('/');

  if (split_point < name.length() - 1) {
    if (path_part) {
      *path_part = std::string(name, 0, split_point);
    }
    if (name_part) {
      *name_part = std::string(name, split_point + 1, name.length());
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

inline std::string FilenamePathPart(const std::string& name) {
  std::string path_part;
  SplitFileName(name, &path_part, NULL);
  return path_part;
}

inline std::string FilenameFilePart(const std::string& name) {
  std::string file_part;
  SplitFileName(name, NULL, &file_part);
  return file_part;
}

#endif  // PATH_H_
