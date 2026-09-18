#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "Framework/CityProgress.h"
#include "Character/CharacterAppearanceDefinition.h"
#include "CityGameplaySubsystem.generated.h"

UCLASS()
class WROCLAWTHEGAME_API UCityProgressSave : public USaveGame
{
    GENERATED_BODY()
  public:
    UPROPERTY(SaveGame) int32 Version = 1;
    UPROPERTY(SaveGame) FCharacterCustomizationSaveData CharacterCustomization;
    UPROPERTY(SaveGame) FString Payload;
    UPROPERTY(SaveGame) FVector Anchor = FVector::ZeroVector;
    UPROPERTY(SaveGame) bool HasVehicle = false;
    UPROPERTY(SaveGame) FVector VehicleAnchor = FVector::ZeroVector;
    UPROPERTY(SaveGame) float VehicleYaw = 0;
    UPROPERTY(SaveGame) float VehicleHealth = 100;
};

UCLASS()
class WROCLAWTHEGAME_API UCityGameplaySubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
  public:
    void Activate(const FVector &DefaultSpawn);
    bool IsActive() const { return bActive; }
    bool IsWriteBlocked() const { return bWriteBlocked; }
    FVector SpawnPoint() const { return SavedAnchor; }
    void Register(class ACityActivity *Actor);
    void Travel(class ASliceCharacter *Player, const FVector &Destination);
    void Interact(class ACityActivity *Actor, class ASliceCharacter *Player);
    bool LoadSaved();
    bool NewRun();
    bool SaveAppearance() { return Persist(SavedAnchor); }
    void RestoreVehicle(class ADriveableVehicle *Vehicle);
    bool IsComplete(const FString &Id) const;
    int32 CompletedActivityCount() const;
    int32 TrackableActivityCount() const;
    int32 CompletedEventCount() const;
    int32 EventCount() const;
    int32 AvailableActivityCount() const;
    FString Journal() const;
    FString NearbyObjective() const;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override
    { RETURN_QUICK_DECLARE_CYCLE_STAT(UCityGameplaySubsystem, STATGROUP_Tickables); }
    virtual bool DoesSupportWorldType(EWorldType::Type Type) const override
    { return Type == EWorldType::Game || Type == EWorldType::PIE; }
  private:
    TWeakObjectPtr<class ASliceCharacter> Travelling;
    FVector TravelTarget, TravelOrigin;
    double TravelStarted = 0;
    bool bReturningToOrigin = false;
    Wroclaw::CityProgress Progress;
    TArray<TWeakObjectPtr<class ACityActivity>> Activities;
    FVector InitialAnchor = FVector::ZeroVector, SavedAnchor = FVector::ZeroVector;
    UPROPERTY() TObjectPtr<UCityProgressSave> LoadedSave;
    bool bActive = false, bWriteBlocked = false;
    double NextSaveAttempt = 0;
    double NextSecurityUpdate = 0;
    bool Persist(const FVector &Anchor, bool IncludeVehicle = true);
    void Apply(class ACityActivity *Actor, class APawn *Player, float DeltaTime, bool bInteract);
};
