#include "Core/SliceProgress.h"
#include <cassert>
#include <iostream>
#include <queue>
#include <set>
#include <tuple>
using namespace Wroclaw;
int main() {
 Progress p;
 assert(p.Valid() && p.Current()==0 && !p.Finished());
 assert(!p.Apply(Event::ReachSafe) && !p.Apply(Event::TakeKey));
 assert(!p.Apply(Event::UnlockPhone) && !p.Apply(Event::RestorePower));
 // Discover clues and tools out of order, then complete the authored sequence.
 assert(p.Apply(Event::ReadPhoneNote));assert(p.Apply(Event::FindFuse));
 assert(p.Apply(Event::FindCharger));assert(p.Apply(Event::Look));
 assert(p.Apply(Event::FindPhone));assert(p.Apply(Event::UnlockPhone));
 assert(!p.Apply(Event::RestorePower));assert(p.Apply(Event::ReadMessage));
 assert(p.Apply(Event::RestorePower));assert(!p.Has(Item::Fuse));
 assert(!p.Apply(Event::FindFuse));assert(!p.Apply(Event::OpenCabinet));
 assert(p.Apply(Event::RevealCode));assert(p.Apply(Event::OpenCabinet));
 Progress puzzleCheckpoint=p;
 assert(p.Apply(Event::TakeKey));assert(p.Apply(Event::ExitApartment));
 Progress doorCheckpoint=p;
 assert(p.Apply(Event::GroundFloor));assert(p.Apply(Event::Street));
 Progress streetCheckpoint=p;
 assert(!p.Apply(Event::LosePursuit));assert(!p.Apply(Event::ReachSafe));
 p.pursuitStarted=true;
 assert(p.Apply(Event::LosePursuit));assert(p.Apply(Event::ReachSafe));
 assert(p.Finished() && p.Valid());
 // Reload snapshots and verify required keys and puzzle side effects remain coherent.
 p=puzzleCheckpoint;assert(p.Current()==7 && !p.Has(Item::ExitKey));assert(p.Apply(Event::TakeKey));
 p=doorCheckpoint;assert(p.Current()==9 && p.Has(Item::ExitKey));
 p=streetCheckpoint;assert(p.Current()==11 && !p.pursuitStarted);
 // Reject corrupt/future state rather than soft-locking a loaded run.
 p.objectives=1u<<20;assert(!p.Valid());
 p={};p.objectives=4;assert(!p.Valid());
 p={};p.items=1u<<30;assert(!p.Valid());
 p=streetCheckpoint;p.items=0;assert(!p.Valid());
 // Exhaust all reachable event permutations, including repeated and premature interactions.
 using Key=std::tuple<uint32_t,uint32_t,bool,bool>;
 std::set<Key> seen;
 std::queue<Progress> pending;pending.push({});
 bool reachedEnd=false;
 while(!pending.empty()) {
  auto state=pending.front();pending.pop();
  if(!seen.emplace(state.objectives,state.items,state.phoneUnlocked,state.pursuitStarted).second) continue;
  assert(state.Valid());reachedEnd|=state.Finished();
  for(int i=0;i<=static_cast<int>(Event::ReachSafe);++i) {
   auto next=state;next.Apply(static_cast<Event>(i));
   assert(next.Valid());assert((next.objectives & state.objectives)==state.objectives);
   pending.push(next);
  }
  if(state.Complete(10) && !state.pursuitStarted) {state.pursuitStarted=true;pending.push(state);}
 }
 assert(reachedEnd);
 std::cout << "PASS: authored route, checkpoint recovery, corrupt states and " << seen.size() << " reachable progression states\n";
}
