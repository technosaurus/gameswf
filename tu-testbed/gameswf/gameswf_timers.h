// gameswf_xml.h      -- Rob Savoye <rob@welcomehome.org> 2005

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef __TIMERS_H__
#define __TIMERS_H_

#include <sys/time.h>

#include "gameswf_log.h"
#include "gameswf_action.h"
#include "gameswf_impl.h"
#include "gameswf_log.h"

#include "base/tu_timer.h"
#include <sys/time.h>

namespace gameswf 
{
  
  struct variable {
    tu_string name;
    as_value value;
  };

  class Timer
    {
    public:
      Timer();
      Timer(as_value *obj, int ms);
      Timer(as_value method, int ms);
      ~Timer();
      int setInterval(as_value obj, int ms);
      int setInterval(as_value obj, int ms, as_object *this_ptr, as_environment *env);
      int setInterval(as_value obj, int ms, as_environment *env);
      int setInterval(as_value obj, int ms, array<variable *> *locals);
      void setInterval(int ms) 
      {
        _interval = ms * 0.000001;
      }

      void clearInterval();
      void start();
      bool expired();
      void setObject(as_object *ao) { _object = ao; }
      as_object *getObject() { return _object; }
      
      // Accessors
      as_value *getASFunction() { return &_function;  }
      as_environment *getASEnvironment() { return _env;  }
      as_object *getASObject() { return _object;  }
      array<struct variable *> *getLocals() { return _locals;  }
      int getIntervalID()  { return _which;  }
      void add_local(tu_string name, as_value value) {
        struct variable *var = new struct variable;
        var->name = name;
        var->value = value;
        _locals->push_back(var);
      }
      

    private:
      tu_string       _method;
      int             _which;                // Which timer
      double          _interval;
      double          _start;
      as_value        _function;
      as_object      *_object;
      as_environment *_env;
      array<struct variable *> *_locals;
      
    };
  
  struct timer_as_object : public gameswf::as_object
  {
    Timer obj;
  };
  
  void timer_setinterval(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg);

  void timer_clearinterval(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg);


  void timer_expire(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env);
  
} // end of namespace gameswf

  // __TIMERS_H_
#endif
