#include "MapReduceClient.h"
#include "MapReduceFramework.h"
#include <string>
#include <vector>
#include <iostream>

#define EXIT_SUCC 0
#define EXIT_FAIL 1

// Constants:
const int MIN_ARGS = 2;
const std::string USAGE_ERROR_MESSAGE = "Usage: <substring to search> "
	"<folders, separated by spaces>";

class FileNameKey : k1Base
{
public:
	FileNameKey(const std::string f) : fileName(f) {}	

	virtual ~FileNameKey() {}

	virtual bool operator<(const FileNameKey &other) const
	{
		return fileName.compare(other.fileName);
	}
private:
	const std::string fileName;
};

//intermediate key and value.
//the key, value for the Reduce function created by the Map function
class k2Base {
public:
	virtual ~k2Base(){}
    virtual bool operator<(const k2Base &other) const = 0;
};

class v2Base {
public:
	virtual ~v2Base(){}
};

//output key and value
//the key,value for the Reduce function created by the Map function
class k3Base {
public:
	virtual ~k3Base()  {}
    virtual bool operator<(const k3Base &other) const = 0;
};

class v3Base {
public:
	virtual ~v3Base() {}
};

typedef std::list<v2Base *> V2_LIST;

class MapReduceBase {
public:
    virtual void Map(const k1Base *const key, const v1Base *const val) const = 0;
    virtual void Reduce(const k2Base *const key, const V2_LIST &vals) const = 0;
};


// Implementation
int main(int argc, char* argv[])
{
	std::vector<std::pair<FileNameKey, v1Base>> inList;
	if (argc >= MIN_ARGS)
	{
				
	}
	else
	{
		std::cerr << USAGE_ERROR_MESSAGE << std::endl;
		std::exit(EXIT_FAIL);
	}
	return EXIT_SUCC;
}
