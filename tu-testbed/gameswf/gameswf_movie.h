#ifndef __MOVIE_H__
#define __MOVIE_H__

#include "gameswf_log.h"
#include "gameswf_action.h"
#include "gameswf_impl.h"

namespace gameswf
{
  
class MovieClipLoader : public gameswf::character_def
{
 public:
  MovieClipLoader();
  ~MovieClipLoader();

  void load(const tu_string& filespec);
  
  gameswf::as_object *getProgress(gameswf::as_object *ao);

  bool loadClip(const tu_string& str, void *);
  void unloadClip(void *);
  void addListener(void *);
  void removeListener(void *);
  
  // Callbacks
  void onLoadStart(void *);
  void onLoadProgress(void *);
  void onLoadInit(void *);
  void onLoadComplete(void *);
  void onLoadError(void *);
 private:
  bool          _started;
  bool          _completed;
  tu_string     _filespec;
  int           _progress;
  bool          _error;
};

struct moviecliploader_as_object : public gameswf::as_object
{
  MovieClipLoader mov_obj;
};

void
moviecliploader_loadclip(gameswf::as_value* result, gameswf::as_object_interface* this_ptr,
                        gameswf::as_environment* env, int nargs, int first_arg);

void
moviecliploader_unloadclip(gameswf::as_value* result, gameswf::as_object_interface* this_ptr,
                        gameswf::as_environment* env, int nargs, int first_arg);

void
moviecliploader_getprogress(gameswf::as_value* result, gameswf::as_object_interface* this_ptr,
                        gameswf::as_environment* env, int nargs, int first_arg);

void
moviecliploader_new(gameswf::as_value* result, gameswf::as_object_interface* this_ptr,
                        gameswf::as_environment* env, int nargs, int first_arg);

void
moviecliploader_onload_init(gameswf::as_value* result, gameswf::as_object_interface* this_ptr,
                        gameswf::as_environment* env, int nargs, int first_arg);

void
moviecliploader_onload_start(gameswf::as_value* result, gameswf::as_object_interface* this_ptr,
                        gameswf::as_environment* env, int nargs, int first_arg);

void
moviecliploader_onload_progress(gameswf::as_value* result, gameswf::as_object_interface* this_ptr,
                        gameswf::as_environment* env, int nargs, int first_arg);

void
moviecliploader_onload_complete(gameswf::as_value* result, gameswf::as_object_interface* this_ptr,
                        gameswf::as_environment* env, int nargs, int first_arg);

void
moviecliploader_onload_error(gameswf::as_value* result, gameswf::as_object_interface* this_ptr,
                        gameswf::as_environment* env, int nargs, int first_arg);

void
moviecliploader_default(gameswf::as_value* result, gameswf::as_object_interface* this_ptr,
                        gameswf::as_environment* env, int nargs, int first_arg);


} // end of gameswf namespace

// __MOVIE_H__
#endif
