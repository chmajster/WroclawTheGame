#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/CityPopulation.h"
#include "DistrictRegistry.generated.h"

USTRUCT(BlueprintType)
struct FWTGDistrictStreet
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) FString Name;
    UPROPERTY(EditAnywhere) TArray<FVector> Points;
    UPROPERTY(EditAnywhere) bool bWalkway = false;
};

// Authored by the source GIS generator. No binary map is the source of truth.
UCLASS()
class WROCLAWTHEGAME_API AWTGDistrictRegistry : public AActor
{
    GENERATED_BODY()
public:
    AWTGDistrictRegistry();
    FRotator StartViewRotation() const
    {
        const FVector Direction = MeetingPoint - PlayerStart;
        return FRotator(-12.f, Direction.IsNearlyZero() ? 0.f : Direction.Rotation().Yaw, 0.f);
    }
    UPROPERTY(EditAnywhere) FString DistrictId = TEXT("nadodrze_odra");
    UPROPERTY(EditAnywhere) FString Title = TEXT("Nadodrze — przesyłka znad Odry");
    UPROPERTY(EditAnywhere) FVector MeetingPoint;
    UPROPERTY(EditAnywhere) FVector DeliveryPoint;
    UPROPERTY(EditAnywhere) FVector VehicleSpawn;
    UPROPERTY(EditAnywhere) FVector PlayerStart;
    UPROPERTY(EditAnywhere) float VehicleYaw = 0;
    UPROPERTY(EditAnywhere) FVector BoundsMin;
    UPROPERTY(EditAnywhere) FVector BoundsMax;
    UPROPERTY(EditAnywhere) TArray<FVector> DeliveryRoute;
    UPROPERTY(EditAnywhere) TArray<FVector> Intersections;
    UPROPERTY(EditAnywhere) TArray<FWTGDistrictStreet> Streets;
    UPROPERTY(EditAnywhere) TArray<FCityPopulationRoute> PopulationRoutes;
    UPROPERTY(EditAnywhere) int64 DeliveryRewardCents = 18000;
    UPROPERTY(EditAnywhere) float DeliveryTimeLimit = 420;
    UPROPERTY(EditAnywhere) TObjectPtr<class ADriveableVehicle> DeliveryVehicle;
    UPROPERTY(EditAnywhere) TObjectPtr<class ACityPopulation> Population;
};
