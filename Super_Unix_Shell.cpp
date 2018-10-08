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
				break; //the infinite for loop breaks ==>  basically it means the end of the entire program 
			}
			if (*(args.begin()) == "history") //if user types in the history command, then history is printed
			{
				printHistory(history); //history of user input printed
				continue; //The history command has been ran ==> no need to run the code block below ==> re-rerun the loop for next user's command
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
Purpose: rewrites the context/image of the process.  runs the program described in the command parameter
Parameters:command is the user command and any other modifying user arguments.
*/
bool shellExec(list<string> &command)
{
	char **argList = new char *[command.size() + 1]; //creates a pointer to a char pointer, an array of c-strings to be terminated by a null terminator
	int i = 0;
	for (auto it = command.begin(); it != command.end(); it++) //traverses the cstrings (just the command + any modifying arguments)
	{
		argList[i] = strdup(it->c_str()); //returns a null termianted c-string createed in heap into the array
		i++;
	}
	argList[i] = NULL; //last arg is null
	int res = execvp(command.begin()->c_str(), argList); //rewrites the image/context of the current process.  runs the program/file with the argument list provided
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
PURPOSE: if the user's command has the & command modifier, then the parent process should NOT wait() until the child process ends
PARAMETERS: args is the entire user command, it is to be checked if & modifier is added to the command.
*/
bool checkAwait(list<string> &args) //args is passed by reference because modification made to it should reflect in the function caller
{
	bool toAwait = true;
	auto it = args.begin(); //"it" is pointer to args, "it" will be used to traverse args 
	it++; //start with the second since the first must be the command

	//searches args for a "&"
	for (; it != args.end(); it++)
	{
		if (*it == "&") //a & was found ==> the command should run a child process in the background
		{
			toAwait = false;
			args.erase(it); //args is modified here, the & has been handled ==> it is removed so the program can focus on the rest of the commands
			it--; //go back one, avoid nullptr exception
		}
	} 
	return toAwait;
}

/*
PURPOSE: this function checks whether the user's command included redirects ("<" and/or ">").  if either is found
then the redirects will be handled.
PARAMETERS:command is the list of the user's command and any modifying arguments.  it will be modified so its passed by ref.
*/
void checkRedirects(list<string> &command)
{
	auto it = command.begin(); //this variable is used to traverse the user's arguments used on the user's command
	it++; //start with the second since the first must be the command
	for (; it != command.end(); it++)
	{
		if (*it == ">")
		{
			auto firstErase = it; //we want to keep a reference to the > so we can handle it later
			it++; //travsere to the empty space that SHOULD come after ">"
			//int outFile = open(it->c_str(), O_WRONLY | O_CREAT | O_TRUNC);
			FILE *outF = fopen(it->c_str(), "w"); //set the file pointer to write mode, writes to the file named by the null terminated string "it" is pointing at
			int outFile = fileno(outF); //returns the file integer descriptor of outF
			if (outFile == -1)
			{
				perror(strerror(errno)); //strerror returns an error message in form of a string, perror prints it out to standard error output stream (to console)
			}
			it++; //traverses past the empty space onto the next character of the list command
			command.erase(firstErase, it); // ">" has been handled, therefore it will be erased
			dup2(outFile, STDOUT_FILENO); //redirects stdoutput to the file that is discripted by outfile
			fclose(outF); //close the file
			it = command.begin(); //start over, avoids nullptr exception
			continue;
		}

		
		if (*it == "<")
		{
			auto firstErase = it;
			it++;
			FILE *inF = fopen(it->c_str(), "r"); //opens a file named after the null terminated string, sets mode to read from that file
			int inFile = fileno(inF);//returns file descriptor of inF
			//int inFile = open(it->c_str(), O_RDONLY);
			if (inFile == -1)
			{
				perror(strerror(errno));
			}
			it++;
			command.erase(firstErase, it); //"<" has been handled ==> erase it, less trouble to deal within the rest of the program
			dup2(inFile, STDIN_FILENO); //std output redirected to the file descripted by inFile
			fclose(inF); //close file we done
			it = command.begin(); //start over, avoids nullptr exception
		}
	}
}

/*
PURPOSE: Handles the user command that is Not History or Exit
PARAMETERS: args is a list containing all the "words" of the user's command
*/
pair<pid_t, bool> spinProc(list<string> &args)
{
	bool await = checkAwait(args); //checks if & modifier is added to the command.
	auto proc = fork(); 
	if (proc == -1) //failure, blame Linus
	{
		exit(EXIT_FAILURE);
	}
	else if (proc == 0) //child process
	{
		checkRedirects(args); // checks for ">" or "<" and handles them
		shellExec(args);//calls an exec function to rewrite the image/context of this child process to the apporpriate programe/file
	}
	else //parent process
	{
		if (await) //if the parent process should wait, then it will wait, else it will continue to run
		{
			int result; //********************************** whutttt????
			wait(&result);
		}
	}
	return make_pair(proc, await); //returns 
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
PURPOSE: returns information regarding the history of user commands
PARAMETERS: a request string is used as an argument for specific info regarding the user history.  the history vector is list of all user commands (excluding exit and history)
*/
pair<string, bool> historyRequest(string request, vector<string> &history)
{
	if (request == "!!") //return last command
	{
		return make_pair(history[history.size() - 1], true);
	}
	long num = 0;
	num = strtol(request.substr(1).c_str(), NULL, 0); //num will be used to return an exact user command of the history vector
	if (num <= 0 || num > history.size())
	{
		cout << "that is not a valid position in the history" << endl;
		return make_pair("", false);
	}
	//else valid history
	return make_pair(history[history.size() - num], true);
}

/*
PURPOSE: checks the running process list.  if the processes checked are finished running they will be removed from this list
PARAMETERS: runningProcs is a list of processes that should be running
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
		if (finishedProc != 0) //if the process is finished, it will print out some info of it and will be removed from the running process List
		{
			cout << endl << "Process with pid " << finishedProc
				<< " exited with code: " << status << endl;

			runningProcs.erase(it);
			it = runningProcs.begin(); //restart after erase to avoid nullptrs
		}
	}
}
