#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NoiseThrowable.generated.h"
UCLASS()
class WROCLAWTHEGAME_API ANoiseThrowable : public AActor
{
    GENERATED_BODY()
  public:
    ANoiseThrowable();
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Mesh;
    virtual void BeginPlay() override;

  private:
    bool bReported = false;
    UFUNCTION()
    void Hit(UPrimitiveComponent *Component, AActor *Other, UPrimitiveComponent *OtherComponent,
             FVector Impulse, const FHitResult &Result);
};
