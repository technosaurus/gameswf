
#include <string>
#include "gameswf_movie.h"
#include "gameswf_log.h"

namespace gameswf
{

MovieClipLoader::MovieClipLoader()
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
}

MovieClipLoader::~MovieClipLoader()
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
}

void
MovieClipLoader::load(std::string filespec)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
}

// progress of the downloaded file(s).
as_object *
MovieClipLoader::getProgress(as_object *ao)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
  return NULL;
}


bool
MovieClipLoader::loadClip(std::string str, void *)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
  return false;
}

void
MovieClipLoader::unloadClip(void *)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
}


void
MovieClipLoader::addListener(void *)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
}


void
MovieClipLoader::removeListener(void *)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
}

  
// Callbacks
  
void
MovieClipLoader::onLoadStart(void *)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
}

void
MovieClipLoader::onLoadProgress(void *)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
}

void
MovieClipLoader::onLoadInit(void *)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
}

void
MovieClipLoader::onLoadComplete(void *)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
}

void
MovieClipLoader::onLoadError(void *)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
}

void
moviecliploader_loadclip(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
  // const tu_string filespec = env->bottom(first_arg).to_string();
  log_msg("%s: FIXME: nargs = %d\n", __PRETTY_FUNCTION__, nargs);
  
}

void
moviecliploader_unloadclip(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
  const tu_string filespec = env->bottom(first_arg).to_string();
  log_msg("%s: FIXME: Load Movie Clip: %s\n", __PRETTY_FUNCTION__, filespec.c_str());
  
}

void
moviecliploader_new(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
    log_msg("%s: args=%d\n", __PRETTY_FUNCTION__, nargs);
    
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
  log_msg("%s: FIXME: Default event handler, you shouldn't be here!\n", __PRETTY_FUNCTION__);
}

// Invoked when a call to MovieClipLoader.loadClip() has successfully
// begun to download a file.
void
moviecliploader_onload_start(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
  log_msg("%s: FIXME: Default event handler, you shouldn't be here!\n", __PRETTY_FUNCTION__);
}

// Invoked every time the loading content is written to disk during
// the loading process.
void
moviecliploader_getprogress(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
  log_msg("%s: FIXME: Default event handler, you shouldn't be here!\n", __PRETTY_FUNCTION__);

  as_value	method;

  log_msg("%s:\n", __PRETTY_FUNCTION__);
    
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
  log_msg("%s: FIXME: Default event handler, you shouldn't be here!\n", __PRETTY_FUNCTION__);
}

// Invoked when a file loaded with MovieClipLoader.loadClip() has failed to load.
void
moviecliploader_onload_error(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
  log_msg("%s: FIXME: Default event handler, you shouldn't be here!\n", __PRETTY_FUNCTION__);
}

// This is the default event handler. To wind up here is an error.
void
moviecliploader_default(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
  log_msg("%s: FIXME: Default event handler, you shouldn't be here!\n", __PRETTY_FUNCTION__);
}

} // end of gameswf namespace
