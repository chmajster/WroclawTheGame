#include "Interaction/SliceProp.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/GameInstance.h"
#include "Character/SliceCharacter.h"
#include "Mission/SliceMission.h"
#include "UI/SliceController.h"
#include "Audio/SliceAudio.h"
#include "Materials/MaterialInterface.h"
namespace {
 bool Gate(const std::string& Id){return Id=="apartment_unlocked" || Id=="basement_enter" || Id=="basement_symbols" || Id=="technical_enter" || Id=="technical_lock" || Id=="shop_code" || Id=="shop_back" || Id=="panel" || Id=="garage" || Id=="workshop";}
}
ASliceProp::ASliceProp() {
 PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=0.1f;
 Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));RootComponent=Mesh;
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 Mesh->SetStaticMesh(Cube.Object);Mesh->SetCollisionProfileName(TEXT("Interactable"));
}
void ASliceProp::Configure(const FString& Id,const FVector& Size) {
 ActionId=Id;ClosedPosition=GetActorLocation();Mesh->SetWorldScale3D(Size/100);
 const auto* A=Wroclaw::Progress::Find(TCHAR_TO_UTF8(*Id));
 const TCHAR* Path=A && Gate(A->id)?TEXT("/Game/Generated/M_Wood.M_Wood"):TEXT("/Game/Generated/M_Paper.M_Paper");
 if(auto* Mat=LoadObject<UMaterialInterface>(nullptr,Path)) Mesh->SetMaterial(0,Mat);
 Tick(0);
}
FText ASliceProp::Prompt(ASliceCharacter* Player) const {
 const auto* A=Wroclaw::Progress::Find(TCHAR_TO_UTF8(*ActionId));
 if(!A)return FText::GetEmpty();
 FString Text=UTF8_TO_TCHAR(A->label.c_str());
 if(Player->Mission()->State.Done(A->id)) Text+=TEXT(" [sprawdzone]");
 return FText::FromString(Text);
}
void ASliceProp::Interact(ASliceCharacter* Player) {
 const auto* A=Wroclaw::Progress::Find(TCHAR_TO_UTF8(*ActionId));if(!A)return;
 auto* M=Player->Mission();auto* PC=CastChecked<ASliceController>(Player->GetController());
 if(!M->State.Done(A->id) && (!A->answer.empty() || A->kind=="choice")) {PC->OpenKeypad(ActionId);return;}
 const auto R=M->Act(ActionId);
 if(R!=Wroclaw::Result::Applied && R!=Wroclaw::Result::AlreadyDone) return;
 if(R==Wroclaw::Result::Applied) USliceAudio::Play(this,Gate(A->id)?TEXT("Door"):(A->kind=="pickup"?TEXT("Pickup"):TEXT("Switch")),GetActorLocation());
 if(A->id=="peephole") {PC->Peek();return;}
 if(!A->body.empty()) PC->ShowMessage(UTF8_TO_TCHAR(A->body.c_str()));
 Tick(0);
}
void ASliceProp::Tick(float Dt) {
 Super::Tick(Dt);
 const auto* A=Wroclaw::Progress::Find(TCHAR_TO_UTF8(*ActionId));if(!A)return;
 auto* M=GetGameInstance()->GetSubsystem<USliceMission>();
 if(M->State.Done(A->id)) {
  if(A->kind=="pickup"){SetActorHiddenInGame(true);SetActorEnableCollision(false);SetActorTickEnabled(false);}
  if(Gate(A->id) && !bOpened){bOpened=true;Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);SetActorRotation(FRotator(0,90,0));SetActorLocation(ClosedPosition+FVector(-80,-80,0));SetActorTickEnabled(false);}
 }
}
