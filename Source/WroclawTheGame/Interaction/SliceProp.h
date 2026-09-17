#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Core/SliceProgress.h"
#include "SliceProp.generated.h"
UENUM()
enum class EPropKind : uint8 { Intro, PhoneDrawer, Charger, PinNote, FuseCupboard, FuseBox, Lamp, Cabinet, ExitDoor, BuildingDoor, SafeDoor };
UCLASS()
class WROCLAWTHEGAME_API ASliceProp : public AActor, public IInteractable {
 GENERATED_BODY()
public:
 ASliceProp();
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Mesh;
 UPROPERTY() EPropKind Kind = EPropKind::Intro;
 UPROPERTY() bool bOpened = false;
 void Configure(EPropKind Type, const FVector& Size);
 virtual FText Prompt(ASliceCharacter* Player) const override;
 virtual void Interact(ASliceCharacter* Player) override;
 void SubmitCode(const FString& Code, ASliceCharacter* Player);
 void Restore();
 virtual void Tick(float DeltaSeconds) override;
private:
 FVector ClosedPosition, TargetPosition;
 FRotator TargetRotation;
 void Open(bool bInstant=false);
};
