// gameswf_xml.h      -- Rob Savoye <rob@welcomehome.org> 2005

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef __TEXTFIELD_H__
#define __TEXTFIELD_H__

#include <string>

#include "gameswf_log.h"
#include "gameswf_action.h"
#include "gameswf_impl.h"
#include "gameswf_log.h"

namespace gameswf 
{

  class TextField
    {
    public:
      TextField();
      ~TextField();
      // setTextField
    private:
      
    };
  
  struct textfield_as_object : public gameswf::as_object
  {
    TextField text_obj;
  };
  
  void
    textfield_new(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg);

} // end of gameswf namespace

// __TEXTFIELD_H__
#endif
