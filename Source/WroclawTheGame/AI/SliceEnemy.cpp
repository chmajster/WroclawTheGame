#include "AI/SliceEnemy.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Engine/DamageEvents.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Character/SliceCharacter.h"
#include "Mission/SliceMission.h"
#include "Kismet/GameplayStatics.h"
#include "Audio/SliceAudio.h"
ASliceEnemy::ASliceEnemy() {
 AIControllerClass=ASliceEnemyController::StaticClass(); AutoPossessAI=EAutoPossessAI::PlacedInWorldOrSpawned;
 GetCapsuleComponent()->InitCapsuleSize(34,90);
 GetCharacterMovement()->MaxStepHeight=90;GetCharacterMovement()->MaxWalkSpeed=210; GetCharacterMovement()->bOrientRotationToMovement=true;
 bUseControllerRotationYaw=false;
 auto* Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProxyBody"));Body->SetupAttachment(RootComponent);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 Body->SetStaticMesh(Cube.Object);Body->SetRelativeScale3D(FVector(0.45,0.5,1.75));Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);Body->SetCanEverAffectNavigation(false);
}
void ASliceEnemy::BeginPlay() {
 Super::BeginPlay();
 if(auto* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Generated/M_Enemy.M_Enemy"))) FindComponentByClass<UStaticMeshComponent>()->SetMaterial(0,M);
}
float ASliceEnemy::TakeDamage(float Damage,const FDamageEvent& Event,AController* Instigator,AActor* Causer) {
 if(Health<=0 || !bActive) return 0;
 Health=FMath::Max(0.f,Health-Damage);
 if(Health==0) {
  if(auto* AI=Cast<ASliceEnemyController>(GetController())) {AI->StopMovement();AI->State=EEnemyState::Defeated;}
  GetCharacterMovement()->DisableMovement(); GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  SetActorRotation(FRotator(0,GetActorRotation().Yaw,90));
  auto* M=GetGameInstance()->GetSubsystem<USliceMission>(); M->State.neutralized.insert(TCHAR_TO_UTF8(*GuardId));M->Notify(TEXT("Napastnik ogłuszony. Możesz uciekać."));
 }
 return Damage;
}
bool ASliceEnemy::IsThreat() const {
 auto* AI=Cast<ASliceEnemyController>(GetController());
 return bActive && Health>0 && AI && AI->State!=EEnemyState::Patrol && AI->State!=EEnemyState::ReturnToPatrol;
}
FString ASliceEnemy::StatusText() const {
 if(!bActive || Health<=0) return TEXT("");
 auto* AI=Cast<ASliceEnemyController>(GetController()); if(!AI) return TEXT("");
 const TCHAR* Labels[]={TEXT("PATROL"),TEXT("PODEJRZENIE"),TEXT("POŚCIG — ZERWIJ KONTAKT"),TEXT("ATAK"),TEXT("ZGUBIŁ CIĘ — UKRYJ SIĘ"),TEXT("PRZESZUKIWANIE OKOLICY"),TEXT("POŚCIG ZAKOŃCZONY"),TEXT("NAPASTNIK OGŁUSZONY")};
 return Labels[static_cast<int32>(AI->State)];
}
ASliceEnemyController::ASliceEnemyController() {
 PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.TickInterval=0.15f;
 Senses=CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception")); SetPerceptionComponent(*Senses);
 Sight=CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("Sight"));
 Sight->SightRadius=1800; Sight->LoseSightRadius=2100; Sight->PeripheralVisionAngleDegrees=65;Sight->SetMaxAge(3);
 Sight->DetectionByAffiliation.bDetectEnemies=true;Sight->DetectionByAffiliation.bDetectFriendlies=true;Sight->DetectionByAffiliation.bDetectNeutrals=true;
 Hearing=CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("Hearing")); Hearing->HearingRange=1700;Hearing->SetMaxAge(4);
 Hearing->DetectionByAffiliation=Sight->DetectionByAffiliation;
 Senses->ConfigureSense(*Sight);Senses->ConfigureSense(*Hearing);Senses->SetDominantSense(Sight->GetSenseImplementation());
}
void ASliceEnemyController::OnPossess(APawn* Pawn) {Super::OnPossess(Pawn);Senses->OnTargetPerceptionUpdated.AddDynamic(this,&ASliceEnemyController::Perceived);ResetBrain();}
void ASliceEnemyController::ResetBrain(){State=EEnemyState::Patrol;bSees=false;LastSeen=-100;LastMove=-100;StateSince=0;StopMovement();Senses->ForgetAll();}
void ASliceEnemyController::Investigate(const FVector& Location){LastKnown=Location;Transition(EEnemyState::Suspicious);}
void ASliceEnemyController::Transition(EEnemyState Next){if(State!=Next){State=Next;StateSince=GetWorld()->GetTimeSeconds();LastMove=-100;}}
void ASliceEnemyController::Perceived(AActor* Actor,FAIStimulus Stimulus) {
 auto* Enemy=Cast<ASliceEnemy>(GetPawn()); auto* Player=Cast<ASliceCharacter>(Actor);
 if(!Enemy || !Enemy->bActive || Enemy->Health<=0 || !Player) return;
 if(Stimulus.Type==UAISense::GetSenseID<UAISense_Sight>()) {
  bSees=Stimulus.WasSuccessfullySensed();
  if(bSees){ LastKnown=Player->GetActorLocation();LastSeen=GetWorld()->GetTimeSeconds();Transition(EEnemyState::Chase);if(Enemy->GuardId==TEXT("courtyard") && !Player->Mission()->State.Done("street")) Player->Mission()->State.courtyardDetected=true; }
 } else if(Stimulus.WasSuccessfullySensed() && !bSees) {
  LastKnown=Stimulus.StimulusLocation;
  if(State==EEnemyState::Patrol || State==EEnemyState::ReturnToPatrol) Transition(EEnemyState::Suspicious);
 }
}
void ASliceEnemyController::Tick(float Dt) {
 Super::Tick(Dt);
 auto* Enemy=Cast<ASliceEnemy>(GetPawn()); auto* Player=Cast<ASliceCharacter>(UGameplayStatics::GetPlayerPawn(this,0));
 if(!Enemy || !Player || !Enemy->bActive || Enemy->Health<=0 || Player->Mission()->State.Finished()) return;
 const double Now=GetWorld()->GetTimeSeconds();
 const float Distance=FVector::Dist(Enemy->GetActorLocation(),Player->GetActorLocation());
 if(bSees && LineOfSightTo(Player)) {LastKnown=Player->GetActorLocation();LastSeen=Now;Transition(Distance<155?EEnemyState::Attack:EEnemyState::Chase);}
 else if((State==EEnemyState::Chase || State==EEnemyState::Attack) && Now-LastSeen>1.8) {bSees=false;Transition(EEnemyState::LostPlayer);}
 FVector Goal=LastKnown;
 Enemy->GetCharacterMovement()->MaxWalkSpeed=(State==EEnemyState::Chase)?400:200;
 switch(State) {
 case EEnemyState::Attack:
  StopMovement(); SetFocus(Player);
  if(Distance<165 && LineOfSightTo(Player) && Now-LastAttack>1.1) {LastAttack=Now;UGameplayStatics::ApplyDamage(Player,22,this,Enemy,nullptr);USliceAudio::Play(this,TEXT("Hit"),Player->GetActorLocation());} break;
 case EEnemyState::LostPlayer: ClearFocus(EAIFocusPriority::Gameplay);Transition(EEnemyState::Search); break;
 case EEnemyState::Search:
  if(Now-StateSince>9 && FVector::Dist(Enemy->GetActorLocation(),LastKnown)<170) {
   Transition(EEnemyState::ReturnToPatrol);
  } else if(Now-StateSince>16) {Transition(EEnemyState::ReturnToPatrol);}
  break;
 case EEnemyState::Suspicious: if(Now-StateSince>5) Transition(EEnemyState::Search);break;
 case EEnemyState::ReturnToPatrol: Transition(EEnemyState::Patrol);break;
 case EEnemyState::Patrol: {
  if(Enemy->PatrolPoints.IsEmpty()) return;
  PatrolIndex%=Enemy->PatrolPoints.Num();Goal=Enemy->PatrolPoints[PatrolIndex];
  if(FVector::Dist2D(Enemy->GetActorLocation(),Goal)<110) PatrolIndex=(PatrolIndex+1)%Enemy->PatrolPoints.Num();
  break;
 }
 default:break;
 }
 if(State!=EEnemyState::Attack && Now-LastMove>0.5) {ClearFocus(EAIFocusPriority::Gameplay);MoveToLocation(Goal,70,true,true,true,false);LastMove=Now;}
}
