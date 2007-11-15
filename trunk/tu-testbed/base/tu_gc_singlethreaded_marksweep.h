// tu_gc_singlethreaded_marksweep.h  -- Thatcher Ulrich <http://tulrich.com> 2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A simple but useful singlethreaded mark/sweep garbage collector,
// for use with tu_gc::gc_ptr<>.  It is not thread-safe.
//
// See tu_gc.h for basic usage.  See the documented methods below for
// additional (optional) interfaces.
//
// This implementation is very vanilla C++ and hopefully portable to
// any runtime environment, at the expense of not being especially
// optimized for space or time.
//
// Performance: it's much slower and uses much more space than manual
// explicit memory management.  Using GCBench.cpp (from
// http://www.hpl.hp.com/personal/Hans_Boehm/gc/gc_bench/), it
// measures about 10x slower and 4x fatter than explicit memory
// management (with an initial spike of 7x size).  That's actually not
// too terrible, and there is room for optimization.  See notes at the
// end of the .cpp file.
//
// TODO: compare against a pure ref-counting collector using GCBench.

#include "base/tu_gc.h"

namespace tu_gc {

	class singlethreaded_marksweep : public collector_access {
	public:
		// Client interfaces.
		struct stats {
			size_t live_heap_bytes;
			size_t garbage_bytes;
			size_t root_pointers;
			size_t live_pointers;
		};
		// Gets basic stats.  garbage_bytes will be zero
		// (since we don't know what is garbage) and
		// live_heap_bytes will include everything including
		// potential garbage.
		static void get_stats(stats* s);

		// Collects all garbage.
		//
		// If s is not NULL, fills it with interesting
		// statistics.
		static void collect_garbage(stats* s);

		// Determines how often to automatically collect
		// garbage.  Automatic collection is triggered within
		// the allocator, when the total number of heap bytes
		// exceeds the last-known live byte count by the given
		// percentage.  E.g. if the arg is 100, collection is
		// initiated when the heap size has doubled since the
		// previous collection.
		//
		// 0 will force a collection before every allocation.
		//
		// The default is 100.
		static void set_collection_rate(int percent_growth_before_next_collection);

		// Semi-private interfaces.  TODO: figure out how to
		// protect these.

		// Called by the block construction locker after the
		// new() expression completes (presumably after the
		// new block has been assigned to a gc_ptr).
		static void block_construction_finished(void* block);
		
		// Called by the smart ptr during construction.
		//
		// This collector doesn't care, until the pointer
		// takes a non-null value, which is notified via
		// write_barrier().
		static void construct_pointer(gc_ptr_base* gc_ptr_p) {}
		
		// Called by the smart ptr during destruction.
		//
		// This collector doesn't care.  What it does care
		// about is when the pointer is cleared, which is
		// notified via write_barrier().
		static void destruct_pointer(gc_ptr_base* gc_ptr_p) {}
		
		// Used by the smart ptr to change the value of the
		// pointer.
		//
		// We take notice whenever the pointer changes from
		// null to a value or vice-versa.  The collector
		// doesn't care about null pointers.
		static void write_barrier(gc_ptr_base* gc_ptr_p, void* new_val_p) {
			assert(gc_ptr_p);
			if (ptr(gc_ptr_p)) {
				if (!new_val_p) {
					clearing_pointer(gc_ptr_p);
				} else {
					// change of value, no-op
				}
			} else if (new_val_p) {
				setting_pointer(gc_ptr_p, new_val_p);
			}
			ptr(gc_ptr_p) = new_val_p;
		}
		
	private:
		friend class gc_object_base<singlethreaded_marksweep>;

		// Used by gc_object_base new/delete.
		static void* allocate(size_t sz, block_construction_locker_base* lock);
		static void deallocate(void* p);

		// Notifications from write_barrier().
		static void clearing_pointer(gc_ptr_base*);
		static void setting_pointer(gc_ptr_base*, void* new_val_p);
	};
}  // tu_gc

