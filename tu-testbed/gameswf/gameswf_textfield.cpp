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


