// hash_util.cpp -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include <stdio.h>
#include "hash_util.h"
#include "path.h"

// Given a file path, normalize it to a valid local filename that
// represents the path.
static std::string LocalFileStemFromPath(const std::string& path) {
  // TODO: for now we're just taking the file part, but in the future
  // we might want to take the whole path and normalize it to a valid
  // local file name (i.e. replace slashes and ".." with other
  // characters).
  return FilenameFilePart(path);
}

void ComputeIfNecessaryAndWriteFileHash(const std::string& out_dir,
					const std::string& file_path,
					const ContentHash& hash_in) {
  ContentHash hash = hash_in;
  if (hash.IsZero()) {
    ComputeFileHash(out_dir, file_path, &hash);
  }

  std::string hash_fname = PathJoin(out_dir, LocalFileStemFromPath(file_path));
  hash_fname += ".hash";

  FILE* f = fopen(hash_fname.c_str(), "w");
  if (f) {
    fwrite(hash.data(), hash.size(), 1, f);
    fclose(f);
  }
}

void ReadFileHash(const std::string& out_dir,
		  const std::string& file_path,
		  ContentHash* hash) {
  std::string hash_fname = PathJoin(out_dir, LocalFileStemFromPath(file_path));
  hash_fname += ".hash";

  hash->Reset();
  FILE* f = fopen(hash_fname.c_str(), "r");
  if (f) {
    if (fread(hash->data(), hash->size(), 1, f)) {
      // OK
    } else {
      hash->Reset();
    }
    fclose(f);
  }
}

void ComputeFileHash(const std::string& out_dir,
		     const std::string& file_path,
		     ContentHash* hash) {
  std::string full_path = PathJoin(out_dir, file_path);
  hash->InitFromFile(full_path.c_str());
}
