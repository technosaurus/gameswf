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
  int a_end = a.length();
  if (a_end > 0 && a[a_end - 1] == '/') {
    a_end--;
  }

  // Process any parent specs at the start of b.
  int b_start = 0;
  while (b_start < b.size() && a_end > 0) {
    if (b.find("../", b_start) == b_start) {
      // Starts with ../
      b_start += 3;
    } else if (b.length() - b_start == 2 &&
               b[b_start] == '.' && b[b_start + 1] == '.') {
      // Whole of b is now ".."
      b_start += 2;
    } else {
      break;
    }

    a_end = a.rfind("/", a_end - 1);
    if (a_end < 0) {
      a_end = 0;
    }
  }

  string result(a, 0, a_end);
  if (a_end && b_start < b.length()) {
    result += '/';
  }
  result += string(b, b_start);
  return result;
}

string Canonicalize(const string& base_dir, const string& relative_path) {
  if (relative_path.size() && relative_path[0] == '#') {
    // '#' indicates a path relative to the project root.
    return string(relative_path, 1);
  }
  return PathJoin(base_dir, relative_path);
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
  assert(Canonicalize("", "") == "");
  assert(Canonicalize("", "baker") == "baker");
  assert(Canonicalize("able", "") == "able");
  assert(Canonicalize("able", "baker") == "able/baker");
  assert(Canonicalize("able/", "baker") == "able/baker");
  assert(Canonicalize("baker/charlie", "#delta") == "delta");
  assert(Canonicalize("baker/charlie", "#../external") == "../external");
  assert(Canonicalize("baker/charlie", "..") == "baker");
  assert(Canonicalize("baker/charlie", "delta") == "baker/charlie/delta");
  assert(Canonicalize("baker/charlie", "../delta") == "baker/delta");
  assert(Canonicalize("baker/charlie/", "../delta") == "baker/delta");
  assert(Canonicalize("baker/charlie", "../../delta") == "delta");
  assert(Canonicalize("baker/charlie", "../..") == "");
  assert(Canonicalize("baker/charlie", "../../../external") == "../external");
}
