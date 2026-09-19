#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VehicleAIDriverComponent.generated.h"

UCLASS(ClassGroup=(Wroclaw), meta=(BlueprintSpawnableComponent))
class WROCLAWTHEGAME_API UVehicleAIDriverComponent : public UActorComponent
{
    GENERATED_BODY()
  public:
    UVehicleAIDriverComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction *ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category="Wroclaw|VehicleAI")
    bool ConfigureRoute(const TArray<FVector> &Points, int32 StartIndex = 0, bool bLoop = true);
    UFUNCTION(BlueprintCallable, Category="Wroclaw|VehicleAI")
    void StopAI();

    bool IsActive() const { return bActive; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wroclaw|VehicleAI")
    float CruiseSpeed = 1200.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wroclaw|VehicleAI")
    float WaypointRadius = 260.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wroclaw|VehicleAI")
    float ObstacleLookAhead = 700.0f;

  private:
    UPROPERTY() TObjectPtr<class ADriveableVehicle> Vehicle;
    TArray<FVector> Route;
    int32 TargetIndex = 0;
    bool bLoopRoute = true;
    bool bActive = false;
};
