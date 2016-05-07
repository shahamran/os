#include "MapReduceFramework.h"
#include <pthread.h>
#include <vector>
#include <queue>
#include <map>
#include <cmath> 		/* for fmin */

using std::vector;
using std::queue;
using std::map;
using std::list;
using std::pair;

#define CHUNK_SIZE 10


/* Struct for the map procedure. nextToRead is the only variable that 
 * will be mutexed, as specified in the instructions. */
typedef struct MapData
{
	MapReduceBase *mapReduce;
	vector<IN_ITEM> *itemsList;
	int numOfItems;
	int nextToRead;
} MapData;

typedef struct ReduceData
{

} ReduceData;

// A list of existing threads
vector<pthread_t> threadList;
// In words: a list of pairs in which the first element is the pthread_id
// and the second is that thread's list (pointer) of the map-function-outputs
vector	<pair<pthread_t,
	      queue<pair<k2Base*, v2Base*>>*>
	> mapResultLists;
map<k2Base*, V2_LIST&> shuffleOut;
// Same as before, but for the reduce-function-outputs
vector	<pair<pthread_t,
	      queue<pair<k3Base*, v3Base*>>*>
	> reduceResultLists;
pthread_mutex_t mutexMapRead;
pthread_mutex_t mutexShuffleRead;
pthread_mutex_t mutexReduceRead;


/**
 * The ExecMap procedure: Reads data from the input <k1,v1> list and uses
 * the user provided Map function to create a list of <k2,v2> values (for
 * each thread running this function).
 * Takes a MapData struct pointer as an argument.
 */
void* ExecMap(void *arg)
{
	MapData* mapdata = (MapData*) arg;
	int myItems; // The start index for this thread's data
	// If there's no more data to read, return.
	if ((myItems = mapdata->nextToRead) >= mapdata->numOfItems)
	{
		pthread_exit((void*)0);
	}
	// Lock the index counter
	pthread_mutex_lock(&mutexMapRead);
	// Reserve items to map and get the start index of them.
	myItems = (mapdata->nextToRead += CHUNK_SIZE) - CHUNK_SIZE;
	// Unlock
	pthread_mutex_unlock(&mutexMapRead);
	int end = fmin(myItems + CHUNK_SIZE, mapdata->numOfItems);
	for (int i=myItems; i < end; ++i)
	{
		mapdata->mapReduce->Map(mapdata->itemsList->at(i).first,
				        mapdata->itemsList->at(i).second);
	}
	// Notify the shuffle thread
	
	return nullptr;		
}

void* Shuffle(void *arg)
{
	
	pthread_mutex_lock(&mutexShuffleRead);
	pair<k2Base*, v2Base*> currPair;
	while (true)
	{
		//pthread_cond_timedwait(...)
		for (size_t i=0; i < mapResultLists.size(); ++i)
		{
			while (!mapResultLists[i].second->empty())
			{
				currPair = mapResultLists[i].second->front();
				mapResultLists[i].second->pop();
				shuffleOut[currPair.first].push_back(currPair.second);
			}
		}
	}	
	return arg;
}

OUT_ITEMS_LIST runMapReduceFramework(MapReduceBase &mapReduce, 
		IN_ITEMS_LIST &itemsList, int multiThreadLevel)
{
	// Initialize mutexes, cvs, datastructures...
	
	// Turn the in_items_list into a vector
	vector<IN_ITEM> itemsVector{std::begin(itemsList),
				    std::end(itemsList)  };
	// Initalize the mapdata struct
	MapData mapdata;
	mapdata.mapReduce = &mapReduce;
	mapdata.itemsList = &itemsVector;
	mapdata.numOfItems = itemsVector.size();
	mapdata.nextToRead = 0;

	// Create the ExecMap threads!
	for (int i=0; i < multiThreadLevel; ++i)
	{

	}
	// Create the shuffle thread
	
	// Wait for the ExecMaps to finish
	for (int i=0; i < multiThreadLevel; ++i)
	{

	}
	// Wait for the shuffle to end
	
	// Reduce!
	for (int i=0; i < multiThreadLevel; ++i)
	{

	}

	// "Shuffle" reduce's output!
	

	OUT_ITEMS_LIST* l = new OUT_ITEMS_LIST;	
	return *l;
}

void Emit2(k2Base *key, v2Base *val)
{
	// Figure out which thread is handling this function
	pthread_t currThread = pthread_self();
	size_t end = mapResultLists.size();
	// Add the given key,val pair to the correct list:
	for (unsigned int i=0; i < end; ++i)
	{
		if (pthread_equal(mapResultLists[i].first, currThread))
		{
			mapResultLists[i].second->push(
					          std::make_pair(key, val) );
			return;
		}
	}
}

void Emit3(k3Base *key, v3Base *val)
{

}
