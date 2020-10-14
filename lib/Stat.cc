#include <cerrno>
#include <cstring>
#include "Stat.h"

Stat::Stat(char const * fn)
{
	error_ = 0;

#ifndef P_WIN32
	uid_ = geteuid();
	gid_ = getegid();
#endif

	errno = 0;
	if (stat(fn, &statbuf_) != 0)
	{
		switch (errno)
		{
		case EBADF: // filedes is bad.
			error_ |= FILE_ERR_BAD_FILE_DESC;
			break;
		case ENOENT:
			// A component of the path file_name does not
			// exist, or the path is an empty string.
			error_ |= FILE_ERR_NON_EXISTENT;
			break;
		case ENOTDIR:
			// A component of the path is not a directory.
			error_ |= FILE_ERR_NON_DIRECTORY;
			break;
#ifndef P_WIN32
		case ELOOP:
			// Too many symbolic links encountered
			// while traversing the path.
			error_ |= FILE_ERR_NAME_TOO_LONG;
			break;
#endif
		case EFAULT:
			// Bad address.
			error_ |= FILE_ERR_BAD_ADDRESS;
			break;
		case EACCES:
			// Permission denied.
			error_ |= FILE_ERR_PERMISSION_DENIED;
			break;
		case ENOMEM:
			// Out of memory (i.e. kernel memory).
			error_ |= FILE_ERR_OUT_OF_MEMORY;
			break;
		case ENAMETOOLONG:
			// File name too long.
			error_ |= FILE_ERR_NAME_TOO_LONG;
			break;
		} // switch (errno)
	} // if (stat == 0)
	errno = 0;
}

bool Stat::exists() const
{
	return (error_ & FILE_ERR_NON_EXISTENT) == 0;
}

int Stat::getError() const
{
	return error_;
}

bool Stat::isReadable() const
{
	// wenn stat() fehlschlägt, ist der Rückgabewert immer false
	if (error_ != 0) return false;

	if (statbuf_.st_uid == uid_)
		return (statbuf_.st_mode & S_IRWXU) & S_IRUSR;
#ifndef P_WIN32
	else if (statbuf_.st_gid == gid_)
		return (statbuf_.st_mode & S_IRWXG) & S_IRGRP;
	else // Rechte für "others"
		return (statbuf_.st_mode & S_IRWXO) & S_IROTH;
#else
	return false;
#endif
}

bool Stat::isWritable() const
{
	// wenn stat() fehlschlägt, ist der Rückgabewert immer false
	if (error_ != 0) return false;

	if (statbuf_.st_uid == uid_)
		return (statbuf_.st_mode & S_IRWXU) & S_IWUSR;
#ifndef P_WIN32
	else if (statbuf_.st_gid == gid_)
		return (statbuf_.st_mode & S_IRWXG) & S_IWGRP;
	else // Rechte für "others"
		return (statbuf_.st_mode & S_IRWXO) & S_IWOTH;
#else
	return false;
#endif
}

bool Stat::isExecutable() const
{
	// wenn stat() fehlschlägt, ist der Rückgabewert immer false
	if (error_ != 0) return false;

	if (statbuf_.st_uid == uid_)
		return (statbuf_.st_mode & S_IRWXU) & S_IXUSR;
#ifndef P_WIN32
	else if (statbuf_.st_gid == gid_)
		return (statbuf_.st_mode & S_IRWXG) & S_IXGRP;
	else // Rechte für "others"
		return (statbuf_.st_mode & S_IRWXO) & S_IXOTH;
#else
	return false;
#endif
}

bool Stat::isSymLink() const
{
	// wenn stat() fehlschlägt, ist der Rückgabewert immer false
	if (error_ != 0) return false;
#ifndef P_WIN32
	return S_ISLNK(statbuf_.st_mode);
#else
	return false; // unter Win32 gibt es keine Symlinks
#endif
}

bool Stat::isRegularFile() const
{
	// wenn stat() fehlschlägt, ist der Rückgabewert immer false
	if (error_ != 0) return false;
	return S_ISREG(statbuf_.st_mode);
}

bool Stat::isDirectory() const
{
	// wenn stat() fehlschlägt, ist der Rückgabewert immer false
	if (error_ != 0) return false;
	return S_ISDIR(statbuf_.st_mode);
}

bool Stat::isSocket() const
{
	return statbuf_.st_mode & S_IFSOCK; 
}

