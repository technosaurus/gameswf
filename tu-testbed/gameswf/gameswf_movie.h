#ifndef __MOVIE_H__
#define __MOVIE_H__

#include "gameswf_button.h"
#include "gameswf_log.h"
#include "gameswf_action.h"
#include "gameswf_impl.h"

namespace gameswf
{
  struct mcl {
    int bytes_loaded;
    int bytes_total;
  };

#if 0
struct MovieClipLoader : public character
{
  MovieClipLoader(movie* parent, int id) :
      character(parent, id)
  {
    log_msg("%s: \n", __FUNCTION__);
  }
#else
  class MovieClipLoader
  {
#endif
  public:
    MovieClipLoader();

  ~MovieClipLoader();

  void load(const tu_string& filespec);
  
  struct mcl *getProgress(as_object *ao);

  bool loadClip(const tu_string& str, void *);
  void unloadClip(void *);
  void addListener(void *);
  void removeListener(void *);

  void	on_button_event(event_id event);
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
  struct mcl    _mcl;
  mouse_state   _mouse_state;
};

struct moviecliploader_as_object : public gameswf::as_object
{
  MovieClipLoader mov_obj;
};

struct mcl_as_object : public gameswf::as_object
{
  struct mcl data;
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
