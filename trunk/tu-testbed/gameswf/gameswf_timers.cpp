// gameswf_xml.h      -- Rob Savoye <rob@welcomehome.org> 2005

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "gameswf_log.h"
#include "gameswf_action.h"
#include "gameswf_impl.h"
#include "gameswf_log.h"
#include "base/smart_ptr.h"

#include "gameswf_timers.h"
#include "gameswf_xml.h"

using namespace std;

namespace gameswf 
{
  Timer::Timer() :
      _which(0),
      _interval(0.0),
      _start(0.0),
      _object(0),
      _env(0)
  {
  }
  
  Timer::Timer(as_value *obj, int ms)
  {
    setInterval(*obj, ms);
    start();
  }
  
  Timer::~Timer()
  {
    log_msg("%s: \n", __FUNCTION__);
  }
  
  int
  Timer::setInterval(as_value obj, int ms)
  {
    _function = obj;
    _interval = ms * 0.01;
    // _interval = ms * 0.000001;
    start();

    return 0;
  }

  int
  Timer::setInterval(as_value obj, int ms, as_environment *en)
  {
    _function = obj;
    _interval = ms * 0.01;
    _env = en;
    // _interval = ms * 0.000001;
    start();

    return 0;
  }
  int
  Timer::setInterval(as_value obj, int ms, array<struct variable *> *locals)
  {
    _function = obj;
    _interval = ms * 0.01;
    _locals = locals;
    // _interval = ms * 0.000001;
    start();

    return 0;
  }

  int
  Timer::setInterval(as_value obj, int ms, as_object *this_ptr, as_environment *en)
  {
    _function = obj;
    _interval = ms * 0.01;
    _env = en;
    _object = this_ptr;
    // _interval = ms * 0.000001;
    start();

    return 0;
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
    _start = tu_timer::profile_ticks_to_seconds(ticks);
  }
  

  bool
  Timer::expired()
  {
    if (_start > 0.0) {
      uint64 ticks = tu_timer::get_profile_ticks();
      double now = tu_timer::profile_ticks_to_seconds(ticks);
      //printf("FIXME: %s: now is %f, start time is %f, interval is %f\n", __FUNCTION__, now, _start, _interval);
      if (now > _start + _interval) {
        _start = now;               // reset the timer
        //log_msg("Timer expired! \n");
        return true;
      }
    }
    // log_msg("Timer not enabled! \n");
    return false;
  }

  void
  timer_setinterval(as_value* result, as_object_interface* this_ptr, as_environment* env, int nargs, int first_arg)
  {
    int i;
    as_value	method;
    log_msg("%s: args=%d\n", __FUNCTION__, nargs);
    
    timer_as_object *ptr = new timer_as_object;
    
    //  action_buffer	m_action_buffer;
    //timer_as_object*	ptr = (timer_as_object*) (as_object*) this_ptr;
    assert(ptr);
    
    movie*	mov = env->get_target()->get_root_movie();
    as_as_function *as_func = (as_as_function *)env->bottom(first_arg).to_as_function();
    as_value val(as_func);
    int ms = (int)env->bottom(first_arg-1).to_number();

    tu_string local_name;
    as_value local_val;
    array<with_stack_entry>	dummy_with_stack;

    env->add_frame_barrier();
    //method = env->get_variable("loopvar", dummy_with_stack);

    // FIXME: This is pretty gross, but something is broke elsewhere and it doesn't
    // seem to effect anything else. When a function is called from a executing
    // function, like calling setInterval() from within the callback to
    // XMLSocket::onConnect(), the local variables of the parent function need to
    // be propogated to the local stack as regular variables (not locals) or
    // they can't be found in the scope of the executing chld function. There is
    // probably a better way to do this... but at least this works.
    for (i=0; i< env->get_local_frame_top(); i++) {
      if (env->m_local_frames[i].m_name.size()) {
        //method = env->get_variable(env->m_local_frames[i].m_name, dummy_with_stack);
        //if (method.get_type() != as_value::UNDEFINED)
        {
          local_name  = env->m_local_frames[i].m_name;
          local_val = env->m_local_frames[i].m_value;
          env->set_variable(local_name, local_val, 0);
        }
      }
    }
    //    ptr->obj.setInterval(val, ms, (as_object *)ptr, env);
    
    //Ptr->obj.setInterval(val, ms);
    ptr->obj.setInterval(val, ms, (as_object *)ptr, env);
    
    result->set_int(mov->add_interval_timer(&ptr->obj));
  }
  
  void
  timer_expire(as_value* result, as_object_interface* this_ptr, as_environment* env)
  {
    //log_msg("%s:\n", __FUNCTION__);

    timer_as_object*	ptr = (timer_as_object*) (as_object*) this_ptr;
    assert(ptr);
    const as_value&	val = ptr->obj.getASFunction();
    
    if (as_as_function* as_func = val.to_as_function()) {
      // It's an ActionScript function.  Call it.
      log_msg("Calling ActionScript function for setInterval Timer\n");
      (*as_func)(result, this_ptr, env, 0, 0);
    } else {
      log_error("FIXME: Couldn't find setInterval Timer!\n");
    }
  }
  
  void
  timer_clearinterval(as_value* result, as_object_interface* this_ptr, as_environment* env, int nargs, int first_arg)
  {
    //log_msg("%s: nargs = %d\n", __FUNCTION__, nargs);

    double id = env->bottom(first_arg).to_number();

    movie*	mov = env->get_target()->get_root_movie();
    mov->clear_interval_timer((int)id);
    result->set_bool(true); 
  }
}
