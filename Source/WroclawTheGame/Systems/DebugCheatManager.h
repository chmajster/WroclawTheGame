#pragma once
#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "DebugCheatManager.generated.h"
UCLASS()
class WROCLAWTHEGAME_API UDebugCheatManager : public UCheatManager
{
    GENERATED_BODY()
  public:
    UFUNCTION(Exec) void CityCoverage();
    UFUNCTION(Exec) void SetHeatLevel(int32 Level);
    UFUNCTION(Exec) void CompleteQuest(const FString &Id);
    UFUNCTION(Exec) void StartQuest(const FString &Id);
    UFUNCTION(Exec) void TeleportToDistrict(const FString &Id);
    UFUNCTION(Exec) void GiveItem(const FString &Id, int32 Count = 1);
    UFUNCTION(Exec) void AddEvidence(const FString &Id);
    UFUNCTION(Exec) void SetTime(float Hour);
    UFUNCTION(Exec) void SetWeather(const FString &Id);
    UFUNCTION(Exec) void ToggleAI();
    UFUNCTION(Exec) void ShowNoiseEvents();
    UFUNCTION(Exec) void ShowAIPerception();

  private:
    class USliceMission *Session();
};
