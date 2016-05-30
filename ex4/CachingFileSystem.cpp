#define FUSE_USE_VERSION 26

#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>
#include <dirent.h>

#include "Cache.h"
#include <climits>
#include <algorithm>
// CL Arguments
#define NUM_ARGS 6
#define ROOT_ARG 1
#define MOUNT_ARG 2
#define BLOCK_ARG 3
#define OLD_ARG 4
#define NEW_ARG 5
// Some constants
#define USAGE_MSG "Usage: CachingFileSystem rootdir mountdir " \
	"numberOfBlocks fOld fNew"
#define SYSERROR_MSG(f) "System Error: \"" << f << "\" has failed."
#define EXIT_SUCC 0
#define EXIT_FAIL 1
#define OPEN_FLAGS O_RDONLY | O_DIRECT | O_SYNC

using namespace std;

struct fuse_operations caching_oper;

/* ========== Helper Functions ========== */

/**
 * Displays a usage message and exits. Does not return.
 */
void caching_usage()
{
	cout << USAGE_MSG << endl;
	exit(EXIT_SUCC);
}

/**
 * Displays an error message and exits. Does not return.
 */
void caching_syserror(const string &func)
{
	cerr << SYSERROR_MSG(func) << endl;
	exit(EXIT_FAIL);
}

/**
 * Returns an absolute path (to fpath) from a relative one (from path)
 */
static void caching_fullpath(char fpath[PATH_MAX], const char *path)
{
	// Get absolute path from the given path
	// (note that this is absolute from mountdir root)
	char abspath[PATH_MAX];
	string fullpath = CACHING_STATE->rootdir + "/" + path;
	// Remove double slashes...
	realpath(fullpath.c_str(), abspath);
	fullpath = abspath;
	// Return full path as c_str
	strcpy(fpath, fullpath.c_str());
}

/**
 * Returns true if the given path is the path to the log file, and false o/w
 */ 
static bool caching_is_log_path(const string &path)
{
	return path.find(LOG_FILE) == 0;
}

/* ========== Fuse Functions ========== */

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int caching_getattr(const char *path, struct stat *statbuf)
{
	// Write to log
	writeToLog("getattr");	

	int ret = 0;
	char fpath[PATH_MAX];
	caching_fullpath(fpath, path);
	
	// Make sure this isn't the log file
	if (caching_is_log_path(fpath))
	{
		return -ENOENT;
	}	
	// Fill the statbuf
	ret = lstat(fpath, statbuf);
	if (ret < 0)
	{
		ret = -errno;
	}

	return ret;
}

/**
 * Get attributes from an open file
 *
 * This method is called instead of the getattr() method if the
 * file information is available.
 *
 * Currently this is only called after the create() method if that
 * is implemented (see above).  Later it may be called for
 * invocations of fstat() too.
 *
 * Introduced in version 2.5
 */
int caching_fgetattr(const char *path, struct stat *statbuf, 
		     struct fuse_file_info *fi)
{
	// Write to log
	writeToLog("fgetattr");	

	int ret = 0;
	char fpath[PATH_MAX];
	caching_fullpath(fpath, path);

	// Make sure this isn't the log file
	if (caching_is_log_path(fpath))
	{
		return -ENOENT;
	}	

	ret = fstat(fi->fh, statbuf);
	if (ret < 0)
	{
		ret = -errno;
	}
	return ret;
}

/**
 * Check file access permissions
 *
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 *
 * This method is not called under Linux kernel versions 2.4.x
 *
 * Introduced in version 2.5
 */
int caching_access(const char *path, int mask)
{
	// Write to log
	writeToLog("access");	

	int ret = 0;
	char fpath[PATH_MAX];
	caching_fullpath(fpath, path);

	// Make sure this isn't the log file
	if (caching_is_log_path(fpath))
	{
		return -ENOENT;
	}

	ret = access(fpath, mask);

	if (ret < 0)
	{
		ret = -errno;
	}
	return ret;
}


/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * initialize an arbitrary filehandle (fh) in the fuse_file_info 
 * structure, which will be passed to all file operations.

 * pay attention that the max allowed path is PATH_MAX (in limits.h).
 * if the path is longer, return error.

 * Changed in version 2.2
 */
int caching_open(const char *path, struct fuse_file_info *fi)
{
	// Write to log
	writeToLog("open");	

	int ret = 0, fd;
	char fpath[PATH_MAX];
	caching_fullpath(fpath, path);

	// Make sure this isn't the log file
	if (caching_is_log_path(fpath))
	{
		return -ENOENT;
	}
	// Make sure the file is opened in RDONLY mode
	if ((fi->flags & 3) != O_RDONLY)
	{
		return -EACCES;
	}
	
	fd = open(fpath, OPEN_FLAGS);
	if (fd < 0)
	{
		ret = -errno;
	}
	// Update the fd in the fuse_info struct, and set direct_io to 1
	fi->fh = fd;
	fi->direct_io = 1;
	
	return ret;
}


/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error. For example, if you receive size=100, offest=0,
 * but the size of the file is 10, you will init only the first 
   ten bytes in the buff and return the number 10.
   
   In order to read a file from the disk, 
   we strongly advise you to use "pread" rather than "read".
   Pay attention, in pread the offset is valid as long it is 
   a multipication of the block size.
   More specifically, pread returns 0 for negative offset 
   and an offset after the end of the file
   (as long as the the rest of the requirements are fulfiiled).
   You are suppose to preserve this behavior also in your implementation.

 * Changed in version 2.2
 */
int caching_read(const char *path, char *buf, size_t size, off_t offset, 
		 struct fuse_file_info *fi)
{
	// Write to log
	writeToLog("read");

	int ret = 0;

	char fpath[PATH_MAX];
	caching_fullpath(fpath, path);

	size_t bytesRead = 0;		// Total bytes read
	bool shouldStop = false;

	// Get the file's size
	struct stat sb;
	if (fstat(fi->fh, &sb) < 0)
	{
		return -errno;
	}

	size_t fileSize = sb.st_size;
	// If the offset is beyond the file's data, return EOF (0 bytes read)
	if ((size_t)offset >= fileSize)
	{
		return 0;
	}

	// Indices for the first block to read, the last and the current offst
	size_t endOffset = std::min(offset + size, fileSize), 
	       startBlock = offset / Block::size,
	       endBlock = (endOffset - 1) / Block::size, 
	       currOff = 0;		// The offset in the FILE (for pread)

	// A buffer that holds the minimal number of entire blocks that 
	// contain all the data the user wants.
	char *aligned_buf = (char*)malloc(Block::size * 
			(endBlock - startBlock + 1));
	
	// For every block, check if we have it in the cache, and if not
	// create a new Block object, read the data to it and add to cache
	for (size_t blockNum = startBlock; blockNum <= endBlock; ++blockNum)
	{
		if (shouldStop)
		{
			break;
		}
		currOff = blockNum * Block::size;
		if (getBlock(fpath, blockNum) == NOT_IN_CACHE)
		{
			Block newBlock(fpath, blockNum);
			ret = pread(fi->fh, newBlock.data, Block::size, 
					currOff);
			if (ret < 0)
			{
				return -errno;
			} // From here, ret is non-negative...
			else if (ret == 0)
			{
				break; // Means EOF
			}
			else
			{
				newBlock.written = ret;
			}
			addToCache(std::move(newBlock));
			if ((size_t)ret < Block::size)
			{
				shouldStop = true;
			}
		}
		// Now assuming that the relevant block is cached and on top
		// of the stack (back of the cache vector), copy the data
		// from the cached block to the buffer.
		memcpy(aligned_buf + (blockNum - startBlock) * Block::size, 
				cache.back().data,
				cache.back().written);
		bytesRead += cache.back().written;
	}
	// Remove the extra data read from the first block
	bytesRead -= offset % Block::size;
	if (bytesRead > size)
	{
		// remove extra data from last block's end, only if such
		// exists...
		bytesRead = size;
	}
	// Move data to output buffer and free allocated memory
	memcpy(buf, aligned_buf + offset % Block::size, bytesRead);
	free(aligned_buf);
	return bytesRead;
}

/** Possibly flush cached data
 *
 * BIG NOTE: This is not equivalent to fsync().  It's not a
 * request to sync dirty data.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 *
 * NOTE: The flush() method may be called more than once for each
 * open().  This happens if more than one file descriptor refers
 * to an opened file due to dup(), dup2() or fork() calls.  It is
 * not possible to determine if a flush is final, so each flush
 * should be treated equally.  Multiple write-flush sequences are
 * relatively rare, so this shouldn't be a problem.
 *
 * Filesystems shouldn't assume that flush will always be called
 * after some writes, or that if will be called at all.
 *
 * Changed in version 2.2
 */
int caching_flush(const char *, struct fuse_file_info *)
{
	// Write to log
	writeToLog("flush");	

	return 0;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int caching_release(const char *, struct fuse_file_info *fi)
{
	// Write to log
	writeToLog("release");	

	return close(fi->fh);
}

/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int caching_opendir(const char *path, struct fuse_file_info *fi)
{
	// Write to log
	writeToLog("opendir");	

	DIR *dirp;
	int ret = 0;
	char fpath[PATH_MAX];
	caching_fullpath(fpath, path);

	// Make sure this isn't the log file
	if (caching_is_log_path(fpath))
	{
		return -ENOENT;
	}

	dirp = opendir(fpath);
	if (dirp == nullptr)
	{
		ret = -errno;
	}
	// Update fd
	fi->fh = (intptr_t) dirp;

	return ret;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * Introduced in version 2.3
 */
int caching_readdir(const char *, void *buf, fuse_fill_dir_t filler, 
		    off_t, struct fuse_file_info *fi)
{
	// Write to log
	writeToLog("readdir");	

	int ret = 0;
	DIR *dirp;
	struct dirent *dent;
	dirp = (DIR*) (uintptr_t) fi->fh;
	// If first entry is null, an error occured
	dent = readdir(dirp);
	if (dent == nullptr)
	{
		return -errno;
	}
	do
	{
		// don't list log
		if (caching_is_log_path(dent->d_name))
		{
			continue;
		}
		if (filler(buf, dent->d_name, nullptr, 0) != 0)
		{
			return -ENOMEM;
		}
	} while ((dent = readdir(dirp)) != nullptr);
	return ret;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int caching_releasedir(const char *, struct fuse_file_info *fi)
{
	// Write to log
	writeToLog("releasedir");	

	return closedir((DIR*) (uintptr_t) fi->fh);
}

/** Rename a file */
int caching_rename(const char *path, const char *newpath)
{
	// Write to log
	writeToLog("rename");	
	
	int ret = 0;
	char fpath[PATH_MAX], fnewpath[PATH_MAX];
	caching_fullpath(fpath, path);
	// Make sure the oldpath isn't the log file
	if (caching_is_log_path(fpath))
	{
		return -ENOENT;
	}

	caching_fullpath(fnewpath, newpath);
	ret = rename(fpath, fnewpath);
	if (ret == 0)	// Check if renaming failed.
	{	// otherwise update cache blocks
		renameInCache(fpath, fnewpath);
	}
	return ret;
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 
If a failure occurs in this function, do nothing (absorb the failure 
and don't report it). 
For your task, the function needs to return NULL always 
(if you do something else, be sure to use the fuse_context correctly).
 * Introduced in version 2.3
 * Changed in version 2.6
 */
void *caching_init(struct fuse_conn_info *)
{
	return CACHING_STATE;
}


/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
  
If a failure occurs in this function, do nothing 
(absorb the failure and don't report it). 
 
 * Introduced in version 2.3
 */
void caching_destroy(void *userdata)
{
	cache.clear(); // This frees cached blocks' data!
	delete (CachingState*) userdata;	
}


/**
 * Ioctl from the FUSE sepc:
 * flags will have FUSE_IOCTL_COMPAT set for 32bit ioctls in
 * 64bit environment.  The size and direction of data is
 * determined by _IOC_*() decoding of cmd.  For _IOC_NONE,
 * data will be NULL, for _IOC_WRITE data is out area, for
 * _IOC_READ in area and if both are set in/out area.  In all
 * non-NULL cases, the area is of _IOC_SIZE(cmd) bytes.
 *
 * However, in our case, this function only needs to print 
 cache table to the log file .
 * 
 * Introduced in version 2.8
 */
int caching_ioctl(const char *, int, void *, struct fuse_file_info *, 
		  unsigned int, void *)
{
	writeToLog("ioctl");	
	string rel_path, rootpath = CACHING_STATE->rootdir;

	for (size_t i = 0; i < cache.size(); ++i)
	{
		rel_path = cache[i].filename; // Exclude rootpath
		if (rel_path.find(rootpath) != string::npos)
		{
			rel_path = rel_path.substr(rootpath.length() + 1);
		}
		CACHING_STATE->logfile << rel_path << DELIM 
			<< cache[i].number + 1 << DELIM 
			<< cache[i].refCount << endl;
	}
	return 0;
}


// Initialise the operations. 
// You are not supposed to change this function.
void init_caching_oper()
{

	caching_oper.getattr = caching_getattr;
	caching_oper.access = caching_access;
	caching_oper.open = caching_open;
	caching_oper.read = caching_read;
	caching_oper.flush = caching_flush;
	caching_oper.release = caching_release;
	caching_oper.opendir = caching_opendir;
	caching_oper.readdir = caching_readdir;
	caching_oper.releasedir = caching_releasedir;
	caching_oper.rename = caching_rename;
	caching_oper.init = caching_init;
	caching_oper.destroy = caching_destroy;
	caching_oper.ioctl = caching_ioctl;
	caching_oper.fgetattr = caching_fgetattr;


	caching_oper.readlink = NULL;
	caching_oper.getdir = NULL;
	caching_oper.mknod = NULL;
	caching_oper.mkdir = NULL;
	caching_oper.unlink = NULL;
	caching_oper.rmdir = NULL;
	caching_oper.symlink = NULL;
	caching_oper.link = NULL;
	caching_oper.chmod = NULL;
	caching_oper.chown = NULL;
	caching_oper.truncate = NULL;
	caching_oper.utime = NULL;
	caching_oper.write = NULL;
	caching_oper.statfs = NULL;
	caching_oper.fsync = NULL;
	caching_oper.setxattr = NULL;
	caching_oper.getxattr = NULL;
	caching_oper.listxattr = NULL;
	caching_oper.removexattr = NULL;
	caching_oper.fsyncdir = NULL;
	caching_oper.create = NULL;
	caching_oper.ftruncate = NULL;
}

/**
 * Note that caching_usage and caching_syserror do not return.
 */
int main(int argc, char* argv[])
{
	// Check that the number of arguments is valid.
	if (argc < NUM_ARGS)
	{
		caching_usage();
	}
	
	struct stat sb; // For block size and dir checks
	
	// Get the relevant data to variables
	char absrootdir[PATH_MAX], absmountdir[PATH_MAX];
	string rootdir = argv[ROOT_ARG], mountdir = argv[MOUNT_ARG];
	// Check that paths exist and are directories
	if (realpath(argv[ROOT_ARG], absrootdir) == nullptr ||
	    realpath(argv[MOUNT_ARG], absmountdir) == nullptr ||
	    stat(absrootdir, &sb) != 0 || !S_ISDIR(sb.st_mode) ||
	    stat(absmountdir, &sb) != 0 || !S_ISDIR(sb.st_mode))
	{
		caching_usage();
	}
	rootdir = absrootdir; mountdir = absmountdir;

	maxSize = atoi(argv[BLOCK_ARG]); 
	double fOld = atof(argv[OLD_ARG]), fNew = atof(argv[NEW_ARG]);
	newIdx = maxSize * fNew;
	oldIdx = maxSize * (1 - fOld);
	// Check if one of the arguments is invalid.
	if (maxSize <= 0 || fOld > 1 || fOld < 0 || 
	    fNew > 1 || fNew < 0 || fNew + fOld > 1 ||
	    newIdx <= 0 || oldIdx >= maxSize) // Size of partitions
	{
		caching_usage();
	}
	// Init static constant and private data
	Block::size = sb.st_blksize;
	CachingState *cachingData = new(std::notrhow) CachingState(rootdir);
	if (cachingData == nullptr)
	{
		caching_syserror("new operator");
	}

	init_caching_oper();

	argv[1] = argv[2];
	for (int i = 2; i < (argc - 1); i++)
	{
		argv[i] = NULL;
	}
        argv[2] = (char*) "-s";
	argc = 3;
	
	int fuse_stat = fuse_main(argc, argv, &caching_oper, cachingData);
	return fuse_stat;
}
