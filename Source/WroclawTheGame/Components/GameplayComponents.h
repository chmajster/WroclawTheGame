#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayComponents.generated.h"
UCLASS(ClassGroup = (WTG), meta = (BlueprintSpawnableComponent))
class WROCLAWTHEGAME_API UHealthComponent : public UActorComponent
{
    GENERATED_BODY()
  public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float Maximum = 100;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Value = 100;
    float Damage(float Amount)
    {
        if (!FMath::IsFinite(Amount) || Amount <= 0)
            return 0;
        const float Old = Value;
        Value = FMath::Clamp(Value - Amount, 0.f, Maximum);
        return Old - Value;
    }
    void Heal(float Amount)
    {
        if (FMath::IsFinite(Amount) && Amount > 0)
            Value = FMath::Min(Maximum, Value + Amount);
    }
};
UCLASS(ClassGroup = (WTG), meta = (BlueprintSpawnableComponent))
class WROCLAWTHEGAME_API UStaminaComponent : public UActorComponent
{
    GENERATED_BODY()
  public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float Maximum = 100;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Value = 100;
    bool Spend(float Cost)
    {
        if (!FMath::IsFinite(Cost) || Cost < 0 || Value < Cost)
            return false;
        Value -= Cost;
        return true;
    }
    void Recover(float Amount)
    {
        Value = FMath::Clamp(Value + Amount, 0.f, Maximum);
    }
};
UCLASS(ClassGroup = (WTG), meta = (BlueprintSpawnableComponent))
class WROCLAWTHEGAME_API UCombatComponent : public UActorComponent
{
    GENERATED_BODY()
  public:
    UPROPERTY(EditAnywhere) float LightDamage = 24;
    UPROPERTY(EditAnywhere) float HeavyDamage = 42;
    UPROPERTY(EditAnywhere) float LightCost = 18;
    UPROPERTY(EditAnywhere) float HeavyCost = 32;
    UPROPERTY(EditAnywhere) float Range = 145;
    UPROPERTY(VisibleAnywhere) bool bBlocking = false;
    double LastAttack = -100, InvulnerableUntil = 0;
    bool Attack(bool Heavy);
    bool Dodge(const FVector &Direction);
    float Receive(float Damage, AActor *Causer);
};
UCLASS(ClassGroup = (WTG), meta = (BlueprintSpawnableComponent))
class WROCLAWTHEGAME_API UHideableComponent : public UActorComponent
{
    GENERATED_BODY()
  public:
    UPROPERTY(EditAnywhere) FName HideId;
    UPROPERTY(EditAnywhere) FVector ExitLocation = FVector::ZeroVector;
    UPROPERTY(VisibleAnywhere) TWeakObjectPtr<class ASliceCharacter> Occupant;
    bool Enter(class ASliceCharacter *Player);
    bool Leave();
    bool Inspect(AActor *Observer, const FVector &LastKnown, float Suspicion);
};
UCLASS(ClassGroup = (WTG), meta = (BlueprintSpawnableComponent))
class WROCLAWTHEGAME_API UPowerConsumerComponent : public UActorComponent
{
    GENERATED_BODY()
  public:
    UPROPERTY(EditAnywhere) FGameplayTag RequiredPower;
    UPROPERTY(EditAnywhere) FGameplayTag DisabledBy;
    bool Powered() const;
};
UENUM(BlueprintType)
enum class EWTGDoorState : uint8
{
    Unlocked,
    Locked,
    KeyLocked,
    CodeLocked,
    ElectronicLocked,
    QuestLocked,
    Broken
};
UCLASS(ClassGroup = (WTG), meta = (BlueprintSpawnableComponent))
class WROCLAWTHEGAME_API UDoorComponent : public UActorComponent
{
    GENERATED_BODY()
  public:
    UPROPERTY(EditAnywhere) EWTGDoorState State = EWTGDoorState::QuestLocked;
    UPROPERTY(VisibleAnywhere) bool bOpen = false;
    bool SetOpen(bool Open, bool Authorized);
};
UCLASS(ClassGroup = (WTG), meta = (BlueprintSpawnableComponent))
class WROCLAWTHEGAME_API UBasePuzzleComponent : public UActorComponent
{
    GENERATED_BODY()
  public:
    UPROPERTY(EditAnywhere) FName DefinitionId;
    bool Submit(const FString &Answer);
    FString Hint() const;
};
UCLASS(ClassGroup = (WTG), meta = (BlueprintSpawnableComponent))
class WROCLAWTHEGAME_API UInventoryComponent : public UActorComponent
{
    GENERATED_BODY()
  public:
    bool Has(FName Item) const;
    bool Use(FName Item);
    bool HasTag(FGameplayTag Tag) const;
};
UCLASS(ClassGroup = (WTG), meta = (BlueprintSpawnableComponent))
class WROCLAWTHEGAME_API UInteractionComponent : public UActorComponent
{
    GENERATED_BODY()
  public:
    UPROPERTY(EditAnywhere) float Range = 230;
    AActor *Find(const FVector &CameraPosition, const FVector &Direction) const;
    bool Interact(AActor *Target);
};
UCLASS(ClassGroup = (WTG), meta = (BlueprintSpawnableComponent))
class WROCLAWTHEGAME_API UFactionComponent : public UActorComponent
{
    GENERATED_BODY()
  public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName FactionId;
    FString RelationTo(const UFactionComponent *Other) const;
};
