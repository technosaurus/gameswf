// gameswf_xml.h      -- Rob Savoye <rob@welcomehome.org> 2005

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "gameswf_log.h"
#include "gameswf_textformat.h"

namespace gameswf
{  

  text_format::text_format() :
      _underline(-1),
      _italic(-1),
      _bold(-1),
      _bullet(-1),
      _block_indent(-1),
      _color(0),
      _indent(-1),
      _leading(-1),
      _left_margin(-1),
      _right_margin(-1),
      _point_size(-1),
      _tab_stops(-1),
      _target(-1)
{
  //log_msg("%s: \n", __FUNCTION__);
}

text_format::~text_format()
{
  // don't need to clean up anything
}

// Copy one text_format object to another.
text_format *
text_format::operator = (text_format &format)
{
  log_msg("%s: \n", __FUNCTION__);

  _underline = format._underline;
  _bold = format._bold;
  _italic = format._italic;
  _bullet = format._bullet;
  
  _align = format._align;
  _block_indent = format._block_indent;
  _color = format._color;
  _font = format._font;
  _indent = format._indent;
  _leading = format._leading;
  _left_margin = format._left_margin;
  _right_margin = format._right_margin;
  _point_size = format._point_size;
  _tab_stops = format._tab_stops;
  _target = format._target;
  _url = format._url;
  
  return this;
}

// In a paragraph, change the format of a range of characters.
void
text_format::setTextFormat (text_format &format)
{
  //log_msg("%s: \n", __FUNCTION__);
}

void
text_format::setTextFormat (int index, text_format &format)
{
  //log_msg("%s: \n", __FUNCTION__);
}

void
text_format::setTextFormat (int start, int end, text_format &format)
{
  //log_msg("%s: \n", __FUNCTION__);
}

text_format &
text_format::getTextFormat ()
{
  log_msg("%s: \n", __FUNCTION__);
}

text_format &
text_format::getTextFormat (int index)
{
  log_msg("%s: \n", __FUNCTION__);
}

text_format &
text_format::getTextFormat (int start, int end)
{
  log_msg("%s: \n", __FUNCTION__);
}

void
textformat_new(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
  //log_msg("%s: args=%d\n", __FUNCTION__, nargs);

  textformat_as_object*	text_obj = new textformat_as_object;
  log_msg("\tCreated New TextFormat object at %p\n", text_obj);
  
  env->set_variable("setTextFormat", &textformat_setformat, 0);
  
  result->set_as_object_interface(text_obj);
}


void
textformat_setformat(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
    log_msg("%s: args=%d unfinished implementation\n", __FUNCTION__, nargs);
    textformat_as_object*	ptr = (textformat_as_object*)this_ptr;
    double start = env->bottom(first_arg).to_number();
    double end = env->bottom(first_arg-1).to_number();
    textformat_as_object *obj = (textformat_as_object *)env->bottom(first_arg-2).to_object();
    assert(ptr);
    
    ptr->obj.setTextFormat(start, end, obj->obj);
    result->set_bool(true);
}

void
textformat_getformat(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
    log_msg("%s: args=%d unfinished implementation\n", __FUNCTION__, nargs);
    textformat_as_object*	ptr = (textformat_as_object*)this_ptr;
    double start = env->bottom(first_arg).to_number();
    double end = env->bottom(first_arg-1).to_number();
    textformat_as_object *obj = (textformat_as_object *)env->bottom(first_arg-2).to_object();
    assert(ptr);
    
    ptr->obj = ptr->obj.getTextFormat();
    result->set_bool(true);
}

} // end of gameswf namespace
