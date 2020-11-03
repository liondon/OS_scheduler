#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <regex>
using namespace std;

#include "Process.h"
#include "Event.h"

vector<int> createRandArray(const string);
int myrandom(const int, const vector<int> &);
multimap<int, Event *> createEventQ(const string, const vector<int> &, const int);

// TODO: should move the implementation to ~.cpp after making the makefile that compiles ~.cpp
vector<int> createRandArray(const string randFilePath)
{
  vector<int> randArray;
  ifstream randFile;
  string str;

  randFile.open(randFilePath);

  // get the #randNum is this file
  getline(randFile, str);
  const int amount = stoi(str);

  // read in the randNums
  while (getline(randFile, str))
  {
    if (randFile.is_open())
    {
      randArray.emplace_back(stoi(str));
    }
  }
  randFile.close();

  if (amount != randArray.size())
  {
    // TODO: more appropriate error handling
    cout << "Something wrong with the random file." << endl;
    return {-1};
  }

  return randArray;
}

int myrandom(const int burst, const vector<int> &randArray)
{
  static int ofs = 0;
  if (ofs >= randArray.size())
  {
    ofs = 0;
  }
  return 1 + (randArray[ofs++] % burst);
}

multimap<int, Event *> createEventQ(const string inputPath, const vector<int> &randArray, const int maxprio)
{
  // Delimiters are spaces (\s) and/or commas
  regex delimiter("[\\s]+");
  ifstream inputfile;
  multimap<int, Event *> evtQ;
  string str;
  int at = 0, tc = 0, cb = 0, io = 0;

  inputfile.open(inputPath);

  while (getline(inputfile, str))
  {
    if (inputfile.is_open())
    {
      vector<string> tokens(sregex_token_iterator(str.begin(), str.end(), delimiter, -1), {});
      const int timeStamp = stoi(tokens[0]);

      // create a Process obj
      const int staticPrio = myrandom(maxprio, randArray);
      Process *proc = new Process(timeStamp, stoi(tokens[1]), stoi(tokens[2]), stoi(tokens[3]), staticPrio);

      // create a Process-CREATE event obj & put it into event queue
      Event *evt = new Event(timeStamp, proc, Trans::TRANS_TO_READY);
      evtQ.emplace(pair<int, Event *>(timeStamp, evt));
    }
  }
  inputfile.close();

  return evtQ;
}
