#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "SliceEnemy.generated.h"
UENUM()
enum class EEnemyState : uint8
{
    Patrol,
    Suspicious,
    Chase,
    Attack,
    LostPlayer,
    Search,
    ReturnToPatrol,
    Defeated
};
UCLASS()
class WROCLAWTHEGAME_API ASliceEnemy : public ACharacter
{
    GENERATED_BODY()
  public:
    ASliceEnemy();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual float TakeDamage(float Damage, const FDamageEvent &Event, AController *EventInstigator,
                             AActor *Causer) override;
    FString StatusText() const;
    bool IsThreat() const;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString GuardId;
    UPROPERTY() TArray<FVector> PatrolPoints;
    bool bActive = false;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UHealthComponent> HealthState;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaminaComponent> StaminaState;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UCombatComponent> Combat;
};
UCLASS()
class WROCLAWTHEGAME_API ASliceEnemyController : public AAIController
{
    GENERATED_BODY()
  public:
    ASliceEnemyController();
    virtual void Tick(float DeltaSeconds) override;
    virtual void OnPossess(APawn *InPawn) override;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UAIPerceptionComponent> Senses;
    UPROPERTY() EEnemyState State = EEnemyState::Patrol;
    void ResetBrain();
    void Investigate(const FVector &Location);

  private:
    UFUNCTION() void Perceived(AActor *Actor, FAIStimulus Stimulus);
    UPROPERTY() TObjectPtr<class UAISenseConfig_Sight> Sight;
    UPROPERTY() TObjectPtr<class UAISenseConfig_Hearing> Hearing;
    bool bSees = false, bVisualCandidate = false, bRadioSent = false;
    double AlertSince = -1;
    FVector LastKnown = FVector::ZeroVector;
    double LastSeen = -100, StateSince = 0, LastAttack = -100, LastMove = -100;
    int32 PatrolIndex = 0;
    void Transition(EEnemyState Next);
};
