#ifndef _CACHE_H
#define _CACHE_H

#include <string>
#include <unordered_map>
#include <vector>

#define DEF_REF_COUNT 1

namespace Cache
{
	class Block
	{
	public:
		static size_t size;	// Block size in the filesystem

		std::string filename;	// The file this block belongs to
		int number;		// The number of block in the file
		int refCount;
		char *data;		// The actual data of the block

		Block(std::string file, int num) : filename(file), number(num),
						refCount(DEF_REF_COUNT)
		{
			data = (char*) aligned_alloc(Block::size, Block::size);
			if (data == nullptr)
			{

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
	typedef std::unordered_map<std::string, size_t> BlocksMap;

	size_t newIdx, oldIdx, maxSize;
	void addToCache(BlocksCache *cachse, Block block);

	int getBlock(BlocksCache *cache, const std::string& fileName, int num);
	void evictBlock(BlocksCache *cache);

}

#endif
