// Fastlib.cpp : test program by Richard Berg
//

#include "fastlib.h"

using namespace std;

int main(int argc, char* argv[])
{
  int method;

  cout << "Which routine would you like to test?\n";
  cout << "[1] <stdlib.h>         [4] Pentium II\n";
  cout << "[2] K6-2 or K6-3       [5] Pentium III\n";
  cout << "[3] Athlon             [6] Pentium IV\n";  
  cout << "-> ";
  cin >> method;

  Fastlib * fl;
  switch(method)
  {
  case 1:
    fl = new P1Lib; break;
  case 2:
    fl = new K6Lib; break;
  case 3:
    fl = new K7Lib; break;
  case 4:
    fl = new P2Lib; break;
  case 5:
    fl = new P3Lib; break;
  case 6:
    fl = new P4Lib; break;
  }

  UL *beforeCpy = new UL;
  UL *afterCpy = new UL;
  UL *beforeSet = new UL;
  UL *afterSet = new UL;
  UL *freq = new UL;
  QueryPerformanceFrequency( (LARGE_INTEGER*)freq );
  
  unsigned numTimes = 30, numBytes = 10000000;
  double speedCpy[6];
  double speedSet[6];
  size_t iter=5;

  char * src = new char[numBytes];
  char * dst = new char[numBytes];
  
  for (; numBytes >= 100; numBytes /= 10)
  {
    // fMemset test
    QueryPerformanceCounter( (LARGE_INTEGER*)beforeSet );
    for (unsigned i=0; i<numTimes; ++i)  
      fl->fMemset(src, numTimes, numBytes);  // numTimes changes, so compiler can't cheat
    QueryPerformanceCounter( (LARGE_INTEGER*)afterSet );
    
    speedSet[iter] = (double)numBytes * numTimes /  // total bytes copied, divide by...
                     ((double)(*afterSet - *beforeSet) / *freq * 1000000);  // time
    
    // fMemcpy test
    QueryPerformanceCounter( (LARGE_INTEGER*)beforeCpy );
    for (unsigned j=0; j<numTimes; ++j)  
      fl->fMemcpy(dst, src, numBytes);
    QueryPerformanceCounter( (LARGE_INTEGER*)afterCpy );
    
    speedCpy[iter--] = (double)numBytes * numTimes /  // total bytes copied, divide by...
                       ((double)(*afterCpy - *beforeCpy) / *freq * 1000000);  // time
    
    
    
    numTimes *= 3;  // I would run all the tests @ 200+ iterations -- doesn't take that
                    // long -- but you start getting weird OS caching results on the 10MB test
                    // (e.g. 10GB/s), and you don't need too many iterations on the big tests
                    // anyway, so I have it scale up by a seems-random-but-works-well value
                    // (yes, 10x would be logical, but run the 100b test that many times and an
                    // ever-more-likely preemption will totally screw up the stats)
  }
    
  cout << fixed << setprecision(1) << setiosflags(ios::left);
  
  cout << "                               [Note: 1MB = 1000KB = 1,000,000b]\n";
  cout << "  Block size:             100b    1KB     10KB    100KB   1MB     10MB\n";
  cout << "----------------------------------------------------------------------------------\n";
  cout << "  Throughput (MB/s):     "
       << setw(8) << speedCpy[0] << setw(8) << speedCpy[1] << setw(8) << speedCpy[2] << setw(8) 
       << speedCpy[3] << setw(8) << speedCpy[4] << setw(8) << speedCpy[5] 
       << "   fMemcpy" << endl;
  cout << "                         "
       << setw(8) << speedSet[0] << setw(8) << speedSet[1] << setw(8) << speedSet[2] << setw(8) 
       << speedSet[3] << setw(8) << speedSet[4] << setw(8) << speedSet[5] 
       << "   fMemset" << endl;

  return 0;
}

