// file_deps.cpp -- Thatcher Ulrich <tu@tulrich.com> 2009

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "file_deps.h"
#include "context.h"
#include "hash_util.h"
#include "object_store.h"
#include "os.h"
#include "target.h"
#include "util.h"

// build all, if any of these changed since last successful compile:
// * compile template
// * compile environment
// * inc_dirs
//
// Compile a source file if:
// * build_all is true
// * source changed since last successfully compiled
// * any of its headers changed (recursive check)
//
// Re-link/re-archive a target if any of these changed since last link/archive:
// * any source file compiled
// * any deps compiled


// #include path search order (observed in both cygwin gcc 3.4.4 and
// Visual Studio 2005):
//
// * quoted include paths (e.g. #include "myfile.h") are first
//   searched for relative to the path of the file that has the
//   #include directive.  Then they are searched for in the list of
//   paths from -I options.
//
// * bracketed include paths (e.g. #include <myfile.h>) are searched
//   for in the list of paths from -I options.
//
// * paths in -I options are expressed relative to the current working
//   dir of the compiler process.

// Returns ERR_PARSE if the line was invalid.
Res NextDepsLine(ContentHash* hash, std::string* dep_path, FILE* fp) {
  static const int BUFSIZE = 1000;
  char linebuf[BUFSIZE];
  while (fgets(linebuf, BUFSIZE, fp)) {
    if (linebuf[0] == '\r' || linebuf[0] == '\n' || linebuf[0] == '#') {
      // Skip.
      continue;
    }
    int len = strlen(linebuf);

    // Trim.
    while (len >= 0 && (linebuf[len - 1] == '\n' || linebuf[len - 1] == '\r')) {
      linebuf[len - 1] = 0;
      len--;
    }

    // Deps line should look like:
    //
    // abcdef[27 chars of sha1]012345 path/to/some/header/file.h
    if (len < 29 || !hash->InitFromReadable(linebuf)) {
      // Malformed deps line; need at least 27 chars for the sha1
      // content hash, plus a space, plus at least one char for the
      // dep filename.
      return Res(ERR_PARSE, std::string("bad deps line: ") + linebuf);
    }

    *dep_path = (linebuf + 28);
    return Res(OK);
  }

  return Res(ERR_END_OF_FILE);
}

bool AnyIncludesChanged(const Target* t, const Context* context,
                        const std::string& inc_dirs_str,
                        const std::string& src_path,
                        const ContentHash& src_hash) {
  Res res;
  ContentHash deps_file_id;
  deps_file_id.AppendString("src_deps");
  deps_file_id.AppendHash(src_hash);
  deps_file_id.AppendString(inc_dirs_str);

  ObjectStore* ostore = context->GetObjectStore();
  FILE* fp = ostore->Read(deps_file_id);
  if (!fp) {
    // No deps file; assume that deps may have changed.
    context->LogVerbose("no deps file for " + src_path +
                        "; assuming changed\n");
    return true;
  }

  // Read the deps file, and recursively check the deps.
  std::vector<std::string> to_check;
  int linenum = 0;
  ContentHash previous_dep_hash;
  std::string dep_path;
  for (;;) {
    res = NextDepsLine(&previous_dep_hash, &dep_path, fp);
    if (res.value() == ERR_END_OF_FILE) {
      // Done.
      break;
    }
    if (res.value() == ERR_PARSE) {
      fclose(fp);
      context->LogVerbose("parse error while processing deps for " + src_path +
                          "; " + res.detail());
      // Wipe this deps file since it's not valid.
      ostore->Erase(deps_file_id);
      // Need to assume something changed.
      return true;
    }

    ContentHash current_dep_hash;
    res = context->ComputeOrGetFileContentHash(dep_path, &current_dep_hash);
    if (!res.Ok()) {
      context->LogVerbose("While processing deps for " + src_path +
                          ", couldn't get content hash of " + dep_path);
      fclose(fp);
      return true;
    }
    if (current_dep_hash != previous_dep_hash) {
      // The dependency changed.
      fclose(fp);

      char previous_hash_readable[28];
      previous_dep_hash.GetReadable(previous_hash_readable);
      previous_hash_readable[27] = 0;
      char current_hash_readable[28];
      current_dep_hash.GetReadable(current_hash_readable);
      current_hash_readable[27] = 0;
      context->LogVerbose("found deps change while processing " + src_path +
                          "; line:\n" + previous_hash_readable + " " +
                          dep_path + "\n" +
                          current_hash_readable + "\n");

      return true;
    }

    // The dependency hasn't changed itself.  Check *its* dependencies
    // recursively.
    to_check.push_back(dep_path);
  }
  fclose(fp);

  ContentHash dep_hash;
  for (size_t i = 0; i < to_check.size(); i++) {
    // TODO: don't re-check files we've already checked.

    if (!context->ComputeOrGetFileContentHash(to_check[i], &dep_hash).Ok()) {
      context->LogVerbose("can't get hash while processing " + src_path +
                          " for file " + to_check[i] + "\n");
      return true;
    }
    if (AnyIncludesChanged(t, context, inc_dirs_str, to_check[i], dep_hash)) {
      return true;
    }
  }

  return false;
}

// Return true and fill *header_file if we found an #include line.
bool ParseIncludeLine(const char* line, std::string* header_file, bool* is_quoted) {
  assert(header_file);
  assert(is_quoted);

  // Skip leading space.
  while (char c = *line) {
    if (c != ' ' && c != '\t') {
      break;
    }
    line++;
  }

  if (strncmp("#include", line, 8) != 0) {
    // No #include on this line.
    return false;
  }
  line += 8;

  for (;;) {
    char c = *line;
    if (c == 0) {
      return false;
    }
    if (c != ' ' && c != '\t') {
      if (c == '"' || c == '<') {
        if (c == '"') {
          *is_quoted = true;
        }
        // Looks like the #include filename is here.
        *line++;
        const char* p = line;
        while (char d = *p) {
          if ((*is_quoted && d == '"') || (*is_quoted == false && d == '>')) {
            // End of filename.
            header_file->assign(line, p - line);
            return true;
          }
          p++;
        }
      }
      // Bad format.
      return false;
    }
    line++;
  }

  return false;
}

// Searches for an existing header file.  Returns true and fills in
// *header_path if it finds one.
bool FindHeader(const std::string& src_dir, const Target* t,
                const Context* context, const std::string& header_file,
                bool is_quoted, std::string* header_path) {
  // TODO: handle absolute header file names,
  // (e.g. #include "/abs/path/header.h" or "c:/somefile.h")

  if (is_quoted) {
    // First look in the directory where the source file was found.
    std::string path = PathJoin(src_dir, header_file);
    if (FileExists(path)) {
      *header_path = path;
      return true;
    }
  }

  for (size_t i = 0; i < t->inc_dirs().size(); i++) {
    // TODO: handle absolute inc_dirs

    // TODO: tulrich: hm, shouldn't these be relative to the abs
    // output dir???
    std::string path = PathJoin(context->tree_root(),
                                t->inc_dirs()[i]);
    path = PathJoin(path, header_file);
    if (FileExists(path)) {
      *header_path = path;
      return true;
    }
  }
  return false;
}

Res GenerateDepsFile(const Target* t, const Context* context,
                     const std::string& inc_dirs_str,
                     const std::string& src_path,
                     const ContentHash& src_hash) {
  Res res;
  ContentHash deps_file_id;
  deps_file_id.AppendString("src_deps");
  deps_file_id.AppendHash(src_hash);
  deps_file_id.AppendString(inc_dirs_str);

  ObjectStore* ostore = context->GetObjectStore();
  if (ostore->Exists(deps_file_id)) {
    if (context->rebuild_all()) {
      // Regenerate.
      ostore->Erase(deps_file_id);
    } else {
      // Recursively check the contents.
      return Res(OK);
    }
  }

  // Create the deps file.
  FILE* fp_out = ostore->Write(deps_file_id);
  if (!fp_out) {
    return Res(ERR_FILE_ERROR, "Couldn't open deps file output for: " +
               src_path);
  }

  // Just for debugging.
  fprintf(fp_out, "# src_deps %s\n", src_path.c_str());

  FILE* fp_src = fopen(src_path.c_str(), "rb");
  if (!fp_src) {
    fclose(fp_out);
    return Res(ERR_FILE_ERROR, "Couldn't open file for include scanning: " +
               src_path);
  }

  // Scan the source file for #include lines.
  std::vector<std::string> include_paths;
  static const int BUFSIZE = 1000;
  char linebuf[BUFSIZE];
  ContentHash header_hash;
  std::string header_file;
  std::string header_path;
  char readable_hash[27];

  std::string src_dir = FilenamePathPart(src_path);
  
  while (fgets(linebuf, BUFSIZE, fp_src)) {
    bool is_quoted = false;
    if (ParseIncludeLine(linebuf, &header_file, &is_quoted)) {
      if (FindHeader(src_dir, t, context, header_file, is_quoted, &header_path)) {
        include_paths.push_back(header_path);
        res = context->ComputeOrGetFileContentHash(header_path,
                                                   &header_hash);
        if (!res.Ok()) {
          return res;
        }

        header_hash.GetReadable(readable_hash);
        bool ok =
          (fwrite(readable_hash, sizeof(readable_hash),
                  1, fp_out) == 1) &&
          (fputc(' ', fp_out) != EOF) &&
          (fwrite(header_path.c_str(), header_path.size(), 1, fp_out) == 1) &&
          (fputc('\n', fp_out) != EOF);
        if (!ok) {
          fclose(fp_src);
          fclose(fp_out);
          return Res(ERR_FILE_ERROR, "Error writing to deps file for " +
                     src_path);
        }
      }
    }
  }
  fclose(fp_src);
  fclose(fp_out);

  // Recurse to process the various headers we found.
  for (size_t i = 0; i < include_paths.size(); i++) {
    res = context->ComputeOrGetFileContentHash(include_paths[i], &header_hash);
    if (!res.Ok()) {
      return res;
    }
    res = GenerateDepsFile(t, context, inc_dirs_str, include_paths[i],
                           header_hash);
    if (!res.Ok()) {
      return res;
    }
  }

  return Res(OK);
}


