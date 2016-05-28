#include "Cache.h"
#include <cstdio>
#include <cstdlib>

using std::string;
using std::unordered_map;
using std::vector;

int Cache::getBlock(BlocksCache *cache, const string& fileName, int num)
{
	for (size_t i = 0; i < cache->size(); ++i)
	{
		if (cache->at(i).filename.compare(fileName) == 0 &&
		    cache->at(i).number == num)
		{
			if (i >= newIdx)
			{
				++cache->at(i).refCount;
			}
			cache->erase(cache->begin() + i);
			return i;
		}
	}
	return -1;
}

void Cache::addToCache(BlocksCache *cache, Block block)
{
	if (cache->size() == maxSize)
	{
		evictBlock(cache);
	}
}
