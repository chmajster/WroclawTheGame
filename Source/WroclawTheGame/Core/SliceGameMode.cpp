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
#include "Audio/SliceAudio.h"
#include "Perception/AISense_Hearing.h"
ASliceGameMode::ASliceGameMode(){DefaultPawnClass=ASliceCharacter::StaticClass();PlayerControllerClass=ASliceController::StaticClass();HUDClass=ASliceHUD::StaticClass();PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=.1;}
void ASliceGameMode::SpawnGuard(const FString& Id,const FVector& Position,const TArray<FVector>& Patrol){
 auto* M=GetGameInstance()->GetSubsystem<USliceMission>();
 FActorSpawnParameters P;P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
 auto* G=GetWorld()->SpawnActor<ASliceEnemy>(Position,FRotator(0,-150,0),P);G->GuardId=Id;G->PatrolPoints=Patrol;
 if(M->State.neutralized.count(TCHAR_TO_UTF8(*Id))){G->SetActorHiddenInGame(true);G->SetActorEnableCollision(false);G->Health=0;}
 Guards.Add(G);if(Id==TEXT("pursuer"))Enemy=G;
}
void ASliceGameMode::BeginPlay(){
 Super::BeginPlay();auto* M=GetGameInstance()->GetSubsystem<USliceMission>();GetWorld()->SpawnActor<ASliceWorld>();
 auto* PC=Cast<ASliceController>(UGameplayStatics::GetPlayerController(this,0));
 if(auto* P=Cast<ASliceCharacter>(PC?PC->GetPawn():nullptr)){if(!P->TeleportTo(M->SpawnPoint(),FRotator::ZeroRotator,false,false))M->Notify(TEXT("Punkt zapisu jest zajęty. Gracz pozostaje w bezpiecznym punkcie startowym."));PC->SetControlRotation(FRotator(-10,0,0));}
 SpawnGuard(TEXT("hall"),{2180,620,96},{{2180,620,96},{1940,400,136},{2200,800,96}});
 SpawnGuard(TEXT("courtyard"),{3200,1850,96},{{2900,1950,96},{4100,1480,96},{3700,2040,96}});
 SpawnGuard(TEXT("pursuer"),{7300,2900,96},{{7300,2900,96},{7800,3000,96}});
 if(auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))for(TActorIterator<ANavMeshBoundsVolume> It(GetWorld());It;++It)Nav->OnNavigationBoundsUpdated(*It);
 if(PC && (M->bShowMenu || M->State.Finished()))PC->SetPause(true);
}
bool ASliceGameMode::HasThreat() const {for(ASliceEnemy* G:Guards)if(G && G->IsThreat())return true;return false;}
FString ASliceGameMode::ThreatText() const {for(ASliceEnemy* G:Guards)if(G && G->IsThreat())return G->StatusText();return TEXT("");}
void ASliceGameMode::Tick(float Dt){
 Super::Tick(Dt);auto* M=GetGameInstance()->GetSubsystem<USliceMission>();if(!M->bInGame || M->bDead || M->State.Finished())return;
 M->State.Tick(Dt);auto* P=Cast<ASliceCharacter>(UGameplayStatics::GetPlayerPawn(this,0));if(!P)return;
 for(ASliceEnemy* G:Guards){
  const bool Should=(G->GuardId==TEXT("hall")?M->State.Done("target"):(G->GuardId==TEXT("courtyard")?M->State.Done("apartment_exit"):M->State.Done("usb")));
  if(Should && !G->bActive && G->Health>0){
   G->bActive=true;if(auto* AI=Cast<ASliceEnemyController>(G->GetController())){AI->ResetBrain();if(G==Enemy)AI->Investigate({7100,3250,96});}
   USliceAudio::Play(this,G==Enemy?TEXT("Alarm"):TEXT("Door"),G->GetActorLocation());
  }
 }
 const FVector L=P->GetActorLocation();
 for(const auto& A:Wroclaw::Catalog())if(A.kind=="zone" && !M->State.Done(A.id) && M->State.CanDo(A)){
  if(FVector::Dist2D(L,FVector(A.x,A.y,A.z))<180 && FMath::Abs(L.Z-A.z)<100){
   if(M->Act(UTF8_TO_TCHAR(A.id.c_str()))==Wroclaw::Result::Applied){
    if(!A.body.empty())M->Notify(UTF8_TO_TCHAR(A.body.c_str()));
    if(A.id=="garage_enter"){
     M->Notify(TEXT("Napastnik nadchodzi. LPM atak, PPM blok, ALT unik. Możesz ogłuszyć go lub uciec tyłem."));
     UAISense_Hearing::ReportNoiseEvent(GetWorld(),L,1.5,P,2200);
    }
   }
  }
 }
 if(M->State.Done("garage_escape") && !HasThreat())M->Act(TEXT("lost_pursuit"));
 if(M->State.Finished())if(auto* PC=Cast<ASliceController>(P->GetController()))PC->SetPause(true);
}
