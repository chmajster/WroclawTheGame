#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Character/CharacterAppearanceDefinition.h"
#include "SliceSave.generated.h"
USTRUCT()
struct FWTGNPCSnapshot
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FTransform Transform;
    UPROPERTY(SaveGame) float Health = 100;
};
UCLASS()
class WROCLAWTHEGAME_API USliceSave : public USaveGame
{
    GENERATED_BODY()
  public:
    UPROPERTY(SaveGame) int32 Version = 3;
    UPROPERTY(SaveGame) FString CoordinateSpace = TEXT("BlockoutV1");
    UPROPERTY(SaveGame) FCharacterCustomizationSaveData CharacterCustomization;
    UPROPERTY(SaveGame) int32 Variant = 0;
    UPROPERTY(SaveGame) FString WorldPayload;
    UPROPERTY(SaveGame) TMap<FString, FWTGNPCSnapshot> NPCs;
    UPROPERTY(SaveGame) TMap<FString, int32> UsedItems;
    UPROPERTY(SaveGame) TMap<FString, FString> Settings;
    UPROPERTY(SaveGame) TArray<FString> History;
    UPROPERTY(SaveGame) TArray<FString> Timeline;
    UPROPERTY(SaveGame) TMap<FString, double> ObjectiveCounters;
    UPROPERTY(SaveGame) TArray<FString> Neutralized;
    UPROPERTY(SaveGame) int32 Kills = 0;
    UPROPERTY(SaveGame) int32 MedkitsUsed = 0;
    UPROPERTY(SaveGame) int32 DistractionsUsed = 0;
    UPROPERTY(SaveGame) bool CourtyardDetected = false;
    UPROPERTY(SaveGame) bool GarageUnderThreat = false;
    UPROPERTY(SaveGame) double Elapsed = 0;
    UPROPERTY(SaveGame) double QuestSince = 0;
    UPROPERTY(SaveGame) FVector Anchor = FVector(250, 400, 456);
    UPROPERTY(SaveGame) TMap<FString, int32> Failures;
    UPROPERTY(SaveGame) TMap<FString, double> LockUntil;
};
UCLASS()
class WROCLAWTHEGAME_API USliceProfile : public USaveGame
{
    GENERATED_BODY()
  public:
    UPROPERTY(SaveGame) int32 Achievements = 0;
};
