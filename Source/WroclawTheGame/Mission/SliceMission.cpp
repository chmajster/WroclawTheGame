#include "Mission/SliceMission.h"
#include "Save/SliceSave.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
namespace { const TCHAR* Slot = TEXT("Przebudzenie_v1"); }
void USliceMission::NewGame() {
 State = {}; Checkpoint=0; bInGame=true; bShowMenu=false; bDead=false;
 // A new game overwrites the old run only after saving succeeds.
 SaveCheckpoint(0);
}
bool USliceMission::HasSave() const {
 if(!bSaveChecked) {bHasValidSave=const_cast<USliceMission*>(this)->LoadState(false);bSaveChecked=true;}
 return bHasValidSave;
}
bool USliceMission::LoadState(bool bApply) {
 auto* Save = Cast<USliceSave>(UGameplayStatics::LoadGameFromSlot(Slot,0));
 if(!Save || Save->Version!=Wroclaw::Progress::Version || Save->Checkpoint<0 || Save->Checkpoint>4) return false;
 Wroclaw::Progress Candidate;
 Candidate.objectives=Save->Objectives; Candidate.items=Save->Items;
 Candidate.phoneUnlocked=Save->PhoneUnlocked; Candidate.pursuitStarted=Save->PursuitStarted;
 if(!Candidate.Valid()) return false;
 const int32 Minimum[] = {0,7,9,11,13};
 if(Candidate.Current()!=Minimum[Save->Checkpoint]) return false;
 if(bApply) { State=Candidate; Checkpoint=Save->Checkpoint; }
 return true;
}
bool USliceMission::ContinueGame() {
 if(!LoadState(true)) { Notify(TEXT("Brak poprawnego zapisu. Rozpocznij nową grę.")); return false; }
 bInGame=true; bShowMenu=false; bDead=false; bLastSaveSucceeded=true; return true;
}
bool USliceMission::SaveCheckpoint(int32 Index) {
 auto* Save=Cast<USliceSave>(UGameplayStatics::CreateSaveGameObject(USliceSave::StaticClass()));
 Save->Objectives=State.objectives; Save->Items=State.items;
 Save->PhoneUnlocked=State.phoneUnlocked; Save->PursuitStarted=State.pursuitStarted; Save->Checkpoint=Index;
 bLastSaveSucceeded=UGameplayStatics::SaveGameToSlot(Save,Slot,0); bSaveChecked=false;
 if(!bLastSaveSucceeded) { Notify(TEXT("Błąd zapisu checkpointu. Sprawdź wolne miejsce na dysku.")); return false; }
 Checkpoint=Index; Notify(TEXT("Zapisano checkpoint.")); return true;
}
bool USliceMission::Apply(Wroclaw::Event Signal) {
 if(!State.Apply(Signal)) return false;
 using Wroclaw::Event;
 if(Signal==Event::OpenCabinet) SaveCheckpoint(1);
 if(Signal==Event::ExitApartment) SaveCheckpoint(2);
 if(Signal==Event::Street) SaveCheckpoint(3);
 if(Signal==Event::ReachSafe) SaveCheckpoint(4);
 return true;
}
void USliceMission::Notify(const FString& Message) { Notification=Message; NotificationUntil=FPlatformTime::Seconds()+7; }
FString USliceMission::ObjectiveText() const {
 static const TCHAR* Objectives[] = {TEXT("Rozejrzyj się po mieszkaniu — przeczytaj kartkę przy łóżku"),
 TEXT("Znajdź telefon — sprawdź szufladę biurka"),TEXT("Uruchom telefon — znajdź powerbank z kablem i PIN"),
 TEXT("Przeczytaj wiadomość w telefonie [T]"),TEXT("Przywróć zasilanie — sprawdź kuchenną szafkę i rozdzielnię"),
 TEXT("Znajdź kod — włącz lampę nad biurkiem"),TEXT("Otwórz zamkniętą szafkę kodem"),TEXT("Zabierz klucz z otwartej szafki"),
 TEXT("Opuść mieszkanie"),TEXT("Zejdź schodami na parter"),TEXT("Przejdź przez podwórko na ulicę"),
 TEXT("Uniknij napastnika — zerwij kontakt za narożnikiem, ukryj się i poczekaj"),
 TEXT("Dotrzyj do bezpiecznego lokalu przez zaułek"),TEXT("Koniec rozdziału: PRZEBUDZENIE")};
 return Objectives[State.Current()];
}
FString USliceMission::InventoryText() const {
 static const TCHAR* Names[]={TEXT("Telefon"),TEXT("Powerbank + kabel USB"),TEXT("Bezpiecznik"),TEXT("Klucz do mieszkania"),TEXT("Kartka: PIN 0417")};
 FString Result;
 for(int32 I=0;I<static_cast<int32>(Wroclaw::Item::Count);++I) if(State.Has(static_cast<Wroclaw::Item>(I))) Result+=FString(Names[I])+TEXT("\n");
 return Result.IsEmpty()?TEXT("Brak przedmiotów"):Result;
}
FVector USliceMission::SpawnPoint() const {
 static const FVector Positions[]={{250,400,456},{880,600,456},{1290,400,456},{2400,2140,96},{4800,4900,96}};
 return Positions[FMath::Clamp(Checkpoint,0,4)];
}
