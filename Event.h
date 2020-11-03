#ifndef EVENT_H
#define EVENT_H

#include <iostream>
#include <string>
using namespace std;

#include "Process.h"

enum class Trans : char
{
  TRANS_TO_READY,
  TRANS_TO_RUNNING,
  TRANS_TO_BLOCKED,
  TRANS_TO_PREEMPT,
  TRANS_TO_DONE
};

string enumToString(Trans);

class Event
{
public:
  // making data member public to simplify the code (anti pattern)
  Process *const process;
  const int timeStamp;
  const Trans transition;

  Event(const int, Process *const, const Trans);
  void log();
};

// TODO: should move the implementation to ~.cpp after making the makefile that compiles ~.cpp
Event::Event(const int ts, Process *const proc, const Trans trans)
    : timeStamp(ts), process(proc), transition(trans)
{
}

void Event::log()
{
  int time = this->timeStamp;
  const Process *proc = this->process;
  int prev = time - proc->state_ts;
  Trans state = this->transition;
  cout << time << " " << proc->id << " " << prev << ": ";
  if (state == Trans::TRANS_TO_DONE)
  {
    cout << "Done" << endl;
    return;
  }
  cout << proc->state << " -> ";
  switch (state)
  {
  case Trans::TRANS_TO_READY:
    cout << "READY cb=" << proc->remain_cb << " rem=" << proc->remainCpuTime << " prio=" << proc->dynamicPriority;
    break;
  case Trans::TRANS_TO_RUNNING:
    cout << "RUNNG cb=" << proc->remain_cb << " rem=" << proc->remainCpuTime << " prio=" << proc->dynamicPriority;
    break;
  case Trans::TRANS_TO_BLOCKED:
    cout << "BLOCK  ib=" << proc->remain_ib << " rem=" << proc->remainCpuTime;
    break;
  case Trans::TRANS_TO_PREEMPT:
    cout << "PREEMPT";
    break;
  default:
    break;
  }
  cout << endl;
  return;
}

std::ostream &operator<<(std::ostream &os, const Event &evt)
{
  os << "timeStamp: " << evt.timeStamp << " | "
     << "process: {" << evt.process << "} | "
     << "transition: " << enumToString(evt.transition);
  return os;
}

std::ostream &operator<<(std::ostream &os, const Trans trans)
{
  os << enumToString(trans);
  return os;
}

string enumToString(Trans trans)
{
  char c = +static_cast<std::underlying_type_t<Trans>>(trans);
  switch (c)
  {
  case 0:
    return "TRANS_TO_READY";
  case 1:
    return "TRANS_TO_RUNNING";
  case 2:
    return "TRANS_TO_BLOCKED";
  case 3:
    return "TRANS_TO_PREEMPT";
  case 4:
    return "TRANS_TO_DONE";
  default:
    return "Error!";
  }
}

#endif