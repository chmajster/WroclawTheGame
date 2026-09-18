#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "VehiclePursuitSubsystem.generated.h"

UENUM(BlueprintType)
enum class EVehiclePursuitState : uint8
{
    Inactive,
    Locate,
    Chase,
    Search,
    Lost
};

UCLASS()
class WROCLAWTHEGAME_API UVehiclePursuitSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
  public:
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UVehiclePursuitSubsystem, STATGROUP_Tickables);
    }
    virtual bool DoesSupportWorldType(EWorldType::Type Type) const override
    {
        return Type == EWorldType::Game || Type == EWorldType::PIE;
    }

    UFUNCTION(BlueprintCallable, Category="Wroclaw|VehiclePursuit")
    bool StartPursuit(int32 PursuerCount = 2);
    UFUNCTION(BlueprintCallable, Category="Wroclaw|VehiclePursuit")
    void StopPursuit();
    UFUNCTION(BlueprintPure, Category="Wroclaw|VehiclePursuit")
    EVehiclePursuitState GetState() const { return State; }
    UFUNCTION(BlueprintPure, Category="Wroclaw|VehiclePursuit")
    int32 ActivePursuerCount() const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wroclaw|VehiclePursuit")
    float LoseSightSeconds = 2.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wroclaw|VehiclePursuit")
    float SearchSeconds = 12.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wroclaw|VehiclePursuit")
    float MaxVisualDistance = 14000.0f;

  private:
    UPROPERTY() TArray<TObjectPtr<class ACityTrafficVehicle>> Pursuers;
    EVehiclePursuitState State = EVehiclePursuitState::Inactive;
    FVector LastKnown = FVector::ZeroVector;
    double LastSeenAt = -1000.0;
    float Accumulator = 0.0f;
    void EnterLost();
    bool HasVisualContact(class ACityTrafficVehicle *Pursuer, class ADriveableVehicle *PlayerVehicle) const;
};
