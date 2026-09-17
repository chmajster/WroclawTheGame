#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SliceWorld.generated.h"
// Small global ambience controller. Environment geometry and gameplay actors are baked into WP cells.
UCLASS()
class WROCLAWTHEGAME_API ASliceWorld : public AActor
{
    GENERATED_BODY()
  public:
    ASliceWorld();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

  private:
    UPROPERTY() TObjectPtr<class UPointLightComponent> PuzzleLight;
    UPROPERTY() TObjectPtr<class UPowerConsumerComponent> Power;
    UPROPERTY() TObjectPtr<class UAudioComponent> RoomAudio;
    UPROPERTY() TObjectPtr<class UAudioComponent> StreetAudio;
    UPROPERTY() TObjectPtr<class UAudioComponent> ChaseAudio;
    UPROPERTY() TObjectPtr<class ADirectionalLight> Sun;
    UPROPERTY() TObjectPtr<class AExponentialHeightFog> Fog;
    void AudioLoop(TObjectPtr<UAudioComponent> &Component, const TCHAR *Name, float Volume);
};
