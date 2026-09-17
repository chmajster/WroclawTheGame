#pragma once
#include <cstdint>
namespace Wroclaw {
enum class Item : uint8_t { Phone, Charger, Fuse, ExitKey, PhoneNote, Count };
enum class Event : uint8_t { Look, FindPhone, FindCharger, ReadPhoneNote, UnlockPhone,
 ReadMessage, FindFuse, RestorePower, RevealCode, OpenCabinet, TakeKey,
 ExitApartment, GroundFloor, Street, LosePursuit, ReachSafe };
struct Progress {
 static constexpr int Version = 1;
 static constexpr uint32_t AllObjectives = (1u << 13) - 1;
 uint32_t objectives = 0;
 uint32_t items = 0;
 bool phoneUnlocked = false;
 bool pursuitStarted = false;
 bool Complete(int index) const { return index >= 0 && index < 13 && (objectives & (1u << index)); }
 bool Has(Item item) const { return (items & (1u << static_cast<int>(item))) != 0; }
 void Add(Item item) { items |= 1u << static_cast<int>(item); }
 bool Remove(Item item) { if (!Has(item)) return false; items &= ~(1u << static_cast<int>(item)); return true; }
 int Current() const { for(int i=0;i<13;++i) if(!Complete(i)) return i; return 13; }
 bool Finished() const { return objectives == AllObjectives; }
 bool Apply(Event event) {
  switch(event) {
   case Event::Look: return Mark(0);
   case Event::FindPhone: Add(Item::Phone); Mark(0); return Mark(1);
   case Event::FindCharger: if(Has(Item::Charger)) return false; Add(Item::Charger); return true;
   case Event::ReadPhoneNote: if(Has(Item::PhoneNote)) return false; Add(Item::PhoneNote); return true;
   case Event::UnlockPhone:
    if(!Has(Item::Phone) || !Has(Item::Charger) || !Has(Item::PhoneNote) || phoneUnlocked) return false;
    phoneUnlocked=true; return Mark(2);
   case Event::ReadMessage: if(!Complete(2)) return false; return Mark(3);
   case Event::FindFuse: if(Has(Item::Fuse) || Complete(4)) return false; Add(Item::Fuse); return true;
   case Event::RestorePower:
    if(!Complete(3) || !Has(Item::Fuse) || Complete(4)) return false;
    Remove(Item::Fuse); return Mark(4);
   case Event::RevealCode: if(!Complete(4)) return false; return Mark(5);
   case Event::OpenCabinet: if(!Complete(5)) return false; return Mark(6);
   case Event::TakeKey: if(!Complete(6) || Complete(7)) return false; Add(Item::ExitKey); return Mark(7);
   case Event::ExitApartment: if(!Complete(7) || !Has(Item::ExitKey)) return false; return Mark(8);
   case Event::GroundFloor: if(!Complete(8)) return false; return Mark(9);
   case Event::Street: if(!Complete(9)) return false; return Mark(10);
   case Event::LosePursuit: if(!Complete(10) || !pursuitStarted) return false; return Mark(11);
   case Event::ReachSafe: if(!Complete(11)) return false; return Mark(12);
  }
  return false;
 }
 bool Valid() const {
  if(objectives & ~AllObjectives || items & ~((1u << static_cast<int>(Item::Count))-1)) return false;
  // Mission objectives form a prefix. Pickups may be discovered out of order.
  if((objectives & (objectives+1)) != 0) return false;
  if(Complete(1) != Has(Item::Phone)) return false;
  if(Complete(2) != phoneUnlocked) return false;
  if(phoneUnlocked && (!Has(Item::Charger) || !Has(Item::PhoneNote))) return false;
  if(Complete(4) && Has(Item::Fuse)) return false;
  if(Complete(7) != Has(Item::ExitKey)) return false;
  if(Complete(11) && !pursuitStarted) return false;
  return true;
 }
private:
 bool Mark(int index) { if(Complete(index)) return false; objectives |= 1u << index; return true; }
};
}
