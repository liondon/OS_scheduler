#ifndef PROCESS_H
#define PROCESS_H

#include <iostream>
#include <iomanip>
#include <string>
using namespace std;

enum class ProcState : char
{
  CREATED,
  READY,
  RUNNING,
  BLOCKED,
  PREEMPTION,
  DONE
};

string enumToString(ProcState);

class Process
{
public:
  // making data member public to simplify the code (anti pattern)
  inline static int i;
  const int id, arrival_ts, totalCpuTime, cpuBurst, ioBurst, staticPriority;
  int remainCpuTime, dynamicPriority, state_ts, remain_cb, remain_ib;
  ProcState state;
  int finish_ts, totalIO, totalWaiting;
  // turnAround = finish_ts - arrival_ts

  Process(const int, const int, const int, const int, const int);
  void updateState(const ProcState, const int);
};

// TODO: should move the implementation to ~.cpp after making the makefile that compiles ~.cpp
Process::Process(const int at, const int ct, const int cb,
                 const int ib, const int staticPrio)
    : id(i++), arrival_ts(at), totalCpuTime(ct), cpuBurst(cb), ioBurst(ib), staticPriority(staticPrio),
      remainCpuTime(totalCpuTime), dynamicPriority(staticPriority - 1), state_ts(arrival_ts),
      remain_cb(0), remain_ib(0), state(ProcState::CREATED), finish_ts(0), totalIO(0), totalWaiting(0)
{
}

void Process::updateState(const ProcState state, const int timeStamp)
{
  this->state = state;
  this->state_ts = timeStamp;
  return;
}

std::ostream &operator<<(std::ostream &os, const Process *proc)
{
  // printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n")
  // note " %4d %4d" is not equivalent to "%5d%5d"
  os << setfill('0') << setw(4) << proc->id << ": " << setfill(' ')
     << setw(4) << proc->arrival_ts << " "
     << setw(4) << proc->totalCpuTime << " "
     << setw(4) << proc->cpuBurst << " "
     << setw(4) << proc->ioBurst << " "
     << setw(1) << proc->staticPriority << " | "
     << setw(5) << proc->finish_ts << " "
     << setw(5) << (proc->finish_ts - proc->arrival_ts) << " "
     << setw(5) << proc->totalIO << " "
     << setw(5) << proc->totalWaiting;
  return os;
}

std::ostream &operator<<(std::ostream &os, const ProcState state)
{
  os << enumToString(state);
  return os;
}

string enumToString(ProcState state)
{
  char c = +static_cast<std::underlying_type_t<ProcState>>(state);
  switch (c)
  {
  case 0:
    return "CREATED";
  case 1:
    return "READY";
  case 2:
    return "RUNNG";
  case 3:
    return "BLOCK";
  case 4:
    return "READY"; //PREEMPTION
  case 5:
    return "DONE";
  default:
    return "Error!";
  }
}

#endif