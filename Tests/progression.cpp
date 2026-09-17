#include "Core/SliceProgress.h"
#include <cassert>
#include <iostream>
#include <random>
using namespace Wroclaw;
void do_action(Progress& p,const std::string& id,bool threat=false){
 const auto* a=Progress::Find(id);assert(a);
 const auto r=a->answer.empty()?p.Apply(id,threat):p.Submit(id,p.Answer(*a),threat);
 if(r!=Result::Applied){std::cerr<<"Blocked: "<<id<<" result="<<static_cast<int>(r)<<"\n";std::abort();}
 assert(p.Valid());
}
void apartment(Progress& p){for(const char* id:{"awake","look","door_checked","phone","phone_attempt","charger","cable","outlet_check","fuse_inspect","kitchen","date_photo","date_rule","stash","fuse","power","charge","dog_photo","pin_rule","unlock_phone","sms","contact","book","computer","address_photo","target","peephole","routes","apartment_unlocked","apartment_exit"})do_action(p,id);}
void route(Progress& p,bool basement){
 if(basement){for(const char* id:{"basement_key","flashlight","batteries","basement_enter","graffiti","building_plan","basement_symbols","basement_exit"})do_action(p,id);}
 else{for(const char* id:{"technical_enter","technical_clue","technical_lock","technical_exit"})do_action(p,id);}
 do_action(p,"courtyard");do_action(p,"street");
}
void shop(Progress& p,bool key){
 if(key){do_action(p,"shop_key");do_action(p,"shop_back");}
 else{do_action(p,"shop_rule");do_action(p,"shop_sign");do_action(p,"shop_code");}
 for(const char* id:{"receipt","schedule","terminal","cctv","panel","package","file_dates","usb","ambush","alley"})do_action(p,id);
}
void finale(Progress& p){
 do_action(p,"garage",true);do_action(p,"garage_enter");do_action(p,"garage_escape");
 assert(p.Apply("lost_pursuit",true)==Result::Unsafe);
 do_action(p,"lost_pursuit");assert(p.Submit("workshop","4807",true)==Result::Unsafe);
 do_action(p,"workshop");do_action(p,"blockade_map");assert(p.Finished() && p.Chapter2Unlocked());
}
int main(){
 int runs=0;
 for(int variant=0;variant<3;++variant)for(bool basement:{false,true})for(bool key:{false,true}){
  Progress p(variant);assert(p.Valid());apartment(p);route(p,basement);shop(p,key);finale(p);++runs;
  assert(p.Current()==17);assert(p.Achievements()&Pacifist);assert(p.Achievements()&QuickThinking);
  for(const char* optional:{"neighbor_help","neighbor_leave","safe","car","street_phone","secret_1","secret_2","secret_3","secret_4","secret_5"})assert(!p.Done(optional));
  assert(!p.Has("fuse"));assert(p.Apply("power")==Result::AlreadyDone);
  Progress restored=p;assert(restored.Valid() && restored.variant==variant && restored.Finished());
  restored.inventory["medkit"]+=10;assert(!restored.Valid());
  restored=p;restored.history.insert(restored.history.begin(),"workshop");assert(!restored.Valid());
  restored=p;restored.variant=99;assert(!restored.Valid());
  restored=p;restored.kills=1;assert(!(restored.Achievements()&Pacifist));
 }
 // Both exit paths are real alternatives, not two mandatory objectives.
 Progress p;apartment(p);route(p,false);assert(p.Apply("basement_exit")==Result::Locked);
 assert(p.Submit("terminal","0624")==Result::Locked);
 do_action(p,"shop_rule");do_action(p,"shop_sign");
 assert(p.Submit("shop_code","1111")==Result::Wrong);
 assert(p.Submit("shop_code","2222")==Result::Wrong);
 assert(p.Submit("shop_code","3333")==Result::Wrong);
 assert(p.Submit("shop_code","1986")==Result::Cooldown);
 Progress snapshot=p;assert(snapshot.Valid());
 for(int i=0;i<81;++i)snapshot.Tick(.1);
 assert(snapshot.Submit("shop_code","1986")==Result::Applied);assert(snapshot.Valid());
 // Evidence is required even if the player guesses a correct numeric answer.
 Progress clues;apartment(clues);route(clues,true);assert(clues.Submit("shop_code","1986")==Result::MissingClue);
 // Optional rewards, irreversible neighbour choice and exact consumable accounting.
 do_action(clues,"neighbor_help");assert(clues.Apply("neighbor_leave")==Result::Locked);
 assert(clues.Use("medkit"));assert(!clues.Use("medkit"));assert(clues.Valid());
 do_action(clues,"bottle");assert(clues.Use("distraction"));assert(!clues.Use("distraction"));assert(clues.Valid());
 Progress skipped;apartment(skipped);do_action(skipped,"neighbor_leave");route(skipped,true);shop(skipped,false);finale(skipped);
 // Every evidence entry and all side quests are obtainable before the finale.
 Progress detective;apartment(detective);route(detective,true);shop(detective,false);
 do_action(detective,"garage",true);do_action(detective,"garage_enter");
 for(const char* id:{"neighbor_help","safe_a","safe_b","safe_c","safe","car_clue","car_key","car","street_phone_clue","street_phone","technical_enter","technical_clue","secret_1","secret_2","secret_3","secret_4","secret_5"})do_action(detective,id);
 do_action(detective,"garage_escape");do_action(detective,"lost_pursuit");do_action(detective,"workshop");
 assert(detective.Apply("blockade_map",true)==Result::Unsafe);do_action(detective,"blockade_map");
 assert(detective.Achievements()&Detective);assert(detective.Apply("can")==Result::Locked);
 // Hints have three timed levels and reset only on real quest progress.
 Progress hint;for(int i=0;i<901;++i)hint.Tick(.1);assert(hint.HintLevel()==1);
 for(int i=0;i<900;++i){hint.Tick(.1);}
 assert(hint.HintLevel()==2);
 for(int i=0;i<1200;++i){hint.Tick(.1);}
 assert(hint.HintLevel()==3);
 apartment(hint);assert(hint.HintLevel()==0);
 // Model-based adversarial interaction orders exercise the production engine, not a duplicate model.
 std::mt19937 rng(20260917);
 for(int run=0;run<50;++run){Progress state(run%3);
  for(int i=0;i<1000;++i){const auto& a=Catalog()[rng()%Catalog().size()];
   if(a.answer.empty())state.Apply(a.id);else state.Submit(a.id,(rng()%3)?state.Answer(a):"wrong");
   state.Tick(.1);assert(state.Valid());
  }
 }
 std::cout<<"PASS: "<<runs<<" main routes, optional skips/rewards, save invariants, lockouts, hints and 50000 mixed interactions\n";
}
