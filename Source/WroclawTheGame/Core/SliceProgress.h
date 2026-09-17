#pragma once
#include "Content/ChapterCatalog.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <set>
namespace Wroclaw {
enum class Result { Applied, AlreadyDone, Locked, MissingClue, Wrong, Cooldown, Invalid, Unsafe };
enum Achievement : uint32_t { FirstSteps=1, EscapeArtist=2, Unseen=4, Detective=8, Pacifist=16, QuickThinking=32 };
struct LockState { int failures=0; double until=0; };
struct Progress {
 static constexpr int Version=2;
 int variant=0, kills=0, medkitsUsed=0, distractionsUsed=0;
 double elapsed=0, questSince=0;
 bool courtyardDetected=false, chaseStarted=false, garageUnderThreat=false;
 std::vector<std::string> history;
 std::set<std::string> completed, evidence, neutralized;
 std::map<std::string,int> inventory;
 std::map<std::string,LockState> locks;
 explicit Progress(int Variant=0):variant(Variant) {}
 static const ActionDef* Find(const std::string& id) {
  for(const auto& a:Catalog()) if(a.id==id) return &a;
  return nullptr;
 }
 bool Done(const std::string& id) const {return completed.count(id)!=0;}
 bool Has(const std::string& id) const {auto i=inventory.find(id);return i!=inventory.end() && i->second>0;}
 bool All(const std::vector<std::string>& ids) const {for(const auto& id:ids) if(!Done(id)) return false;return true;}
 bool Any(const std::vector<std::string>& ids) const {for(const auto& id:ids) if(Done(id)) return true;return ids.empty();}
 int Current() const {
  int i=0;for(const auto& q:Quests()){if(!All(q.all) || !Any(q.any)) return i;++i;}return i;
 }
 bool Finished() const {return Done("blockade_map") && Current()==static_cast<int>(Quests().size());}
 bool Chapter2Unlocked() const {return Finished();}
 int HintLevel() const {double age=elapsed-questSince;return age>=300?3:(age>=180?2:(age>=90?1:0));}
 void Tick(double dt) {if(std::isfinite(dt) && dt>0 && dt<5 && !Finished()) elapsed+=dt;}
 bool CanDo(const ActionDef& a) const {
  if(!All(a.prerequisites) || !Any(a.anyOf)) return false;
  for(const auto& item:a.items) if(!Has(item)) return false;
  if(!a.choice.empty()) for(const auto& other:Catalog())
   if(other.choice==a.choice && other.id!=a.id && Done(other.id)) return false;
  return true;
 }
 std::string Answer(const ActionDef& a) const {
  if(a.answer=="@variant") return variant>=0 && variant<static_cast<int>(LightVariants().size())?LightVariants()[variant]:"";
  return a.answer;
 }
 Result Apply(const std::string& id,bool threat=false) {
  const auto* a=Find(id);if(!a) return Result::Invalid;
  if(Done(id)) return Result::AlreadyDone;
  if(Finished()) return Result::Locked;
  if(!CanDo(*a)) return Result::Locked;
  if(!All(a->clues)) return Result::MissingClue;
  if((id=="workshop" || id=="lost_pursuit" || id=="blockade_map") && threat) return Result::Unsafe;
  if(id=="lost_pursuit" && !chaseStarted) return Result::Locked;
  const int previous=Current();
  completed.insert(id); history.push_back(id);
  if(!a->evidence.empty()) evidence.insert(a->evidence);
  for(const auto& item:a->reward) {
   if(item=="medkit" || item=="distraction" || item=="cash") ++inventory[item];
   else inventory[item]=1;
  }
  if(id=="power") inventory["fuse"]=0;
  if(id=="usb") chaseStarted=true;
  if(id=="garage" && threat) garageUnderThreat=true;
  if(Current()!=previous) questSince=elapsed;
  return Result::Applied;
 }
 Result Submit(const std::string& id,const std::string& input,bool threat=false) {
  const auto* a=Find(id);if(!a || a->answer.empty()) return Result::Invalid;
  if(Done(id)) return Result::AlreadyDone;
  if(Finished()) return Result::Locked;
  if(!CanDo(*a)) return Result::Locked;
  if(!All(a->clues)) return Result::MissingClue;
  auto& lock=locks[id];
  if(elapsed<lock.until) return Result::Cooldown;
  if(input!=Answer(*a)) {
   if(++lock.failures>=3){lock.failures=0;lock.until=elapsed+8;}
   return Result::Wrong;
  }
  const auto result=Apply(id,threat);
  if(result==Result::Applied) {lock.failures=0;lock.until=0;}
  return result;
 }
 bool Use(const std::string& id) {
  if((id!="medkit" && id!="distraction") || !Has(id)) return false;
  --inventory[id]; if(id=="medkit") ++medkitsUsed;else ++distractionsUsed; return true;
 }
 uint32_t Achievements() const {
  uint32_t value=Done("awake")?FirstSteps:0u;
  if(Done("apartment_exit")) value|=EscapeArtist;
  if(Done("street") && !courtyardDetected) value|=Unseen;
  bool allEvidence=true;for(const auto& a:Catalog())if(!a.evidence.empty() && !evidence.count(a.evidence))allEvidence=false;
  if(allEvidence) value|=Detective;
  if(Finished() && kills==0) value|=Pacifist;
  if(garageUnderThreat) value|=QuickThinking;
  return value;
 }
 // Replays stable event IDs and derives inventory/evidence. Rejects inconsistent or future snapshots.
 bool Valid() const {
  if(variant<0 || variant>=static_cast<int>(LightVariants().size()) || kills<0 || kills>3 || medkitsUsed<0 || distractionsUsed<0 ||
   !std::isfinite(elapsed) || elapsed<0 || !std::isfinite(questSince) || questSince<0 || questSince>elapsed || history.size()>Catalog().size()) return false;
  for(const auto& id:neutralized) if(id!="hall" && id!="courtyard" && id!="pursuer") return false;
  Progress replay(variant);
  for(const auto& id:history) if(replay.Apply(id,id=="garage" && garageUnderThreat)!=Result::Applied) return false;
  for(int i=0;i<medkitsUsed;++i) if(!replay.Use("medkit")) return false;
  for(int i=0;i<distractionsUsed;++i) if(!replay.Use("distraction")) return false;
  if(replay.completed!=completed || replay.inventory!=inventory || replay.evidence!=evidence || replay.chaseStarted!=chaseStarted || replay.garageUnderThreat!=garageUnderThreat) return false;
  for(const auto& pair:locks) {
   const auto* a=Find(pair.first);
   if(!a || a->answer.empty() || pair.second.failures<0 || pair.second.failures>2 || !std::isfinite(pair.second.until) || pair.second.until<0 || pair.second.until>elapsed+8.01) return false;
  }
  return true;
 }
};
}
