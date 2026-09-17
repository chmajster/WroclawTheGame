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
 bool bInGame=false,bShowMenu=true,bDead=false,bLastSaveSucceeded=true;
 mutable bool bSaveChecked=false,bHasValidSave=false;
 int32 LifetimeAchievements=0;
 FVector Anchor=FVector(250,400,456);
 UPROPERTY() TMap<FString,TObjectPtr<class USoundBase>> Sounds;
 FString Notification;
 double NotificationUntil=0;
 void NewGame(); bool ContinueGame(); bool SaveCheckpoint(); bool HasSave() const;
 Wroclaw::Result Act(const FString& Id);
 Wroclaw::Result Submit(const FString& Id,const FString& Code);
 bool Threat() const;
 void Notify(const FString& Message);
 FString ObjectiveText() const; FString InventoryText() const;
 FString InvestigationText(int32 Category) const;
 FString PhoneText(int32 Page);
 FString QuestLogText() const;FString HintText() const; FString AchievementsText() const;
 FVector SpawnPoint() const {return Anchor;}
 void UpdateAchievements();
private:
 bool LoadState(bool bApply);
 void After(const FString& Id,Wroclaw::Result Result);
};
