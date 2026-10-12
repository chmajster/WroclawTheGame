#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Systems/WTGEconomySubsystem.h"
#include "Character/CharacterAppearanceDefinition.h"
#include "DistrictSaveGame.generated.h"

UCLASS()
class WROCLAWTHEGAME_API UDistrictSaveGame : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) int32 Version=1;
    UPROPERTY(SaveGame) FString DistrictId;
    UPROPERTY(SaveGame) FVector PlayerPosition;
    UPROPERTY(SaveGame) float PlayerYaw=0;
    UPROPERTY(SaveGame) float PlayerHealth=100;
    UPROPERTY(SaveGame) FVector CarPosition;
    UPROPERTY(SaveGame) float CarYaw=0;
    UPROPERTY(SaveGame) float CarHealth=100;
    UPROPERTY(SaveGame) bool bInVehicle=false;
    UPROPERTY(SaveGame) FWTGEconomySaveState Economy;
    UPROPERTY(SaveGame) uint8 DeliveryStage=0;
    UPROPERTY(SaveGame) float Remaining=420;
    UPROPERTY(SaveGame) uint32 Attempts=0;
    UPROPERTY(SaveGame) FString FailureReason;
    UPROPERTY(SaveGame) float WantedHeat=0;
    UPROPERTY(SaveGame) FCharacterCustomizationSaveData Appearance;
    bool Validate(const FVector& BoundsMin,const FVector& BoundsMax,const FString& ExpectedId) const;
};
