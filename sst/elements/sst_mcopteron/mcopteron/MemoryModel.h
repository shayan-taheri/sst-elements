
#ifndef MEMORYMODEL_H
#define MEMORYMODEL_H

#include "OpteronDefs.h"
#include "Token.h"
#include "Random.h"
//#include <CycleTracker.h>

namespace McOpteron{ //Scoggin: Added a namespace to reduce possible conflicts as library

// Memory operation types
enum class MemOpType : uint8_t {
  UNKNOWN=0,
  MEMLOAD=1, 
  MEMSTORE=2
};

//-------------------------------------------------------------------
/// @brief Memory model class
///
/// Memory model should:
/// - keep track of how many loads and stores are outstanding in order
///   to stall if buffers are full
/// - should sample st2ld hist on a load to see if load is satisfied
///   from store buffer
/// - should use memory hierarchy probabilities to sim memory access
///   and report latency
/// - TODO: currently, memory model is not doing anything with ld-ld
///   and st-st distances, maybe it should. It does keep the mem queue
///   and stalls accordingly if mem ops happen close to each other, but
///   this might not capture the true distribution
//-------------------------------------------------------------------
class MemoryModel
{

  // Various constant costs
  class Cost {
   public:
    static const unsigned int LoadAfterLoad = 1;
    static const unsigned int LoadFromSTB = 2;
    static const unsigned int AverageStoreLatency = 25;
    static const unsigned int StoreAfterStore = 2;
  };

  class Config {
   public:
    static const unsigned int StoreBufferSize = 8;
  };

  /// Memory operation list node type
  struct MemoryOp 
  {
    InstructionCount insnNum;
    Address address;
    unsigned int numBytes;
    CycleCount issueCycle;
    CycleCount satisfiedCycle;
    MemOpType op;
    struct MemoryOp *next;
  };

 public:
  MemoryModel();
  ~MemoryModel();
  void initLatencies(unsigned int latTLB1, unsigned int latTLB2, 
                     unsigned int latL1, unsigned int latL2, 
                     unsigned int latL3, unsigned int latMem);
  void initProbabilities(double pSTBHit, double pL1Miss, double pTLB1Miss, 
                         double pTLB2Miss, double pICMiss, double pITLB1Miss, double pITLB2Miss,
                         double pL2Miss, double pL3Miss);
  void getDataLoadStats(unsigned long long *numLoads,
                        unsigned long long *numSTBHits,
                        unsigned long long *numL1Hits,
                        unsigned long long *numL2Hits,
                        unsigned long long *numL3Hits,
                        unsigned long long *numMemoryHits,
                        unsigned long long *numTLB1Misses,
                        unsigned long long *numTLB2Misses);
  void getInstLoadStats(unsigned long long *numILoads,
                        unsigned long long *numICHits,
                        unsigned long long *numIL2Hits,
                        unsigned long long *numIL3Hits,
                        unsigned long long *numIMemoryHits,
                        unsigned long long *numITLB1Misses,
                        unsigned long long *numITLB2Misses);
  void getStoreStats(unsigned long long *numStores );
  CycleCount serveLoad(CycleCount currentCycle, Address address,
                       unsigned int numBytes, Token *tkn);
  CycleCount serveILoad(CycleCount currentCycle, Address address,
                        unsigned int numBytes, CPIStack *cpiStack);
  CycleCount serveStore(CycleCount currentCycle, Address address,
                        unsigned int numBytes);
  void setRandFunc(RandomFunc_t _R){rand.setrand_function(_R);}
  void setSeedFunc(SeedFunc_t _S){rand.setseed_function(_S);}
 private:
  double genRandomProbability(){return rand.next();}
  Random rand;
  int addToMemoryQ(CycleCount whenSatisfied, MemOpType type);
  unsigned int numberInMemoryQ(MemOpType memOp);
  double purgeMemoryQ(CycleCount upToCycle);

  MemoryOp *memQHead, *memQTail, *lastLoad, *lastStore;
  unsigned int numLoadsInQ, numStoresInQ;
  unsigned int latencyL1, latencyL2, latencyL3, latencyMem, latencyTLB1,
               latencyTLB2;
  double cdfSTBHit, cdfL1Hit, cdfL2Hit, cdfL3Hit, cdfTLB1Hit, cdfTLB2Hit,
         cdfICHit, cdfIL2Hit, cdfIL3Hit, cdfITLB1Hit, cdfITLB2Hit;
  unsigned long long numL1Hits, numL2Hits, numL3Hits, numMemoryHits, 
                numTLB1Misses, numTLB2Misses;
  unsigned long long numICHits, numIL2Hits, numIL3Hits, numIMemoryHits,
                numITLB1Misses, numITLB2Misses;
  unsigned long long numSTBHits, numStores, numLoads, numILoads;
};
}//end namespace McOpteron
#endif
