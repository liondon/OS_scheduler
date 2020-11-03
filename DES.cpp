#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <limits>
#include <unordered_set>
#include <map>
using namespace std;

#include "Process.h"
#include "Event.h"
#include "Scheduler.h"
#include "Helpers.h"

void Simulation(multimap<int, Event *> &, const vector<int> &, const char, const int, const size_t = 4, bool = false);

int main(int argc, char **argv)
{
  bool verbose = 0;
  char *schedspec = nullptr, sched;
  int quantum = numeric_limits<int>::max(); // i.e. no quantum exist
  int maxprio = 4;
  char *inputPath = nullptr, *randPath = nullptr;
  int index, c;

  opterr = 0;

  while ((c = getopt(argc, argv, "vets:")) != -1)
    switch (c)
    {
    case 'v':
      verbose = 1;
      break;
    case 'e':
    case 't':
      // not implemented
      break;
    case 's':
      schedspec = optarg;
      sscanf(optarg, "%c%d:%d", &sched, &quantum, &maxprio);
      break;
    case '?':
      if (optopt == 's')
        fprintf(stderr, "Option -%c requires an argument.\n", optopt);
      else if (isprint(optopt))
        fprintf(stderr, "Unknown option '-%c'.\n", optopt);
      else
        fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
      return 1;
    default:
      abort();
    }

  // printf("verbose = %d, schedspec = %s, sched = %c, quantum = %d, maxprio = %d\n", verbose, schedspec, sched, quantum, maxprio);

  inputPath = argv[optind];
  randPath = argv[++optind];
  // printf("input file path: %s\n", inputPath);
  // printf("random file path: %s\n", randPath);

  vector<int> randArray = createRandArray(randPath);
  multimap<int, Event *> evtQ = createEventQ(inputPath, randArray, maxprio);
  Simulation(evtQ, randArray, sched, quantum, maxprio, verbose);

  return 0;
}

void Simulation(multimap<int, Event *> &evtQ, const vector<int> &randArray,
                const char sched, const int quantum, const size_t maxprio, bool verbose)
{
  vector<Process *>
      procTable;
  Event *evt;
  Process *CURRENT_RUNNING_PROCESS = nullptr;
  bool CALL_SCHEDULER = false;
  int CURRENT_TIME = 0;
  int CPU_totalIdelTime = 0, CPU_startIdeling_ts = 0;
  int IO_crrentProcCount = 0, IO_totalIdelTime = 0, IO_startIdeling_ts = 0;

  Scheduler *scheduler = nullptr;
  string schedspec;
  switch (sched)
  {
  case 'F':
    scheduler = new FCFS();
    schedspec = "FCFS";
    break;
  case 'L':
    scheduler = new LCFS();
    schedspec = "LCFS";
    break;
  case 'S':
    schedspec = "SRTF";
    scheduler = new SRTF();
    break;
  case 'R':
    schedspec = "RR " + to_string(quantum);
    scheduler = new RR();
    break;
  case 'P':
    schedspec = "PRIO " + to_string(quantum);
    scheduler = new PRIO(maxprio);
    break;
  case 'E':
    schedspec = "PREPRIO " + to_string(quantum);
    scheduler = new PREPRIO(maxprio);
    break;
  default:
    // TODO: make more proper error handlers.
    cout << "Error: Cannot understand the scheduler spec. No Scheduler object created.";
    exit(1);
  }

  while (!evtQ.empty())
  {
    evt = evtQ.extract(evtQ.begin()).mapped();
    // cout << "New Event arriving: " << *evt << endl;

    Process *const proc = evt->process;                  // this is the process the event works on
    CURRENT_TIME = evt->timeStamp;                       // time jumps discretely
    int timeInPrevState = CURRENT_TIME - proc->state_ts; // good for accounting

    switch (evt->transition)
    {
    case Trans::TRANS_TO_DONE:
    {
      // exit from running
      proc->remain_cb -= timeInPrevState;
      proc->remainCpuTime -= timeInPrevState;
      CPU_startIdeling_ts = CURRENT_TIME;
      CURRENT_RUNNING_PROCESS = nullptr;

      if (verbose)
      {
        evt->log();
      }

      proc->updateState(ProcState::DONE, CURRENT_TIME);
      proc->finish_ts = CURRENT_TIME;
      CALL_SCHEDULER = true;

      break;
    }

    case Trans::TRANS_TO_READY:
    {
      // must come from CREATED, BLOCKED or from RUNNING
      switch (proc->state)
      {
      case ProcState::CREATED:
        procTable.emplace_back(proc);
        break;
      case ProcState::BLOCKED:
        proc->dynamicPriority = proc->staticPriority - 1;
        proc->totalIO += proc->remain_ib;
        --IO_crrentProcCount;
        if (IO_crrentProcCount == 0)
        {
          IO_startIdeling_ts = CURRENT_TIME;
        }
        break;
      case ProcState::RUNNING:
        proc->dynamicPriority--;
        // exit from running
        proc->remain_cb -= timeInPrevState;
        proc->remainCpuTime -= timeInPrevState;
        CPU_startIdeling_ts = CURRENT_TIME;
        CURRENT_RUNNING_PROCESS = nullptr;
        break;
      }

      if (verbose)
      {
        evt->log();
      }

      proc->updateState(ProcState::READY, CURRENT_TIME);

      // must add to run queue
      scheduler->add_to_readyQ(proc);
      CALL_SCHEDULER = true;

      break;
    }

    case Trans::TRANS_TO_RUNNING:
    {
      proc->totalWaiting += timeInPrevState;

      if (proc->remain_cb <= 0)
      {
        int cpuBurst = myrandom(proc->cpuBurst, randArray);
        proc->remain_cb = min(cpuBurst, proc->remainCpuTime);
      }
      int actualBurst = min(proc->remain_cb, quantum);
      if (verbose)
      {
        evt->log();
      }

      proc->updateState(ProcState::RUNNING, CURRENT_TIME);
      CPU_totalIdelTime += (CURRENT_TIME - CPU_startIdeling_ts);

      // CREATE NEXT EVENT
      int timeStamp = CURRENT_TIME + actualBurst;

      // create event for DONE
      if ((proc->remainCpuTime - actualBurst) == 0)
      {
        evtQ.emplace(pair<int, Event *>(timeStamp, new Event(timeStamp, proc, Trans::TRANS_TO_DONE)));
        break;
      }

      // create event for blocking
      if ((proc->remain_cb - actualBurst) == 0)
      {
        evtQ.emplace(pair<int, Event *>(timeStamp, new Event(timeStamp, proc, Trans::TRANS_TO_BLOCKED)));
        break;
      }

      // create event for quantum expiration
      evtQ.emplace(pair<int, Event *>(timeStamp, new Event(timeStamp, proc, Trans::TRANS_TO_READY)));
      break;
    }

    case Trans::TRANS_TO_BLOCKED:
    {
      // exit from running
      proc->remain_cb -= timeInPrevState;
      proc->remainCpuTime -= timeInPrevState;
      CPU_startIdeling_ts = CURRENT_TIME;
      CURRENT_RUNNING_PROCESS = nullptr;

      int ioBurst = myrandom(proc->ioBurst, randArray);
      proc->remain_ib = ioBurst;
      if (verbose)
      {
        evt->log();
      }

      proc->updateState(ProcState::BLOCKED, CURRENT_TIME);

      IO_crrentProcCount++;
      if (IO_crrentProcCount == 1)
      {
        IO_totalIdelTime += (CURRENT_TIME - IO_startIdeling_ts);
      }
      CALL_SCHEDULER = true;

      //create an event for when process becomes READY again
      int timeStamp = CURRENT_TIME + ioBurst;
      evtQ.emplace(pair<int, Event *>(timeStamp, new Event(timeStamp, proc, Trans::TRANS_TO_READY)));

      break;
    }

    case Trans::TRANS_TO_PREEMPT:
    {
      // exit from running
      proc->dynamicPriority--; // dynamic priority decreases even fro preemption
      proc->remain_cb -= timeInPrevState;
      proc->remainCpuTime -= timeInPrevState;
      CPU_startIdeling_ts = CURRENT_TIME;
      CURRENT_RUNNING_PROCESS = nullptr;

      if (verbose)
      {
        evt->log();
      }

      proc->updateState(ProcState::READY, CURRENT_TIME);

      // remove the future event for the process
      // TODO: a nother map(index) of Process_id:Event* might help?
      for (auto iter = evtQ.begin(); iter != evtQ.end(); iter++)
      {
        if (iter->second->process->id == proc->id)
        {
          delete iter->second;
          evtQ.erase(iter);
          break; // there is only one future event for the process
        }
      }

      // add to runqueue (no event is generated)
      scheduler->add_to_readyQ(proc);
      CALL_SCHEDULER = true;

      break;
    }
    }

    //remove current event object from Memory
    delete evt;
    evt = nullptr;

    if (CALL_SCHEDULER)
    {
      // create preemption events if needed
      if (CURRENT_RUNNING_PROCESS != nullptr && scheduler->test_preempt(CURRENT_RUNNING_PROCESS, proc, CURRENT_TIME, evtQ))
      {
        // create event for preemption
        evt = new Event(CURRENT_TIME, CURRENT_RUNNING_PROCESS, Trans::TRANS_TO_PREEMPT);
        evtQ.emplace(pair<int, Event *>(CURRENT_TIME, evt));
        CURRENT_RUNNING_PROCESS == nullptr;
      }

      if ((*evtQ.begin()).first == CURRENT_TIME)
      {
        continue; // keep process next event from Event queue
      }

      CALL_SCHEDULER = false;

      if (CURRENT_RUNNING_PROCESS == nullptr) // no process running or preemption occurs
      {
        // cout << "Calling Scheduler..." << endl;
        CURRENT_RUNNING_PROCESS = scheduler->get_next_process();
        if (CURRENT_RUNNING_PROCESS == nullptr)
        {
          // cout << "readyQ is empty..." << endl;
          continue;
        }

        // create event to make process runnable for same time.
        evt = new Event(CURRENT_TIME, CURRENT_RUNNING_PROCESS, Trans::TRANS_TO_RUNNING);
        evtQ.emplace(pair<int, Event *>(CURRENT_TIME, evt));
      }
    }
  }

  delete scheduler;

  IO_totalIdelTime += (CURRENT_TIME - IO_startIdeling_ts);

  // print schedspec
  cout << schedspec << endl;

  // print statistics of each processes
  double procCount = static_cast<double>(procTable.size());
  int totalTurnAround = 0, totalWaitTime = 0;
  for (Process *proc : procTable)
  {
    totalTurnAround += (proc->finish_ts - proc->arrival_ts);
    totalWaitTime += proc->totalWaiting;
    cout << proc << endl;
    delete proc;
  }

  // printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n",
  cout << "SUM: " << CURRENT_TIME << " "
       << fixed << setprecision(2)
       << (CURRENT_TIME - CPU_totalIdelTime) / (CURRENT_TIME / 100.0) << " " // CPU utilization
       << (CURRENT_TIME - IO_totalIdelTime) / (CURRENT_TIME / 100.0) << " "  // IO utilization
       << totalTurnAround / procCount << " "
       << totalWaitTime / procCount << " "
       << setprecision(3)
       << procCount / (CURRENT_TIME / 100.0) << endl;

  return;
}
