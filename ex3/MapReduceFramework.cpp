#include "MapReduceFramework.h"

//#include <pthread.h>
#include "my_pthread.h"
#include <vector>
#include <queue>
#include <map>

#include <cmath> 		/* for fmin */
#include <sys/time.h>		/* gettimeofday */
#include <algorithm>		/* for std::move */

// To reduce line-length and increase readability
using std::vector;
using std::queue;
using std::map;
using std::multimap;
using std::list;
using std::pair;

// Some constants
#define CHUNK_SIZE 10
#define SEC_TO_NSEC(x) ((x) * 1000000000)
#define USEC_TO_NSEC(x) ((x) * 1000)
#define TIME_TO_WAIT 0.01
#define PTHREAD_SUCCESS 0

/* Struct for the map procedure. nextToRead is the only variable that *
 * will be mutexed, as specified in the instructions. */
typedef struct MapData
{
	MapReduceBase &mapReduce;
	vector<IN_ITEM> &itemsList;
	size_t numOfItems;
	size_t nextToRead;
} MapData;

/* Same for reduce, the items list is static */
typedef struct ReduceData
{
	MapReduceBase &mapReduce;
	size_t numOfItems;
	size_t nextToRead;
} ReduceData;

// A list of existing ExecMap/ExecReduce threads
vector<pthread_t> threadList;
// The shuffle thread
pthread_t shuffleThread;

typedef queue<pair<k2Base*, v2Base*>> K2_V2_LIST;
/* a list of pairs in which the first element is the pthread_id and *
 * the second is that thread's list (pointer) of the map-function-outputs */
vector	<pair<pthread_t, K2_V2_LIST*>> mapResultLists;

map<k2Base*, V2_LIST&> shuffleOut;
vector<pair<k2Base*, V2_LIST&>> reduceIn;

typedef queue<pair<k3Base*, v3Base*>> K3_V3_LIST;
// Same as before, but for the reduce-function-outputs
vector	<pair<pthread_t, K3_V3_LIST*>> reduceResultLists;
multimap<k3Base*, v3Base*> finalOutputMap;

// Mutexes
pthread_mutex_t mutex_map_read;
pthread_mutex_t mutex_more_to_shuffle;
pthread_mutex_t mutex_reduce_read;
// Condition Variables
pthread_cond_t cv_more_to_shuffle;
bool more_to_shuffle;

void init()
{
	my_pthread_mutex_init(&mutex_map_read, nullptr);
	my_pthread_mutex_init(&mutex_more_to_shuffle, nullptr);
	my_pthread_cond_init(&cv_more_to_shuffle, nullptr);
	my_pthread_mutex_init(&mutex_reduce_read, nullptr);
	more_to_shuffle = true;
}

void cleanup()
{
	my_pthread_mutex_destroy(&mutex_map_read);
	my_pthread_mutex_destroy(&mutex_more_to_shuffle);
	my_pthread_cond_destroy(&cv_more_to_shuffle);
	my_pthread_mutex_destroy(&mutex_reduce_read);

	threadList.clear();
	shuffleThread = 0;
	mapResultLists.clear();
	shuffleOut.clear();
	reduceIn.clear();
	reduceResultLists.clear();
	finalOutputMap.clear();
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
		pthread_exit(nullptr); 
	}
	// Lock the index counter
	my_pthread_mutex_lock(&mutex_map_read); 
	// Reserve items to map and get the start index of them.
	myItems = (mapdata->nextToRead += CHUNK_SIZE) - CHUNK_SIZE;
	// Unlock
	my_pthread_mutex_unlock(&mutex_map_read); 
	int end = fmin(myItems + CHUNK_SIZE, mapdata->numOfItems); 
	for (int i=myItems; i < end; ++i)
	{
		mapdata->mapReduce.Map(mapdata->itemsList.at(i).first,
				       mapdata->itemsList.at(i).second);
	}
	// Notify the shuffle thread
	my_pthread_cond_signal(&cv_more_to_shuffle);
	return nullptr;		
}

void* Shuffle(void *arg)
{
	struct timespec timeToWake;
	struct timeval timeNow;
	pair<k2Base*, v2Base*> currPair;
	my_pthread_mutex_lock(&mutex_more_to_shuffle); 
	while (more_to_shuffle)
	{
		if (gettimeofday(&timeNow, nullptr) < 0)
		{
			cerr << DEF_ERROR_MSG("gettimeofday") << endl;
			exit(EXIT_FAIL);
		}
		timeToWake.tv_sec = timeNow.tv_sec;
		timeToWake.tv_nsec = USEC_TO_NSEC(timeNow.tv_usec) + 
			SEC_TO_NSEC(TIME_TO_WAIT);
		my_pthread_cond_timedwait(&cv_more_to_shuffle, 
				&mutex_more_to_shuffle, &timeToWake); 
		
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
	my_pthread_mutex_unlock(&mutex_more_to_shuffle); 
	return arg;
}

void* ExecReduce(void *arg)
{
	ReduceData* reducedata = (ReduceData*) arg;
	size_t myItems; // The start index for this thread's data
	// If there's no more data to read, return.
	if ((myItems = reducedata->nextToRead) >= reducedata->numOfItems)
	{
		pthread_exit(nullptr); 
	}
	// Lock the index counter
	my_pthread_mutex_lock(&mutex_reduce_read); 
	// Reserve items to map and get the start index of them.
	myItems = (reducedata->nextToRead += CHUNK_SIZE) - CHUNK_SIZE;
	// Unlock
	my_pthread_mutex_unlock(&mutex_reduce_read); 
	int end = fmin(myItems + CHUNK_SIZE, reducedata->numOfItems);
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
	threadList.resize(multiThreadLevel);
	// Turn the in_items_list into a vector
	vector<IN_ITEM> itemsVector{std::begin(itemsList),
				    std::end(itemsList)  };
	// Initalize the mapdata struct
	MapData mapdata = {.mapReduce = mapReduce, .itemsList = itemsVector, 
		.numOfItems = itemsVector.size(), .nextToRead = 0};

	// Create the ExecMap threads!
	for (int i = 0; i < multiThreadLevel; ++i)
	{
		my_pthread_create(&threadList[i], nullptr, &ExecMap, &mapdata);
	}
	// Create the shuffle thread
	my_pthread_create(&shuffleThread, nullptr, &Shuffle, nullptr);
	// Wait for the ExecMaps to finish
	for (int i = 0; i < multiThreadLevel; ++i)
	{
		my_pthread_join(threadList[i], nullptr);
	}
	// Wait for the shuffle to finish
	my_pthread_mutex_lock(&mutex_more_to_shuffle); 
	more_to_shuffle = false;
	my_pthread_mutex_unlock(&mutex_more_to_shuffle); 
	my_pthread_cond_signal(&cv_more_to_shuffle); 
	my_pthread_join(shuffleThread, nullptr); 
	// Transform the shuffle output map to a vector.	
	reduceIn.resize(shuffleOut.size());
	std::move(shuffleOut.begin(), shuffleOut.end(), reduceIn.begin());
	// Initialize the reducedata struct
	ReduceData reducedata = {.mapReduce = mapReduce, .nextToRead = 0};

	// Reduce!
	for (int i = 0; i < multiThreadLevel; ++i)
	{
		my_pthread_create(&threadList[i], nullptr, 
				&ExecReduce, &reducedata); 		
	}
	// Wait for reduce to finish
	for (int i = 0; i < multiThreadLevel; ++i)
	{
		my_pthread_join(threadList[i], nullptr); 
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
	currThread = pthread_self(); 
	size_t end = mapResultLists.size();
	// Add the given key,val pair to the correct list:
	for (size_t i=0; i < end; ++i)
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
	pthread_t currThread;
	currThread = pthread_self(); 
	size_t end = reduceResultLists.size();
	for (size_t i = 0; i < end; ++i)
	{
		if (pthread_equal(reduceResultLists[i].first, currThread)) 
		{
			reduceResultLists[i].second->push(
						     std::make_pair(key, val));
			return;
		}
	}
}
