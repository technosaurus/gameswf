// gameswf_xml.h      -- Rob Savoye <rob@welcomehome.org> 2005

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include <string>
#include "gameswf_textfield.h"
#include "gameswf_log.h"

namespace gameswf 
{
  TextField::TextField()
  {
    log_msg("%s: \n", __PRETTY_FUNCTION__);
  }

  TextField::~TextField()
  {
  }
  

  void
  textfield_new(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
  {
    log_msg("%s: args=%d\n", __PRETTY_FUNCTION__, nargs);
  }
}


