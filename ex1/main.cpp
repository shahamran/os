#include <iostream>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include "osm.h"

#define FINISH_ERROR -1
#define FINISH_SUCC 0

void printStruct(const timeMeasurmentStructure& tv)
{
    std::cout << "Machine Name: " << tv.machineName << std::endl;
    std::cout << "Instruction Time: " << tv.instructionTimeNanoSecond << " ns" << std::endl;
    std::cout << "Function Time: " << tv.functionTimeNanoSecond << " ns" <<  std::endl;
    std::cout << "Trap Time: " << tv.trapTimeNanoSecond << " ns" <<  std::endl;
    std::cout << "Disk Time: " << tv.diskTimeNanoSecond << " ns" <<  std::endl;
    std::cout << "Func/Instruction: " << tv.functionInstructionRatio << std::endl;
    std::cout << "Trap/Instruction: " << tv.trapInstructionRatio << std::endl;
    std::cout << "Disk/Instruction: " << tv.diskInstructionRatio << std::endl;
}

int main()
{
    if (osm_init() != FINISH_SUCC)
    {
        return FINISH_ERROR;
    }
    std::string buff = "";
    double retVal = 0.0;
    timeMeasurmentStructure measuredTime;
    while (buff != "exit")
    {
        std::cout << "What to do? (exit/basic/...)" << std::endl;
        std::cin >> buff;

        if (buff == "basic")
        {
            retVal = osm_operation_time(20000);
            std::cout << "The average basic operations time measured is: " << retVal << std::endl;
        }
        else if (buff == "func")
        {
            retVal = osm_function_time(20000);
            std::cout << "The average empty function time measured is: " << retVal << std::endl;
        }
        else if (buff == "trap")
        {
            retVal = osm_syscall_time(80000);
            std::cout << "The average trap time measured is: " << retVal << std::endl;
        }
        else if (buff == "disk")
        {
            retVal = osm_disk_time(10000);
            std::cout << "The average disk time measured is: " << retVal << std::endl;
        }
        else if (buff == "all")
        {
            measuredTime = measureTimes(20000, 20000, 80000, 10000);
        }
    }
    if (osm_finalizer() != FINISH_SUCC)
    {
        return FINISH_ERROR;
    }
    return FINISH_SUCC;
}
