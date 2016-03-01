#include <iostream>
#include <string>
#include "osm.h"

int main()
{
    std::string buff = "";
    double retVal = 0.0;
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
            std:: cout << "The average trap time measured is: " << retVal << std::endl;
        }
    }
}
