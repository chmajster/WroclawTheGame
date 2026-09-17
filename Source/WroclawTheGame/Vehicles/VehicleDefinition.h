#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VehicleDefinition.generated.h"
UCLASS(BlueprintType)
class WROCLAWTHEGAME_API UVehicleDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
  public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName VehicleId = TEXT("sedan");
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float MassKg = 1400;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float Acceleration = 450;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float BrakeDeceleration = 950;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float MaxSpeed = 2200;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float ReverseSpeed = 500;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float Grip = 5;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float MaxHealth = 100;
};
