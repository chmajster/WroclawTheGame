#include "Interaction/SliceProp.h"
#include "Engine/GameInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Components/StaticMeshComponent.h"
#include "Character/SliceCharacter.h"
#include "Mission/SliceMission.h"
#include "UI/SliceController.h"
#include "Audio/SliceAudio.h"
#include "Core/SliceGameMode.h"
#include "AI/SliceEnemy.h"
#include "Kismet/GameplayStatics.h"
ASliceProp::ASliceProp() {
 PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.bStartWithTickEnabled=false;
 Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh")); RootComponent=Mesh;
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 Mesh->SetStaticMesh(Cube.Object); Mesh->SetCollisionProfileName(TEXT("Interactable"));
}
void ASliceProp::Configure(EPropKind Type,const FVector& Size) {
 Kind=Type; Mesh->SetWorldScale3D(Size/100); ClosedPosition=GetActorLocation();
 const TCHAR* Mat=(Kind==EPropKind::PinNote || Kind==EPropKind::Intro)?TEXT("/Game/Generated/M_Paper.M_Paper"):TEXT("/Game/Generated/M_Wood.M_Wood");
 if(auto* M=LoadObject<UMaterialInterface>(nullptr,Mat)) Mesh->SetMaterial(0,M);
 Restore();
}
FText ASliceProp::Prompt(ASliceCharacter* Player) const {
 const TCHAR* Texts[]={TEXT("Przeczytaj kartkę"),TEXT("Otwórz szufladę / zabierz telefon"),TEXT("Zabierz powerbank z kablem"),TEXT("Przeczytaj kartkę z PIN-em"),
 TEXT("Otwórz szafkę kuchenną / zabierz bezpiecznik"),TEXT("Włóż bezpiecznik i włącz prąd"),TEXT("Włącz lampę — odczytaj kod"),
 TEXT("Otwórz szafkę / zabierz klucz"),TEXT("Otwórz drzwi mieszkania"),TEXT("Otwórz drzwi budynku"),TEXT("Wejdź do bezpiecznego lokalu")};
 return FText::FromString(Texts[static_cast<int32>(Kind)]);
}
void ASliceProp::Open(bool bInstant) {
 if(bOpened) return;
 bOpened=true;
 const bool Door=Kind==EPropKind::ExitDoor || Kind==EPropKind::BuildingDoor || Kind==EPropKind::SafeDoor;
 TargetRotation=FRotator::ZeroRotator;
 TargetPosition=ClosedPosition+FVector(-30,0,0);
 if(Door) {
  Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  if(Kind==EPropKind::ExitDoor) {TargetPosition=ClosedPosition+FVector(-95,-95,0);TargetRotation.Yaw=90;}
  else if(Kind==EPropKind::BuildingDoor) {TargetPosition=ClosedPosition+FVector(-90,90,0);TargetRotation.Yaw=90;}
  else {TargetPosition=ClosedPosition+FVector(87.5,87.5,0);TargetRotation.Yaw=-90;}
 }
 if(bInstant) SetActorLocationAndRotation(TargetPosition,TargetRotation);
 else {SetActorTickEnabled(true);USliceAudio::Play(this,Door?TEXT("Door"):TEXT("Drawer"),ClosedPosition,0.7f);}
}
void ASliceProp::Tick(float Dt) {
 Super::Tick(Dt);
 SetActorLocationAndRotation(FMath::VInterpTo(GetActorLocation(),TargetPosition,Dt,8),FMath::RInterpTo(GetActorRotation(),TargetRotation,Dt,8));
 if(GetActorLocation().Equals(TargetPosition,0.1) && GetActorRotation().Equals(TargetRotation,0.1)) {
  SetActorLocationAndRotation(TargetPosition,TargetRotation);SetActorTickEnabled(false);
 }
}
void ASliceProp::Restore() {
 auto* M=GetGameInstance()->GetSubsystem<USliceMission>(); const auto& S=M->State;
 if((Kind==EPropKind::Cabinet && S.Complete(6)) || (Kind==EPropKind::ExitDoor && S.Complete(8)) ||
 (Kind==EPropKind::BuildingDoor && S.Complete(10)) || (Kind==EPropKind::SafeDoor && S.Finished()) ||
 (Kind==EPropKind::PhoneDrawer && S.Has(Wroclaw::Item::Phone)) ||
 (Kind==EPropKind::FuseCupboard && (S.Has(Wroclaw::Item::Fuse) || S.Complete(4)))) Open(true);
 if((Kind==EPropKind::Charger && S.Has(Wroclaw::Item::Charger)) || (Kind==EPropKind::PinNote && S.Has(Wroclaw::Item::PhoneNote))) {SetActorHiddenInGame(true); SetActorEnableCollision(false);}
}
void ASliceProp::Interact(ASliceCharacter* Player) {
 using Wroclaw::Event; auto* M=Player->Mission(); auto* PC=CastChecked<ASliceController>(Player->GetController());
 switch(Kind) {
 case EPropKind::Intro:
  M->Apply(Event::Look); PC->ShowMessage(TEXT("17 września. Wrocław, Nadodrze.\nNie pamiętam tej nocy. Drzwi zamknięte.\nTelefon zostawiłem w szufladzie biurka. Zapasowe zasilanie jest przy kanapie.")); break;
 case EPropKind::PhoneDrawer:
  if(!bOpened){Open(); M->Notify(TEXT("W szufladzie leży telefon. Naciśnij E, żeby go zabrać."));}
  else if(M->Apply(Event::FindPhone)){ USliceAudio::Play(this,TEXT("Pickup"),GetActorLocation()); M->Notify(TEXT("Telefon zabrany. Naciśnij T, żeby go użyć.")); } break;
 case EPropKind::Charger:
  if(M->Apply(Event::FindCharger)){SetActorHiddenInGame(true); SetActorEnableCollision(false); USliceAudio::Play(this,TEXT("Pickup"),GetActorLocation()); M->Notify(TEXT("Powerbank z kablem USB zabrany. Telefon: T."));} break;
 case EPropKind::PinNote:
  M->Apply(Event::ReadPhoneNote); PC->ShowMessage(TEXT("Kartka przy zdjęciu: PIN do telefonu — 0417.\nKopia została dodana do ekwipunku [I].")); break;
 case EPropKind::FuseCupboard:
  if(!bOpened){ Open(); M->Notify(TEXT("W środku jest zapasowy bezpiecznik. Naciśnij E, żeby go zabrać.")); }
  else if(M->Apply(Event::FindFuse)){USliceAudio::Play(this,TEXT("Pickup"),GetActorLocation()); M->Notify(TEXT("Zabrano bezpiecznik."));} break;
 case EPropKind::FuseBox:
  if(M->Apply(Event::RestorePower)){USliceAudio::Play(this,TEXT("Switch"),GetActorLocation()); M->Notify(TEXT("Prąd przywrócony. Włącz lampę na biurku."));}
  else M->Notify(M->State.Complete(4)?TEXT("Zasilanie działa."):TEXT("Potrzebujesz bezpiecznika i wskazówki z wiadomości w telefonie.")); break;
 case EPropKind::Lamp:
  if(!M->State.Complete(4)){M->Notify(TEXT("Brak zasilania."));break;}
  M->Apply(Event::RevealCode); USliceAudio::Play(this,TEXT("Switch"),GetActorLocation());
  PC->ShowMessage(TEXT("Pod lampą pojawia się zapis: 7 — 3 — 1 — 9.\nTo kod do zamkniętej szafki obok drzwi.")); break;
 case EPropKind::Cabinet:
  if(!M->State.Complete(6)) PC->OpenKeypad(this);
  else if(M->Apply(Event::TakeKey)){ USliceAudio::Play(this,TEXT("Pickup"),GetActorLocation()); M->Notify(TEXT("Zabrano klucz do wyjścia z mieszkania."));} break;
 case EPropKind::ExitDoor:
  if(M->State.Has(Wroclaw::Item::ExitKey)) Open();
  else M->Notify(TEXT("Drzwi zamknięte. Klucz jest w szafce z zamkiem kodowym.")); break;
 case EPropKind::BuildingDoor: Open(); break;
 case EPropKind::SafeDoor:
  if(M->State.Complete(11) && !CastChecked<ASliceGameMode>(GetWorld()->GetAuthGameMode())->Enemy->IsThreat()) {Open(); M->Notify(TEXT("Wejdź do lokalu. Tutaj będziesz bezpieczny."));}
  else M->Notify(TEXT("Nie sprowadzaj tu napastnika. Zgub pościg i przeczekaj poszukiwania.")); break;
 }
}
void ASliceProp::SubmitCode(const FString& Code,ASliceCharacter* Player) {
 auto* M=Player->Mission();
 if(Code!=TEXT("7319")){M->Notify(TEXT("Nieprawidłowy kod."));return;}
 if(!M->State.Complete(5)){M->Notify(TEXT("Najpierw sprawdź wskazówkę pod lampą."));return;}
 if(M->Apply(Wroclaw::Event::OpenCabinet)){Open(); if(M->bLastSaveSucceeded) M->Notify(TEXT("Szafka otwarta. Zabierz klucz [E]. Zapisano checkpoint."));}
}
