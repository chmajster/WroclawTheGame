#pragma once
#include "CoreMinimal.h"
#include "Vehicles/DriveableVehicle.h"
#include "CityTrafficVehicle.generated.h"

UCLASS()
class WROCLAWTHEGAME_API ACityTrafficVehicle : public ADriveableVehicle
{
    GENERATED_BODY()
  public:
    ACityTrafficVehicle();
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UVehicleAIDriverComponent> TrafficDriver;
    bool ConfigureTraffic(const TArray<FVector> &Route, int32 StartIndex);
    void DeactivateTraffic();
};
