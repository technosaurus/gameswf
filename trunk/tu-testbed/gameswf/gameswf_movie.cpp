
#include "gameswf_movie.h"
#include "gameswf_log.h"

namespace gameswf
{

MovieClipLoader::MovieClipLoader()
{
#ifdef __PRETTY_FUNCTION__
  log_msg("%s: \n", __PRETTY_FUNCTION__);
#endif
}

MovieClipLoader::~MovieClipLoader()
{
#ifdef __PRETTY_FUNCTION__
  log_msg("%s: \n", __PRETTY_FUNCTION__);
#endif
}

void
MovieClipLoader::load(const tu_string& filespec)
{
#ifdef __PRETTY_FUNCTION__
  log_msg("%s: \n", __PRETTY_FUNCTION__);
#endif
}

// progress of the downloaded file(s).
as_object *
MovieClipLoader::getProgress(as_object *ao)
{
#ifdef __PRETTY_FUNCTION__
  log_msg("%s: \n", __PRETTY_FUNCTION__);
#endif
  return NULL;
}


bool
MovieClipLoader::loadClip(const tu_string& str, void *)
{
#ifdef __PRETTY_FUNCTION__
  log_msg("%s: \n", __PRETTY_FUNCTION__);
#endif
  return false;
}

void
MovieClipLoader::unloadClip(void *)
{
#ifdef __PRETTY_FUNCTION__
  log_msg("%s: \n", __PRETTY_FUNCTION__);
#endif
}


void
MovieClipLoader::addListener(void *)
{
#ifdef __PRETTY_FUNCTION__
  log_msg("%s: \n", __PRETTY_FUNCTION__);
#endif
}


void
MovieClipLoader::removeListener(void *)
{
#ifdef __PRETTY_FUNCTION__
  log_msg("%s: \n", __PRETTY_FUNCTION__);
#endif
}

  
// Callbacks
  
void
MovieClipLoader::onLoadStart(void *)
{
#ifdef __PRETTY_FUNCTION__
  log_msg("%s: \n", __PRETTY_FUNCTION__);
#endif
}

void
MovieClipLoader::onLoadProgress(void *)
{
#ifdef __PRETTY_FUNCTION__
  log_msg("%s: \n", __PRETTY_FUNCTION__);
#endif
}

void
MovieClipLoader::onLoadInit(void *)
{
#ifdef __PRETTY_FUNCTION__
  log_msg("%s: \n", __PRETTY_FUNCTION__);
#endif
}

void
MovieClipLoader::onLoadComplete(void *)
{
#ifdef __PRETTY_FUNCTION__
  log_msg("%s: \n", __PRETTY_FUNCTION__);
#endif
}

void
MovieClipLoader::onLoadError(void *)
{
#ifdef __PRETTY_FUNCTION__
  log_msg("%s: \n", __PRETTY_FUNCTION__);
#endif
}

void
moviecliploader_loadclip(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
#ifdef __PRETTY_FUNCTION__
  // const tu_string filespec = env->bottom(first_arg).to_string();
  log_msg("%s: FIXME: nargs = %d\n", __PRETTY_FUNCTION__, nargs);
#endif  
}

void
moviecliploader_unloadclip(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
  const tu_string filespec = env->bottom(first_arg).to_string();
#ifdef __PRETTY_FUNCTION__
  log_msg("%s: FIXME: Load Movie Clip: %s\n", __PRETTY_FUNCTION__, filespec.c_str());
#endif  
}

void
moviecliploader_new(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
#ifdef __PRETTY_FUNCTION__
    log_msg("%s: args=%d\n", __PRETTY_FUNCTION__, nargs);
#endif    
    const tu_string filespec = env->bottom(first_arg).to_string();
    
    // Load the actual movie.
    gameswf::movie_definition*	md = gameswf::create_movie(filespec);
    if (md == NULL)
      {
        fprintf(stderr, "error: can't create a movie from '%s'\n", filespec.c_str());
        exit(1);
      }
    gameswf::movie_interface*	m = md->create_instance();
    if (m == NULL)
      {
        fprintf(stderr, "error: can't create movie instance\n");
        exit(1);
      }
}

void
moviecliploader_onload_init(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
#ifdef __PRETTY_FUNCTION__
  log_msg("%s: FIXME: Default event handler, you shouldn't be here!\n", __PRETTY_FUNCTION__);
#endif
}

// Invoked when a call to MovieClipLoader.loadClip() has successfully
// begun to download a file.
void
moviecliploader_onload_start(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
#ifdef __PRETTY_FUNCTION__
  log_msg("%s: FIXME: Default event handler, you shouldn't be here!\n", __PRETTY_FUNCTION__);
#endif
}

// Invoked every time the loading content is written to disk during
// the loading process.
void
moviecliploader_getprogress(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
#ifdef __PRETTY_FUNCTION__
  log_msg("%s: FIXME: Default event handler, you shouldn't be here!\n", __PRETTY_FUNCTION__);
#endif
  as_value	method;

#ifdef __PRETTY_FUNCTION__
  log_msg("%s:\n", __PRETTY_FUNCTION__);
#endif
    
  moviecliploader_as_object*	ptr = (moviecliploader_as_object*) (as_object*) this_ptr;
  assert(ptr);
  as_object *obj = (as_object *)env->bottom(first_arg).to_object();
  result->set(ptr->mov_obj.getProgress(obj));

}

// Invoked when a file loaded with MovieClipLoader.loadClip() has
// completely downloaded.
void
moviecliploader_onload_complete(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
#ifdef __PRETTY_FUNCTION__
  log_msg("%s: FIXME: Default event handler, you shouldn't be here!\n", __PRETTY_FUNCTION__);
#endif
}

// Invoked when a file loaded with MovieClipLoader.loadClip() has failed to load.
void
moviecliploader_onload_error(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
#ifdef __PRETTY_FUNCTION__
  log_msg("%s: FIXME: Default event handler, you shouldn't be here!\n", __PRETTY_FUNCTION__);
#endif
}

// This is the default event handler. To wind up here is an error.
void
moviecliploader_default(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
#ifdef __PRETTY_FUNCTION__
  log_msg("%s: FIXME: Default event handler, you shouldn't be here!\n", __PRETTY_FUNCTION__);
#endif
}

} // end of gameswf namespace
