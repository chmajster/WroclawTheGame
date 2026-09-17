#include "Systems/OpenWorldSubsystem.h"
#include "Perception/AISense_Hearing.h"
#include "Systems/NoiseSystem.h"
#include "Mission/SliceMission.h"
#include "Character/SliceCharacter.h"
#include "Components/GameplayComponents.h"
#include "Components/SpotLightComponent.h"
#include "AI/SliceEnemy.h"
#include "Core/SliceGameMode.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
void UOpenWorldSubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UGameplayEventBus>();
    GetWorld()->GetSubsystem<UGameplayEventBus>()->OnEvent.AddDynamic(this,
                                                                      &UOpenWorldSubsystem::HandleEvent);
}
void UOpenWorldSubsystem::Deinitialize()
{
    if (auto *Bus = GetWorld()->GetSubsystem<UGameplayEventBus>())
        Bus->OnEvent.RemoveDynamic(this, &UOpenWorldSubsystem::HandleEvent);
    Super::Deinitialize();
}
void UOpenWorldSubsystem::HandleEvent(const FWTGGameplayEvent &E)
{
    auto *M = GetWorld()->GetGameInstance()->GetSubsystem<USliceMission>();
    if (!M->bInGame)
        return;
    if (E.Type.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("Event.Noise"))))
        M->WorldState.AddHeat(E.Value);
    if (E.Type.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("Event.Alarm"))) ||
        E.Type.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("Event.AI.Alert"))))
    {
        M->WorldState.AddHeat(
            E.Type.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("Event.Alarm"))) ? E.Value : 5);
        const float Radius =
            E.Type.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("Event.AI.Alert"))) ? E.Value : 2400;
        for (TActorIterator<ASliceEnemy> It(GetWorld()); It; ++It)
            if (*It != E.Source && It->bActive && It->HealthState->Value > 0 &&
                FVector::Dist(It->GetActorLocation(), E.Location) < Radius)
                if (auto *AI = Cast<ASliceEnemyController>(It->GetController()))
                    AI->Investigate(E.Location);
    }
}
float UOpenWorldSubsystem::VisibilityTo(AActor *Observer, ASliceCharacter *P) const
{
    if (!Observer || !P)
        return 0;
    auto *M = P->Mission();
    const auto *Weather = M->WorldState.Weather();
    const auto *District =
        M->WorldState.District(P->GetActorLocation().X, P->GetActorLocation().Y, P->GetActorLocation().Z);
    const double Light =
        (District ? District->light : 1) * (M->WorldState.hour >= 20 || M->WorldState.hour < 6 ? .55 : 1);
    return Wroclaw::WorldState::Visibility(FVector::Dist(Observer->GetActorLocation(), P->GetActorLocation()),
                                           Light, P->bIsCrouched, P->GetVelocity().Size2D(),
                                           P->Hiding.IsValid(), Weather ? Weather->visibility : 1,
                                           P->Flashlight->IsVisible());
}
void UOpenWorldSubsystem::Tick(float Dt)
{
    auto *M = GetWorld()->GetGameInstance()->GetSubsystem<USliceMission>();
    if (!M->bInGame || M->bDead || M->bShowMenu || M->State.Finished() ||
        UGameplayStatics::IsGamePaused(this))
        return;
    auto *P = Cast<ASliceCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
    auto *GM = Cast<ASliceGameMode>(GetWorld()->GetAuthGameMode());
    if (!P || !GM || !GM->IsReady())
        return;
    M->WorldState.Tick(Dt, M->Threat(), P->Hiding.IsValid());
    Accumulator += Dt;
    if (Accumulator < .5)
        return;
    Accumulator = 0;
    const FVector Position = P->GetActorLocation();
    M->WorldState.Enter(M->WorldState.District(Position.X, Position.Y, Position.Z));
    auto *Bus = GetWorld()->GetSubsystem<UGameplayEventBus>();
    for (const auto &L : Wroclaw::Locations())
    {
        const FVector Point(L.position[0], L.position[1], L.position[2]);
        const float D = FVector::Dist(Position, Point);
        if (D < L.radius)
        {
            const bool New = !M->WorldState.discoveries.count(L.id);
            M->WorldState.Discover(L.id, D < L.radius * .5);
            if (New)
            {
                M->Notify(TEXT("Odkryto: ") + FString(UTF8_TO_TCHAR(L.name.c_str())));
                Bus->Emit(TEXT("Event.Location.Discovered"), FName(UTF8_TO_TCHAR(L.id.c_str())));
            }
        }
    }
    for (const auto *Message : M->WorldState.NewMessages(M->State))
        M->Notify(TEXT("Nowa wiadomość od: ") + FString(UTF8_TO_TCHAR(Message->sender.c_str())));
    if (const auto *E = M->WorldState.SelectEvent(M->State, Position.X, Position.Y, Position.Z, true))
    {
        const FVector Point(E->position[0], E->position[1], E->position[2]);
        if (FVector::Dist(Point, Position) < 3500)
        {
            M->Notify(UTF8_TO_TCHAR(E->name.c_str()));
            Bus->Emit(TEXT("Event.World.Encounter"), FName(UTF8_TO_TCHAR(E->id.c_str())), 0, Point);
            if (E->noise > 0)
                UAISense_Hearing::ReportNoiseEvent(GetWorld(), Point, 1, P, E->noise);
            if (!E->action.empty())
                M->Act(UTF8_TO_TCHAR(E->action.c_str()));
        }
    }
#if !UE_BUILD_SHIPPING
    if (bPerceptionDebug)
        for (TActorIterator<ASliceEnemy> It(GetWorld()); It; ++It)
            DrawDebugLine(GetWorld(), It->GetActorLocation(), Position,
                          VisibilityTo(*It, P) > .12 ? FColor::Red : FColor::Green, false, .5);
#endif
}
