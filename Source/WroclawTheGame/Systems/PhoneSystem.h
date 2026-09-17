#pragma once
#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "PhoneSystem.generated.h"
// A new application registers a presenter; other applications and the controller remain unchanged.
UCLASS()
class WROCLAWTHEGAME_API UPhoneSystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()
  public:
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    void Register(FName App, TFunction<FString()> Presenter);
    FString Render(int32 Page) const;
    void Open(int32 Page);

  private:
    TMap<FName, TFunction<FString()>> Presenters;
    class USliceMission *Mission() const;
};
