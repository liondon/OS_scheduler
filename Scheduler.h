#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <map>
#include <deque>
#include <vector>
using namespace std;

#include "Process.h"
#include "Event.h"
#include "Bitmap.h"

class Scheduler
{
public:
  virtual void add_to_readyQ(Process *) = 0;
  virtual Process *get_next_process() = 0;
  virtual bool test_preempt(Process *, Process *, int, multimap<int, Event *>) = 0; // only for PREPRIO

protected:
  // we can define data members that are for all derived class here.
};

// TODO: Refactory to reduce duplicate codes. HOW?

//////////////// PREEMPTIVE PRIORITY ////////////////////
class PREPRIO : public Scheduler
{
public:
  PREPRIO(const size_t);
  ~PREPRIO();
  void add_to_readyQ(Process *) override;
  Process *get_next_process() override;
  bool test_preempt(Process *, Process *, int, multimap<int, Event *>) override;

private:
  vector<deque<Process *> *> q1, q2;
  vector<deque<Process *> *> *activeQ_ptr, *expiredQ_ptr;
  Bitmap q1Bmap, q2Bmap;
  Bitmap *activeBmap_ptr, *expiredBmap_ptr;
};

PREPRIO::PREPRIO(const size_t maxprio)
    : q1(maxprio, nullptr), q2(maxprio, nullptr),
      q1Bmap(maxprio), q2Bmap(maxprio),
      activeQ_ptr(&q1), expiredQ_ptr(&q2),
      activeBmap_ptr(&q1Bmap), expiredBmap_ptr(&q2Bmap)
{
}

PREPRIO::~PREPRIO()
{
  for (deque<Process *> *readyQ_ptr : *activeQ_ptr)
  {
    if (readyQ_ptr != nullptr)
    {
      delete readyQ_ptr;
    }
  }
  for (deque<Process *> *readyQ_ptr : *expiredQ_ptr)
  {
    if (readyQ_ptr != nullptr)
    {
      delete readyQ_ptr;
    }
  }
}

bool PREPRIO::test_preempt(Process *currentProc, Process *proc,
                           int curtime, multimap<int, Event *> evtQ)
{
  bool existPendingEvtForCrrntProc = false;

  auto evts_range = evtQ.equal_range(curtime);
  for (auto iter = evts_range.first; iter != evts_range.second; iter++)
  {
    if (iter->second->process->id == currentProc->id)
    {
      existPendingEvtForCrrntProc = true;
      break;
    }
  }

  if (!existPendingEvtForCrrntProc &&
      proc->dynamicPriority > currentProc->dynamicPriority)
  {
    return true;
  }
  return false;
}

void PREPRIO::add_to_readyQ(Process *proc)
{
  deque<Process *> *readyQ_ptr;
  Bitmap *bmap_ptr;

  if (proc->dynamicPriority < 0)
  {
    // reset and enter into expiredQ
    proc->dynamicPriority = proc->staticPriority - 1;
    if ((*expiredQ_ptr)[proc->dynamicPriority] == nullptr)
    {
      (*expiredQ_ptr)[proc->dynamicPriority] = new deque<Process *>;
    }
    readyQ_ptr = (*expiredQ_ptr)[proc->dynamicPriority];
    bmap_ptr = expiredBmap_ptr;
  }
  else
  {
    // add to activeQ
    if ((*activeQ_ptr)[proc->dynamicPriority] == nullptr)
    {
      (*activeQ_ptr)[proc->dynamicPriority] = new deque<Process *>;
    }
    readyQ_ptr = (*activeQ_ptr)[proc->dynamicPriority];
    bmap_ptr = activeBmap_ptr;
  }

  if (readyQ_ptr->empty())
  {
    bmap_ptr->setBit(static_cast<size_t>(proc->dynamicPriority));
  }
  readyQ_ptr->emplace_back(proc);
  return;
}

Process *PREPRIO::get_next_process()
{
  deque<Process *> *readyQ_ptr;
  Process *proc;

  int highestPrio = activeBmap_ptr->highestPrio();

  if (highestPrio == -1)
  {
    // activeQ is empty: swap(activeQ, expiredQ)
    swap(activeQ_ptr, expiredQ_ptr);
    swap(activeBmap_ptr, expiredBmap_ptr);
  }

  highestPrio = activeBmap_ptr->highestPrio();
  if (highestPrio == -1)
  {
    // activeQ is still empty, there is no ready process
    return nullptr;
  }
  // activeQ is not empty: pick activeQ[highest prio].front()

  readyQ_ptr = (*activeQ_ptr)[highestPrio];
  proc = readyQ_ptr->empty() ? nullptr : readyQ_ptr->front();

  if (!readyQ_ptr->empty())
  {
    readyQ_ptr->pop_front();
  }
  if (readyQ_ptr->empty())
  {
    activeBmap_ptr->unsetBit(static_cast<size_t>(proc->dynamicPriority));
  }
  return proc;
}
/////////////////////////////////////////////////////////

///////////////////// PRIORITY SCHEDULER/////////////////
class PRIO : public Scheduler
{
public:
  PRIO(const size_t);
  ~PRIO();
  void add_to_readyQ(Process *) override;
  Process *get_next_process() override;
  bool test_preempt(Process *, Process *, int, multimap<int, Event *>) override { return false; };

private:
  vector<deque<Process *> *> q1, q2;
  vector<deque<Process *> *> *activeQ_ptr, *expiredQ_ptr;
  Bitmap q1Bmap, q2Bmap;
  Bitmap *activeBmap_ptr, *expiredBmap_ptr;
};

PRIO::PRIO(const size_t maxprio)
    : q1(maxprio, nullptr), q2(maxprio, nullptr),
      q1Bmap(maxprio), q2Bmap(maxprio),
      activeQ_ptr(&q1), expiredQ_ptr(&q2),
      activeBmap_ptr(&q1Bmap), expiredBmap_ptr(&q2Bmap)
{
}

PRIO::~PRIO()
{
  for (deque<Process *> *readyQ_ptr : *activeQ_ptr)
  {
    if (readyQ_ptr != nullptr)
    {
      delete readyQ_ptr;
    }
  }
  for (deque<Process *> *readyQ_ptr : *expiredQ_ptr)
  {
    if (readyQ_ptr != nullptr)
    {
      delete readyQ_ptr;
    }
  }
}

void PRIO::add_to_readyQ(Process *proc)
{
  deque<Process *> *readyQ_ptr;
  Bitmap *bmap_ptr;

  if (proc->dynamicPriority < 0)
  {
    // reset and enter into expiredQ
    proc->dynamicPriority = proc->staticPriority - 1;
    if ((*expiredQ_ptr)[proc->dynamicPriority] == nullptr)
    {
      (*expiredQ_ptr)[proc->dynamicPriority] = new deque<Process *>;
    }
    readyQ_ptr = (*expiredQ_ptr)[proc->dynamicPriority];
    bmap_ptr = expiredBmap_ptr;
  }
  else
  {
    // add to activeQ
    if ((*activeQ_ptr)[proc->dynamicPriority] == nullptr)
    {
      (*activeQ_ptr)[proc->dynamicPriority] = new deque<Process *>;
    }
    readyQ_ptr = (*activeQ_ptr)[proc->dynamicPriority];
    bmap_ptr = activeBmap_ptr;
  }

  if (readyQ_ptr->empty())
  {
    bmap_ptr->setBit(static_cast<size_t>(proc->dynamicPriority));
  }
  readyQ_ptr->emplace_back(proc);
  return;
}

Process *PRIO::get_next_process()
{
  deque<Process *> *readyQ_ptr;
  Process *proc;
  int highestPrio = activeBmap_ptr->highestPrio();

  if (highestPrio == -1)
  {
    // activeQ is empty: swap(activeQ, expiredQ)
    swap(activeQ_ptr, expiredQ_ptr);
    swap(activeBmap_ptr, expiredBmap_ptr);
  }

  highestPrio = activeBmap_ptr->highestPrio();
  if (highestPrio == -1)
  {
    // activeQ is still empty, there is no ready process
    return nullptr;
  }

  // activeQ is not empty: pick activeQ[highest prio].front()
  readyQ_ptr = (*activeQ_ptr)[highestPrio];
  proc = readyQ_ptr->empty() ? nullptr : readyQ_ptr->front();

  if (!readyQ_ptr->empty())
  {
    readyQ_ptr->pop_front();
  }
  if (readyQ_ptr->empty())
  {
    activeBmap_ptr->unsetBit(static_cast<size_t>(proc->dynamicPriority));
  }
  return proc;
}
/////////////////////////////////////////////////////////

///////////////////// Round Robin ///////////////////////
class RR : public Scheduler
{
public:
  void add_to_readyQ(Process *) override;
  Process *get_next_process() override;
  bool test_preempt(Process *, Process *, int, multimap<int, Event *>) override { return false; };

private:
  deque<Process *> readyQ;
};

void RR::add_to_readyQ(Process *proc)
{
  if (proc->dynamicPriority < 0)
  {
    proc->dynamicPriority = proc->staticPriority - 1;
  }
  readyQ.emplace_back(proc);
  return;
}

Process *RR::get_next_process()
{
  Process *proc = readyQ.empty() ? nullptr : readyQ.front();
  if (!readyQ.empty())
  {
    readyQ.pop_front();
  }
  return proc;
}
/////////////////////////////////////////////////////////

///////////////////// S R T F ///////////////////////////
class SRTF : public Scheduler
{
public:
  void add_to_readyQ(Process *) override;
  Process *get_next_process() override;
  bool test_preempt(Process *, Process *, int, multimap<int, Event *>) override { return false; };

private:
  multimap<int, Process *> readyQ;
};

void SRTF::add_to_readyQ(Process *proc)
{
  if (proc->dynamicPriority < 0)
  {
    proc->dynamicPriority = proc->staticPriority - 1;
  }
  readyQ.emplace(pair<int, Process *>(proc->remainCpuTime, proc));
  return;
}

Process *SRTF::get_next_process()
{
  Process *proc = readyQ.empty() ? nullptr : (*readyQ.begin()).second;
  if (!readyQ.empty())
  {
    readyQ.extract(readyQ.begin());
  }
  return proc;
}
/////////////////////////////////////////////////////////

///////////////////// L C F S ///////////////////////////
class LCFS : public Scheduler
{
public:
  void add_to_readyQ(Process *) override;
  Process *get_next_process() override;
  bool test_preempt(Process *, Process *, int, multimap<int, Event *>) override { return false; };

private:
  deque<Process *> readyQ;
};

void LCFS::add_to_readyQ(Process *proc)
{
  if (proc->dynamicPriority < 0)
  {
    proc->dynamicPriority = proc->staticPriority - 1;
  }
  readyQ.emplace_back(proc);
  return;
}

Process *LCFS::get_next_process()
{
  Process *proc = readyQ.empty() ? nullptr : readyQ.back();
  if (!readyQ.empty())
  {
    readyQ.pop_back();
  }
  return proc;
}
/////////////////////////////////////////////////////////

///////////////////// F C F S ///////////////////////////
class FCFS : public Scheduler
{
public:
  void add_to_readyQ(Process *) override;
  Process *get_next_process() override;
  bool test_preempt(Process *, Process *, int, multimap<int, Event *>) override { return false; };

private:
  deque<Process *> readyQ;
};

void FCFS::add_to_readyQ(Process *proc)
{
  if (proc->dynamicPriority < 0)
  {
    proc->dynamicPriority = proc->staticPriority - 1;
  }
  readyQ.emplace_back(proc);
  return;
}

Process *FCFS::get_next_process()
{
  Process *proc = readyQ.empty() ? nullptr : readyQ.front();
  if (!readyQ.empty())
  {
    readyQ.pop_front();
  }
  return proc;
}
/////////////////////////////////////////////////////////

#endif