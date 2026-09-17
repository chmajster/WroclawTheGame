#include "Character/SliceCharacter.h"
#include "Engine/GameInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Engine/DamageEvents.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputModifiers.h"
#include "Interaction/Interactable.h"
#include "Mission/SliceMission.h"
#include "UI/SliceController.h"
#include "AI/SliceEnemy.h"
#include "Perception/AISense_Hearing.h"
#include "Kismet/GameplayStatics.h"
#include "Audio/SliceAudio.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
ASliceCharacter::ASliceCharacter() {
 PrimaryActorTick.bCanEverTick=true;
 GetCapsuleComponent()->InitCapsuleSize(34,90);
 bUseControllerRotationYaw=false;
 GetCharacterMovement()->bOrientRotationToMovement=true;
 GetCharacterMovement()->RotationRate=FRotator(0,600,0);
 GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch=true;
 GetCharacterMovement()->MaxWalkSpeed=260; GetCharacterMovement()->MaxWalkSpeedCrouched=140;
 GetCharacterMovement()->JumpZVelocity=420; GetCharacterMovement()->AirControl=0.25f;
 Boom=CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom")); Boom->SetupAttachment(RootComponent);
 Boom->TargetArmLength=250; Boom->SocketOffset=FVector(0,35,55); Boom->bUsePawnControlRotation=true;
 Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("Camera")); Camera->SetupAttachment(Boom);
 Camera->FieldOfView=85;
 // Asset-independent articulated proxy: replace with a skeletal mesh without changing gameplay.
 static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
 UStaticMesh* Cube=CubeFinder.Object;
 const FVector Positions[]={{0,0,6},{0,0,63},{0,-30,5},{0,30,5},{0,-13,-57},{0,13,-57}};
 const FVector Sizes[]={{35,44,65},{26,26,28},{20,16,60},{20,16,60},{23,20,64},{23,20,64}};
 for(int I=0;I<6;++I) {
  auto* Part=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Body%d"),I));
  Part->SetupAttachment(RootComponent); Part->SetStaticMesh(Cube); Part->SetRelativeLocation(Positions[I]);
  Part->SetRelativeScale3D(Sizes[I]/100); Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Part->SetCanEverAffectNavigation(false); Limbs.Add(Part);
 }
}
void ASliceCharacter::BeginPlay() {
 Super::BeginPlay();
 if(auto* Skin=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Generated/M_Player.M_Player")))
  for(UStaticMeshComponent* Part:Limbs) Part->SetMaterial(0,Skin);
}
USliceMission* ASliceCharacter::Mission() const { return GetGameInstance()->GetSubsystem<USliceMission>(); }
void ASliceCharacter::SetupPlayerInputComponent(UInputComponent* Input) {
 Super::SetupPlayerInputComponent(Input);
 auto* EI=CastChecked<UEnhancedInputComponent>(Input);
 Mapping=NewObject<UInputMappingContext>(this);
 auto Axis=[&](const TCHAR* Name,FKey Positive,FKey Negative) {
  auto* Action=NewObject<UInputAction>(this,FName(Name)); Action->ValueType=EInputActionValueType::Axis1D; Actions.Add(Action);
  Mapping->MapKey(Action,Positive);
  if(Negative.IsValid()) Mapping->MapKey(Action,Negative).Modifiers.Add(NewObject<UInputModifierNegate>(Mapping));
  return Action;
 };
 auto Button=[&](const TCHAR* Name,FKey Key) {auto* A=NewObject<UInputAction>(this,FName(Name)); Actions.Add(A); Mapping->MapKey(A,Key); return A;};
 EI->BindAction(Axis(TEXT("Forward"),EKeys::W,EKeys::S),ETriggerEvent::Triggered,this,&ASliceCharacter::MoveForward);
 EI->BindAction(Axis(TEXT("Right"),EKeys::D,EKeys::A),ETriggerEvent::Triggered,this,&ASliceCharacter::MoveRight);
 EI->BindAction(Axis(TEXT("Yaw"),EKeys::MouseX,FKey()),ETriggerEvent::Triggered,this,&ASliceCharacter::LookX);
 EI->BindAction(Axis(TEXT("Pitch"),EKeys::MouseY,FKey()),ETriggerEvent::Triggered,this,&ASliceCharacter::LookY);
 auto* Run=Button(TEXT("Sprint"),EKeys::LeftShift);
 EI->BindAction(Run,ETriggerEvent::Started,this,&ASliceCharacter::SprintStart); EI->BindAction(Run,ETriggerEvent::Completed,this,&ASliceCharacter::SprintEnd);
 EI->BindAction(Run,ETriggerEvent::Canceled,this,&ASliceCharacter::SprintEnd);
 auto* Block=Button(TEXT("Block"),EKeys::RightMouseButton);
 EI->BindAction(Block,ETriggerEvent::Started,this,&ASliceCharacter::BlockStart); EI->BindAction(Block,ETriggerEvent::Completed,this,&ASliceCharacter::BlockEnd);
 EI->BindAction(Block,ETriggerEvent::Canceled,this,&ASliceCharacter::BlockEnd);
 auto* JumpAction=Button(TEXT("Jump"),EKeys::SpaceBar);
 EI->BindAction(JumpAction,ETriggerEvent::Started,this,&ASliceCharacter::JumpStart); EI->BindAction(JumpAction,ETriggerEvent::Completed,this,&ASliceCharacter::JumpEnd);
 EI->BindAction(Button(TEXT("Crouch"),EKeys::LeftControl),ETriggerEvent::Started,this,&ASliceCharacter::CrouchToggle);
 EI->BindAction(Button(TEXT("Interact"),EKeys::E),ETriggerEvent::Started,this,&ASliceCharacter::Interact);
 EI->BindAction(Button(TEXT("Phone"),EKeys::T),ETriggerEvent::Started,this,&ASliceCharacter::Phone);
 EI->BindAction(Button(TEXT("Attack"),EKeys::LeftMouseButton),ETriggerEvent::Started,this,&ASliceCharacter::Attack);
 if(auto* PC=Cast<APlayerController>(Controller)) if(auto* LP=PC->GetLocalPlayer())
  LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()->AddMappingContext(Mapping,0);
}
void ASliceCharacter::MoveForward(const FInputActionValue& V) { AddMovementInput(FRotationMatrix(FRotator(0,GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::X),V.Get<float>()); }
void ASliceCharacter::MoveRight(const FInputActionValue& V) { AddMovementInput(FRotationMatrix(FRotator(0,GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::Y),V.Get<float>()); }
void ASliceCharacter::LookX(const FInputActionValue& V) { AddControllerYawInput(V.Get<float>()); }
void ASliceCharacter::LookY(const FInputActionValue& V) { AddControllerPitchInput(-V.Get<float>()); }
void ASliceCharacter::CrouchToggle() { if(bIsCrouched) UnCrouch(); else Crouch(); }
void ASliceCharacter::SprintStart(){ bSprint=true; } void ASliceCharacter::SprintEnd(){ bSprint=false; }
void ASliceCharacter::BlockStart(){ bBlock=true; } void ASliceCharacter::BlockEnd(){ bBlock=false; }
void ASliceCharacter::JumpStart(){ if(Stamina>=12 && !bIsCrouched && CanJump()){ Stamina-=12; Jump(); } } void ASliceCharacter::JumpEnd(){ StopJumping(); }
void ASliceCharacter::Tick(float Dt) {
 Super::Tick(Dt);
 auto* M=Mission(); if(!M->bInGame || M->bDead || M->State.Finished()) return;
 const bool Running=bSprint && Stamina>0 && !bIsCrouched && !bBlock && GetVelocity().Size2D()>20;
 GetCharacterMovement()->MaxWalkSpeed=Running?530:(bBlock?170:260);
 Stamina=FMath::Clamp(Stamina+(Running?-21.f:14.f)*Dt,0.f,100.f);
 if(Stamina<=0) bSprint=false;
 const float Speed=GetVelocity().Size2D();
 if(Speed>15 && !GetCharacterMovement()->IsFalling()) {
  FootstepTime+=Dt;
  const float Interval=bIsCrouched?0.65f:(Running?0.29f:0.46f);
  if(FootstepTime>=Interval) {
   FootstepTime=0; UAISense_Hearing::ReportNoiseEvent(GetWorld(),GetActorLocation(),bIsCrouched?0.12f:(Running?1.f:0.35f),this,Running?1700.f:650.f);
   USliceAudio::Play(this,TEXT("Footstep"),GetActorLocation(),bIsCrouched?0.15f:0.4f);
  }
 }
 const float Swing=FMath::Sin(GetWorld()->GetTimeSeconds()*(Running?14.f:9.f))*FMath::Min(Speed/10,30.f);
 for(int I=2;I<6;++I) Limbs[I]->SetRelativeRotation(FRotator((I%2?1:-1)*Swing,0,0));
 FHitResult Hit; const FVector Start=Camera->GetComponentLocation();
 FCollisionQueryParams Params(SCENE_QUERY_STAT(Interact),false,this);
 GetWorld()->LineTraceSingleByChannel(Hit,Start,Start+Camera->GetForwardVector()*600,ECC_Visibility,Params);
 Focus=(Hit.GetActor() && Hit.GetActor()->Implements<UInteractable>() && FVector::Dist(GetActorLocation(),Hit.ImpactPoint)<230)?Hit.GetActor():nullptr;
 if(GetActorLocation().Z < -400) TakeDamage(1000,FDamageEvent(),nullptr,nullptr);
}
void ASliceCharacter::Interact() { if(auto* Target=Cast<IInteractable>(Focus)) Target->Interact(this); }
void ASliceCharacter::Phone() {
 auto* M=Mission();
 if(!M->State.Has(Wroclaw::Item::Phone)) { M->Notify(TEXT("Najpierw znajdź telefon.")); return; }
 if(!M->State.Has(Wroclaw::Item::Charger)) { M->Notify(TEXT("Bateria pusta. Znajdź powerbank z kablem USB.")); return; }
 if(!M->State.phoneUnlocked) { CastChecked<ASliceController>(Controller)->OpenKeypad(nullptr); return; }
 M->Apply(Wroclaw::Event::ReadMessage);
 CastChecked<ASliceController>(Controller)->ShowMessage(TEXT("NIE ZNASZ MNIE. Oni już tu są.\nBezpiecznik schowałem w kuchennej szafce. Włóż go do rozdzielni przy drzwiach.\nWłącz lampę na biurku — ciepło odsłoni kod szafki. W środku jest klucz.\nNa ulicy skręć w zaułek przy niebieskim szyldzie. Zgub ogon. Czekam w lokalu."));
}
void ASliceCharacter::Attack() {
 const double Now=GetWorld()->GetTimeSeconds();
 if(Now-LastAttack<0.65 || Stamina<18) return;
 LastAttack=Now; Stamina-=18;
 FHitResult Hit; FCollisionQueryParams P(SCENE_QUERY_STAT(Melee),false,this);
 const FVector Start=GetActorLocation(); const FVector End=Start+GetActorForwardVector()*145;
 if(GetWorld()->SweepSingleByChannel(Hit,Start,End,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeSphere(32),P))
  if(auto* Enemy=Cast<ASliceEnemy>(Hit.GetActor())) UGameplayStatics::ApplyDamage(Enemy,24,Controller,this,nullptr);
 UAISense_Hearing::ReportNoiseEvent(GetWorld(),Start,1,this,1200);
 USliceAudio::Play(this,TEXT("Hit"),Start,0.6f);
}
float ASliceCharacter::TakeDamage(float Damage,const FDamageEvent& Event,AController* Instigator,AActor* Causer) {
 auto* M=Mission(); if(M->bDead || !M->bInGame || M->State.Finished()) return 0;
 const bool Facing=Causer && FVector::DotProduct(GetActorForwardVector(),(Causer->GetActorLocation()-GetActorLocation()).GetSafeNormal())>0.2f;
 if(bBlock && Stamina>=20 && Facing) { Stamina-=20; Damage*=0.2f; }
 Health=FMath::Max(0.f,Health-Damage);
 if(Health==0) { M->bDead=true; GetCharacterMovement()->StopMovementImmediately(); CastChecked<ASliceController>(Controller)->SetPause(true); }
 return Damage;
}
