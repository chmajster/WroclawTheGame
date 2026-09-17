#include "Systems/CityGameplaySubsystem.h"
#include "World/CityActivity.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WorldPartition/WorldPartitionStreamingSourceComponent.h"
#include "Character/SliceCharacter.h"
#include "Vehicles/DriveableVehicle.h"
#include "Mission/SliceMission.h"
#include "Audio/SliceAudio.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
namespace { const TCHAR *CitySlot = TEXT("WroclawCity_v1"); }
void UCityGameplaySubsystem::Activate(const FVector &DefaultSpawn)
{
    if (bActive) return;
    InitialAnchor = SavedAnchor = DefaultSpawn;
    bActive = true;
    if (UGameplayStatics::DoesSaveGameExist(CitySlot, 0)) LoadSaved();
}
void UCityGameplaySubsystem::Register(ACityActivity *Actor) { Activities.AddUnique(Actor); }
bool UCityGameplaySubsystem::IsComplete(const FString &Id) const
{ return Progress.completed.count(TCHAR_TO_UTF8(*Id)) != 0; }
bool UCityGameplaySubsystem::LoadSaved()
{
    if (!bActive) return false;
    auto *Save = Cast<UCityProgressSave>(UGameplayStatics::LoadGameFromSlot(CitySlot, 0));
    Wroclaw::CityProgress Loaded;
    if (!Save || Save->Version != 1 || Save->Anchor.ContainsNaN() || Save->Anchor.GetAbsMax() > 20000000 ||
        (Save->HasVehicle && (Save->VehicleAnchor.ContainsNaN() || Save->VehicleAnchor.GetAbsMax()>20000000 ||
                              !FMath::IsFinite(Save->VehicleYaw) || !FMath::IsFinite(Save->VehicleHealth) ||
                              Save->VehicleHealth<0 || Save->VehicleHealth>100)) ||
        !Wroclaw::CityProgress::Deserialize(TCHAR_TO_UTF8(*Save->Payload), Loaded))
    {
        bWriteBlocked = true;
        GetWorld()->GetGameInstance()->GetSubsystem<USliceMission>()->Notify(
            TEXT("Nieprawidłowy lub nowszy zapis miasta. Zachowano plik; automatyczny zapis zablokowany."));
        return false;
    }
    LoadedSave=Save;
    Progress = MoveTemp(Loaded); SavedAnchor = Save->Anchor; bWriteBlocked = false; NextSaveAttempt = 0;
    return true;
}
bool UCityGameplaySubsystem::Persist(const FVector &Anchor, bool IncludeVehicle)
{
    if (!bActive || bWriteBlocked || Anchor.ContainsNaN()) return false;
    const std::string Payload = Progress.Serialize();
    if (Payload.empty()) return false;
    auto *Save = Cast<UCityProgressSave>(UGameplayStatics::CreateSaveGameObject(UCityProgressSave::StaticClass()));
    Save->Payload = UTF8_TO_TCHAR(Payload.c_str()); Save->Anchor = Anchor;
    if (IncludeVehicle)
    for (TActorIterator<ADriveableVehicle> It(GetWorld());It;++It)
    {
        Save->HasVehicle=true;Save->VehicleAnchor=It->GetActorLocation();Save->VehicleYaw=It->GetActorRotation().Yaw;
        Save->VehicleHealth=FMath::Clamp(It->Health,0.f,100.f);break;
    }
    if (!UGameplayStatics::SaveGameToSlot(Save, CitySlot, 0)) return false;
    LoadedSave=Save;SavedAnchor = Anchor; return true;
}
bool UCityGameplaySubsystem::NewRun()
{
    if (!bActive) return false;
    const auto Before = Progress; const bool WasBlocked = bWriteBlocked;
    Progress = Wroclaw::CityProgress(); bWriteBlocked = false;
    if (Persist(InitialAnchor, false)) return true;
    Progress = Before; bWriteBlocked = WasBlocked; return false;
}
void UCityGameplaySubsystem::Apply(ACityActivity *Actor, APawn *Player, float Dt, bool bInteract)
{
    if (!bActive || bWriteBlocked || !Actor || !Player || GetWorld()->GetTimeSeconds() < NextSaveAttempt) return;
    const auto *Action = Wroclaw::CityProgress::Find(TCHAR_TO_UTF8(*Actor->ActionId));
    if (!Action || !Progress.Available(*Action)) return;
    const auto *Character = Cast<ASliceCharacter>(Player);
    const auto *Vehicle = Cast<ADriveableVehicle>(Player);
    const float Radius = Action->seconds > 0 ? 3000.f : (Action->kind == "event" ? 1200.f : 250.f);
    const bool InRange = FVector::Dist(Actor->GetActorLocation(), Player->GetActorLocation()) <= Radius;
    const auto Before = Progress;
    if (!Progress.Step(Action->id, Dt, InRange, Vehicle && FMath::Abs(Vehicle->Speed) < 50,
                       Character && Character->bIsCrouched, bInteract)) return;
    auto *Mission = GetWorld()->GetGameInstance()->GetSubsystem<USliceMission>();
    if (!Persist(Player->GetActorLocation()))
    {
        Progress = Before; Progress.timers.erase(Action->id);
        NextSaveAttempt = GetWorld()->GetTimeSeconds() + 5;
        Mission->Notify(TEXT("Nie udało się zapisać miasta. Aktywność nie została zaliczona."));
        return;
    }
    Mission->Notify(UTF8_TO_TCHAR(Action->body.c_str()));
    USliceAudio::Play(this, Action->kind == "event" ? TEXT("Horn") : TEXT("Switch"), Actor->GetActorLocation(), .4);
}
void UCityGameplaySubsystem::Interact(ACityActivity *Actor, ASliceCharacter *Player)
{
    if (!bActive || !Actor || !Player) return;
    if (bWriteBlocked)
    {
        GetWorld()->GetGameInstance()->GetSubsystem<USliceMission>()->Notify(TEXT("Zapis miasta jest zablokowany. Istniejący plik został zachowany."));return;
    }
    if (GetWorld()->GetTimeSeconds()<NextSaveAttempt) return;
    const auto *Action = Wroclaw::CityProgress::Find(TCHAR_TO_UTF8(*Actor->ActionId));
    if (!Action) return;
    auto *Mission = GetWorld()->GetGameInstance()->GetSubsystem<USliceMission>();
    if (IsComplete(Actor->ActionId)) { Mission->Notify(UTF8_TO_TCHAR(Action->body.c_str())); return; }
    if (!Progress.Available(*Action)) { Mission->Notify(TEXT("Najpierw zbierz trzy tropy tej dzielnicy.")); return; }
    if (Action->vehicle) { Mission->Notify(TEXT("Obserwuj punkt przez 20 sekund z zatrzymanego samochodu.")); return; }
    if (Action->crouch && !Player->bIsCrouched) { Mission->Notify(TEXT("Przykucnij, aby dyskretnie sprawdzić skrytkę.")); return; }
    Apply(Actor, Player, 0, true);
}
void UCityGameplaySubsystem::Tick(float Dt)
{
    if (!bActive || UGameplayStatics::IsGamePaused(this)) return;
    if (Travelling.IsValid())
    {
        auto *Character=Travelling.Get();
        FHitResult Floor;FCollisionQueryParams Params(SCENE_QUERY_STAT(CityInteriorArrival),false,Character);
        const bool Loaded=Character->StreamingSource->IsStreamingCompleted();
        if (Loaded && GetWorld()->LineTraceSingleByChannel(Floor,TravelTarget+FVector(0,0,100),TravelTarget-FVector(0,0,250),ECC_Visibility,Params))
        {
            const FVector Point=Floor.ImpactPoint+FVector(0,0,94);
            if (!GetWorld()->OverlapBlockingTestByChannel(Point,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(36,92),Params))
            {
                Character->SetActorLocation(Point);Character->SetActorEnableCollision(true);
                Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);Travelling.Reset();
            }
        }
        if (Travelling.IsValid() && GetWorld()->GetTimeSeconds()-TravelStarted>15)
        {
            TravelTarget=TravelOrigin;Character->SetActorLocation(TravelOrigin);
            TravelStarted=GetWorld()->GetTimeSeconds();
            GetWorld()->GetGameInstance()->GetSubsystem<USliceMission>()->Notify(bReturningToOrigin ?
                TEXT("Oczekiwanie na bezpieczną kolizję wejścia. ESC pozwala wczytać zapis miasta.") :
                TEXT("Nie załadowano wnętrza; powrót do wejścia."));
            bReturningToOrigin=true;
        }
        return;
    }
    auto *Mission = GetWorld()->GetGameInstance()->GetSubsystem<USliceMission>();
    if (!Mission->bInGame || Mission->bDead || Mission->bShowMenu) return;
    auto *Player = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!Player || !Player->GetActorEnableCollision()) return;
    std::set<std::string> LoadedIds;
    for (int32 I = Activities.Num() - 1; I >= 0; --I)
    {
        if (!Activities[I].IsValid()) { Activities.RemoveAtSwap(I); continue; }
        LoadedIds.insert(TCHAR_TO_UTF8(*Activities[I]->ActionId));
        Apply(Activities[I].Get(), Player, Dt, false);
    }
    for (auto It = Progress.timers.begin(); It != Progress.timers.end();)
        if (!LoadedIds.count(It->first)) It = Progress.timers.erase(It); else ++It;
}
FString UCityGameplaySubsystem::Journal() const
{
    if (!bActive) return FString();
    FString Text = TEXT("WROCŁAW — DZIELNICE\n\n");
    for (const auto &Action : Wroclaw::CityActions())
    {
        if (Action.kind == "event") continue;
        Text += FString(Progress.completed.count(Action.id) ? TEXT("[zapisano] ") : TEXT("[ ] ")) +
                UTF8_TO_TCHAR(Action.title.c_str()) + TEXT("\n");
    }
    return Text + TEXT("\nT — telefon | szukaj znaczników w dzielnicach; postęp zapisuje się automatycznie.");
}
FString UCityGameplaySubsystem::NearbyObjective() const
{
    const auto *Player = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!bActive || !Player) return FString();
    const ACityActivity *Best = nullptr; double Distance = TNumericLimits<double>::Max();
    for (const auto &Weak : Activities)
    {
        if (!Weak.IsValid()) continue;
        const auto *Action = Wroclaw::CityProgress::Find(TCHAR_TO_UTF8(*Weak->ActionId));
        if (!Action || Action->kind == "event" || !Progress.Available(*Action)) continue;
        const double D = FVector::Dist(Weak->GetActorLocation(), Player->GetActorLocation());
        if (D < Distance) { Distance = D; Best = Weak.Get(); }
    }
    if (!Best) return TEXT("Odkrywaj południowe dzielnice. B — dziennik miasta");
    const auto *Action = Wroclaw::CityProgress::Find(TCHAR_TO_UTF8(*Best->ActionId));
    const auto Timer = Progress.timers.find(Action->id);
    return FString::Printf(TEXT("%s — %.0f m%s"), UTF8_TO_TCHAR(Action->title.c_str()), Distance / 100,
        Timer != Progress.timers.end() ? *FString::Printf(TEXT(" | obserwacja %.0f / %.0f s"),Timer->second,Action->seconds) : TEXT(""));
}

void UCityGameplaySubsystem::Travel(ASliceCharacter *Player, const FVector &Destination)
{
    if (!bActive || !Player || Travelling.IsValid() || Destination.ContainsNaN()) return;
    Progress.timers.clear();Travelling=Player;TravelOrigin=Player->GetActorLocation();TravelTarget=Destination;
    TravelStarted=GetWorld()->GetTimeSeconds();bReturningToOrigin=false;
    Player->GetCharacterMovement()->DisableMovement();Player->SetActorEnableCollision(false);
    Player->SetActorLocation(Destination);Player->StreamingSource->EnableStreamingSource();
}

void UCityGameplaySubsystem::RestoreVehicle(ADriveableVehicle *Vehicle)
{
    if (!Vehicle || !LoadedSave || !LoadedSave->HasVehicle) return;
    Vehicle->Chassis->SetSimulatePhysics(false);
    Vehicle->SetActorLocation(LoadedSave->VehicleAnchor,false,nullptr,ETeleportType::TeleportPhysics);
    Vehicle->SetActorRotation(FRotator(0,LoadedSave->VehicleYaw,0));Vehicle->Health=LoadedSave->VehicleHealth;
}
