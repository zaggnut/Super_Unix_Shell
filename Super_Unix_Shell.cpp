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
#include <errno.h>
#include <string>
#include <iostream>
#include <cstdio>
#include <cstring> //strdup, strtok
#include <list>
#include <vector>
#include <utility>

using namespace std;

bool shellExec(list<string> &command);
void checkRedirects(list<string> &command);
list<string> splitArgs(string input);
bool checkAwait(list<string> &args);
void printHistory(vector<string> &history);
void spinProc(list<string> &args);
pair<string, bool> historyRequest(string request, vector<string> &history);

int main()
{
    vector<string> history;
    for (;;)
    {
        cout << "TheFakeShell-> "; //because it is not a real shell
        string input;              //string to hold the next line
        getline(cin, input);
        auto args = splitArgs(input); //split into "words" by ' ' or '\t'
        if (args.size() > 0)
        {
            if (args.begin()->operator[](0) == '!')
            {
                if (history.empty())
                {
                    cout << "There is nothing in the history!" << endl;
                }
                auto res = historyRequest(*(args.begin()), history);
                if (res.second == false) //failed, do nothing
                {
                    continue;
                }
                cout << res.first << endl; //first is the history item
                input = res.first;
                args = splitArgs(input);
            }
            history.push_back(input);
            if (*(args.begin()) == "exit") //exit if first arg is exit
            {
                break;
            }
            if (*(args.begin()) == "history")
            {
                printHistory(history);
                continue;
            }
            spinProc(args);
        }
    }
    return 0;
}

bool shellExec(list<string> &command)
{
    char **argList = new char *[command.size() + 1];
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
    argList[i] = NULL; //last arg is null
    int res = execvp(command.begin()->c_str(), argList);
    _exit(res); //error, exit without flushing buffers
}

list<string> splitArgs(string input)
{

    list<string> output;
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

bool checkAwait(list<string> &args)
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
            it--; //go back one, avoid nullptr exception
        }
    }
    return toAwait;
}

void checkRedirects(list<string> &command)
{
    auto it = command.begin();
    it++; //start with the second since the first must be the command
    for (; it != command.end(); it++)
    {
        if (*it == ">")
        {
            auto firstErase = it;
            it++;
            //int outFile = open(it->c_str(), O_WRONLY | O_CREAT | O_TRUNC);
            FILE *outF = fopen(it->c_str(), "w");
            int outFile = fileno(outF);
            if (outFile == -1)
            {
                perror(strerror(errno));
            }
            it++;
            command.erase(firstErase, it);
            dup2(outFile, STDOUT_FILENO);
            fclose(outF);
            it = command.begin(); //start over, avoids nullptr exception
            continue;
        }

        if (*it == "<")
        {
            auto firstErase = it;
            it++;
            FILE *inF = fopen(it->c_str(), "r");
            int inFile = fileno(inF);
            //int inFile = open(it->c_str(), O_RDONLY);
            if (inFile == -1)
            {
                perror(strerror(errno));
            }
            it++;
            command.erase(firstErase, it);
            dup2(inFile, STDIN_FILENO);
            fclose(inF);
            it = command.begin(); //start over, avoids nullptr exception
        }
    }
}

void spinProc(list<string> &args)
{
    bool await = checkAwait(args);
    auto proc = fork();
    if (proc == -1) //failure
    {
        exit(EXIT_FAILURE);
    }
    else if (proc == 0) //child process
    {
        checkRedirects(args);
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

void printHistory(vector<string> &history)
{
    if (history.empty()) //empty history
    {
        cout << "The history is empty" << endl;
        return;
    }
    unsigned j = history.size();
    for (unsigned i = 0; i < history.size(); i++)
    {
        cout << j << " " << history[i] << endl;
        j--;
    }
}

pair<string, bool> historyRequest(string request, vector<string> &history)
{
    if (request == "!!") //return last command
    {
        return make_pair(history[history.size() - 1], true);
    }
    long num = 0;
    num = strtol(request.substr(1).c_str(), NULL, 0);
    if (num <= 0 || num > history.size())
    {
        cout << "that is not a valid position in the history" << endl;
        return make_pair("", false);
    }
    //else valid history
    return make_pair(history[history.size() - num], true);
}