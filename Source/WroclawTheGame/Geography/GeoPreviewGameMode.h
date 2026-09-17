#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GeoPreviewGameMode.generated.h"
// Geographic/vehicle integration laboratory. Never writes campaign checkpoints.
UCLASS()
class WROCLAWTHEGAME_API AGeoPreviewGameMode : public AGameModeBase
{
    GENERATED_BODY()
  public:
    AGeoPreviewGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

  private:
    bool bReady = false;
    FVector Start;
};
