#include "gameswf_log.h"
#include "gameswf_action.h"
#include "gameswf_impl.h"
#include "gameswf_log.h"
#include "base/smart_ptr.h"

#include "gameswf_timers.h"

using namespace std;

namespace gameswf 
{
  Timer::Timer() :
      _which(0),
      _interval(0.0),
      _start(0.0)
  {
  }
  
  Timer::Timer(as_value *obj, int ms)
  {
    setInterval(obj, ms);
    start();
  }
  
  Timer::~Timer()
  {
  }
  
  int
  Timer::setInterval(as_value obj, int ms)
  {
    _function = obj;
    _interval = ms * 0.01;
    // _interval = ms * 0.000001;
    start();
  }

  void
  Timer::clearInterval()
  {
    _interval = 0.0;
    _start = 0.0;
  }
  
  void
  Timer::start()
  {
    uint64 ticks = tu_timer::get_profile_ticks();
    _start = (time_t)tu_timer::profile_ticks_to_seconds(ticks);
  }
  

  bool
  Timer::expired()
  {
    if (_start > 0.0) {
      uint64 ticks = tu_timer::get_profile_ticks();
      double now = tu_timer::profile_ticks_to_seconds(ticks);
      // log_msg("%s: now is %f, start time is %f, interval is %f\n", __PRETTY_FUNCTION__, now, _start, _interval);
      if (now > _start + _interval) {
        _start = now;               // reset the timer
        // log_msg("Timer expired! \n");
        return true;
      }
    }
    // log_msg("Timer not enabled! \n");
    return false;
  }

  void
  timer_setinterval(as_value* result, as_object_interface* this_ptr, as_environment* env, int nargs, int first_arg)
  {
    int ret;
    as_value	method;
    log_msg("%s: args=%d\n", __PRETTY_FUNCTION__, nargs);
    
    timer_as_object *ptr = new timer_as_object;
    
    //  action_buffer	m_action_buffer;
    //timer_as_object*	ptr = (timer_as_object*) (as_object*) this_ptr;
    assert(ptr);
    
    movie*	mov = env->get_target()->get_root_movie();
    as_as_function *as_func = (as_as_function *)env->bottom(first_arg).to_as_function();
    as_value val(as_func);
    int ms = (int)env->bottom(first_arg-1).to_number();

    ptr->obj.setInterval(val, ms);
    
    result->set(mov->add_interval_timer(&ptr->obj));
  }
  
  void
  timer_expire(as_value* result, as_object_interface* this_ptr, as_environment* env)
  {
    log_msg("%s:\n", __PRETTY_FUNCTION__);
    as_value	*val;

    timer_as_object*	ptr = (timer_as_object*) (as_object*) this_ptr;
    assert(ptr);
    val = ptr->obj.getASFunction();
    
    if (as_as_function* as_func = val->to_as_function()) {
      // It's an ActionScript function.  Call it.
      log_msg("Calling ActionScript function for setInterval Timer\n");
      (*as_func)(val, this_ptr, env, 0, 0);
    } else {
      log_error("FIXME: Couldn't find setInterval Timer!\n");
    }
    
    result->set(val); 
  }
  
  void
  timer_clearinterval(as_value* result, as_object_interface* this_ptr, as_environment* env, int nargs, int first_arg)
  {
    log_msg("%s: nargs = %d\n", __PRETTY_FUNCTION__, nargs);
    as_value	*val;

    double id = env->bottom(first_arg).to_number();

    movie*	mov = env->get_target()->get_root_movie();
    //mov->clear_interval_timer((int)id);
    result->set(val); 
  }
}



