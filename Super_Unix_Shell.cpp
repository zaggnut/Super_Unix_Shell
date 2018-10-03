/*
Purpose: To create the Super Unix Shell
Requires at least C++11
Creation Date: 9/26/2018
Author(s): Shane Laskowski & Michael Lingo
Date Modified: 10/02/2018
Author Of Modification: Michael Lingo
*/

//insert amazing code here

#include <unistd.h> //fork, execvp, dup2
#include <sys/wait.h> //wait, waitpid
#include <cerrno> //perror
#include <string>
#include <iostream> //cout, cin
#include <cstdio> //fopen
#include <cstring> //strdup, strtok
#include <list>
#include <vector>
#include <utility> //pair

using namespace std;

bool shellExec(list<string> &command);
void checkRedirects(list<string> &command);
list<string> splitArgs(string input);
bool checkAwait(list<string> &args);
void printHistory(vector<string> &history);
pair<pid_t, bool> spinProc(list<string> &args);
pair<string, bool> historyRequest(string request, vector<string> &history);
void checkRunningProcs(list<pid_t> &runningProcs);

int main()
{
	vector<string> history;
	list<pid_t> runningProcs;

	for (;;)
	{
		cout << "TheFakeShell-> "; //because it is not a real shell
		string input;              //string to hold the next line
		getline(cin, input);
		auto args = splitArgs(input); //split into "words" by ' ' or '\t'
		checkRunningProcs(runningProcs);
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
			auto status = spinProc(args);
			if (status.second == false)
			{
				runningProcs.push_back(status.first);
			}
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
		argList[i] = strdup(it->c_str());
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
	char *buffer = strdup(input.c_str());
	arg = strtok(buffer, " \t");
	for (;;) //go until next token is null;
	{
		if (arg == NULL)
			break;
		output.push_back(arg);
		arg = strtok(NULL, " \t");
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

pair<pid_t, bool> spinProc(list<string> &args)
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
	return make_pair(proc, await);
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

void checkRunningProcs(list<pid_t> &runningProcs)
{
	if (runningProcs.empty()) //no processes to check for
	{
		return;
	}
	int status = 0;
	int finishedProc = 0;
	for (auto it = runningProcs.begin(); it != runningProcs.end(); it++)
	{
		finishedProc = waitpid(*it, &status, WNOHANG); //don't block, returns 0 if not finished
		if (finishedProc != 0)
		{
			cout << endl << "Process with pid " << finishedProc
				<< " exited with code: " << status << endl;

			runningProcs.erase(it);
			it = runningProcs.begin(); //restart after erase to avoid nullptrs
		}
	}
}
