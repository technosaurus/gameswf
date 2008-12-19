// hash_util.h -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef HASH_UTIL_H_
#define HASH_UTIL_H_

#include <string>

#include "content_hash.h"

// Write the content-hash to the given out_dir, for the file in the
// given file_path.  Name the content-hash according to the filename
// in file_path.
//
// file_path should be relative to out_dir.
//
// If the given hash is zero, then compute the real hash from the file
// contents and write the computed hash.
void ComputeIfNecessaryAndWriteFileHash(const std::string& out_dir,
					const std::string& file_path,
					const ContentHash& hash);

void ReadFileHash(const std::string& out_dir,
		  const std::string& file_path,
		  ContentHash* hash);

void ComputeFileHash(const std::string& out_dir,
		     const std::string& file_path,
		     ContentHash* hash);

#endif  // HASH_UTIL_H_
