#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CityPopulation.generated.h"

UENUM(BlueprintType)
enum class ECityAgentSimulationLevel : uint8
{
    Full,
    Simplified,
    Dormant
};

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
    UPROPERTY() TArray<FVector> Route;
    int32 Target = 1;
    bool bVehicle = false;
    virtual void Tick(float DeltaTime) override;
    void Configure(const FCityPopulationRoute &Definition, int32 StartIndex);
    void SetSimulationLevel(ECityAgentSimulationLevel Level);
    ECityAgentSimulationLevel GetSimulationLevel() const { return SimulationLevel; }

  private:
    ECityAgentSimulationLevel SimulationLevel = ECityAgentSimulationLevel::Dormant;
    float CurrentSpeed = 0.0f;
};

UCLASS()
class WROCLAWTHEGAME_API ACityPopulation : public AActor
{
    GENERATED_BODY()
  public:
    ACityPopulation();
    UPROPERTY(EditAnywhere) TArray<FCityPopulationRoute> Routes;
    UPROPERTY(EditAnywhere, Category = "Population", meta = (ClampMin = "1000"))
    float FullSimulationRadius = 8000.0f;
    UPROPERTY(EditAnywhere, Category = "Population", meta = (ClampMin = "1000"))
    float SimplifiedSimulationRadius = 22000.0f;
    UPROPERTY(EditAnywhere, Category = "Population", meta = (ClampMin = "1", ClampMax = "128"))
    int32 MaxAgents = 24;
    UPROPERTY(EditAnywhere, Category = "Population", meta = (ClampMin = "0", ClampMax = "24"))
    int32 MaxTrafficVehicles = 6;
    UPROPERTY(EditAnywhere, Category = "Population", meta = (ClampMin = "1000"))
    float TrafficActivationRadius = 12000.0f;
    virtual void Tick(float DeltaTime) override;

  private:
    UPROPERTY() TArray<TObjectPtr<ACityAmbientAgent>> Pool;
    TArray<int32> AssignedRoutes;
    UPROPERTY() TArray<TObjectPtr<class ACityTrafficVehicle>> TrafficPool;
    TArray<int32> AssignedTrafficRoutes;
};
