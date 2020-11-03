#include <iostream>
#include <vector>
#include <bitset>
#include <climits>
#include <cmath>
using namespace std;

#define BITS_PER_ULL (sizeof(unsigned long long) * CHAR_BIT)

class Bitmap
{
  const size_t maxprio;

public:
  // making data member public to simplify the code (anti pattern)
  const int numOfWords;
  vector<unsigned long long> words;

  Bitmap(size_t);
  void setBit(size_t);
  void unsetBit(size_t);
  int highestPrio();
};

// TODO: should move the implementation to ~.cpp after making the makefile that compiles ~.cpp
Bitmap::Bitmap(size_t maxprio)
    : maxprio(maxprio), numOfWords((maxprio % BITS_PER_ULL) ? 1 + (maxprio / BITS_PER_ULL) : (maxprio / BITS_PER_ULL)),
      words(numOfWords, 0)
{
}

void Bitmap::setBit(size_t dynamic_prio)
{
  if (dynamic_prio >= maxprio)
  {
    cout << "Error: dynamic_prio >= maxprio." << endl;
    return;
  }
  size_t ofs = dynamic_prio / BITS_PER_ULL;
  size_t idx = dynamic_prio % BITS_PER_ULL;
  unsigned long long mask = 1;
  mask <<= idx;

  words[ofs] |= mask;
  return;
}

void Bitmap::unsetBit(size_t dynamic_prio)
{
  if (dynamic_prio >= maxprio)
  {
    cout << "Error: dynamic_prio >= maxprio." << endl;
    return;
  }
  size_t ofs = dynamic_prio / BITS_PER_ULL;
  size_t idx = dynamic_prio % BITS_PER_ULL;
  unsigned long long mask = 1;
  mask <<= idx;
  mask = ~mask;

  words[ofs] &= mask;
  return;
}

int Bitmap::highestPrio()
{
  for (size_t i = numOfWords; i > 0; i--)
  {
    if (words[i - 1] != 0)
    {
      return (BITS_PER_ULL - __builtin_clzll(words[i - 1]) - 1 + BITS_PER_ULL * (i - 1));
      break;
    }
  }
  return -1;
}

std::ostream &operator<<(std::ostream &os, const Bitmap *bitmap)
{
  const int numOfWords = bitmap->numOfWords;
  vector<bitset<BITS_PER_ULL>> b;
  for (int i = 0; i < numOfWords; i++)
  {
    b.emplace_back(bitmap->words[i]);
  }
  for (size_t i = numOfWords; i > 0; i--)
  {
    os << b[i - 1];
  }
  return os;
}

// int main()
// {
//   // for testing
//   const size_t maxprio = 140;
//   Bitmap bitmap(maxprio);
//   cout << &bitmap << endl;
//   cout << bitmap.highestPrio() << endl;

//   bitmap.setBit(0);
//   cout << &bitmap << endl;
//   cout << bitmap.highestPrio() << endl;

//   bitmap.setBit(64);
//   cout << &bitmap << endl;
//   cout << bitmap.highestPrio() << endl;

//   bitmap.setBit(128);
//   cout << &bitmap << endl;
//   cout << bitmap.highestPrio() << endl;

//   bitmap.setBit(139);
//   cout << &bitmap << endl;
//   cout << bitmap.highestPrio() << endl;

//   bitmap.setBit(140);
//   cout << &bitmap << endl;
//   cout << bitmap.highestPrio() << endl;

//   bitmap.unsetBit(139);
//   cout << &bitmap << endl;
//   cout << bitmap.highestPrio() << endl;

//   bitmap.unsetBit(128);
//   cout << &bitmap << endl;
//   cout << bitmap.highestPrio() << endl;

//   bitmap.unsetBit(80);
//   cout << &bitmap << endl;
//   cout << bitmap.highestPrio() << endl;

//   bitmap.setBit(64);
//   cout << &bitmap << endl;
//   cout << bitmap.highestPrio() << endl;

//   return 0;
// }