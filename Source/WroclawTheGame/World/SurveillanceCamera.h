#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SurveillanceCamera.generated.h"
UCLASS()
class WROCLAWTHEGAME_API ASurveillanceCamera : public AActor
{
    GENERATED_BODY()
  public:
    ASurveillanceCamera();
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName DefinitionId;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class USceneCaptureComponent2D> Capture;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UPowerConsumerComponent> Power;
    UPROPERTY() TObjectPtr<class UTextureRenderTarget2D> RenderTarget;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    UTextureRenderTarget2D *Feed();

  private:
    double LastAlarm = -100;
};
