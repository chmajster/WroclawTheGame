#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "SliceCharacter.generated.h"
UCLASS()
class WROCLAWTHEGAME_API ASliceCharacter : public ACharacter
{
    GENERATED_BODY()
  public:
    ASliceCharacter();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent *Input) override;
    virtual float TakeDamage(float Damage, const FDamageEvent &Event, AController *EventInstigator,
                             AActor *Causer) override;
    class USliceMission *Mission() const;
    void Interact();
    void Phone();
    void Attack();
    void ThrowObject();
    void Dodge();
    void Heal();
    void ToggleFlashlight();
    UPROPERTY(VisibleAnywhere) TObjectPtr<class USpotLightComponent> Flashlight;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UWorldPartitionStreamingSourceComponent> StreamingSource;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UNavigationInvokerComponent> NavigationInvoker;
    UPROPERTY() TWeakObjectPtr<class UHideableComponent> Hiding;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UHealthComponent> HealthState;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaminaComponent> StaminaState;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UCombatComponent> Combat;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UInventoryComponent> InventoryState;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UInteractionComponent> Interaction;
    void HeavyAttack();
    void SetSprint(bool Value)
    {
        bSprint = Value;
    }
    void SetBlock(bool Value);

    bool bSprint = false, bBlock = false;
    UPROPERTY() TObjectPtr<AActor> Focus;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class USpringArmComponent> Boom;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UCameraComponent> Camera;

  private:
    UPROPERTY() TObjectPtr<class UInputMappingContext> Mapping;
    UPROPERTY() TArray<TObjectPtr<class UInputAction>> Actions;
    UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> Limbs;

    float FootstepTime = 0;
    void MoveForward(const FInputActionValue &Value);
    void MoveRight(const FInputActionValue &Value);
    void LookX(const FInputActionValue &Value);
    void LookY(const FInputActionValue &Value);
    void CrouchToggle();
    void SprintStart();
    void SprintEnd();
    void BlockStart();
    void BlockEnd();
    void JumpStart();
    void JumpEnd();
};
