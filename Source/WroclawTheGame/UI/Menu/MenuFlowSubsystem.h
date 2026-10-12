#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "MenuFlowSubsystem.generated.h"

enum class EWTGMenuScreen : uint8 { Boot, Main, Pause, Playing };
UCLASS()
class WROCLAWTHEGAME_API UMenuFlowSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    void Attach(class ASliceController* PlayerController);
    void ShowMain();
    void ShowPause();
    void Resume();
    bool StartNew();
    bool Load(const FString& Slot);
    bool Save();
    void Quit();
    bool IsFrontEnd() const { return Screen==EWTGMenuScreen::Main || Screen==EWTGMenuScreen::Boot; }
    bool IsMenuOpen() const { return Screen!=EWTGMenuScreen::Playing; }
    EWTGMenuScreen Screen=EWTGMenuScreen::Boot;
    bool bHasSession=false;
    FString Notice;
    double PlayedSeconds=0,WalkedCm=0,DrivenCm=0;
    TOptional<FVector> MapWaypoint;
    void ResetTracking(){LastTrackedPawn.Reset();}
    void HideForCreator();
    void RestoreAfterCreator();
    void CreatedSession();
    TArray<FString> SaveSlots() const;
    FString DescribeSave(const FString& Slot, bool& Valid) const;
    class ASliceController* Controller() const { return PC.Get(); }
    class UWorld* MenuWorld() const;
    float TransitionOpacity=1;
    bool IsTransitioning() const { return bool(DeferredOperation); }
    TSharedPtr<class SMenuShell> SlateMenu() const;
    void ApplicationActivationChanged(bool Active);
private:
    UPROPERTY() TObjectPtr<class UFrontEndMenuWidget> Widget;
    UPROPERTY() TObjectPtr<class ACameraActor> Camera;
    TWeakObjectPtr<class ASliceController> PC;
    FTSTicker::FDelegateHandle Ticker;
    bool Tick(float Dt);
    void Open(EWTGMenuScreen Target);
    TWeakObjectPtr<class APawn> LastTrackedPawn;
    FVector LastPosition=FVector::ZeroVector;
    FVector CameraBase,CameraLook;
    double CameraSeconds=0,AudioElapsed=0;
    float AppliedFrameCap=-1,TransitionSeconds=0;
    TFunction<void()> DeferredOperation;
    bool ExecutingTransition=false;
    FDelegateHandle ActivationHandle,LoadHandle;
    void PrepareLoadingScreen(const FString& Map);
    bool Transition(TFunction<void()> Operation);
};
