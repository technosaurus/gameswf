// ogl.cpp	-- by Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some OpenGL helpers; mainly to generically deal with extensions.


#include <SDL/SDL.h>
#include <engine/ogl.h>
#include <engine/utility.h>


#ifdef WIN32
#include <windows.h>	// for wglGetProcAddress
#endif // WIN32

#include <gl/gl.h>


namespace ogl {

	bool	is_open = false;


	// Pointers to extension functions.
	typedef void * (APIENTRY * PFNWGLALLOCATEMEMORYNVPROC) (int size, float readfreq, float writefreq, float priority);
	typedef void (APIENTRY * PFNWGLFREEMEMORYNVPROC) (void *pointer);
	typedef void (APIENTRY * PFNGLVERTEXARRAYRANGENVPROC) (int size, void* buffer);

	PFNWGLALLOCATEMEMORYNVPROC	wglAllocateMemoryNV = 0;
	PFNWGLFREEMEMORYNVPROC	wglFreeMemoryNV = 0;
	PFNGLVERTEXARRAYRANGENVPROC	glVertexArrayRangeNV = 0;


	// Big, fast vertex-memory buffer.
	const int	VERTEX_BUFFER_SIZE = 4 << 20;
	void*	vertex_memory_buffer = 0;
	int	vertex_memory_top = 0;
	bool	vertex_memory_from_malloc = false;	// tells us whether to wglFreeMemoryNV() or free() the buffer when we're done.

	void	open()
	// Scan for extensions.
	{
		wglAllocateMemoryNV = (PFNWGLALLOCATEMEMORYNVPROC) wglGetProcAddress( "wglAllocateMemoryNV" );
		wglFreeMemoryNV = (PFNWGLFREEMEMORYNVPROC) wglGetProcAddress( "wglFreeMemoryNV" );
		glVertexArrayRangeNV = (PFNGLVERTEXARRAYRANGENVPROC) SDL_GL_GetProcAddress( "glVertexArrayRangeNV" );
	}


	void	close()
	// Release anything we need to.
	{
		// @@ free that mongo vertex buffer.
	}


	void*	allocate_vertex_memory( int size )
	// Allocate a block of memory for storing vertex data.  Using this
	// allocator will hopefully give you faster glDrawElements(), if
	// you do vertex_array_range on it before rendering.
	{
		// For best results, we must allocate one big ol' chunk of
		// vertex memory on the first call to this function, via
		// wglAllocateMemoryNV, and then allocate sub-chunks out of
		// it.

		if ( vertex_memory_buffer == 0 ) {
			// Need to allocate the big chunk.
			
			// If we have NV's allocator, then use it.
			if ( wglAllocateMemoryNV ) {
				vertex_memory_buffer = wglAllocateMemoryNV( VERTEX_BUFFER_SIZE, 0.f, 0.f, 0.5f );	// @@ this gets us AGP memory.
//				wglAllocateMemoryNV( size, 0.f, 0.f, 1.0f );	// @@ this gets us video memory.
				vertex_memory_from_malloc = false;
				vertex_memory_top = 0;

				if ( vertex_memory_buffer && glVertexArrayRangeNV ) {
					glVertexArrayRangeNV( VERTEX_BUFFER_SIZE, vertex_memory_buffer );
				}

				glEnableClientState(GL_VERTEX_ARRAY_RANGE_NV);	// GL_VERTEX_ARRAY_RANGE_WITHOUT_FLUSH_NV
			}
			// else we'll fall back on malloc() for vb allocations.
		}

		// Carve a chunk out of our big buffer, or out of malloc if
		// the buffer is dry.

		if ( vertex_memory_buffer && vertex_memory_top + size <= VERTEX_BUFFER_SIZE ) {
			// Just allocate from the end of the big buffer and increment the top.
			unsigned char*	buffer = (unsigned char*) vertex_memory_buffer + vertex_memory_top;
			vertex_memory_top += size;

			return (void*) buffer;

		} else {
			// Fall back to malloc.
			printf( "avm: warning, falling back to malloc!\n" );
			return malloc( size );
		}
	}


	void	free_vertex_memory( void* buffer )
	// Frees a buffer previously allocated via allocate_vertex_memory().
	{
		// this function is not ready for prime-time.
		assert( 0 );
	}


	void	vertex_array_range( int size, void* buffer )
	// Lock a section of vertex memory.  Do this before rendering, to
	// get fast DMA'd renders.
	{
	}

	// @@ fences!

};

