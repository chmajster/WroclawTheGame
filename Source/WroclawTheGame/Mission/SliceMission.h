#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/SliceProgress.h"
#include "SliceMission.generated.h"
UCLASS()
class WROCLAWTHEGAME_API USliceMission : public UGameInstanceSubsystem {
 GENERATED_BODY()
public:
 Wroclaw::Progress State;
 bool bInGame = false;
 bool bShowMenu = true;
 bool bDead = false;
 int32 Checkpoint = 0;
 bool bLastSaveSucceeded = true;
 mutable bool bSaveChecked = false;
 mutable bool bHasValidSave = false;
 UPROPERTY() TMap<FString,TObjectPtr<class USoundBase>> Sounds;
 FString Notification;
 double NotificationUntil = 0;
 void NewGame();
 bool ContinueGame();
 bool SaveCheckpoint(int32 Index);
 bool HasSave() const;
 bool Apply(Wroclaw::Event Event);
 void Notify(const FString& Message);
 FString ObjectiveText() const;
 FString InventoryText() const;
 FVector SpawnPoint() const;
private:
 bool LoadState(bool bApply);
};
