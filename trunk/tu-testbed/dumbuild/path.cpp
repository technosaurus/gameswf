// path.cpp -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include <stdio.h>
#include "path.h"
#include "test.h"

string GetPath(const string& filename) {
  // TODO
  return "./";
}

string PathJoin(const string& a, const string& b) {
  // TODO deal with cwd and "/" and ".." and so on; i.e. normalize the path.
  if (a.length() == 0 || a[a.length() - 1] == '/') {
    return a + b;
  } else {
    return a + '/' + b;
  }
}

bool HasParentDir(const string& path) {
  size_t last_slash = path.rfind('/');
  if (last_slash == string::npos || last_slash == path.length() - 1) {
    return false;
  }
  return true;
}

// Doesn't work correctly on paths that contain ".." elements.
string ParentDir(const string& path) {
  size_t last_slash = path.rfind('/');
  assert(last_slash < path.length() - 1);
  if (last_slash == 0) {
    assert(path.length() > 1);
    return "/";
  }
  return string(path, 0, last_slash);
}

bool IsCanonicalPath(const string& path) {
  if (path == "") {  // root
    return true;
  }
  if (path[path.length() - 1] == '/') {
    return false;  // no trailing '/' allowed
  }
  // if (relative elements) return false;
  // assert(no relative elements);
  
  return true;
}

bool IsFileNamePart(const string& path) {
  if (path.rfind('/') < path.length()) {
    return false;
  }
  return true;
}

void SplitFileName(const string& name, string* path_part,
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

string FilenamePathPart(const string& name) {
  string path_part;
  SplitFileName(name, &path_part, NULL);
  return path_part;
}

string FilenameFilePart(const string& name) {
  string file_part;
  SplitFileName(name, NULL, &file_part);
  return file_part;
}

// Tests -----------------------------------------------------------------

void TestPath() {
  printf("TestPath()\n");
//   assert(IsCanonicalPath("#"));
//   assert(IsCanonicalPath("#subdir"));
//   assert(IsCanonicalPath("#subdir/subdir2"));
//   assert(IsCanonicalPath("subdir"));

//   assert(!IsCanonicalPath("subdir:target"));
}
