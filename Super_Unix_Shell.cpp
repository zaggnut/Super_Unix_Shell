/*
Purpose: To create the Super Unix Shell
Creation Date: 9/26/2018
Author(s): Shane Laskowski & Michael Lingo
Date Modified: 9/28/2018
Author Of Modification: Michel Lingo
*/

//insert amazing code here

#include <unistd.h>
#include <sys/types.h>
#include <cstdlib>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <iostream>
#include <cstdio>
#include <cstring> //strdup, strtok
#include <list>

bool shellExec(std::list<std::string> &command);
void checkRedirects(std::list<std::string> &command);
std::list<std::string> splitArgs(std::string input);
bool checkAwait(std::list<std::string> &args);

int main()
{
    for (;;)
    {
        std::cout << "TheFakeShell-$ "; //because it is not a real shell
        std::string input;              //string to hold the next line
        std::cin >> input;
        auto args = splitArgs(input); //split into "words" by ' ' or '\t'
        if (args.size() > 0)
        {
            if (*(args.begin()) == "exit") //exit if first arg is exit
            {
                break;
            }
            bool await = checkAwait(args);
            auto proc = fork();
            if (proc == -1) //failure
            {
                exit(EXIT_FAILURE);
            }
            else if (proc == 0)
            {
                shellExec(args);
            }
            else //parent process
            {
                if (await)
                {
                    int result;
                    wait(&result);
                }
            }
        }
    }
    return 0;
}

bool shellExec(std::list<std::string> &command)
{
    char **argList = new char *[command.size() - 1];
    int i = 0;
    for (auto it = command.begin(); it != command.end(); it++)
    {
#ifdef _MSC_VER //strdup is prefixed with an '_' on the microsoft compiler
        argList[i] = _strdup(it->c_str());
#else
        argList[i] = strdup(it->c_str());
#endif
        i++;
    }
    int res = execvp(command.begin()->c_str(), argList);
    exit(res);
}

std::list<std::string> splitArgs(std::string input)
{

    std::list<std::string> output;
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

bool checkAwait(std::list<std::string> &args)
{
    bool toAwait = true;
    auto it = args.begin();
    it++; //start with the second since the first must be the command
    for (; it != args.end(); it++)
    {
        if (*it == "&")
        {
            toAwait = false;
            args.erase(it);
        }
    }
    return toAwait;
}

void checkRedirects(std::list<std::string> &command)
{
    auto it = command.begin();
    it++; //start with the second since the first must be the command
    for (; it != command.end(); it++)
    {
        if (*it == ">")
        {
            command.erase(it);
            it++;
            int outFile = open(it->c_str(), O_WRONLY);
            command.erase(it);
            dup2(STDOUT_FILENO, outFile);
            continue;
        }
        if(*it == "<")
        {
            command.erase(it);
            it++;
            int inFile = open(it->c_str(), O_RDONLY);
            command.erase(it);
            dup2(STDIN_FILENO, inFile);
        }
    }

}