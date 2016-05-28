#ifndef _CACHE_H
#define _CACHE_H

#include <string>
#include <vector>
#include <fstream>
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

	CachingState(const string& root) : 
		rootdir(root), logfile(root + "/" LOG_FILE, 
				std::ofstream::out | std::ofstream::app)
	{
	}
};

void writeToLog(const string &func)
{
	CACHING_STATE->logfile << time(nullptr) << DELIM << func << std::endl;
}

class Block
{
public:
	static size_t size;	// Block size in the filesystem

	std::string filename;	// The file this block belongs to
	int number;		// The number of block in the file
	int refCount;		// Reference count
	char *data;		// The actual (aligned) data of the block

	Block(std::string file, int num) : filename(file), number(num),
					refCount(DEF_REF_COUNT)
	{
		data = (char*) aligned_alloc(Block::size, Block::size);
		if (data == nullptr)
		{
			// Handle alloc error
		}
	}

	~Block()
	{
		if (data != nullptr)
		{
			free(data);
			data = nullptr;
		}
	}
};

typedef std::vector<Block> BlocksCache;
static BlocksCache cache;
static size_t newIdx, oldIdx, maxSize;

void evictBlock()
{
	if (cache.size() <= oldIdx)
		return;

	std::pair<size_t, int> lfuBlock;
	size_t cacheSize = cache.size();
	size_t endIdx = std::min(cacheSize, maxSize);
	int j = cacheSize;
	
	lfuBlock.first = 0;
	lfuBlock.second = cache[0].refCount;
	for (size_t i = endIdx - 1; i >= oldIdx; --i)
	{
		j = cacheSize - i - 1;
		if (cache[j].refCount < lfuBlock.second)
		{
			lfuBlock.first = j;
			lfuBlock.second = cache[j].refCount;
		}
	}
	cache.erase(cache.begin() + lfuBlock.first);
}

void addToCache(Block block)
{
	if (cache.size() == maxSize)
	{
		evictBlock();
	}
	
	cache.push_back(block);
}

int getBlock(const std::string& fileName, int num)
{
	auto itCurrBlock = cache.end();
	size_t j = 0;	// logical index of the item in the vector
	for (int i = cache.size() - 1; i >= 0; --i)
	{
		j = cache.size() - i - 1;
		itCurrBlock = cache.begin() + i;
		if (itCurrBlock->filename.compare(fileName) == 0 &&
		    itCurrBlock->number == num)
		{
			if (j >= newIdx)
			{
				++(itCurrBlock->refCount);
			}
			Block currBlock = *itCurrBlock;
			itCurrBlock->data = nullptr;
			cache.erase(itCurrBlock);
			cache.push_back(currBlock);
			return 0;
		}
	}
	return -1;
}

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
