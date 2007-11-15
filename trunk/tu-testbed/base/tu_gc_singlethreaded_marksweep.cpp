// tu_gc_singlethreaded_marksweep.cpp  -- Thatcher Ulrich <http://tulrich.com> 2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Single-threaded mark-sweep collector.


#include "base/tu_gc_singlethreaded_marksweep.h"
#include <map>
#include <set>
#include <vector>

namespace tu_gc {

	struct blockinfo {
		void* p;
		size_t sz;

		blockinfo() : p(0), sz(0) {
		}
		blockinfo(void* p_in, size_t sz_in) : p(p_in), sz(sz_in) {
		}

		void* end() const {
			return static_cast<char*>(p) + sz;
		}

		bool operator<(const blockinfo& bi) const {
			return p < bi.p;
		}
	};

	struct singlethreaded_marksweep_state : public collector_access {
		// Convenience typedefs.
		typedef std::set<gc_ptr_base*>::iterator ptr_iterator;
		typedef std::set<void*>::iterator floating_blocks_iterator;
		typedef std::map<blockinfo, bool>::iterator heap_block_iterator;
		typedef std::map<blockinfo, bool>::reverse_iterator heap_block_reverse_iterator;

		// roots
		std::set<gc_ptr_base*> m_roots;

		// A set of blocks that are not yet under the control
		// of a gc_ptr.  Typically this happens between the
		// time gc_object_base::new returns void* and when the
		// value is assigned to a gc_ptr.  We have to treat
		// such blocks as rooted to avoid problems with code
		// like:
		//
		//   gc_ptr<MyObj> p = new MyObj(SomeFunctionThatMightTriggerACollection());
		//
		// since the block for MyObj must be allocated before
		// the MyObj constructor is called, and the MyObj
		// constructor must happen before p is assigned.
		std::set<void*> m_floating_blocks;

		// non-roots
		std::set<gc_ptr_base*> m_heap_ptrs;

		// heap blocks
		std::map<blockinfo, bool> m_heap_blocks;

		// Stats & control values.
		int m_percent_growth;
		size_t m_current_heap_bytes;
		size_t m_next_collection_heap_size;
		size_t m_last_collection_heap_size;

		singlethreaded_marksweep_state() :
			m_percent_growth(100),
			m_current_heap_bytes(0),
			m_next_collection_heap_size(1 << 16),
			m_last_collection_heap_size(1 << 16) {
		}

		void set_collection_rate(int percent) {
			m_percent_growth = percent;
			size_t a = m_last_collection_heap_size;
			double b = m_percent_growth / 100.0;
			double c = a * (1 + b);
			m_next_collection_heap_size = static_cast<size_t>(c);
		}

		void* allocate(size_t sz, block_construction_locker_base* lock) {
			assert(sz > 0);
			assert(lock);

			if (m_current_heap_bytes >= m_next_collection_heap_size) {
				// It's time to collect.
				collect_garbage(NULL);
			}
		
			void* block = operator new(sz /* + overhead? */);
			m_heap_blocks.insert(std::make_pair(blockinfo(block, sz), false));
			m_current_heap_bytes += sz;

			// Keep this block from being collected during
			// construction, before it has a change to be
			// assigned to a gc_ptr.
			m_floating_blocks.insert(block);
			block_ref(lock) = block;

			return block;
		}

		void deallocate(void* p) {
			// assert(no heap pointers inside this block);
		
			// Fill with junk.
			//TODO 0xCAFFE14E

			// Return the memory to the C++ heap.
			operator delete(p);
		}

		void block_construction_finished(void* block) {
			floating_blocks_iterator it = m_floating_blocks.find(block);
			assert(it != m_floating_blocks.end());
			m_floating_blocks.erase(it);
		}

		void get_stats(singlethreaded_marksweep::stats* s) {
			assert(s);
			s->live_heap_bytes = m_current_heap_bytes;
			s->garbage_bytes = 0;
			s->root_pointers = m_roots.size();
			s->live_pointers = s->root_pointers + m_heap_ptrs.size();
		}

		void collect_garbage(singlethreaded_marksweep::stats* s) {
			size_t precollection_heap_bytes = m_current_heap_bytes;

			mark_live_objects();
			sweep_dead_objects();
		
			m_last_collection_heap_size = m_current_heap_bytes;
			set_collection_rate(m_percent_growth);

			if (s) {
				s->live_heap_bytes = m_current_heap_bytes;
				s->garbage_bytes = precollection_heap_bytes - s->live_heap_bytes;
				s->root_pointers = m_roots.size();
				s->live_pointers = s->root_pointers + m_heap_ptrs.size();
			}
		}


		void mark_live_objects() {
			std::vector<void*> to_mark;
			to_mark.reserve(m_heap_blocks.size() /* heuristic */);

			// Mark all blocks pointed to by roots.
			for (ptr_iterator it = m_roots.begin();
			     it != m_roots.end();
			     ++it) {
				void* p = ptr(*it);
				assert(p);
				to_mark.push_back(p);
				//push_heap(to_mark);
			}

			// Mark all the floating blocks.
			for (floating_blocks_iterator it = m_floating_blocks.begin();
			     it != m_floating_blocks.end();
			     ++it) {
				assert(*it);
				to_mark.push_back(*it);
			}

			while (to_mark.size()) {
				//pop_heap(to_mark);
				void* b = to_mark.back();
				to_mark.resize(to_mark.size() - 1);

				heap_block_iterator it = m_heap_blocks.find(blockinfo(b, 0));
				assert(it != m_heap_blocks.end());
				if (it->second == false) {
					// Mark this block.
					it->second = true;

					// Find all gc pointers that are inside this heap block.
					// Mark the blocks that they point to.
					ptr_iterator it_ptr = m_heap_ptrs.lower_bound(static_cast<gc_ptr_base*>(it->first.p));
					void* block_end = it->first.end();
					while (it_ptr != m_heap_ptrs.end() && (*it_ptr) < block_end) {
						void* p = ptr(*it_ptr);
						assert(p);
						to_mark.push_back(p);
						//push_heap(to_mark);
						it_ptr++;
					}
				}
			}
		}

		// Scan through all the heap blocks.  Anything that is
		// not marked is garbage -- delete it!  Anything that
		// is marked is live, and should have its mark
		// cleared.
		void sweep_dead_objects() {
			size_t heap_bytes = 0;

			for (heap_block_iterator it = m_heap_blocks.begin(); it != m_heap_blocks.end(); ) {
				if (it->second == true) {
					// Ham.
					heap_bytes += it->first.sz;
					it->second = false;
					++it;
				} else {
					// Spam.
					gc_object_base<singlethreaded_marksweep>* block = static_cast<gc_object_base<singlethreaded_marksweep>* >(it->first.p);
					delete block;
					m_heap_blocks.erase(it++);
				}
			}

			m_current_heap_bytes = heap_bytes;
		}

		void clearing_pointer(gc_ptr_base* gcp) {
			assert(gcp);
			// remove gcp from pointer list
			ptr_iterator it = m_heap_ptrs.find(gcp);
			if (it != m_heap_ptrs.end()) {
				m_heap_ptrs.erase(it);
			} else {
				it = m_roots.find(gcp);
				assert(it != m_roots.end());
				m_roots.erase(it);
			}
		}

		// Return true if gcp is inside a heap block.
		bool inside_heap_block(gc_ptr_base* gcp) {
			heap_block_iterator it(m_heap_blocks.upper_bound(blockinfo(gcp, 0)));
			if (it != m_heap_blocks.begin()) {
				--it;
				if (gcp < it->first.end()) {
					assert(gcp >= it->first.p);
					// Inside a heap block.
					return true;
				}
			}
			return false;
		}

		void setting_pointer(gc_ptr_base* gcp, void* val) {
			assert(gcp);
			assert(m_roots.find(gcp) == m_roots.end());
			assert(m_heap_ptrs.find(gcp) == m_heap_ptrs.end());
		
			// Inside a heap block?
			if (inside_heap_block(gcp)) {
				m_heap_ptrs.insert(gcp);
			} else {
				m_roots.insert(gcp);
			}
		}

	} sm_state;
	
	/*static*/ void singlethreaded_marksweep::get_stats(singlethreaded_marksweep::stats* s) {
		sm_state.get_stats(s);
	}

	/*static*/ void singlethreaded_marksweep::collect_garbage(singlethreaded_marksweep::stats* s) {
		sm_state.collect_garbage(s);
	}
	
	/*static*/ void singlethreaded_marksweep::set_collection_rate(
		int percent_growth_before_next_collection) {
		sm_state.set_collection_rate(percent_growth_before_next_collection);
	}

	/*static*/ void* singlethreaded_marksweep::allocate(size_t sz, block_construction_locker_base* lock) {
		return sm_state.allocate(sz, lock);
	}

	/*static*/ void singlethreaded_marksweep::deallocate(void* p) {
		sm_state.deallocate(p);
	}

	/*static*/ void singlethreaded_marksweep::block_construction_finished(void* block) {
		sm_state.block_construction_finished(block);
	}

	/*static*/ void singlethreaded_marksweep::clearing_pointer(gc_ptr_base* gcp) {
		sm_state.clearing_pointer(gcp);
	}

	/*static*/ void singlethreaded_marksweep::setting_pointer(gc_ptr_base* gcp, void* new_val) {
		sm_state.setting_pointer(gcp, new_val);
	}

	// In the initial implementation, overhead is pretty high.
	// Using GCBench:
	//
	// * Real storage runs around 2x to 4x of live storage, and
	//   peaks at ~10x of live storage (though this includes all
	//   overheads, internal and external).
	//
	// * Time is 10x slower than explicit mem management.
	//
	// Tempting optimizations:
	//
	// Use root ref-counts on the blocks (i.e. if a pointer is a
	// root, inc/dec ref counts on the pointed-to block; if a
	// pointer is not a root, don't do ref-counting.  This skips
	// the first step of mark-sweep: no need to mark the blocks
	// pointed to by roots -- just scan through the heap looking
	// for blocks that have a positive ref-count; mark their
	// descendents.
	//
	// * maybe not much of a win -- have to scan the heap to get
	//   the root blocks, instead of walking an internal set.
	//
	// * not free -- adds a ref count to each block, adds
	//   ref-count inc/dec to each root ptr change, and we still
	//   need a fast way to decide if a gc_ptr is a root.
	//
	// Decide if a gc_ptr is a root at construction time -- if
	// it's inside a heap block, it's a heap ptr until the block
	// is deleted; otherwise it's a root.
	//
	// * adds a bit to the gc_ptr.  Bloats the ptr, or else we
	//   stash it in the low bit or something; requires unmasking
	//   on pointer use, and re-masking on pointer change.
	//
	// * still need a list of roots for initial marking.  Either
	//   that, or combine with the ref-counting scheme.
	//
	// Conservative determination of root ptrs -- bloom filter?
	// Still need a way for initial marking.
	//
	// Use doubly-linked list for m_roots & m_heap_ptrs?  Might be
	// a win.  Intrudes into the gc_ptr (ideally), so maybe the
	// thing to do is subclass gc_ptr and require that this
	// collector use the subclassed version.
	//
	// Put the mark bits in the block headers.
	//
	// Put the floating_block bits in the block headers.
	//
	// Replace the m_heap_blocks map with links in the block
	// headers (e.g. maybe a skip list).
	
}  // tu_gc


#ifdef TEST_GC_SINGLETHREADED_MARKSWEEP

// cl -GX tu_gc_singlethreaded_marksweep.cpp -I.. -DTEST_GC_SINGLETHREADED_MARKSWEEP
#include <stdio.h>

DECLARE_GC_TYPES(tu_gc::singlethreaded_marksweep);

struct cons : public gc_object {
public:
	cons() : value(0) {
	}
	cons(int val, cons* cdr_in) : value(val), cdr(cdr_in) {
	}
	cons(cons* car_in, cons* cdr_in) : value(0), car(car_in), cdr(cdr_in) {
	}

	int value;
	gc_ptr<cons> car;
	gc_ptr<cons> cdr;
};

gc_ptr<cons> build_list_of_numbers(int low, int high) {
	if (low <= high) {
		return new cons(low, build_list_of_numbers(low + 1, high).get());
	} else {
		return NULL;
	}
}

int main() {
	gc_ptr<cons> root;

	root = build_list_of_numbers(1, 10);

	tu_gc::singlethreaded_marksweep::stats s;
	gc_collector::get_stats(&s);
	printf("%d %d %d %d\n", s.live_heap_bytes, s.garbage_bytes, s.root_pointers, s.live_pointers);

 	printf("collecting...\n");
	gc_collector::collect_garbage(&s);
	printf("%d %d %d %d\n", s.live_heap_bytes, s.garbage_bytes, s.root_pointers, s.live_pointers);

	root = NULL;

 	printf("collecting...\n");
	gc_collector::collect_garbage(&s);
	printf("%d %d %d %d\n", s.live_heap_bytes, s.garbage_bytes, s.root_pointers, s.live_pointers);

	// Try putting a gc_object on the stack...
	{
		cons cell;
		cell.car = build_list_of_numbers(3, 8);

		printf("collecting...\n");
		gc_collector::collect_garbage(&s);
		printf("%d %d %d %d\n", s.live_heap_bytes, s.garbage_bytes, s.root_pointers, s.live_pointers);
	}

	printf("collecting...\n");
	gc_collector::collect_garbage(&s);
	printf("%d %d %d %d\n", s.live_heap_bytes, s.garbage_bytes, s.root_pointers, s.live_pointers);

	return 0;
}


#endif  // TEST_GC_SINGLETHREADED_MARKSWEEP
