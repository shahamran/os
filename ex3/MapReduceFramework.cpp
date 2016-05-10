/* == Includes == */
#include "MapReduceFramework.h"

#include "my_pthread.h"		/* inlcudes pthread.h + error handling */
#include <fstream>
#include <vector>
#include <queue>
#include <map>

#include <cmath> 		/* for fmin */
#include <sys/time.h>		/* gettimeofday */
#include <ctime>		/* for strftime */
#include <chrono>		/* time measurements */
#include <algorithm>		/* for std::move */

/* To reduce line-length and increase readability */
using std::vector;
using std::queue;
using std::map;
using std::multimap;
using std::list;
using std::pair;

/* == Some constants == */
#define CHUNK_SIZE 10
#define USEC_TO_NSEC(x) ((x) * 1000)
#define NANO_IN_SEC 1000000000
#define TIME_TO_WAIT 1000000

#define TIME_FORMAT "[%d.%m.%Y %T]"

#define LOG_FILENAME ".MapReduceFramework.log"
#define LOG_OPEN_MODE std::ofstream::out | std::ofstream::app

#define THREAD_EXECMAP "ExecMap"
#define THREAD_SHUFFLE "Shuffle"
#define THREAD_EXECREDUCE "ExecReduce"

#define MSG_FRAMEWORK_START(t) "runMapReduceFramework started with " << t \
	<< " threads"
#define MSG_FRAMEWORK_END "runMapReduceFramework finished"
#define MSG_THREAD_CREATED(th) "Thread " th " created "
#define MSG_THREAD_TERMINATED(th) "Thread " th " terminated "
#define MSG_MAP_SHUFFLE_TIME(t) "Map and Shuffle took " << t << " ns"
#define MSG_REDUCE_TIME(t) "Reduce took " << t << " ns"

/* Struct for the map procedure. nextToRead is the only variable that *
 * will be mutexed, as specified in the instructions. */
typedef struct MapData
{
	MapReduceBase &mapReduce;
	vector<IN_ITEM> &itemsList;
	size_t numOfItems;
	size_t nextToRead;
	std::ofstream &logofs;
} MapData;

/* Same for reduce, the items list is static and therefore not in here. */
typedef struct ReduceData
{
	MapReduceBase &mapReduce;
	size_t numOfItems;
	size_t nextToRead;
	std::ofstream &logofs;
} ReduceData;

/* Comparator to sort keys by values, instead of adresses */
template<class T> struct ptr_less 
{
    bool operator()(T* lhs, T* rhs) 
    {
	return *lhs < *rhs;
    }
};

/* Threads variables */
vector<pthread_t> threadList; /* A list of existing ExecMap/Reduce threads */
pthread_t shuffleThread;      /* The shuffle thread */

/* ExecMap variables */
typedef vector<pair<k2Base*, v2Base*>> K2_V2_LIST;
/* a list of pairs in which the first element is the pthread_t (id) and *
 * the second is that thread's list (pointer) of the map-function-outputs */
vector<pair<pthread_t, K2_V2_LIST*>> mapResultLists;

/* Shuffle Variables */
/* An array of indices that specify till what index in the map-lists *
 * the shuffle thread has already read */
vector<size_t> shuffleCurrIndex;
/* Note that the map compares keys using ptr_less defined above */
map<k2Base*, V2_LIST*, ptr_less<k2Base>> shuffleOut;

/* Reduce Variables */
vector<pair<k2Base*, V2_LIST*>> reduceIn;
typedef queue<pair<k3Base*, v3Base*>> K3_V3_LIST;
/* Same as before, but for the reduce-function-outputs */
vector	<pair<pthread_t, K3_V3_LIST*>> reduceResultLists;
multimap<k3Base*, v3Base*, ptr_less<k3Base>> finalOutputMap;

/* Mutexes */
pthread_mutex_t mutex_map_read;		/* protect input list */
pthread_mutex_t mutex_em_map;		/* barrier to init maps */
pthread_mutex_t mutex_er_list;		/* barrier to init reduce dasts */
pthread_mutex_t mutex_more_to_shuffle;	/* condition of shuffle loop */
pthread_mutex_t mutex_reduce_read;	/* protect reduce input */
pthread_mutex_t mutex_log_write;	/* protect writing to log */
/* Condition Variables */
pthread_cond_t cv_more_to_shuffle;
bool more_to_shuffle;


/**
 * Gets the current time as a string in the exercise format. i.e
 * "[DD.MM.YYYY HH:MM:SS]"
 */
string getTimeStr()
{
	size_t MAX_CHARS = 64;
	time_t rawtime;
	struct tm * timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	char buff[MAX_CHARS];

	strftime(buff, MAX_CHARS, TIME_FORMAT, timeinfo);
	return buff;
}


/**
 * Initializes all mutexes, cv and relevant static variables for the library.
 */
void init(int multiThreadLevel)
{
	// Pthread types
	my_pthread_mutex_init(&mutex_map_read, nullptr);
	my_pthread_mutex_init(&mutex_em_map, nullptr);
	my_pthread_mutex_init(&mutex_er_list, nullptr);
	my_pthread_mutex_init(&mutex_more_to_shuffle, nullptr);
	my_pthread_cond_init(&cv_more_to_shuffle, nullptr);
	my_pthread_mutex_init(&mutex_reduce_read, nullptr);
	my_pthread_mutex_init(&mutex_log_write, nullptr);
	// Variables and data structures
	more_to_shuffle = true;
	threadList.resize(multiThreadLevel);
	mapResultLists.resize(multiThreadLevel);
	shuffleCurrIndex.resize(multiThreadLevel);
	reduceResultLists.resize(multiThreadLevel);
}

/**
 * Destroys mutexes, cv and clear the static data structures in use.
 */
void cleanup()
{
	// Pthread types
	my_pthread_mutex_destroy(&mutex_map_read);
	my_pthread_mutex_destroy(&mutex_em_map);
	my_pthread_mutex_destroy(&mutex_er_list);
	my_pthread_mutex_destroy(&mutex_more_to_shuffle);
	my_pthread_cond_destroy(&cv_more_to_shuffle);
	my_pthread_mutex_destroy(&mutex_reduce_read);
	my_pthread_mutex_destroy(&mutex_log_write);
	// heap-allocated data delete
	for (size_t i = 0; i < mapResultLists.size(); ++i)
	{
		delete mapResultLists[i].second;
		delete reduceResultLists[i].second;
		mapResultLists[i].second = nullptr;
		reduceResultLists[i].second = nullptr;
	}
	for (auto it = reduceIn.begin(); it != reduceIn.end(); ++it)
	{
		delete it->second;
		it->second = nullptr;
	}
	// Variable & data structures reset
	shuffleThread = 0;
	threadList.clear();
	mapResultLists.clear();
	reduceResultLists.clear();
	shuffleCurrIndex.clear();
	shuffleOut.clear();
	reduceIn.clear();
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
	/* A barrier, opens when all ExecMap data structures are *
	 * initialized */
	my_pthread_mutex_lock(&mutex_em_map);
	my_pthread_mutex_unlock(&mutex_em_map);
	MapData* mapdata = (MapData*) arg;
	size_t myItems; // The start index for this thread's data
	// While there's more data to read, read a chunk and map.
	while ((myItems = mapdata->nextToRead) < mapdata->numOfItems)
	{
		// Lock the index counter
		my_pthread_mutex_lock(&mutex_map_read); 
		// Reserve items to map and get the start index of them.
		myItems = (mapdata->nextToRead += CHUNK_SIZE) - CHUNK_SIZE;
		// Unlock
		my_pthread_mutex_unlock(&mutex_map_read); 
		// Map this thread's data
		int end = fmin(myItems + CHUNK_SIZE, mapdata->numOfItems); 
		for (int i=myItems; i < end; ++i)
		{
			mapdata->mapReduce.Map(mapdata->itemsList.at(i).first,
					      mapdata->itemsList.at(i).second);
		}
		// Notify the shuffle thread
		my_pthread_cond_signal(&cv_more_to_shuffle);
	}
	my_pthread_mutex_lock(&mutex_log_write);
	mapdata->logofs << MSG_THREAD_TERMINATED(THREAD_EXECMAP) 
		<< getTimeStr() << endl;
	my_pthread_mutex_unlock(&mutex_log_write);
	return nullptr;		
}


/**
 * The Shuffle procedure: Merge <k2,v2> pairs from the ExecMap containers
 * to <k2, queue<v2>> containers.
 */
void* Shuffle(void *)
{
	struct timespec timeToWake;
	struct timeval timeNow;
	pair<k2Base*, v2Base*> currPair;
	k2Base* currKey;
	v2Base* currVal;
	auto foundList = shuffleOut.end();
	V2_LIST* newList = nullptr;
	K2_V2_LIST* currList = nullptr;
	size_t currIdx = 0;

	my_pthread_mutex_lock(&mutex_more_to_shuffle); 
	while (more_to_shuffle)
	{
		if (gettimeofday(&timeNow, nullptr) < 0)
		{
			handleError("gettimeofday");
		}
		timeToWake.tv_sec = timeNow.tv_sec +  
			(USEC_TO_NSEC(timeNow.tv_usec) + TIME_TO_WAIT) / 
						NANO_IN_SEC;
		timeToWake.tv_nsec = fmod(USEC_TO_NSEC(timeNow.tv_usec) + 
			TIME_TO_WAIT, NANO_IN_SEC);
		// Wait for data (or time passing)
		my_pthread_cond_timedwait(&cv_more_to_shuffle, 
				&mutex_more_to_shuffle, &timeToWake); 
		// Check every ExecMap container for items to shuffle
		for (size_t i = 0; i < mapResultLists.size(); ++i)
		{
			currList = mapResultLists[i].second;
			// Read all unread data and shuffle it.
			for (currIdx = shuffleCurrIndex[i];
			     currList != nullptr && currIdx < currList->size();
			     ++currIdx)
			{
				currPair = currList->at(currIdx);
				currKey = currPair.first;
				currVal = currPair.second;
				foundList = shuffleOut.find(currKey);
				// If no k2 entry exists, create it.
				if (foundList == shuffleOut.end())
				{
					newList = new(std::nothrow) V2_LIST;
					// If new operator has failed
					if (newList == nullptr)
					{
						handleError("new operator");
					}
					newList->push_back(currVal);
					shuffleOut.insert(std::make_pair(
								currKey,
								newList));
					newList = nullptr;
				}
				else
				{
					foundList->second->push_back(currVal);
				}
			}
			// 'Mark' all read data as read
			shuffleCurrIndex[i] = currIdx;
		}
	}
	my_pthread_mutex_unlock(&mutex_more_to_shuffle);
	return nullptr;
}


/**
 * The Reduce procedure: Read input from a data structure and run the reduce
 * function on it to produce the output data
 */
void* ExecReduce(void *arg)
{
	my_pthread_mutex_lock(&mutex_er_list);
	my_pthread_mutex_unlock(&mutex_er_list);
	ReduceData* reducedata = (ReduceData*) arg;
	size_t myItems; // The start index for this thread's data
	// While there's more data to read, read a chunk
	while ((myItems = reducedata->nextToRead) < reducedata->numOfItems)
	{
		// Lock the index counter
		my_pthread_mutex_lock(&mutex_reduce_read); 
		// Reserve items to map and get the start index of them.
		myItems = (reducedata->nextToRead += CHUNK_SIZE) - CHUNK_SIZE;
		// Unlock
		my_pthread_mutex_unlock(&mutex_reduce_read); 
		int end = fmin(myItems + CHUNK_SIZE, reducedata->numOfItems);
		for (int i = myItems; i < end; ++i)
		{
			reducedata->mapReduce.Reduce( reduceIn.at(i).first,
						     *reduceIn.at(i).second);
		}
	}
	my_pthread_mutex_lock(&mutex_log_write);
	reducedata->logofs << MSG_THREAD_TERMINATED(THREAD_EXECREDUCE) 
		<< getTimeStr() << endl;
	my_pthread_mutex_unlock(&mutex_log_write);
	return nullptr;
}


/**
 * Transforms the many reduce-thread-data-structures to one big map.
 * Same as shuffle, but for reduce.
 */
void finalize_output()
{
	pair<k3Base*, v3Base*> currPair;
	for (size_t i = 0; i < reduceResultLists.size(); ++i)
	{
		while( reduceResultLists[i].second != nullptr &&
		      !reduceResultLists[i].second->empty() )
		{
			currPair = reduceResultLists[i].second->front();
			reduceResultLists[i].second->pop();
			finalOutputMap.insert(currPair);
		}
	}
}


/**
 * The main framework function.
 */
OUT_ITEMS_LIST runMapReduceFramework(MapReduceBase &mapReduce, 
		IN_ITEMS_LIST &itemsList, int multiThreadLevel)
{
	// open the log file, write and start measuring time
	std::ofstream ofs(LOG_FILENAME, LOG_OPEN_MODE);
	ofs << MSG_FRAMEWORK_START(multiThreadLevel) << endl;
	std::chrono::high_resolution_clock::time_point t1 = 
		std::chrono::high_resolution_clock::now();

	// Initialize mutexes, cvs, datastructures...
	init(multiThreadLevel);
	// Turn the in_items_list into a vector for random access
	vector<IN_ITEM> itemsVector( std::begin(itemsList),
				     std::end(itemsList)  );
	
	MapData mapdata = {.mapReduce = mapReduce, .itemsList = itemsVector, 
		.numOfItems = itemsVector.size(), 
		.nextToRead = 0, .logofs = ofs};

	// Create the ExecMap threads!
	my_pthread_mutex_lock(&mutex_em_map); // Barrier to prevent sigsegv
	for (int i = 0; i < multiThreadLevel; ++i)
	{
		my_pthread_mutex_lock(&mutex_log_write);
		ofs << MSG_THREAD_CREATED(THREAD_EXECMAP) << getTimeStr() 
		    << endl;
		my_pthread_mutex_unlock(&mutex_log_write);
		my_pthread_create(&threadList[i], nullptr, &ExecMap, &mapdata);
	}
	// Initialize all threads' maps
	for (int i = 0; i < multiThreadLevel; ++i)
	{
		mapResultLists[i] = std::make_pair(threadList[i], 
				new(std::nothrow) K2_V2_LIST);
		if (mapResultLists[i].second == nullptr)
		{
			handleError("new operator");
		}
	}
	my_pthread_mutex_unlock(&mutex_em_map); // Remove the barrier

	// Create the shuffle thread
	my_pthread_mutex_lock(&mutex_log_write);
	ofs << MSG_THREAD_CREATED(THREAD_SHUFFLE) << getTimeStr() << endl;
	my_pthread_mutex_unlock(&mutex_log_write);
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

	// Stop time measuring
	std::chrono::high_resolution_clock::time_point t2 =
		std::chrono::high_resolution_clock::now();
	auto mapShuffleDuration = std::chrono::duration_cast
		<std::chrono::nanoseconds>(t2 - t1).count();
	ofs << MSG_THREAD_TERMINATED(THREAD_SHUFFLE) << getTimeStr() << endl;
	
	// Transform the shuffle output map to a vector.	
	reduceIn.resize(shuffleOut.size());
	std::move(shuffleOut.begin(), shuffleOut.end(), reduceIn.begin());
	// Initialize the reducedata struct
	ReduceData reducedata = {.mapReduce = mapReduce, 
		.numOfItems = reduceIn.size(), .nextToRead = 0, .logofs = ofs};

	// Measure reduce time
	t1 = std::chrono::high_resolution_clock::now();
	// Reduce!
	my_pthread_mutex_lock(&mutex_er_list);
	for (int i = 0; i < multiThreadLevel; ++i)
	{
		my_pthread_mutex_lock(&mutex_log_write);
		ofs << MSG_THREAD_CREATED(THREAD_EXECREDUCE) << getTimeStr()
			<< endl;
		my_pthread_mutex_unlock(&mutex_log_write);
		my_pthread_create(&threadList[i], nullptr, 
				&ExecReduce, &reducedata); 		
	}
	for (int i = 0; i < multiThreadLevel; ++i)
	{
		reduceResultLists[i] = std::make_pair(threadList[i], 
				new(std::nothrow) K3_V3_LIST);
		if (reduceResultLists[i].second == nullptr)
		{
			handleError("new operator");
		}
	}
	my_pthread_mutex_unlock(&mutex_er_list);
	// Wait for reduce to finish
	for (int i = 0; i < multiThreadLevel; ++i)
	{
		my_pthread_join(threadList[i], nullptr); 
	}
	// "Shuffle" reduce's output!
	finalize_output();
	// Convert to output type and return.
	OUT_ITEMS_LIST finalOutput(finalOutputMap.size());
	std::move(finalOutputMap.begin(), finalOutputMap.end(), 
			finalOutput.begin());
	// Clean up
	cleanup();

	// Stop measuring time, write to log and return
	t2 = std::chrono::high_resolution_clock::now();
	auto reduceDuration = std::chrono::duration_cast
		<std::chrono::nanoseconds>(t2 - t1).count();
	ofs << MSG_MAP_SHUFFLE_TIME(mapShuffleDuration) << endl;
	ofs << MSG_REDUCE_TIME(reduceDuration) << endl;
	ofs << MSG_FRAMEWORK_END << endl;
	return finalOutput;
}


/**
 * Add <k2*,v2*> to the map-threads data structures
 */
void Emit2(k2Base *key, v2Base *val)
{
	// Figure out which thread is handling this function
	pthread_t currThread;
	currThread = pthread_self(); 
	size_t end = mapResultLists.size();
	// Add the given key,val pair to the correct list:
	for (size_t i = 0; i < end; ++i)
	{
		if (pthread_equal(mapResultLists[i].first, currThread)) 
		{
			mapResultLists[i].second->push_back(
					          std::make_pair(key, val) );
			return;
		}
	}
}

/**
 * Add <k3*,v3*> to the reduce-threads data structures 
 */
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
