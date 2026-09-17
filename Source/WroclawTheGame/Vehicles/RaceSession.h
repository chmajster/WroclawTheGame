#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "GameFramework/SaveGame.h"
#include "Framework/RaceProgress.h"
#include "RaceSession.generated.h"
UCLASS(BlueprintType)
class WROCLAWTHEGAME_API URaceDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
  public:
    UPROPERTY(EditAnywhere) FName RaceId;
    UPROPERTY(EditAnywhere) FText Title;
    UPROPERTY(EditAnywhere) TArray<FVector> Checkpoints;
    UPROPERTY(EditAnywhere) FTransform Start;
    UPROPERTY(EditAnywhere) float TimeLimit = 120;
    UPROPERTY(EditAnywhere) bool bDelivery = false;
};
UCLASS()
class WROCLAWTHEGAME_API URaceRecords : public USaveGame
{
    GENERATED_BODY()
  public:
    UPROPERTY(SaveGame) int32 Version = 1;
    UPROPERTY(SaveGame) TMap<FName, float> BestTimes;
};
UCLASS(ClassGroup = (Wroclaw), meta = (BlueprintSpawnableComponent))
class WROCLAWTHEGAME_API URaceSession : public UActorComponent
{
    GENERATED_BODY()
  public:
    URaceSession();
    virtual void BeginPlay() override;
    virtual void TickComponent(float Dt, ELevelTick Tick, FActorComponentTickFunction *Function) override;
    UPROPERTY(EditAnywhere) TArray<TObjectPtr<URaceDefinition>> Definitions;
    UPROPERTY() TObjectPtr<URaceRecords> Records;
    int32 Selected = 0;
    Wroclaw::RaceProgress Progress;
    bool Start(int32 Index);
    FString StatusText() const;

  private:
    float LastHealth = 100;
    bool bAwaitingStreaming = false;
    bool bRecordSaved = true;
};
