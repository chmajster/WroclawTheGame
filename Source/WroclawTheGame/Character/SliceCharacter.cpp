#include "Character/SliceCharacter.h"
#include "Character/CharacterAppearanceComponent.h"
#include "Geography/GeoPreviewGameMode.h"
#include "Components/WorldPartitionStreamingSourceComponent.h"
#include "NavigationInvokerComponent.h"
#include "Components/GameplayComponents.h"
#include "Systems/NoiseSystem.h"
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
#include "Components/SpotLightComponent.h"
#include "Interaction/NoiseThrowable.h"
#include "Engine/World.h"
ASliceCharacter::ASliceCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    Appearance=CreateDefaultSubobject<UCharacterAppearanceComponent>(TEXT("Appearance"));
    Wardrobe=CreateDefaultSubobject<UWardrobeComponent>(TEXT("Wardrobe"));
    StreamingSource =
        CreateDefaultSubobject<UWorldPartitionStreamingSourceComponent>(TEXT("StreamingSource"));
    NavigationInvoker = CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavigationInvoker"));
    auto *Faction = CreateDefaultSubobject<UFactionComponent>(TEXT("Faction"));
    Faction->FactionId = TEXT("player");
    HealthState = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
    StaminaState = CreateDefaultSubobject<UStaminaComponent>(TEXT("Stamina"));
    Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));
    InventoryState = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));
    Interaction = CreateDefaultSubobject<UInteractionComponent>(TEXT("Interaction"));
    // Stable movement envelope accommodates every allowed 160–195 cm appearance.
    GetCapsuleComponent()->InitCapsuleSize(34, 100);
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0, 600, 0);
    GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
    GetCharacterMovement()->MaxWalkSpeed = 260;
    GetCharacterMovement()->MaxWalkSpeedCrouched = 140;
    GetCharacterMovement()->JumpZVelocity = 420;
    GetCharacterMovement()->AirControl = 0.25f;
    Boom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    Boom->SetupAttachment(RootComponent);
    Boom->TargetArmLength = 250;
    Boom->SocketOffset = FVector(0, 35, 55);
    Boom->bUsePawnControlRotation = true;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(Boom);
    Camera->FieldOfView = 85;
    Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight"));
    Flashlight->SetupAttachment(Camera);
    Flashlight->SetIntensity(5000);
    Flashlight->SetAttenuationRadius(1800);
    Flashlight->SetOuterConeAngle(28);
    Flashlight->SetVisibility(false);
    // Asset-independent articulated proxy: replace with a skeletal mesh without changing gameplay.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMesh *Cube = CubeFinder.Object;
    const FVector Positions[] = {{0, 0, 6}, {0, 0, 63}, {0, -30, 5}, {0, 30, 5}, {0, -13, -57}, {0, 13, -57}};
    const FVector Sizes[] = {{35, 44, 65}, {26, 26, 28}, {20, 16, 60},
                             {20, 16, 60}, {23, 20, 64}, {23, 20, 64}};
    for (int I = 0; I < 6; ++I)
    {
        auto *Part = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Body%d"), I));
        Part->SetupAttachment(RootComponent);
        Part->SetStaticMesh(Cube);
        Part->SetRelativeLocation(Positions[I]);
        Part->SetRelativeScale3D(Sizes[I] / 100);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetCanEverAffectNavigation(false);
        Part->SetVisibility(false);
        Limbs.Add(Part);
    }
}
void ASliceCharacter::BeginPlay()
{
    Super::BeginPlay();
    if (auto *Skin = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Generated/M_Player.M_Player")))
        for (UStaticMeshComponent *Part : Limbs)
            Part->SetMaterial(0, Skin);
}
USliceMission *ASliceCharacter::Mission() const
{
    return GetGameInstance()->GetSubsystem<USliceMission>();
}
void ASliceCharacter::SetupPlayerInputComponent(UInputComponent *Input)
{
    Super::SetupPlayerInputComponent(Input);
    auto *EI = CastChecked<UEnhancedInputComponent>(Input);
    Mapping = NewObject<UInputMappingContext>(this);
    auto Axis = [&](const TCHAR *Name, FKey Positive, FKey Negative) {
        auto *Action = NewObject<UInputAction>(Mapping, FName(Name));
        Action->ValueType = EInputActionValueType::Axis1D;
        Actions.Add(Action);
        Mapping->MapKey(Action, Positive);
        if (Negative.IsValid())
            Mapping->MapKey(Action, Negative).Modifiers.Add(NewObject<UInputModifierNegate>(Mapping));
        return Action;
    };
    auto Button = [&](const TCHAR *Name, FKey Key) {
        auto *A = NewObject<UInputAction>(Mapping, FName(Name));
        Actions.Add(A);
        Mapping->MapKey(A, Key);
        return A;
    };
    EI->BindAction(Axis(TEXT("Forward"), EKeys::W, EKeys::S), ETriggerEvent::Triggered, this,
                   &ASliceCharacter::MoveForward);
    EI->BindAction(Axis(TEXT("Right"), EKeys::D, EKeys::A), ETriggerEvent::Triggered, this,
                   &ASliceCharacter::MoveRight);
    EI->BindAction(Axis(TEXT("Yaw"), EKeys::MouseX, FKey()), ETriggerEvent::Triggered, this,
                   &ASliceCharacter::LookX);
    EI->BindAction(Axis(TEXT("Pitch"), EKeys::MouseY, FKey()), ETriggerEvent::Triggered, this,
                   &ASliceCharacter::LookY);
    EI->BindAction(Button(TEXT("Throw"), EKeys::G), ETriggerEvent::Started, this,
                   &ASliceCharacter::ThrowObject);
    EI->BindAction(Button(TEXT("Dodge"), EKeys::LeftAlt), ETriggerEvent::Started, this,
                   &ASliceCharacter::Dodge);
    EI->BindAction(Button(TEXT("Heal"), EKeys::V), ETriggerEvent::Started, this, &ASliceCharacter::Heal);
    EI->BindAction(Button(TEXT("Flashlight"), EKeys::F), ETriggerEvent::Started, this,
                   &ASliceCharacter::ToggleFlashlight);
    auto *Run = Button(TEXT("Sprint"), EKeys::LeftShift);
    EI->BindAction(Run, ETriggerEvent::Started, this, &ASliceCharacter::SprintStart);
    EI->BindAction(Run, ETriggerEvent::Completed, this, &ASliceCharacter::SprintEnd);
    EI->BindAction(Run, ETriggerEvent::Canceled, this, &ASliceCharacter::SprintEnd);
    auto *Block = Button(TEXT("Block"), EKeys::RightMouseButton);
    EI->BindAction(Block, ETriggerEvent::Started, this, &ASliceCharacter::BlockStart);
    EI->BindAction(Block, ETriggerEvent::Completed, this, &ASliceCharacter::BlockEnd);
    EI->BindAction(Block, ETriggerEvent::Canceled, this, &ASliceCharacter::BlockEnd);
    auto *JumpAction = Button(TEXT("Jump"), EKeys::SpaceBar);
    EI->BindAction(JumpAction, ETriggerEvent::Started, this, &ASliceCharacter::JumpStart);
    EI->BindAction(JumpAction, ETriggerEvent::Completed, this, &ASliceCharacter::JumpEnd);
    EI->BindAction(Button(TEXT("Crouch"), EKeys::LeftControl), ETriggerEvent::Started, this,
                   &ASliceCharacter::CrouchToggle);
    EI->BindAction(Button(TEXT("Interact"), EKeys::E), ETriggerEvent::Started, this,
                   &ASliceCharacter::Interact);
    EI->BindAction(Button(TEXT("Phone"), EKeys::T), ETriggerEvent::Started, this, &ASliceCharacter::Phone);
    EI->BindAction(Button(TEXT("HeavyAttack"), EKeys::MiddleMouseButton), ETriggerEvent::Started, this,
                   &ASliceCharacter::HeavyAttack);
    EI->BindAction(Button(TEXT("Attack"), EKeys::LeftMouseButton), ETriggerEvent::Started, this,
                   &ASliceCharacter::Attack);
    if (auto *PC = Cast<APlayerController>(Controller))
        if (auto *LP = PC->GetLocalPlayer())
            LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()->AddMappingContext(Mapping, 0);
}
void ASliceCharacter::MoveForward(const FInputActionValue &V)
{
    AddMovementInput(FRotationMatrix(FRotator(0, GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::X),
                     V.Get<float>());
}
void ASliceCharacter::MoveRight(const FInputActionValue &V)
{
    AddMovementInput(FRotationMatrix(FRotator(0, GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::Y),
                     V.Get<float>());
}
void ASliceCharacter::LookX(const FInputActionValue &V)
{
    AddControllerYawInput(V.Get<float>());
}
void ASliceCharacter::LookY(const FInputActionValue &V)
{
    AddControllerPitchInput(-V.Get<float>());
}
void ASliceCharacter::CrouchToggle()
{
    if (CastChecked<ASliceController>(Controller)->GameplayBlocked())
        return;
    if (bIsCrouched)
        UnCrouch();
    else
        Crouch();
}
void ASliceCharacter::SprintStart()
{
    if (!CastChecked<ASliceController>(Controller)->GameplayBlocked())
        bSprint = true;
}
void ASliceCharacter::SprintEnd()
{
    bSprint = false;
}
void ASliceCharacter::BlockStart()
{
    if (!CastChecked<ASliceController>(Controller)->GameplayBlocked())
        SetBlock(true);
}
void ASliceCharacter::BlockEnd()
{
    SetBlock(false);
}
void ASliceCharacter::JumpStart()
{
    if (!CastChecked<ASliceController>(Controller)->GameplayBlocked() && StaminaState->Value >= 12 &&
        !bIsCrouched && CanJump())
    {
        StaminaState->Value -= 12;
        Jump();
    }
}
void ASliceCharacter::JumpEnd()
{
    StopJumping();
}
void ASliceCharacter::Tick(float Dt)
{
    Super::Tick(Dt);
    auto *M = Mission();
    if (!M->bInGame || M->bDead || M->State.Finished())
        return;
    const bool Running =
        bSprint && StaminaState->Value > 0 && !bIsCrouched && !bBlock && GetVelocity().Size2D() > 20;
    GetCharacterMovement()->MaxWalkSpeed = Running ? 530 : (bBlock ? 170 : 260);
    StaminaState->Value = FMath::Clamp(StaminaState->Value + (Running ? -21.f : 14.f) * Dt, 0.f, 100.f);
    if (StaminaState->Value <= 0)
        bSprint = false;
    const float Speed = GetVelocity().Size2D();
    if (Speed > 15 && !GetCharacterMovement()->IsFalling())
    {
        FootstepTime += Dt;
        const float Interval = bIsCrouched ? 0.65f : (Running ? 0.29f : 0.46f);
        if (FootstepTime >= Interval)
        {
            FootstepTime = 0;
            GetWorld()->GetSubsystem<UNoiseSystem>()->Report(
                bIsCrouched ? TEXT("Crouch") : (Running ? TEXT("Sprint") : TEXT("Walk")), GetActorLocation(),
                this);
            USliceAudio::Play(this, TEXT("Footstep"), GetActorLocation(), bIsCrouched ? 0.15f : 0.4f);
        }
    }
    const float Swing =
        FMath::Sin(GetWorld()->GetTimeSeconds() * (Running ? 14.f : 9.f)) * FMath::Min(Speed / 10, 30.f);
    for (int I = 2; I < 6; ++I)
        Limbs[I]->SetRelativeRotation(FRotator((I % 2 ? 1 : -1) * Swing, 0, 0));
    Focus = Interaction->Find(Camera->GetComponentLocation(), Camera->GetForwardVector());
    if (GetActorLocation().Z < (Cast<AGeoPreviewGameMode>(GetWorld()->GetAuthGameMode()) ? -100000 : -650))
        TakeDamage(1000, FDamageEvent(), nullptr, nullptr);
}
void ASliceCharacter::Interact()
{
    if (Hiding.IsValid())
    {
        Hiding->Leave();
        return;
    }
    if (!CastChecked<ASliceController>(Controller)->GameplayBlocked())
        Interaction->Interact(Focus);
}
void ASliceCharacter::Phone()
{
    auto *PC = CastChecked<ASliceController>(Controller);
    if (PC->GameplayBlocked())
        return;
    auto *M = Mission();
    if (!M->State.Has("phone"))
    {
        M->Notify(TEXT("Najpierw znajdź telefon."));
        return;
    }
    if (!M->State.Done("phone_attempt"))
    {
        M->Act(TEXT("phone_attempt"));
        M->Notify(TEXT("Telefon rozładowany. Znajdź ładowarkę, kabel i sprawdź gniazdko."));
        return;
    }
    if (!M->State.Done("charge"))
    {
        M->Notify(TEXT("Przywróć prąd i podłącz telefon do gniazdka [E]."));
        return;
    }
    if (!M->State.Done("unlock_phone"))
    {
        PC->OpenKeypad(TEXT("unlock_phone"));
        return;
    }
    PC->OpenPhone();
}
void ASliceCharacter::ThrowObject()
{
    if (CastChecked<ASliceController>(Controller)->GameplayBlocked() || !Mission()->State.Has("distraction"))
        return;
    FActorSpawnParameters Params;
    Params.Instigator = this;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
    auto *Object = GetWorld()->SpawnActor<ANoiseThrowable>(
        GetActorLocation() + GetActorForwardVector() * 70 + FVector(0, 0, 30), FRotator::ZeroRotator, Params);
    if (Object)
    {
        InventoryState->Use(TEXT("distraction"));
        Object->Mesh->SetPhysicsLinearVelocity(GetControlRotation().Vector() * 1000 + FVector(0, 0, 200));
    }
}
void ASliceCharacter::Dodge()
{
    if (CastChecked<ASliceController>(Controller)->GameplayBlocked() || Hiding.IsValid())
        return;
    FVector Direction = GetLastMovementInputVector().GetSafeNormal();
    if (Direction.IsNearlyZero())
        Direction = -GetActorForwardVector();
    Combat->Dodge(Direction);
}
void ASliceCharacter::Heal()
{
    if (!CastChecked<ASliceController>(Controller)->GameplayBlocked() && HealthState->Value < 100 &&
        InventoryState->Use(TEXT("medkit")))
        HealthState->Value = FMath::Min(100.f, HealthState->Value + 45);
}
void ASliceCharacter::ToggleFlashlight()
{
    if (CastChecked<ASliceController>(Controller)->GameplayBlocked())
        return;
    if (Mission()->State.Has("flashlight") && Mission()->State.Has("batteries"))
        Flashlight->SetVisibility(!Flashlight->IsVisible());
    else
        Mission()->Notify(TEXT("Latarka wymaga baterii."));
}
void ASliceCharacter::Attack()
{
    if (!CastChecked<ASliceController>(Controller)->GameplayBlocked() && !Hiding.IsValid())
        Combat->Attack(false);
}
void ASliceCharacter::HeavyAttack()
{
    if (!CastChecked<ASliceController>(Controller)->GameplayBlocked() && !Hiding.IsValid())
        Combat->Attack(true);
}
void ASliceCharacter::SetBlock(bool Value)
{
    bBlock = Value;
    Combat->bBlocking = Value;
}
float ASliceCharacter::TakeDamage(float Damage, const FDamageEvent &Event, AController *EventInstigator,
                                  AActor *Causer)
{
    auto *M = Mission();
    if (M->bDead || !M->bInGame || M->State.Finished())
        return 0;
    const float Applied = Combat->Receive(Damage, Causer);
    if (HealthState->Value <= 0)
    {
        M->bDead = true;
        GetCharacterMovement()->StopMovementImmediately();
        CastChecked<ASliceController>(Controller)->SetPause(true);
    }
    return Applied;
}
