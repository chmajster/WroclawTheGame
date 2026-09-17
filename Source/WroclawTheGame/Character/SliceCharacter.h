#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "SliceCharacter.generated.h"
UCLASS()
class WROCLAWTHEGAME_API ASliceCharacter : public ACharacter {
 GENERATED_BODY()
public:
 ASliceCharacter();
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
 virtual float TakeDamage(float Damage, const FDamageEvent& Event, AController* Instigator, AActor* Causer) override;
 class USliceMission* Mission() const;
 void Interact();
 void Phone();
 void Attack();
 void SetSprint(bool Value) { bSprint=Value; }
 void SetBlock(bool Value) { bBlock=Value; }
 float Health=100, Stamina=100;
 bool bSprint=false, bBlock=false;
 UPROPERTY() TObjectPtr<AActor> Focus;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class USpringArmComponent> Boom;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UCameraComponent> Camera;
private:
 UPROPERTY() TObjectPtr<class UInputMappingContext> Mapping;
 UPROPERTY() TArray<TObjectPtr<class UInputAction>> Actions;
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> Limbs;
 double LastAttack=-100;
 float FootstepTime=0;
 void MoveForward(const FInputActionValue& Value);
 void MoveRight(const FInputActionValue& Value);
 void LookX(const FInputActionValue& Value);
 void LookY(const FInputActionValue& Value);
 void CrouchToggle();
 void SprintStart(); void SprintEnd(); void BlockStart(); void BlockEnd();
 void JumpStart(); void JumpEnd();
};
