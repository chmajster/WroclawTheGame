#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CityPopulation.generated.h"
USTRUCT(BlueprintType)
struct FCityPopulationRoute
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) FString Id;
    UPROPERTY(EditAnywhere) bool Vehicle = false;
    UPROPERTY(EditAnywhere) TArray<FVector> Points;
};
UCLASS()
class WROCLAWTHEGAME_API ACityAmbientAgent : public AActor
{
    GENERATED_BODY()
  public:
    ACityAmbientAgent();
    UPROPERTY() TObjectPtr<class UBoxComponent> Body;
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> Visual;
    UPROPERTY() TObjectPtr<class USkeletalMeshComponent> PedestrianVisual;
    UPROPERTY() TArray<FVector> Route;
    int32 Target = 1;
    bool bVehicle = false;
    virtual void Tick(float DeltaTime) override;
    void Configure(const FCityPopulationRoute &Definition, int32 StartIndex, class UStaticMesh *VehicleMesh, class USkeletalMesh *PedestrianMesh);
};
UCLASS()
class WROCLAWTHEGAME_API ACityPopulation : public AActor
{
    GENERATED_BODY()
  public:
    ACityPopulation();
    UPROPERTY(EditAnywhere) TArray<FCityPopulationRoute> Routes;
    UPROPERTY(EditAnywhere) TObjectPtr<class UStaticMesh> VehicleMesh;
    UPROPERTY(EditAnywhere) TObjectPtr<class USkeletalMesh> PedestrianMesh;
    virtual void Tick(float DeltaTime) override;
  private:
    UPROPERTY() TArray<TObjectPtr<ACityAmbientAgent>> Pool;
    TArray<int32> AssignedRoutes;
};
