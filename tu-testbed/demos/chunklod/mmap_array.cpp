// mmap_array.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Datatype for making a memory-mapped 2D array.  The data is stored
// on disk, and mapped into memory using mmap (Posix) or MapViewOfFile
// (Win32).  Use this for processing giant heightfield or texture
// datasets "out-of-core".


#include "mmap_array.h"


#ifdef WIN32


#include <windows.h>
#include <stdio.h>
#include "engine/container.h"


namespace mmap_array_helper {

	hash<void*, char*>	s_temp_filenames;


	void*	map(int size, bool writeable, const char* filename)
	{
		bool	create_file = false;

		if (filename == NULL) {
			// Generate a temporary file name.
			filename = _tempnam(".", "tmp");
			create_file = true;

			printf("temp file = %s\n", filename);//xxxxxx
		}

		HANDLE	filehandle = CreateFile(filename,
										GENERIC_READ | (writeable ? GENERIC_WRITE : 0),
										FILE_SHARE_READ,
										NULL,
										OPEN_ALWAYS,
										FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS | (create_file ? FILE_ATTRIBUTE_TEMPORARY : 0),
										NULL);
		if (filehandle == INVALID_HANDLE_VALUE) {
			// Failure.
			throw "can't CreateFile";

			if (create_file) {
				// _tempnam malloc's the filename.
				free((void*) filename);
			}

			return NULL;
		}

		if (create_file) {
			// Expand the new file to 'size' bytes.
			SetFilePointer(filehandle, size, NULL, FILE_BEGIN);
			SetEndOfFile(filehandle);
		}

		HANDLE	file_mapping = CreateFileMapping(filehandle, NULL, writeable ? PAGE_READWRITE : PAGE_READONLY, 0, 0, NULL);
		if (file_mapping == NULL) {
			// Failure.
			CloseHandle(filehandle);

			if (create_file) {
				_unlink(filename);
				// _tempnam malloc's the filename.
				free((void*) filename);
			}

			return NULL;
		}

		void*	data = MapViewOfFile(file_mapping, writeable ? FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, size);

		CloseHandle(file_mapping);
		CloseHandle(filehandle);

		if (create_file) {
			if (data) {
				// Remember the filename so we can delete it & free the name later.
				s_temp_filenames.add(data, (char*) filename);
			} else {
				_unlink(filename);
				free((void*) filename);
			}
		}

		return data;
	}


	void	unmap(void* data, int size)
	{
		UnmapViewOfFile(data);

		char*	filename = NULL;
		if (s_temp_filenames.get(data, &filename)) {
			// Need to delete this temporary file.
			_unlink(filename);

			// _tempnam malloc's the filename.
			free((void*) filename);

			// @@ need to remove (data, filename) from s_temp_filenames!
		}
	}
};


#else // not WIN32


#include <unistd.h>
#include <sys/mman.h>


// wrappers for mmap/munmap
namespace mmap_array_helper {

	// Create a memory-mapped window into a file.  If filename is
	// NULL, create a temporary file for the data.  If a valid
	// filename is specified, then open the file and use it as the
	// data.  On success, returns a pointer to the beginning of the
	// memory-mapped area.  Returns NULL on failure.
	//
	// Use unmap() to release the mapping.
	//
	// The writable param specifies whether the returned buffer can be
	// written to.  You may get better performance with a read-only
	// buffer.
	void*	map(int size, bool writeable, const char* filename)
	{
		assert(size > 0);

		if (filename == NULL) {
			// make a temporary filename.
			filename = tmpnam(NULL);
		}
		// else if size == 0 then size = filesize(filename);

		int	fildes = open(filename, (writeable ? O_RDWR : O_RDONLY) | O_CREAT);
		if (fildes == -1) {
			// Failure.
			return NULL;
		}

		void*	data = mmap(0, size, PROT_READ | (writable ? PROT_WRITE : 0), MAP_SHARED, fildes, 0);

		if (data == (void*) -1) {
			// Failure.
			close(fildes);
			return NULL;
		}
		return data;
	}


	// Unmap a file mapped via map().
	void	unmap(void* data, int size)
	{
		munmap(data, size);
	}
};


#endif // not WIN32
