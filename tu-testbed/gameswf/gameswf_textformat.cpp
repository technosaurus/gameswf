#include <string>
#include "gameswf_textformat.h"
#include "gameswf_log.h"
using namespace gameswf;


TextFormat::TextFormat()
{
  log_msg("%s: \n", __PRETTY_FUNCTION__);
}

TextFormat::~TextFormat()
{
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

