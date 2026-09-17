#include "Core/SliceGameMode.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Character/SliceCharacter.h"
#include "World/SliceWorld.h"
#include "UI/SliceController.h"
#include "UI/SliceHUD.h"
#include "Mission/SliceMission.h"
#include "AI/SliceEnemy.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "EngineUtils.h"
ASliceGameMode::ASliceGameMode() {
 DefaultPawnClass=ASliceCharacter::StaticClass();PlayerControllerClass=ASliceController::StaticClass();HUDClass=ASliceHUD::StaticClass();
 PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=0.1f;
}
void ASliceGameMode::BeginPlay() {
 Super::BeginPlay();
 auto* M=GetGameInstance()->GetSubsystem<USliceMission>();
 GetWorld()->SpawnActor<ASliceWorld>();
 auto* PC=Cast<ASliceController>(UGameplayStatics::GetPlayerController(this,0));
 if(auto* P=Cast<ASliceCharacter>(PC?PC->GetPawn():nullptr)) {P->SetActorLocation(M->SpawnPoint(),false,nullptr,ETeleportType::TeleportPhysics);PC->SetControlRotation(FRotator(-10,0,0));}
 FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
 Enemy=GetWorld()->SpawnActor<ASliceEnemy>(FVector(3300,2450,96),FRotator(0,-155,0),Params);
 Enemy->bActive=M->State.Complete(10) && !M->State.Finished();
 if(auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
  for(TActorIterator<ANavMeshBoundsVolume> It(GetWorld());It;++It) Nav->OnNavigationBoundsUpdated(*It);
 if(PC && (M->bShowMenu || M->State.Finished())) PC->SetPause(true);
}
void ASliceGameMode::Tick(float Dt) {
 Super::Tick(Dt);auto* M=GetGameInstance()->GetSubsystem<USliceMission>();
 if(!M->bInGame || M->bDead || M->State.Finished()) return;
 auto* P=Cast<ASliceCharacter>(UGameplayStatics::GetPlayerPawn(this,0));if(!P) return;
 const FVector L=P->GetActorLocation();
 if(L.X>1240 && L.X<1500 && L.Y>300 && L.Y<520) M->Apply(Wroclaw::Event::ExitApartment);
 if(L.X>2080 && L.X<2400 && L.Y>300 && L.Y<900 && L.Z<150) M->Apply(Wroclaw::Event::GroundFloor);
 if(L.Y>2110 && L.Y<3200 && L.X>1800 && L.X<5200 && M->Apply(Wroclaw::Event::Street)) {
  Enemy->bActive=true;
  if(auto* AI=Cast<ASliceEnemyController>(Enemy->GetController())) AI->ResetBrain();
  M->Notify(TEXT("Napastnik jest na ulicy. Biegnij do zaułka przy niebieskim szyldzie. Nie prowadź go do lokalu."));
 }
 if(L.Y>4590 && L.Y<5300 && L.X>4000 && L.X<5200 && !Enemy->IsThreat() && M->Apply(Wroclaw::Event::ReachSafe))
  if(auto* PC=Cast<ASliceController>(P->GetController())) PC->SetPause(true);
}
