/*
Purpose: To create the Super Unix Shell
Creation Date: 9/26/2018
Author(s): Shane Laskowski & Michael Lingo
Date Modified: 9/28/2018
Author Of Modification: Michel Lingo
*/

//insert amazing code here

#include <unistd.h>
#include <string>
#include <iostream>
#include <cstring>
#include <vector>

bool shellExec(std::string command);
std::vector<std::string> splitArgs(std::string input);

int main()
{
    for (;;)
    {
        std::cout << "TheFakeShell-$ ";
        std::string input;
        std::cin >> input;
        auto args = splitArgs(input);
        if (args.size() > 0)
        {
            if (args[0] == "exit")
            {
                break;
            }
        }
    }
    return 0;
}

std::vector<std::string> splitArgs(std::string input)
{

    std::vector<std::string> output;
    char *arg;
#ifdef _MSC_VER //if being built with Visual Studio
    char *buffer = _strdup(input.c_str());
    //convert from const char* to char*
    char *nextToken = NULL;
    arg = strtok_s(buffer, " \t", &nextToken);
#else
    char *buffer = strdup(input.c_str());
    arg = strtok(buffer, " \t");
#endif
    for (;;) //go until next token is null;
    {

        if (arg == NULL)
            break;
        output.push_back(arg);
#ifdef _MSC_VER
        arg = strtok_s(NULL, " \t", &nextToken);
#else
        arg = strtok(NULL, " \t");
#endif
    }
    free(buffer);
    return output;
}