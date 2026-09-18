#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Interaction/Interactable.h"
#include "Interaction/VehicleInteractionInterface.h"
#include "DriveableVehicle.generated.h"
UCLASS()
class WROCLAWTHEGAME_API ADriveableVehicle : public APawn,
                                             public IInteractable,
                                             public IVehicleInteractionInterface
{
    GENERATED_BODY()
  public:
    ADriveableVehicle();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float Damage, const FDamageEvent &Event, AController *EventInstigator,
                             AActor *Causer) override;
    virtual FText Prompt(ASliceCharacter *Player) const override;
    virtual void Interact(ASliceCharacter *Player) override;
    virtual bool CanEnterVehicle_Implementation(APawn *Passenger) const override;
    virtual bool OpenStorage_Implementation(APawn *User) override;
    bool Exit();
    FString Status() const;
    UFUNCTION(BlueprintCallable) void SetVisualMeshes(class UStaticMesh *BodyMesh, class UStaticMesh *WheelMesh);
    UPROPERTY(EditAnywhere) TObjectPtr<class UVehicleDefinition> Definition;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UBoxComponent> Chassis;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> BodyVisual;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> CabinVisual;
    UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<class UStaticMeshComponent>> WheelVisuals;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UWorldPartitionStreamingSourceComponent> StreamingSource;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class USpringArmComponent> Boom;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UCameraComponent> Camera;
    UPROPERTY() TArray<TObjectPtr<class USpotLightComponent>> Lights;
    UPROPERTY() TObjectPtr<ASliceCharacter> Driver;
    bool bEngine = false, bLights = false;
    float Health = 100, Speed = 0;
    int32 Gear = 0;

  private:
    UPROPERTY() TObjectPtr<class ACityStreamingProbe> StreamingProbe;
    bool bWaitingForGround = true;
    double EnteredAt = 0, LastImpact = 0, LastHorn = 0;
    UFUNCTION()
    void Collision(UPrimitiveComponent *Hit, AActor *Other, UPrimitiveComponent *OtherComponent,
                   FVector Impulse, const FHitResult &Result);
};
