#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VehicleInteractionInterface.generated.h"
class APawn;
// Parking/storage actors and future driveable pawns share this capability contract.
UINTERFACE(MinimalAPI, Blueprintable)
class UVehicleInteractionInterface : public UInterface
{
    GENERATED_BODY()
};
class WROCLAWTHEGAME_API IVehicleInteractionInterface
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable) bool CanEnterVehicle(APawn *Passenger) const;
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable) bool OpenStorage(APawn *User);
};
