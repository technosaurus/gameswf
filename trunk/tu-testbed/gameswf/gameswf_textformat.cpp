// gameswf_xml.h      -- Rob Savoye <rob@welcomehome.org> 2005

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "gameswf_textformat.h"
#include "gameswf_log.h"


#ifndef __PRETTY_FUNCTION__
#define __PRETTY_FUNCTION__ "__PRETTY_FUNCTION__"
#endif

namespace gameswf
{  

text_format::text_format()
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
}

text_format::~text_format()
{
}


// In a paragraph, change the format of a range of characters.
void
text_format::setTextFormat (text_format &format)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
}

void
text_format::setTextFormat (int index, text_format &format)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
}

void
text_format::setTextFormat (int start, int end, text_format &format)
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
}


void
textformat_new(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
    log_msg("%s: args=%d\n", __PRETTY_FUNCTION__, nargs);

    textformat_as_object*	text_obj = new textformat_as_object;
    text_obj->set_member("underline", &textformat_underline);

    result->set(text_obj);
}

void
textformat_underline(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
    log_msg("%s: args=%d\n", __PRETTY_FUNCTION__, nargs);
    textformat_as_object*	ptr = (textformat_as_object*) (as_object*) this_ptr;
    assert(ptr);
    
    result->set(ptr->text_obj.underlined());
}


} // end of gameswf namespace
