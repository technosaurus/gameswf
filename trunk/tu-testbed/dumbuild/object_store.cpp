// object_store.cpp -- Thatcher Ulrich <tu@tulrich.com> 2009

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include <stdio.h>
#include "object_store.h"
#include "content_hash.h"
#include "os.h"
#include "sha1.h"
#include "util.h"

ObjectStore::ObjectStore(const char* root_path) : root_path_(root_path) {
}

FILE* ObjectStore::Read(const ContentHash& key) const {
  std::string path;
  MakePath(key, &path, NULL);
  return fopen(path.c_str(), "rb");
}

FILE* ObjectStore::Write(const ContentHash& key) {
  std::string fullpath, subdir;
  MakePath(key, &fullpath, &subdir);
  CreatePath(root_path_, subdir);
  return fopen(fullpath.c_str(), "wb");
}

bool ObjectStore::Erase(const ContentHash& key) {
  std::string path;
  MakePath(key, &path, NULL);
  if (EraseFile(path).Ok()) {
    return true;
  }
  return false;
}

bool ObjectStore::Exists(const ContentHash& key) const {
  std::string path;
  MakePath(key, &path, NULL);
  if (FileExists(path)) {
    return true;
  }
  return false;
}

void ObjectStore::MakePath(const ContentHash& key, std::string* path,
                           std::string* subdir) const {
  char readable[27];
  stb_sha1_readable(readable, key.data());
  *path = root_path_;
  path->append("/");
  path->append(readable, 2);
  if (subdir) {
    subdir->clear();
    subdir->append(readable, 2);
  }
  path->append("/");
  path->append(readable + 2, 24);
}
