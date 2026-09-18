#include "World/ResidentNPC.h"
#include "Components/GameplayComponents.h"
#include "AIController.h"
#include "Content/WorldCatalog.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Mission/SliceMission.h"
#include "Character/SliceCharacter.h"
#include "UI/SliceController.h"
AResidentNPC::AResidentNPC()
{
    auto *Faction = CreateDefaultSubobject<UFactionComponent>(TEXT("Faction"));
    Faction->FactionId = TEXT("residents");
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 3;
    AIControllerClass = AAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    GetCharacterMovement()->MaxWalkSpeed = 120;
}
void AResidentNPC::Tick(float Dt)
{
    Super::Tick(Dt);
    auto *M = GetGameInstance()->GetSubsystem<USliceMission>();
    auto *P = UGameplayStatics::GetPlayerPawn(this, 0);
    auto *AI = Cast<AAIController>(GetController());
    if (!P || !AI || !M->bInGame)
        return;
    for (const auto &N : Wroclaw::NPCs())
        if (DefinitionId == FName(UTF8_TO_TCHAR(N.id.c_str())))
        {
            size_t Index = 0;
            for (size_t I = 0; I < N.hours.size(); ++I)
                if (M->WorldState.hour >= N.hours[I])
                    Index = I;
            if (Index >= N.destinations.size())
                return;
            const auto &V = N.destinations[Index];
            if (FVector::Dist(GetActorLocation(), P->GetActorLocation()) < 3500)
                AI->MoveToLocation(FVector(V[0], V[1], V[2]), 100);
            else
                AI->StopMovement();
            return;
        }
}
FText AResidentNPC::Prompt(ASliceCharacter *Player) const
{
    for (const auto &N : Wroclaw::NPCs())
        if (DefinitionId == FName(UTF8_TO_TCHAR(N.id.c_str())))
            return FText::FromString(TEXT("Porozmawiaj: ") + FString(UTF8_TO_TCHAR(N.name.c_str())));
    return FText::GetEmpty();
}
void AResidentNPC::Interact(ASliceCharacter *Player)
{
    for (const auto &N : Wroclaw::NPCs())
        if (DefinitionId == FName(UTF8_TO_TCHAR(N.id.c_str())))
        {
            CastChecked<ASliceController>(Player->GetController())
                ->ShowMessage(UTF8_TO_TCHAR(N.dialogue.c_str()));
            return;
        }
}

void AResidentNPC::BeginPlay()
{
    Super::BeginPlay();
    if (const auto *Snap =
            GetGameInstance()->GetSubsystem<USliceMission>()->NPCs.Find(DefinitionId.ToString()))
        SetActorTransform(Snap->Transform, false, nullptr, ETeleportType::TeleportPhysics);
}
void AResidentNPC::EndPlay(const EEndPlayReason::Type Reason)
{
    if (Reason == EEndPlayReason::RemovedFromWorld)
    {
        FWTGNPCSnapshot Snap;
        Snap.Transform = GetActorTransform();
        GetGameInstance()->GetSubsystem<USliceMission>()->NPCs.Add(DefinitionId.ToString(), Snap);
    }
    Super::EndPlay(Reason);
}
