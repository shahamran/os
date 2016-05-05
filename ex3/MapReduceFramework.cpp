#include "MapReduceFramework.h"
#include <pthread.h>
#include <vector>
#include <map>
#include <cmath>

using std::vector;

#define DATA_CHUNK 10


/* Struct for the map procedure. nextToRead is the only variable that 
 * will be mutexed, as specified in the instructions. */
typedef struct MapData
{
	MapReduceBase *mapReduce;
	vector<IN_ITEM> *itemsList;
	int numOfItems;
	int nextToRead;
} MapData;

MapData mapdata;
vector<pthread_t> threadList;
pthread_mutex_t mutexMapRead;

void* ExecMap(void *th)
{
	int myItems;
	if ((myItems = mapdata.nextToRead) >= mapdata.numOfItems)
	{
		pthread_exit((void*)0);
	}
	pthread_mutex_lock(&mutexMapRead);
	// Reserve items to map and get the start index of them.
	myItems = (mapdata.nextToRead += DATA_CHUNK) - DATA_CHUNK;
	pthread_mutex_unlock(&mutexMapRead);
	int end = fmin(myItems + DATA_CHUNK, mapdata.numOfItems);
	for (int i=myItems; i < end; ++i)
	{
		mapdata.mapReduce->Map(mapdata.itemsList->at(i).first,
				       mapdata.itemsList->at(i).second);
	}
		
}

OUT_ITEMS_LIST runMapReduceFramework(MapReduceBase &mapReduce, 
		IN_ITEMS_LIST &itemsList, int multiThreadLevel)
{
	vector<IN_ITEM> itemsVector{std::begin(itemsList),
				    std::end(itemsList)  };
	mapdata.mapReduce = &mapReduce;
	mapdata.itemsList = &itemsVector;
	mapdata.numOfItems = itemsVector.size();
	mapdata.nextToRead = 0;
	// Create the shuffle thread (before the ExecMaps!)
	
	// Create the ExecMap threads!
	for (int i=0; i < multiThreadLevel; ++i)
	{

	}
	// Wait for the shuffle to end
	
	// Reduce!

	// "Shuffle" reduce's output!

	OUT_ITEMS_LIST* l = new OUT_ITEMS_LIST;	
	return *l;
}

void Emit2(k2Base *key, v2Base *val)
{

}

void Emit3(k3Base *key, v3Base *val)
{

}
