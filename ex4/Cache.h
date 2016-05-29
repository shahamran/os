#ifndef _CACHE_H
#define _CACHE_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <fuse.h>
#include <ctime>

using std::string;
using std::vector;

#define DEF_REF_COUNT 1
#define LOG_FILE ".filesystem.log"
#define DELIM " "
#define CACHING_STATE ((CachingState*) fuse_get_context()->private_data)

/**
 * The private_data object for fuse. Holds the log stream and rootdir.
 */
class CachingState
{
public:
	string rootdir;
	std::ofstream logfile;

	/**
	 * initialize the rootdir to the given one, open the logfile in append
	 * mode.
	 */
	CachingState(const string& root) : 
		rootdir(root), logfile(root + "/" LOG_FILE, 
				std::ofstream::out | std::ofstream::app)
	{
	}
};

/**
 * Write a line to the log that contains the time and the function name
 */
void writeToLog(const string &func)
{
	CACHING_STATE->logfile << time(nullptr) << DELIM << func << std::endl;
}

/**
 * A data block of a file. See comments on data members for details.
 */
class Block
{
public:
	static size_t size;	// Block size in the filesystem

	std::string filename;	// The file this block belongs to
	size_t number;		// The number of block in the file
	size_t refCount;	// Reference count
	char *data;		// The actual (aligned) data of the block
	size_t written;		// Amount of bytes actually written
	
	friend void swap(Block& lhs, Block& rhs)
	{
		using std::swap;
		swap(lhs.filename, rhs.filename);
		swap(lhs.number, rhs.number);
		swap(lhs.refCount, rhs.refCount);
		swap(lhs.written, rhs.written);		
		char *tmp = lhs.data;
		lhs.data = rhs.data;
		rhs.data = tmp;
	}

	Block(std::string file, int num) : filename(file), number(num),
					refCount(DEF_REF_COUNT), written(0)
	{
		// Allocate aligned block
		data = (char*) aligned_alloc(Block::size, Block::size);
		if (data == nullptr)
		{
			// Handle alloc error
		}
	}

	/**
	 * Copy ctor, copy data to a new allocated memory
	 */
	Block(const Block &other) : Block(other.filename, other.number)
	{
		refCount = other.refCount;
	       	written = other.written;
		memcpy(data, other.data, Block::size);
	}

	/**
	 * Move ctor, make sure the moved block's data isn't deleted.
	 */
	Block(Block&& other)
	{
		swap(*this, other);
		other.data = nullptr;
	}

	Block& operator= (Block other)
	{
		swap(*this, other);
		return *this;
	}

	/**
	 * Free allocated data
	 */
	~Block()
	{
		if (data != nullptr)
		{
			free(data);
			data = nullptr;
		}
	}

	bool isEqualTo(const Block& other)
	{
		return  (filename.compare(other.filename) == 0) &&
			(number == other.number);
	}

	bool isEqualTo(const string& otherFile, size_t otherNum)
	{
		return (filename.compare(otherFile) == 0) &&
			number == otherNum;
	}
};

size_t Block::size = 0;

typedef std::vector<Block> BlocksCache;
static BlocksCache cache;		// The data structure for caching
static size_t newIdx, oldIdx, maxSize;

/**
 * Choose the block that has the least refcount in the old partition, and
 * remove it from the cache.
 */
void evictBlock()
{
	// No need to evict if the cache isn't full.
	if (cache.size() < maxSize)
		return;

	size_t lfuBlockIndex = 0, lfuBlockRef = cache[0].refCount;

	int j = maxSize;
	
	for (size_t i = maxSize - 1; i >= oldIdx; --i)
	{
		j = maxSize - i - 1;
		if (cache[j].refCount < lfuBlockRef)
		{
			lfuBlockIndex = j;
			lfuBlockRef = cache[j].refCount;
		}
	}
	cache.erase(cache.begin() + lfuBlockIndex);
}

/**
 * Add a block to the cache.
 */
void addToCache(Block block)
{
	if (cache.size() == maxSize)
	{
		evictBlock();
	}
	
	cache.push_back(block);
}

/**
 * Search for a block in the cache. If found, move it to the end of the 
 * vector (top of the stack) and update its refCount.
 * Return 0 upon success and -1 if the block isn't in the cache.
 */
int getBlock(const std::string& fileName, size_t num)
{
	size_t j = 0;	// logical index of the item in the vector
	for (int i = cache.size() - 1; i >= 0; --i)
	{
		j = cache.size() - i - 1;
		if (cache[i].isEqualTo(fileName, num))
		{
			if (j >= newIdx)
			{
				++cache[i].refCount;
			}
			cache.push_back(cache[i]);
			cache.erase(cache.begin() + i);
			return 0;
		}
	}
	return -1;
}

/**
 * Check the filename of each block. If it matches the oldName argument,
 * replace it with newName.
 */
void renameInCache(const string &oldName, const string &newName)
{
	size_t found = 0;
	for (size_t i = 0; i < cache.size(); ++i)
	{
		if ((found = cache[i].filename.find(oldName)) != string::npos)
		{
			cache[i].filename.replace(found, oldName.size(), 
					newName);
		}
	}
}


#endif
