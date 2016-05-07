#include "MapReduceFramework.h"
#include <pthread.h>
#include <vector>
#include <queue>
#include <map>
#include <cmath> 		/* for fmin */
#include <sys/time.h>

using std::vector;
using std::queue;
using std::map;
using std::list;
using std::pair;

#define CHUNK_SIZE 10
#define SEC_TO_NSEC(x) ((x) * 1000000000)
#define USEC_TO_NSEC(x) ((x) * 1000)
#define TIME_TO_WAIT 0.01


/* Struct for the map procedure. nextToRead is the only variable that 
 * will be mutexed, as specified in the instructions. */
typedef struct MapData
{
	MapReduceBase &mapReduce;
	vector<IN_ITEM> &itemsList;
	size_t numOfItems;
	size_t nextToRead;
} MapData;

typedef struct ReduceData
{

} ReduceData;

// A list of existing threads
vector<pthread_t> threadList;
pthread_t shuffleThread;
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
pthread_mutex_t mutex_more_to_shuffle;

pthread_cond_t cv_more_to_shuffle;
bool more_to_shuffle;

void init()
{

}

void cleanup()
{

}

/**
 * The ExecMap procedure: Reads data from the input <k1,v1> list and uses
 * the user provided Map function to create a list of <k2,v2> values (for
 * each thread running this function).
 * Takes a MapData struct pointer as an argument.
 */
void* ExecMap(void *arg)
{
	MapData* mapdata = (MapData*) arg;
	size_t myItems; // The start index for this thread's data
	// If there's no more data to read, return.
	if ((myItems = mapdata->nextToRead) >= mapdata->numOfItems)
	{
		pthread_exit((void*)0); // PTHREAD CALL
	}
	// Lock the index counter
	pthread_mutex_lock(&mutexMapRead); // PTHREAD CALL
	// Reserve items to map and get the start index of them.
	myItems = (mapdata->nextToRead += CHUNK_SIZE) - CHUNK_SIZE;
	// Unlock
	pthread_mutex_unlock(&mutexMapRead); // PTHREAD CALL
	int end = fmin(myItems + CHUNK_SIZE, mapdata->numOfItems); // CHECK ERROR
	for (int i=myItems; i < end; ++i)
	{
		mapdata->mapReduce.Map(mapdata->itemsList.at(i).first,
				       mapdata->itemsList.at(i).second);
	}
	// Notify the shuffle thread
	pthread_cond_signal(&cv_more_to_shuffle);
	return nullptr;		
}

void* Shuffle(void *arg)
{
	struct timespec timeToWake;
	struct timeval timeNow;
	pair<k2Base*, v2Base*> currPair;
	pthread_mutex_lock(&mutex_more_to_shuffle); // PTHREAD CALL
	while (more_to_shuffle)
	{
		gettimeofday(&timeNow, nullptr); // SYSTEM CALL
		timeToWake.tv_sec = timeNow.tv_sec;
		timeToWake.tv_nsec = USEC_TO_NSEC(timeNow.tv_usec) + 
			SEC_TO_NSEC(TIME_TO_WAIT);
		pthread_cond_timedwait(&cv_more_to_shuffle, 
				&mutex_more_to_shuffle, &timeToWake); // PTHREAD CALL
		
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
	pthread_mutex_unlock(&mutex_more_to_shuffle); // PTHREAD CALL
	return arg;
}

void* ExecReduce(void *arg)
{
	return arg;
}

OUT_ITEMS_LIST runMapReduceFramework(MapReduceBase &mapReduce, 
		IN_ITEMS_LIST &itemsList, int multiThreadLevel)
{
	// Initialize mutexes, cvs, datastructures...
	init();
	// Turn the in_items_list into a vector
	vector<IN_ITEM> itemsVector{std::begin(itemsList),
				    std::end(itemsList)  };
	// Initalize the mapdata struct
	MapData mapdata = {.mapReduce = mapReduce, .itemsList = itemsVector, 
		.numOfItems = itemsVector.size(), .nextToRead = 0};

	// Create the ExecMap threads!
	for (int i=0; i < multiThreadLevel; ++i)
	{
		pthread_create(&threadList[i], NULL, &ExecMap, &mapdata); // PTHREAD CALL
	}
	// Create the shuffle thread
	pthread_create(&shuffleThread, nullptr, &Shuffle, nullptr); // PTHREAD CALL	
	// Wait for the ExecMaps to finish
	for (int i=0; i < multiThreadLevel; ++i)
	{
		pthread_join(threadList[i], nullptr); // PTHREAD CALL
	}
	// Wait for the shuffle to finish
	pthread_mutex_lock(&mutex_more_to_shuffle); // PTHREAD CALL
	more_to_shuffle = false;
	pthread_mutex_unlock(&mutex_more_to_shuffle); // PTHREAD CALL
	pthread_cond_signal(&cv_more_to_shuffle); // PTHREAD CALL
	pthread_join(shuffleThread, nullptr); // PTHREAD CALL
	
	// Reduce!
	for (int i=0; i < multiThreadLevel; ++i)
	{
		pthread_create(&threadList[i], nullptr, &ExecReduce, nullptr); // PTHREAD CALL		
	}

	// "Shuffle" reduce's output!
	
	// Clean up
	cleanup();

	OUT_ITEMS_LIST* l = new OUT_ITEMS_LIST;	
	return *l;
}

void Emit2(k2Base *key, v2Base *val)
{
	// Figure out which thread is handling this function
	pthread_t currThread;
	currThread = pthread_self(); // PTHREAD CALL
	size_t end = mapResultLists.size();
	// Add the given key,val pair to the correct list:
	for (size_t i=0; i < end; ++i)
	{
		if (pthread_equal(mapResultLists[i].first, currThread)) // PTHREAD CALL
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
