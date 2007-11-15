// tu_gc.h  -- Thatcher Ulrich <http://tulrich.com> 2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A simple smart pointer and framework for garbage collection.  A
// variety of different collectors may be plugged in, so the gc_ptr is
// parameterized by the collector.
//
// This differs from the boost gc_ptr proposals I've seen in that it
// is "intrusive" -- any classes that you want garbage-collected must
// derive from gc_object_base.  Any instances that are dynamically
// allocated must be allocated via gc_object_base::new (but C++ will
// handle this automatically).
//
// It's not really recommended practice, but gc_objects can be put on
// the stack or kept as members inside other objects -- with the
// caveat that you should not point gc_ptrs at such instances.
//
// gc_ptr's can live anywhere; on the stack, on the garbage-collected
// heap, on the regular heap.  They are the same size as native
// pointers (but there may be substantial per-pointer overhead inside
// the collector, for bookkeeping).  gc_ptr's outside the
// garbage-collected heap are "roots"; i.e. they cannot be collected
// by the garbage collector.  Objects on the gc heap are collected
// automatically.
//
// gc_ptr's are parameterized by garbage collector type.  Different gc
// types have different characteristics.
//
// Note: if you feel the desire to point a gc_ptr at a type that is
// not derived from gc_object_base, you probably want a scoped_ptr
// instead.
//
// Usage:
//
// #include <base/tu_gc.h>
// #include <base/tu_gc_singlthreaded_marksweep.h>  // specific collector
//
// // Declare the types gc_ptr<T>, gc_object, and gc_collector,
// // parameterized by the specific garbage collector you are using.
// DECLARE_GC_TYPES(tu_gc::singlethreaded_marksweep);
//
// ...
// class my_object : public gc_object {
//   gc_ptr<my_object> m_ptr;
//   gc_ptr<other_object> m_ptr2;
//
//   ...
//     m_ptr = new my_object(args);
//     m_ptr->some_method();
// };
//
// ...
//   my_object local_instance;
//   gc_ptr<my_object> obj = new my_object;
//   local_instance.m_ptr = obj;
//
// // etc.


// Here's an interesting article on C++ garbage collection:
// http://home.elka.pw.edu.pl/~pkolaczk/papers/gc-prodialog.pdf


// idea from cppgc:  "block lock" to register when construction finishes:
//    void* allocate(size_t sz, const gc_block_lock& lock);
//    inline void* operator new(size_t size, cppgc::gc_tag_t tag, const cppgc::gc_block_lock &lock = cppgc::gc_block_lock()) { return allocate(size, lock); }
//    struct gc_block_lock {
//      void* block;
//      ~gc_block_lock() { gc::unlock_block(block); }
//    };
//
// what this is for: the C++ standard guarantees that the temporary
// block_block object is destructed at the end of the full statement
// it's contained in.  So in a statement like:
//
// gc_ptr<MyType> p = new MyType();
//
// the block_lock won't be destructed until p is safely constructed.


// TODO:
//
// test multiple inheritance; i.e. T* may not be the head of the block
//
// test arrays (or forbid them)


#include <assert.h>

namespace tu_gc {

	class collector_access;

	// Used by the collector to help keep blocks alive during
	// construction (between allocation and when they are finally
	// assigned to a gc_ptr).
	class block_construction_locker_base {
	public:
		block_construction_locker_base() : m_block(0) {
		}
	protected:
		friend collector_access;
		void* m_block;
	};

	template<class gc_collector>
	class block_construction_locker : public block_construction_locker_base {
	public:
		~block_construction_locker() {
			gc_collector::block_construction_finished(m_block);
		}
	};
	
	template<class gc_collector>
	class gc_object_base {
	public:
		virtual ~gc_object_base() {}
		static void* operator new(size_t sz, block_construction_locker_base* lock = &block_construction_locker<gc_collector>()) {
			return gc_collector::allocate(sz, lock);
		}
		static void operator delete(void* p, block_construction_locker_base* lock) {
			return gc_collector::deallocate(p);
		}
		static void operator delete(void* p) {
			return gc_collector::deallocate(p);
		}

	private:
		static void* operator new[](size_t sz) {
			assert(0);
		}
	};

	class gc_ptr_base {
	protected:
		friend class collector_access;
		gc_ptr_base(void* p) : m_ptr(p) {
		}
		void* m_ptr;
	};

	// The smart pointer.
	template<class T, class garbage_collector>
	class gc_ptr : public gc_ptr_base {
	public:
		gc_ptr() : gc_ptr_base(0) {
			garbage_collector::construct_pointer(this);
		}
		gc_ptr(T* p) : gc_ptr_base(0) {
			garbage_collector::construct_pointer(this);
			reset(p);
		}
		gc_ptr(const gc_ptr& p) : gc_ptr_base(0) {
			garbage_collector::construct_pointer(this);
			reset((T*) p.m_ptr);
		}

		~gc_ptr() {
			reset(0);
			garbage_collector::destruct_pointer(this);
		}

		T* operator->() const {
			return (T*) m_ptr;
		}

		T& operator*() const {
			return *(T*) m_ptr;
		}

		void operator=(const gc_ptr& p) {
			reset(p.get());
		}

		void operator=(T* p) {
			reset(p);
		}

		bool operator==(const gc_ptr& p) {
			return m_ptr == p.m_ptr;
		}

		bool operator==(const T* p) {
			return m_ptr == p;
		}

		T* get() const {
			return reinterpret_cast<T*>(m_ptr);
		}

		void reset(T* p) {
			garbage_collector::write_barrier(this, p);
		}
	};

	// TODO: gc_weak_ptr


	// Garbage collectors inherit from this to get direct access
	// to gc_ptr_base::m_ptr and block_construction_locker_base::m_block.
	class collector_access {
	protected:
		static void*& ptr(gc_ptr_base* p) {
			return p->m_ptr;
		}

		static void*& block_ref(block_construction_locker_base* lock) {
			return lock->m_block;
		}
	};
	
	// Use this to declare gc_ptr<T>, gc_objectusing your chosen collector,
	// in whatever namespace is convenient for you.
	//
	// E.g.:
	//
	//   namespace my_namespace {
	//     DECLARE_GC_TYPES(tu_gc::singlethreaded_marksweep);
	//   }
	//
	//   Then use gc_ptr<T> and gc_object in my_namespace.
	//
	#define DECLARE_GC_TYPES(collector_classname) 			\
		typedef collector_classname gc_collector;		\
		typedef tu_gc::gc_object_base<gc_collector> gc_object;  \
		template<class T>					\
		class gc_ptr : public tu_gc::gc_ptr<T, gc_collector> {	\
		public:							\
			gc_ptr() {}					\
			gc_ptr(T* p) : tu_gc::gc_ptr<T, gc_collector>(p) {} \
		};							\

	// TODO: arrays of gc_objects?

	// TODO: simple refcounted gc

	// TODO: incremental gc

	// TODO: incremental generational gc

	// TODO: multithreaded variants

}  // tu_gc
