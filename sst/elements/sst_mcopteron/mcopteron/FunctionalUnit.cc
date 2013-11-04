
#include <stdio.h>

#include "FunctionalUnit.h"

namespace McOpteron{ //Scoggin: Added a namespace to reduce possible conflicts as library
/// @brief Constructor
///
FunctionalUnit::FunctionalUnit(FunctionalUnitType _type, const char *_name, unsigned int _id)
{
   occupiedUntilCycle = 0;
   numFreeCycles = numOccupiedCycles = latestCycle = 0;
   type = _type;
   name = _name;
   id = _id;
   next = 0;
   occupied = false;
}


/// @brief Destructor
///
FunctionalUnit::~FunctionalUnit()
{
   fprintf(stdout, "FU%d %s: occupied-cycles: %llu  duty cycle: %g\n",
           id, name, numOccupiedCycles, dutyCycle());
}


/// @brief Attach list link
///
void FunctionalUnit::setNext(FunctionalUnit *other)
{
   next = other;
}


/// @brief Retrieve list link
///
FunctionalUnit* FunctionalUnit::getNext()
{
   return next;
}


/// @brief Retrieve unit type
FunctionalUnitType FunctionalUnit::getType()
{
   return type;
}


/// @brief Get cycle this unit is occupied until (inclusive)
///
CycleCount FunctionalUnit::occupiedUntil(CycleCount atCycle)
{
   return occupiedUntilCycle;
}


/// @brief Assign instruction to occupy this unit for a period of time
///
int FunctionalUnit::occupy(CycleCount atCycle, CycleCount numCycles)
{
   // assumes atCycle >= occupiedUntilCycle
   if (Debug>1) fprintf(stderr, "FU%d %s: occupy from %llu length %llu\n", id, name, 
                      atCycle, numCycles);
   numFreeCycles += atCycle - occupiedUntilCycle;
   numOccupiedCycles += numCycles;
   occupiedUntilCycle = atCycle + numCycles - 1; // inclusive, so -1
   occupied = true;
   return 0;
}


/// @brief Update occupied status
///
/// For an occupied functional unit, this finishes the
/// occupation if the cycle count is reached, and tells 
/// the instruction that it is executing (because of pipelining
/// it may not actually be done yet).
///
int FunctionalUnit::updateStatus(CycleCount currentCycle)
{
   if (currentCycle > occupiedUntilCycle && occupied) {
      occupied = false;
   }
   latestCycle = currentCycle;
   if (Debug>1) fprintf(stderr, "FU%d %s: update status: %s\n", id, name,
              occupied? "busy":"free");
   return 0; 
}


/// @brief Check if unit is available right now
///
bool FunctionalUnit::isAvailable(CycleCount atCycle)
{
   if (occupiedUntilCycle < atCycle)
      return true;
   else
      return false;
}


void FunctionalUnit::flush(CycleCount atCycle)
{
   if (atCycle > occupiedUntilCycle && occupied) {
      occupiedUntilCycle = atCycle;
      occupied = false;
   }
   occupied = false;
   latestCycle = atCycle;
}


/// @brief Report duty cycle of unit
///
double FunctionalUnit::dutyCycle()
{
   return (double) numOccupiedCycles / (latestCycle);
}

}//End namespace McOpteron
