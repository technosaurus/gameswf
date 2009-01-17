// object_store.h -- Thatcher Ulrich <tu@tulrich.com> 2009

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A GIT-inspired content-addressable object store.  Addresses/keys
// are sha1 hashes; contents are files.

#ifndef OBJECT_STORE_H_
#define OBJECT_STORE_H_

#include <string>

class ContentHash;

class ObjectStore {
 public:
  explicit ObjectStore(const char* root_path);

  FILE* Read(const ContentHash& key) const;
  FILE* Write(const ContentHash& key);
  bool Exists(const ContentHash& key) const;
  // Returns true if it erased something.
  bool Erase(const ContentHash& key);

 private:
  void MakePath(const ContentHash& key, std::string* path,
                std::string* subdir) const;

  std::string root_path_;
};

#endif  // OBJECT_STORE_H_
