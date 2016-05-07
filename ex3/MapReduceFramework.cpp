#include "MapReduceFramework.h"
#include <pthread.h>
#include <vector>
#include <queue>
#include <map>
#include <cmath> 		/* for fmin */
#include <sys/time.h>
#include <algorithm>

using std::vector;
using std::queue;
using std::map;
using std::multimap;
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
	MapReduceBase &mapReduce;
	size_t numOfItems;
	size_t nextToRead;
} ReduceData;

// A list of existing threads
vector<pthread_t> threadList;
pthread_t shuffleThread;
// In words: a list of pairs in which the first element is the pthread_id
// and the second is that thread's list (pointer) of the map-function-outputs

typedef queue<pair<k2Base*, v2Base*>> K2_V2_LIST;
vector	<pair<pthread_t, K2_V2_LIST*>> mapResultLists;
map<k2Base*, V2_LIST&> shuffleOut;
vector<pair<k2Base*, V2_LIST&>> reduceIn;
// Same as before, but for the reduce-function-outputs

typedef queue<pair<k3Base*, v3Base*>> K3_V3_LIST;
vector	<pair<pthread_t, K3_V3_LIST*>> reduceResultLists;
multimap<k3Base*, v3Base*> finalOutputMap;

pthread_mutex_t mutex_map_read;
pthread_mutex_t mutex_more_to_shuffle;
pthread_mutex_t mutex_reduce_read;

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
	pthread_mutex_lock(&mutex_map_read); // PTHREAD CALL
	// Reserve items to map and get the start index of them.
	myItems = (mapdata->nextToRead += CHUNK_SIZE) - CHUNK_SIZE;
	// Unlock
	pthread_mutex_unlock(&mutex_map_read); // PTHREAD CALL
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
	ReduceData* reducedata = (ReduceData*) arg;
	size_t myItems; // The start index for this thread's data
	// If there's no more data to read, return.
	if ((myItems = reducedata->nextToRead) >= reducedata->numOfItems)
	{
		pthread_exit(nullptr); // PTHREAD CALL
	}
	// Lock the index counter
	pthread_mutex_lock(&mutex_reduce_read); // PTHREAD CALL
	// Reserve items to map and get the start index of them.
	myItems = (reducedata->nextToRead += CHUNK_SIZE) - CHUNK_SIZE;
	// Unlock
	pthread_mutex_unlock(&mutex_reduce_read); // PTHREAD CALL
	int end = fmin(myItems + CHUNK_SIZE, reducedata->numOfItems); // CHECK ERROR
	for (int i=myItems; i < end; ++i)
	{
		reducedata->mapReduce.Reduce(reduceIn.at(i).first,
				       	     reduceIn.at(i).second);
	}
	return nullptr;
}

void finalize_output()
{
	pair<k3Base*, v3Base*> currPair;
	for (size_t i = 0; i < reduceResultLists.size(); ++i)
	{
		while(!reduceResultLists[i].second->empty())
		{
			currPair = reduceResultLists[i].second->front();
			reduceResultLists[i].second->pop();
			finalOutputMap.insert(currPair);
		}
	}
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
	// Transform the shuffle output map to a vector.	
	reduceIn.resize(shuffleOut.size());
	std::move(shuffleOut.begin(), shuffleOut.end(), reduceIn.begin());
	// Initialize the reducedata struct
	ReduceData reducedata = {.mapReduce = mapReduce, .nextToRead = 0};

	// Reduce!
	for (int i=0; i < multiThreadLevel; ++i)
	{
		pthread_create(&threadList[i], nullptr, &ExecReduce, &reducedata); // PTHREAD CALL		
	}
	// Wait for reduce to finish
	for (int i = 0; i < multiThreadLevel; ++i)
	{
		pthread_join(threadList[i], nullptr); // PTHREAD CALL
	}
	// "Shuffle" reduce's output!
	finalize_output();
	// Clean up
	cleanup();
	// Convert to output type and return.
	OUT_ITEMS_LIST finalOutput(finalOutputMap.size());
	std::move(finalOutputMap.begin(), finalOutputMap.end(), 
			finalOutput.begin());
	return finalOutput;
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
	pthread_t currThread;
	currThread = pthread_self(); // PTHREAD CALL
	size_t end = reduceResultLists.size();
	for (size_t i = 0; i < end; ++i)
	{
		if (pthread_equal(reduceResultLists[i].first, currThread)) // PTHREAD CALL
		{
			reduceResultLists[i].second->push(
						     std::make_pair(key, val));
			return;
		}
	}
}
