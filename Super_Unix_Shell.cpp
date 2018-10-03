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
void printHistory(vector<string> &history);//
pair<pid_t, bool> spinProc(list<string> &args);
pair<string, bool> historyRequest(string request, vector<string> &history);
void checkRunningProcs(list<pid_t> &runningProcs);

int main()
{
	vector<string> history;
	list<pid_t> runningProcs;

	for (;;) //infinite loop (program exits with Ctrl + c while in linux server)
	{
		cout << "TheFakeShell-> "; //because it is not a real shell [or is it!?]
		string input;              //string to hold the next line
		getline(cin, input);
		auto args = splitArgs(input); //split into "words" by ' ' or '\t'
		checkRunningProcs(runningProcs);
		if (args.size() > 0) //if user has actually typed anything into his/her command
		{
			if (args.begin()->operator[](0) == '!') //accesses the [] from the args pointed object and checks 1st index
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
			history.push_back(input); //user command saved into history container
			if (*(args.begin()) == "exit") //exit if first arg is exit
			{
				break; //the infinite for loop breaks ==>  basically it means the end of the program 
			}
			if (*(args.begin()) == "history") //if user types in the history command, then history is printed
			{
				printHistory(history); //history of user input printed
				continue; //
			}
			auto status = spinProc(args); //runs the command that user typed in, passes the pair of PID and bool result into status
			if (status.second == false) //if spinProc returned false bool
			{
				runningProcs.push_back(status.first); //adds the PID held in status within the runningProcs List
			}
		}
	}
	return 0;
}

/*
Purpose:
Parameters:
*/
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


/*
PURPOSE:splits each word in the user input inorder to help determine what each command is.
PARAMETERS: the string input is what the user types in.  each word needs to be seperated to be handled/identified
*/
list<string> splitArgs(string input)
{

	list<string> output; //used to store all the "words"/modifiers of the user command
	char *arg;
	char *buffer = strdup(input.c_str()); //places c-string equivalent of input into the heap and the c-string pointer buffer points to it
	arg = strtok(buffer, " \t"); //the arg pointer points to the "words"/modifiers in variable buffer.  the space + tab delimits each word
	for (;;) //go until next token, arg, is null;
	{
		if (arg == NULL) //no more "words"/modifiers in user input
			break;
		output.push_back(arg); //pushes a word into the output variable
		arg = strtok(NULL, " \t");
	}
	free(buffer); //in order to avoid memory leak, the memory used by buffer is designated as free for re-use
	return output; //returns all the "words"/modifiers of the command
}

/*
PURPOSE:
PARAMETERS:
*/
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

/*
PURPOSE:
PARAMETERS:
*/
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

/*
PURPOSE: Handles the User Input
PARAMETERS: args is a list containing all the "words" of the user command
*/
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

/*
PURPOSE: prints the history of user inputted commands.  prints the historical number of the command.
PARAMETERS: history vector that contains all the commands typed in.  it is to be printed
*/
void printHistory(vector<string> &history)
{
	if (history.empty()) //empty history
	{
		cout << "The history is empty" << endl;
		return;
	}
	unsigned j = history.size(); //**************can't we just replace with i + 1 within the loop ?
	for (unsigned i = 0; i < history.size(); i++)
	{
		cout << j << " " << history[i] << endl;
		j--;
	}
}

/*
PURPOSE:
PARAMETERS:
*/
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

/*
PURPOSE:
PARAMETERS:
*/
void checkRunningProcs(list<pid_t> &runningProcs)
{
	if (runningProcs.empty()) //no processes to check for
	{
		return;
	}
	int status = 0;
	int finishedProc = 0;
	for (auto it = runningProcs.begin(); it != runningProcs.end(); it++) //iterates for the # of processes running
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
