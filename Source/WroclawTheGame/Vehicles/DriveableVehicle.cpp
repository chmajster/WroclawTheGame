#include "Vehicles/DriveableVehicle.h"
#include "World/CityStreamingProbe.h"
#include "Systems/CityGameplaySubsystem.h"
#include "Vehicles/RaceSession.h"
#include "Vehicles/VehicleDefinition.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WorldPartitionStreamingSourceComponent.h"
#include "Character/SliceCharacter.h"
#include "UI/SliceController.h"
#include "Mission/SliceMission.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/DamageEvents.h"
#include "UObject/ConstructorHelpers.h"
#include "Audio/SliceAudio.h"
ADriveableVehicle::ADriveableVehicle()
{
    PrimaryActorTick.bCanEverTick = true;
    CreateDefaultSubobject<URaceSession>(TEXT("RaceSession"));
    Chassis = CreateDefaultSubobject<UBoxComponent>(TEXT("Chassis"));
    SetRootComponent(Chassis);
    Chassis->SetBoxExtent(FVector(210, 90, 28));
    Chassis->SetCollisionProfileName(TEXT("PhysicsActor"));
    Chassis->SetSimulatePhysics(true);
    Chassis->SetNotifyRigidBodyCollision(true);
    Chassis->SetLinearDamping(.04);
    Chassis->SetAngularDamping(2);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto *Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(Chassis);
    Body->SetStaticMesh(Cube.Object);
    Body->SetRelativeScale3D(FVector(4.2, 1.8, .56));
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    auto *Cabin = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cabin"));
    Cabin->SetupAttachment(Chassis);
    Cabin->SetStaticMesh(Cube.Object);
    Cabin->SetRelativeScale3D(FVector(2.1, 1.65, .6));
    Cabin->SetRelativeLocation(FVector(-25, 0, 55));
    Cabin->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Boom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Boom"));
    Boom->SetupAttachment(Chassis);
    Boom->TargetArmLength = 650;
    Boom->SetRelativeLocation(FVector(0, 0, 150));
    Boom->SetRelativeRotation(FRotator(-15, 0, 0));
    Boom->bUsePawnControlRotation = true;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(Boom);
    StreamingSource =
        CreateDefaultSubobject<UWorldPartitionStreamingSourceComponent>(TEXT("StreamingSource"));
    for (int I = 0; I < 2; I++)
    {
        auto *Light = CreateDefaultSubobject<USpotLightComponent>(*FString::Printf(TEXT("Light%d"), I));
        Light->SetupAttachment(Chassis);
        Light->SetRelativeLocation(FVector(210, I ? 65 : -65, 0));
        Light->SetIntensity(12000);
        Light->SetAttenuationRadius(4500);
        Light->SetVisibility(false);
        Lights.Add(Light);
    }
}
void ADriveableVehicle::BeginPlay()
{
    Super::BeginPlay();
    StreamingProbe=GetWorld()->SpawnActor<ACityStreamingProbe>();
    if (!Definition)
        Definition = NewObject<UVehicleDefinition>(this);
    Chassis->SetMassOverrideInKg(NAME_None, FMath::Clamp(Definition->MassKg, 400.f, 5000.f));
    Health = Definition->MaxHealth;
    GetWorld()->GetSubsystem<UCityGameplaySubsystem>()->RestoreVehicle(this);
    Chassis->OnComponentHit.AddDynamic(this, &ADriveableVehicle::Collision);
    StreamingSource->DisableStreamingSource();
    Chassis->SetSimulatePhysics(false);
}
bool ADriveableVehicle::CanEnterVehicle_Implementation(APawn *Passenger) const
{
    return Passenger && !Driver && !bAIControlled && Health > 0 &&
           Chassis->GetPhysicsLinearVelocity().Size() < 100;
}
void ADriveableVehicle::SetAIControl(bool bEnabled, float Throttle, float Steering, bool bBrake)
{
    bAIControlled = bEnabled;
    AIThrottle = bEnabled ? FMath::Clamp(Throttle, -1.0f, 1.0f) : 0.0f;
    AISteering = bEnabled ? FMath::Clamp(Steering, -1.0f, 1.0f) : 0.0f;
    bAIBrake = bEnabled && bBrake;
    if (bEnabled)
    {
        bEngine = Health > 0;
        bWaitingForGround = true;
        SetActorTickEnabled(true);
    }
    else if (!Driver)
    {
        bEngine = false;
        Speed = 0.0f;
        Chassis->SetSimulatePhysics(false);
    }
}
bool ADriveableVehicle::OpenStorage_Implementation(APawn *User)
{
    return false;
}
FText ADriveableVehicle::Prompt(ASliceCharacter *Player) const
{
    return FText::FromString(CanEnterVehicle_Implementation(Player) ? TEXT("E — wsiądź do samochodu")
                                                                    : TEXT("Samochód niedostępny"));
}
void ADriveableVehicle::Interact(ASliceCharacter *Player)
{
    if (!CanEnterVehicle_Implementation(Player))
        return;
    auto *PC = Cast<APlayerController>(Player->GetController());
    if (!PC)
        return;
    Driver = Player;
    Driver->SetSprint(false);
    Driver->SetBlock(false);
    Driver->GetCharacterMovement()->DisableMovement();
    Driver->SetActorEnableCollision(false);
    Driver->SetActorHiddenInGame(true);
    Driver->SetActorTickEnabled(false);
    Driver->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    Driver->StreamingSource->DisableStreamingSource();
    PC->Possess(this);
    PC->SetControlRotation(GetActorRotation());
    StreamingSource->EnableStreamingSource();
    bWaitingForGround=true;Chassis->SetSimulatePhysics(false);
    EnteredAt = GetWorld()->GetTimeSeconds();
}
bool ADriveableVehicle::Exit()
{
    auto *PC = Cast<APlayerController>(GetController());
    if (!PC || !Driver || FMath::Abs(Speed) > 100)
        return false;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(VehicleExit), false, this);
    Params.AddIgnoredActor(Driver);
    for (float Side : {-1.f, 1.f})
    {
        FVector Point = GetActorLocation() + GetActorRightVector() * Side * 170;
        FHitResult Floor;
        if (!GetWorld()->LineTraceSingleByChannel(Floor, Point + FVector(0, 0, 150),
                                                  Point - FVector(0, 0, 350), ECC_Visibility, Params))
            continue;
        Point = Floor.ImpactPoint + FVector(0, 0, 94);
        if (GetWorld()->OverlapBlockingTestByChannel(Point, FQuat::Identity, ECC_Pawn,
                                                     FCollisionShape::MakeCapsule(36, 92), Params))
            continue;
        auto *Passenger = Driver.Get();
        Passenger->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        Passenger->SetActorLocation(Point);
        Passenger->SetActorHiddenInGame(false);
        Passenger->SetActorEnableCollision(true);
        Passenger->SetActorTickEnabled(true);
        Passenger->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        PC->Possess(Passenger);
        Passenger->StreamingSource->EnableStreamingSource();
        Driver = nullptr;
        StreamingSource->DisableStreamingSource();
        if (StreamingProbe) StreamingProbe->Source->DisableStreamingSource();
        bEngine = false;
        return true;
    }
    return false;
}
void ADriveableVehicle::Tick(float Dt)
{
    Super::Tick(Dt);
    if (!Definition)
        return;
    if (bWaitingForGround)
    {
        FHitResult Floor;
        FCollisionQueryParams GroundParams(SCENE_QUERY_STAT(CarInitialFloor), false, this);
        if (!GetWorld()->LineTraceSingleByChannel(Floor, GetActorLocation(),
                                                  GetActorLocation() - FVector(0, 0, 250), ECC_Visibility,
                                                  GroundParams))
            return;
        Chassis->SetSimulatePhysics(true);
        bWaitingForGround = false;
    }
    if (!Driver && !bAIControlled) { Chassis->SetSimulatePhysics(false); Speed=0; return; }
    if (!Chassis->IsSimulatingPhysics())
        return;
    const FVector Velocity = Chassis->GetPhysicsLinearVelocity(), Forward = GetActorForwardVector(),
                  Right = GetActorRightVector();
    Speed = FVector::DotProduct(Velocity, Forward);
    if (StreamingProbe)
    {
        if (Driver && Velocity.Size2D()>500)
        {
            StreamingProbe->SetActorLocation(GetActorLocation()+(Velocity*2.5).GetClampedToMaxSize(15000));
            StreamingProbe->Source->EnableStreamingSource();
        }
        else StreamingProbe->Source->DisableStreamingSource();
    }
    float Throttle = bAIControlled ? AIThrottle : 0.0f;
    float Steering = bAIControlled ? AISteering : 0.0f;
    bool Brake = bAIControlled && bAIBrake;
    auto *PC = Cast<ASliceController>(GetController());
    if (Driver && PC && !PC->GameplayBlocked() && !PC->IsPaused())
    {
        if (PC->WasInputKeyJustPressed(EKeys::E) && GetWorld()->GetTimeSeconds() - EnteredAt > .4)
        {
            if (Exit())
                return;
        }
        if (PC->WasInputKeyJustPressed(EKeys::NumPadZero))
            bEngine = !bEngine && Health > 0;
        if (PC->WasInputKeyJustPressed(EKeys::Add))
        {
            bLights = !bLights;
            for (USpotLightComponent *Light : Lights)
                Light->SetVisibility(bLights);
        }
        if (PC->IsInputKeyDown(EKeys::Multiply) && GetWorld()->GetTimeSeconds() - LastHorn > 1)
        {
            LastHorn = GetWorld()->GetTimeSeconds();
            USliceAudio::Play(this, TEXT("Horn"), GetActorLocation(), .8);
        }
        Throttle = float(PC->IsInputKeyDown(EKeys::W)) - float(PC->IsInputKeyDown(EKeys::S));
        Steering = float(PC->IsInputKeyDown(EKeys::D)) - float(PC->IsInputKeyDown(EKeys::A));
        Brake = PC->IsInputKeyDown(EKeys::SpaceBar);
        float X, Y;
        PC->GetInputMouseDelta(X, Y);
        PC->AddYawInput(X);
        PC->AddPitchInput(-Y);
    }
    int Grounded = 0;
    const float Mass = Chassis->GetMass();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(Suspension), false, this);
    if (Driver)
        Params.AddIgnoredActor(Driver);
    for (float X : {-145.f, 145.f})
        for (float Y : {-70.f, 70.f})
        {
            const FVector Point = GetActorTransform().TransformPosition(FVector(X, Y, 0));
            FHitResult Hit;
            if (GetWorld()->LineTraceSingleByChannel(Hit, Point, Point - FVector(0, 0, 110), ECC_Visibility,
                                                     Params))
            {
                ++Grounded;
                const float Compression = 65 - Hit.Distance;
                const float V = Chassis->GetPhysicsLinearVelocityAtPoint(Point).Z;
                const float Force = FMath::Clamp(980 + Compression * 90 - V * 9, 0.f, 2940.f) * Mass * .25f;
                Chassis->AddForceAtLocation(FVector(0, 0, Force), Point);
            }
        }
    if (Grounded >= 2)
    {
        const bool Opposing = Throttle * Speed < -30;
        const float Target = Throttle >= 0 ? Definition->MaxSpeed : Definition->ReverseSpeed;
        if (Brake || Opposing || (!Driver && !bAIControlled))
            Chassis->AddForce(-Forward *
                              FMath::Clamp(Speed / FMath::Max(Dt, .005f), -Definition->BrakeDeceleration,
                                           Definition->BrakeDeceleration) *
                              Mass);
        else if (bEngine && Health > 0 && FMath::Abs(Speed) < Target)
            Chassis->AddForce(Forward * Throttle * Definition->Acceleration * Mass);
        Chassis->AddForce(-Right * FVector::DotProduct(Velocity, Right) * Definition->Grip * Mass *
                          (Brake ? .35f : 1.f));
        const float Yaw = Steering * FMath::Clamp(Speed / 350.f, -1.f, 1.f) * .65f;
        FVector Angular = Chassis->GetPhysicsAngularVelocityInRadians();
        Chassis->AddTorqueInRadians(FVector(0, 0, (Yaw - Angular.Z) * Mass * 28000));
    }
    Gear = FMath::Abs(Speed) < 20 ? 0 : (Speed < 0 ? -1 : FMath::Clamp(1 + int(Speed / 500), 1, 5));
    if (Health <= 0)
        bEngine = false;
}
float ADriveableVehicle::TakeDamage(float Amount, const FDamageEvent &Event,
                                    AController *EventInstigator,
                                    AActor *Causer)
{
    const float Applied = FMath::Min(Health, FMath::Max(0.f, Amount));
    Health -= Applied;
    return Applied;
}
void ADriveableVehicle::Collision(UPrimitiveComponent *Hit, AActor *Other,
                                  UPrimitiveComponent *OtherComponent, FVector Impulse,
                                  const FHitResult &Result)
{
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now - LastImpact < .3 || Impulse.Size() / Chassis->GetMass() < 250)
        return;
    LastImpact = Now;
    TakeDamage((Impulse.Size() / Chassis->GetMass() - 250) * .035f, FDamageEvent(), nullptr, Other);
}
FString ADriveableVehicle::Status() const
{
    return FString::Printf(
        TEXT("%.0f km/h | bieg %d | stan %.0f%% | silnik %s\nW/S gaz / hamowanie / wstecz | A/D skręt | "
             "SPACJA ręczny\nNUM 0 silnik | NUM + światła | NUM * klakson | E wysiądź po zatrzymaniu"),
        FMath::Abs(Speed) * .036f, Gear, Health, bEngine ? TEXT("ON") : TEXT("OFF"));
}

void ADriveableVehicle::EndPlay(const EEndPlayReason::Type Reason)
{
    if (StreamingProbe) StreamingProbe->Destroy();
    Super::EndPlay(Reason);
}
