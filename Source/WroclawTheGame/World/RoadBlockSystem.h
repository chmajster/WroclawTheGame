#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "RoadBlockSystem.generated.h"

UCLASS()
class WROCLAWTHEGAME_API ACityRoadBlock : public AActor
{
    GENERATED_BODY()
  public:
    ACityRoadBlock();
  private:
    UPROPERTY() TObjectPtr<class USceneComponent> Root;
    UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> Barriers;
};

UCLASS()
class WROCLAWTHEGAME_API URoadBlockSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, Category="Wroclaw|RoadBlock")
    bool UpdateForHeat(int32 HeatLevel, class ADriveableVehicle *PlayerVehicle,
                       class ACityPopulation *Population);
    UFUNCTION(BlueprintCallable, Category="Wroclaw|RoadBlock")
    void ClearRoadBlock();
    UFUNCTION(BlueprintPure, Category="Wroclaw|RoadBlock")
    bool HasRoadBlock() const { return ActiveRoadBlock != nullptr; }

  private:
    UPROPERTY() TObjectPtr<ACityRoadBlock> ActiveRoadBlock;
    FVector LastSpawnPoint = FVector::ZeroVector;
};
