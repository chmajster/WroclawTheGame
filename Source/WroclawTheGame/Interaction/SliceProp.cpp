#include "Interaction/SliceProp.h"
#include "Components/GameplayComponents.h"
#include "Systems/NoiseSystem.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/GameInstance.h"
#include "Character/SliceCharacter.h"
#include "Mission/SliceMission.h"
#include "UI/SliceController.h"
#include "Audio/SliceAudio.h"

namespace
{
UBoxComponent* EnsureInteractionBounds(ASliceProp* Prop)
{
    if (auto* Existing = Prop->FindComponentByClass<UBoxComponent>())
        return Existing;

    auto* Bounds = NewObject<UBoxComponent>(Prop, TEXT("InteractionBounds"));
    Prop->AddInstanceComponent(Bounds);
    Bounds->SetCollisionProfileName(TEXT("Interactable"));
    Bounds->SetCanEverAffectNavigation(false);
    Bounds->SetupAttachment(Prop->GetRootComponent());
    Bounds->SetAbsolute(false, false, true);
    Bounds->RegisterComponent();
    Bounds->SetRelativeLocation(FVector::ZeroVector);
    Bounds->SetRelativeRotation(FRotator::ZeroRotator);
    Bounds->SetWorldScale3D(FVector::OneVector);
    return Bounds;
}
}
ASliceProp::ASliceProp()
{
    Door = CreateDefaultSubobject<UDoorComponent>(TEXT("Door"));
    Puzzle = CreateDefaultSubobject<UBasePuzzleComponent>(TEXT("Puzzle"));
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.1f;
    // Keep the authored-map component graph compatible with the pre-Collider class:
    // Mesh remains the native root. InteractionBounds is added only after loading.
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetCanEverAffectNavigation(false);
}
void ASliceProp::BeginPlay()
{
    Super::BeginPlay();
    if (const auto *A = Wroclaw::Progress::Find(TCHAR_TO_UTF8(*ActionId)))
        Configure(ActionId, FVector(A->sx, A->sy, A->sz));
}
void ASliceProp::Configure(const FString &Id, const FVector &Size)
{
    ActionId = Id;
    Puzzle->DefinitionId = FName(*Id);
    ClosedPosition = GetActorLocation();
    auto* InteractionBounds = EnsureInteractionBounds(this);
    InteractionBounds->SetBoxExtent(FVector(FMath::Max(8.f, Size.X*.5f),
                                             FMath::Max(8.f, Size.Y*.5f),
                                             FMath::Max(8.f, Size.Z*.5f)));
    Tick(0);
}
FText ASliceProp::Prompt(ASliceCharacter *Player) const
{
    const auto *A = Wroclaw::Progress::Find(TCHAR_TO_UTF8(*ActionId));
    if (!A)
        return FText::GetEmpty();
    FString Text = UTF8_TO_TCHAR(A->label.c_str());
    if (Player->Mission()->State.Done(A->id))
        Text += TEXT(" [sprawdzone]");
    return FText::FromString(Text);
}
void ASliceProp::Interact(ASliceCharacter *Player)
{
    const auto *A = Wroclaw::Progress::Find(TCHAR_TO_UTF8(*ActionId));
    if (!A)
        return;
    auto *M = Player->Mission();
    auto *PC = CastChecked<ASliceController>(Player->GetController());
    if (A->gate && M->State.Done(A->id))
    {
        const bool Open = !Door->bOpen;
        if (Door->SetOpen(Open, true))
        {
            SetActorLocation(Open ? ClosedPosition + FVector(-80, -80, 0) : ClosedPosition);
            SetActorRotation(FRotator(0, Open ? 90 : 0, 0));
            const std::string Flag = "Door." + A->id + ".Closed";
            if (Open)
                M->WorldState.flags.erase(Flag);
            else
                M->WorldState.flags.insert(Flag);
            GetWorld()->GetSubsystem<UNoiseSystem>()->Report(TEXT("Door"), GetActorLocation(), Player);
        }
        return;
    }
    if (!M->State.Done(A->id) && (!A->answer.empty() || A->kind == "choice"))
    {
        PC->OpenKeypad(ActionId);
        return;
    }
    const auto R = M->Act(ActionId);
    if (R != Wroclaw::Result::Applied && R != Wroclaw::Result::AlreadyDone)
        return;
    if (R == Wroclaw::Result::Applied)
        USliceAudio::Play(this, UTF8_TO_TCHAR(A->sound.c_str()), GetActorLocation());
    if (A->presentation == "peek")
    {
        PC->Peek();
        return;
    }
    if (A->gate)
        GetWorld()->GetSubsystem<UNoiseSystem>()->Report(TEXT("Door"), GetActorLocation(), Player);
    if (!A->body.empty())
        PC->ShowMessage(UTF8_TO_TCHAR(A->body.c_str()));
    Tick(0);
}
void ASliceProp::Tick(float Dt)
{
    Super::Tick(Dt);
    const auto *A = Wroclaw::Progress::Find(TCHAR_TO_UTF8(*ActionId));
    if (!A)
        return;
    auto *M = GetGameInstance()->GetSubsystem<USliceMission>();
    if (M->State.Done(A->id))
    {
        if (A->kind == "pickup")
        {
            SetActorHiddenInGame(true);
            SetActorEnableCollision(false);
            SetActorTickEnabled(false);
        }
        if (A->gate && !bOpened)
        {
            bOpened = true;
            const bool Open = !M->WorldState.flags.count("Door." + A->id + ".Closed");
            Door->SetOpen(Open, true);
            SetActorRotation(FRotator(0, Open ? 90 : 0, 0));
            SetActorLocation(Open ? ClosedPosition + FVector(-80, -80, 0) : ClosedPosition);
            SetActorTickEnabled(false);
        }
    }
}
