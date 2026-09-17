#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Systems/GameplayEventBus.h"
#include "OpenWorldSubsystem.generated.h"
UCLASS()
class WROCLAWTHEGAME_API UOpenWorldSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
  public:
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UOpenWorldSubsystem, STATGROUP_Tickables);
    }
    virtual bool DoesSupportWorldType(EWorldType::Type Type) const override
    {
        return Type == EWorldType::Game || Type == EWorldType::PIE;
    }
    bool bAIEnabled = true, bPerceptionDebug = false;
    float VisibilityTo(AActor *Observer, class ASliceCharacter *Player) const;
    UFUNCTION() void HandleEvent(const FWTGGameplayEvent &Event);

  private:
    float Accumulator = 0;
};
