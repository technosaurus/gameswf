// container.h	-- Thatcher Ulrich <tu@tulrich.com> 31 July 2001

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Generic C++ containers.  Problem: STL is murder on compile times,
// and is hard to debug.  And also it's really easy to shoot yourself
// in the foot.  These are not as comprehensive, but might be
// better in those other dimensions.


#ifndef CONTAINER_H
#define CONTAINER_H


namespace tu {

	// Butt-simple, somewhat opaque interface to a list class that
	// contains void* only.  User is responsible for memory management
	// of the objects pointed to.
	class	void_list {
	public:
		void_list();
		~void_list();

		void	insert( ... );
		void_list*	next();

		// For STL-style iteration.
		void_list*	begin();
		void_list*	end() { return 0; }
		void_list*	operator++() { return next(); }

	private:
		elem*	head;
	};


	template< class T >
	class array {
		
	};

};


#endif // CONTAINER_H

