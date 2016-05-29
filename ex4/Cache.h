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
	int number;		// The number of block in the file
	int refCount;		// Reference count
	char *data;		// The actual (aligned) data of the block
	size_t written;		// Amount of bytes actually written

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
	Block(const Block &other) : filename(other.filename),
			number(other.number), refCount(other.refCount),
			written(other.written)
	{
		data = (char*) aligned_alloc(Block::size, Block::size);
		memcpy(data, other.data, Block::size);
	}

	/**
	 * Move ctor, make sure the moved block's data isn't deleted.
	 */
	Block(Block &&other) : filename(other.filename), number(other.number),
				refCount(other.refCount), data(other.data),
				written(other.written)
	{
		other.data = nullptr;
	}	

	Block& operator= (Block other)
	{
		*this = Block(std::move(other));
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

	std::pair<size_t, int> lfuBlock;
	int j = maxSize;
	
	lfuBlock.first = 0;
	lfuBlock.second = cache[0].refCount;
	for (size_t i = maxSize - 1; i >= oldIdx; --i)
	{
		j = maxSize - i - 1;
		if (cache[j].refCount < lfuBlock.second)
		{
			lfuBlock.first = j;
			lfuBlock.second = cache[j].refCount;
		}
	}
	cache.erase(cache.begin() + lfuBlock.first);
}

/**
 * Add a block to the cache.
 */
void addToCache(Block &&block)
{
	if (cache.size() == maxSize)
	{
		evictBlock();
	}
	
	cache.push_back(std::move(block));
}

/**
 * Search for a block in the cache. If found, move it to the end of the 
 * vector (top of the stack) and update its refCount.
 * Return 0 upon success and -1 if the block isn't in the cache.
 */
int getBlock(const std::string& fileName, int num)
{
	size_t j = 0;	// logical index of the item in the vector
	for (int i = cache.size() - 1; i >= 0; --i)
	{
		j = cache.size() - i - 1;
		if (cache[i].filename.compare(fileName) == 0 &&
		    cache[i].number == num)
		{
			if (j >= newIdx)
			{
				++cache[i].refCount;
			}
			cache.push_back(std::move(cache[i]));
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
	for (size_t i = 0; i < cache.size(); ++i)
	{
		if (cache[i].filename.compare(oldName) == 0)
		{
			cache[i].filename = newName;
		}
	}
}


#endif
