#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "GameplayEventBus.generated.h"
USTRUCT(BlueprintType)
struct FWTGGameplayEvent
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FGameplayTag Type;
    UPROPERTY(BlueprintReadOnly) FName Subject;
    UPROPERTY(BlueprintReadOnly) FVector Location = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) float Value = 0;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<AActor> Source = nullptr;
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWTGEventDelegate, const FWTGGameplayEvent &, Event);
UCLASS()
class WROCLAWTHEGAME_API UGameplayEventBus : public UWorldSubsystem
{
    GENERATED_BODY()
  public:
    UPROPERTY(BlueprintAssignable) FWTGEventDelegate OnEvent;
    void Emit(const TCHAR *Type, FName Subject, float Value = 0, FVector Location = FVector::ZeroVector,
              AActor *Source = nullptr);
};
