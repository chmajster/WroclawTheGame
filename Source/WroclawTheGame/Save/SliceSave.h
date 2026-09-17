#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SliceSave.generated.h"
UCLASS()
class WROCLAWTHEGAME_API USliceSave : public USaveGame {
 GENERATED_BODY()
public:
 UPROPERTY(SaveGame) int32 Version = 1;
 UPROPERTY(SaveGame) int32 Objectives = 0;
 UPROPERTY(SaveGame) int32 Items = 0;
 UPROPERTY(SaveGame) bool PhoneUnlocked = false;
 UPROPERTY(SaveGame) bool PursuitStarted = false;
 UPROPERTY(SaveGame) int32 Checkpoint = 0;
};
