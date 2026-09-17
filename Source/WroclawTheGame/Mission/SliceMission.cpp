#include "Mission/SliceMission.h"
#include "Save/SliceSave.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Character/SliceCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Core/SliceGameMode.h"
namespace {
 const TCHAR* Slot=TEXT("Przebudzenie_v2");
 FString U(const std::string& S){return UTF8_TO_TCHAR(S.c_str());}
 const TCHAR* Category(const std::string& Id) {
  if(Id=="contact" || Id=="neighbor" || Id=="target" || Id=="street_photo") return TEXT("LUDZIE");
  if(Id=="routes" || Id=="address_photo" || Id=="blockade_map") return TEXT("MIEJSCA");
  if(Id=="sms" || Id=="street_sms" || Id=="workshop_sms" || Id=="computer_mail") return TEXT("WIADOMOŚCI");
  return TEXT("DOWODY");
 }
}
void USliceMission::NewGame() {
 State=Wroclaw::Progress(FMath::RandRange(0,static_cast<int32>(Wroclaw::LightVariants().size())-1));
 Anchor=FVector(250,400,456);bInGame=true;bShowMenu=false;bDead=false;
 if(auto* Profile=Cast<USliceProfile>(UGameplayStatics::LoadGameFromSlot(TEXT("LocalAchievements"),0))) LifetimeAchievements=Profile->Achievements & 63;
 SaveCheckpoint();
}
bool USliceMission::Threat() const {
 auto* GM=GetWorld()?Cast<ASliceGameMode>(GetWorld()->GetAuthGameMode()):nullptr;
 return GM && GM->HasThreat();
}
bool USliceMission::HasSave() const {
 if(!bSaveChecked){bHasValidSave=const_cast<USliceMission*>(this)->LoadState(false);bSaveChecked=true;}
 return bHasValidSave;
}
bool USliceMission::LoadState(bool bApply) {
 auto* S=Cast<USliceSave>(UGameplayStatics::LoadGameFromSlot(Slot,0));
 if(!S || S->Version!=Wroclaw::Progress::Version || S->History.Num()>static_cast<int32>(Wroclaw::Catalog().size()) ||
  S->Anchor.ContainsNaN() || S->Anchor.X<0 || S->Anchor.X>11500 || S->Anchor.Y<0 || S->Anchor.Y>6000 || S->Anchor.Z<-400 || S->Anchor.Z>1500) return false;
 Wroclaw::Progress Candidate(S->Variant);
 for(const auto& Id:S->History) if(Candidate.Apply(TCHAR_TO_UTF8(*Id),Id==TEXT("garage") && S->GarageUnderThreat)!=Wroclaw::Result::Applied) return false;
 if(S->MedkitsUsed<0 || S->MedkitsUsed>10 || S->DistractionsUsed<0 || S->DistractionsUsed>10) return false;
 for(int I=0;I<S->MedkitsUsed;++I) if(!Candidate.Use("medkit")) return false;
 for(int I=0;I<S->DistractionsUsed;++I) if(!Candidate.Use("distraction")) return false;
 Candidate.kills=S->Kills;Candidate.courtyardDetected=S->CourtyardDetected;
 Candidate.elapsed=S->Elapsed;Candidate.questSince=S->QuestSince;
 for(const auto& Id:S->Neutralized) Candidate.neutralized.insert(TCHAR_TO_UTF8(*Id));
 if(S->Failures.Num()!=S->LockUntil.Num()) return false;
 for(const auto& Pair:S->Failures) {
  const auto* Until=S->LockUntil.Find(Pair.Key);if(!Until) return false;
  Candidate.locks[TCHAR_TO_UTF8(*Pair.Key)]={Pair.Value,*Until};
 }
 if(!Candidate.Valid()) return false;
 if(bApply){State=Candidate;Anchor=S->Anchor;}
 return true;
}
bool USliceMission::ContinueGame() {
 if(!LoadState(true)){Notify(TEXT("Brak poprawnego zapisu v2. Zapisy starszego prototypu nie są zgodne z nowymi zagadkami."));return false;}
 bInGame=true;bShowMenu=false;bDead=false;bLastSaveSucceeded=true;
 if(auto* Profile=Cast<USliceProfile>(UGameplayStatics::LoadGameFromSlot(TEXT("LocalAchievements"),0))) LifetimeAchievements=Profile->Achievements & 63;
 return true;
}
bool USliceMission::SaveCheckpoint() {
 if(!State.Valid()){bLastSaveSucceeded=false;Notify(TEXT("Zapis odrzucony: niespójny stan rozdziału."));return false;}
 auto* S=Cast<USliceSave>(UGameplayStatics::CreateSaveGameObject(USliceSave::StaticClass()));
 S->Variant=State.variant;S->Anchor=Anchor;S->Elapsed=State.elapsed;S->QuestSince=State.questSince;
 S->Kills=State.kills;S->MedkitsUsed=State.medkitsUsed;S->DistractionsUsed=State.distractionsUsed;
 S->CourtyardDetected=State.courtyardDetected;S->GarageUnderThreat=State.garageUnderThreat;
 for(const auto& Id:State.history) S->History.Add(U(Id));
 for(const auto& Id:State.neutralized) S->Neutralized.Add(U(Id));
 for(const auto& Pair:State.locks){S->Failures.Add(U(Pair.first),Pair.second.failures);S->LockUntil.Add(U(Pair.first),Pair.second.until);}
 bLastSaveSucceeded=UGameplayStatics::SaveGameToSlot(S,Slot,0);bSaveChecked=false;
 Notify(bLastSaveSucceeded?TEXT("Zapisano checkpoint."):TEXT("Nie udało się zapisać checkpointu. Sprawdź miejsce na dysku."));return bLastSaveSucceeded;
}
void USliceMission::After(const FString& Id,Wroclaw::Result Result) {
 if(Result==Wroclaw::Result::Applied) {
  const auto* A=Wroclaw::Progress::Find(TCHAR_TO_UTF8(*Id));
  UpdateAchievements();
  if(A && A->checkpoint) {
   if(auto* P=Cast<ASliceCharacter>(UGameplayStatics::GetPlayerPawn(this,0))){
    Anchor=P->GetActorLocation();
    // Checkpoints restore a standing capsule, even when activated while crouching.
    Anchor.Z+=90.f-P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
   }
   SaveCheckpoint();
  }
 } else {
  switch(Result) {
   case Wroclaw::Result::Wrong: Notify(TEXT("Błędne rozwiązanie. Po trzech pomyłkach panel blokuje się na 8 sekund."));break;
   case Wroclaw::Result::Cooldown: Notify(TEXT("Panel czasowo zablokowany. Poczekaj, aż zgaśnie ostrzeżenie."));break;
   case Wroclaw::Result::MissingClue: Notify(TEXT("Brakuje wskazówek do tego rozwiązania. Sprawdź śledztwo [J]."));break;
   case Wroclaw::Result::Unsafe: Notify(TEXT("Napastnik nadal cię szuka. Najpierw zgub pościg."));break;
   case Wroclaw::Result::Locked: Notify(TEXT("Brakuje przedmiotu lub wcześniejszego etapu. Sprawdź cel i podpowiedź [H]."));break;
   default:break;
  }
 }
}
Wroclaw::Result USliceMission::Act(const FString& Id) {auto R=State.Apply(TCHAR_TO_UTF8(*Id),Threat());After(Id,R);return R;}
Wroclaw::Result USliceMission::Submit(const FString& Id,const FString& Code){auto R=State.Submit(TCHAR_TO_UTF8(*Id),TCHAR_TO_UTF8(*Code),Threat());After(Id,R);return R;}
void USliceMission::Notify(const FString& Message){Notification=Message;NotificationUntil=FPlatformTime::Seconds()+7;}
FString USliceMission::ObjectiveText() const {
 const int Index=State.Current();if(State.Finished()) return TEXT("Rozdział 1 ukończony. Odblokowano rozdział 2.");
 return FString::Printf(TEXT("%d / 17 — %s"),Index+1,*U(Wroclaw::Quests()[Index].title));
}
FString USliceMission::InventoryText() const {
 static const TMap<FString,FString> Names={{TEXT("phone"),TEXT("Telefon")},{TEXT("charger"),TEXT("Ładowarka")},{TEXT("cable"),TEXT("Kabel USB")},
 {TEXT("fuse"),TEXT("Bezpiecznik")},{TEXT("apartment_key"),TEXT("Klucz do mieszkania")},{TEXT("basement_key"),TEXT("Klucz do piwnicy")},
 {TEXT("flashlight"),TEXT("Latarka")},{TEXT("batteries"),TEXT("Baterie")},{TEXT("shop_key"),TEXT("Klucz sklepu")},{TEXT("garage_key"),TEXT("Klucz garażu")},
 {TEXT("usb"),TEXT("Pendrive")},{TEXT("car_key"),TEXT("Pilot samochodu")},{TEXT("medkit"),TEXT("Opatrunek [V]")},{TEXT("distraction"),TEXT("Przedmiot do rzutu [G]")},{TEXT("cash"),TEXT("Gotówka")}};
 FString Text;
 for(const auto& Pair:State.inventory) if(Pair.second>0){const FString Id=U(Pair.first);const FString* Name=Names.Find(Id);Text+=FString::Printf(TEXT("%s × %d\n"),Name?**Name:*Id,Pair.second);}
 return Text.IsEmpty()?TEXT("Brak przedmiotów"):Text;
}
FString USliceMission::InvestigationText(int32 CategoryIndex) const {
 const TCHAR* Categories[]={TEXT("LUDZIE"),TEXT("MIEJSCA"),TEXT("DOWODY"),TEXT("WIADOMOŚCI")};
 FString Text=TEXT("1 Ludzie | 2 Miejsca | 3 Dowody | 4 Wiadomości\nStrzałki góra/dół — przewijanie\n\n");
 for(const auto& A:Wroclaw::Catalog()) if(!A.evidence.empty() && State.evidence.count(A.evidence) && FString(Category(A.evidence))==Categories[FMath::Clamp(CategoryIndex,0,3)])
  Text+=U(A.label)+TEXT(":\n")+U(A.body)+TEXT("\n\n");
 return Text;
}
FString USliceMission::PhoneText(int32 Page) {
 if(Page==0) Act(TEXT("sms"));if(Page==1 || Page==4) Act(TEXT("contact"));
 FString Text=TEXT("1 SMS | 2 Kontakty | 3 Zdjęcia | 4 Notatki | 5 Połączenia\n\n");
 if(Page==0) {for(const auto* Id:{"sms","street","garage_escape"}) if(State.Done(Id)) Text+=U(Wroclaw::Progress::Find(Id)->body)+TEXT("\n\n");}
 if(Page==1) Text+=TEXT("M. — numer zastrzeżony. Nadawca ostrzega przed głównym wejściem.\n");
 if(Page==2) {for(const auto* Id:{"dog_photo","address_photo","street_phone"}) if(State.Done(Id)) Text+=U(Wroclaw::Progress::Find(Id)->body)+TEXT("\n\n");}
 if(Page==3) Text+=ObjectiveText()+TEXT("\n")+HintText();
 if(Page==4) Text+=TEXT("Ostatnie połączenie: 02:17. Rozmówca: numer zastrzeżony.\nŚlady rozmowy zgadzają się z czasem zapisu monitoringu.");
 return Text;
}
FString USliceMission::HintText() const {
 const int L=State.HintLevel();if(!L) return TEXT("Podpowiedź poziomu 1 pojawi się po 90 sekundach bez postępu.");
 if(State.Current()==6) return TEXT("Wybierz piwnicę po lewej albo schody techniczne po prawej. Pomoc sąsiadowi jest opcjonalna.");
 const auto& Q=Wroclaw::Quests()[FMath::Min(State.Current(),16)];
 const Wroclaw::ActionDef* A=nullptr;
 for(const auto& Id:Q.all) if(!State.Done(Id)){A=Wroclaw::Progress::Find(Id);break;}
 if(!A && !Q.any.empty()) A=Wroclaw::Progress::Find(Q.any.front());
 if(!A) return TEXT("Wróć do ostatniej wiadomości i sprawdź śledztwo.");
 // Follow unmet prerequisites so a late quest points at a reachable clue, never reveals its answer.
 for(int I=0;I<30;++I){const Wroclaw::ActionDef* Next=nullptr;for(const auto& Id:A->prerequisites) if(!State.Done(Id)){Next=Wroclaw::Progress::Find(Id);break;}if(!Next) for(const auto& Item:A->items) if(!State.Has(Item)){for(const auto& Producer:Wroclaw::Catalog())if(!State.Done(Producer.id) && std::find(Producer.reward.begin(),Producer.reward.end(),Item)!=Producer.reward.end()){Next=&Producer;break;}if(Next)break;}if(!Next) break;A=Next;}
 FString Text=FString::Printf(TEXT("Podpowiedź %d: %s."),L,*U(A->label));
 if(L>=2) for(const auto& Id:A->clues){const auto* Clue=Wroclaw::Progress::Find(Id);if(Clue) Text+=TEXT("\nPorównaj: ")+U(Clue->label);}
 if(L>=3) Text+=TEXT("\nSprawdź kolejność liczb, dat i symboli w znalezionych materiałach. Podpowiedź nie podaje kodu.");
 return Text;
}
void USliceMission::UpdateAchievements() {
 const int32 Combined=LifetimeAchievements|State.Achievements();if(Combined==LifetimeAchievements)return;
 auto* Profile=Cast<USliceProfile>(UGameplayStatics::CreateSaveGameObject(USliceProfile::StaticClass()));Profile->Achievements=Combined;
 if(UGameplayStatics::SaveGameToSlot(Profile,TEXT("LocalAchievements"),0)) LifetimeAchievements=Combined;
 else Notify(TEXT("Nie udało się zapisać lokalnego osiągnięcia."));
}
FString USliceMission::AchievementsText() const {
 const TCHAR* Names[]={TEXT("Pierwsze kroki"),TEXT("Escape Artist"),TEXT("Bez śladu"),TEXT("Detektyw"),TEXT("Pacyfista"),TEXT("Szybkie myślenie")};
 FString Text;for(int I=0;I<6;++I) Text+=(LifetimeAchievements&(1<<I)?TEXT("[zdobyte] "):TEXT("[zamknięte] "))+FString(Names[I])+TEXT("\n");return Text;
}

FString USliceMission::QuestLogText() const {
 FString Text=TEXT("GŁÓWNE ETAPY\n");int I=0;
 for(const auto& Q:Wroclaw::Quests()) {const bool Done=State.All(Q.all) && State.Any(Q.any);Text+=FString::Printf(TEXT("%d. [%s] %s\n"),++I,Done?TEXT("ukończony"):TEXT("do wykonania"),*U(Q.title));}
 Text+=TEXT("\nPOBOCZNE — NIE BLOKUJĄ KAMPANII\n");
 Text+=TEXT("Sąsiad: ")+FString(State.Done("neighbor_help")?TEXT("udzielono pomocy"):(State.Done("neighbor_leave")?TEXT("zrezygnowano"):TEXT("nieukończony")))+TEXT("\n");
 for(const auto* Id:{"safe","car","street_phone"})Text+=U(Wroclaw::Progress::Find(Id)->label)+TEXT(": ")+(State.Done(Id)?TEXT("ukończony\n"):TEXT("nieukończony\n"));
 int Secrets=0;for(int J=1;J<=5;++J)if(State.Done("secret_"+std::to_string(J)))++Secrets;
 Text+=FString::Printf(TEXT("\nSekrety: %d / 5"),Secrets);return Text;
}
