#include "Systems/WroclawMapSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

namespace
{
FVector2D SectorCenter(const FCitySectorDefinition &Sector)
{
    if (Sector.Boundary.IsEmpty())
        return FVector2D::ZeroVector;
    FVector2D Sum = FVector2D::ZeroVector;
    for (const FVector2D &Point : Sector.Boundary)
        Sum += Point;
    return Sum / Sector.Boundary.Num();
}

const TCHAR *CoverageLabel(ECityCoverageStatus Status)
{
    switch (Status)
    {
    case ECityCoverageStatus::Missing: return TEXT("Missing");
    case ECityCoverageStatus::GISOnly: return TEXT("GIS");
    case ECityCoverageStatus::Blockout: return TEXT("Blockout");
    case ECityCoverageStatus::Playable: return TEXT("Playable");
    case ECityCoverageStatus::Detailed: return TEXT("Detailed");
    case ECityCoverageStatus::Final: return TEXT("Final");
    default: return TEXT("Unknown");
    }
}
}

void ACityRegistry::BeginPlay()
{
    Super::BeginPlay();
    GetWorld()->GetSubsystem<UWroclawMapSubsystem>()->RegisterCity(Definition);
}
void UWroclawMapSubsystem::RegisterCity(UCityDefinition *City)
{
    Definition = City;
    if (!FindSector(WaypointSector))
        WaypointSector = NAME_None;
}
const FCitySectorDefinition *UWroclawMapSubsystem::SectorAt(const FVector &Position) const
{
    if (!Definition) return nullptr;
    for (const auto &Sector : Definition->Sectors)
    {
        bool Inside = false;
        for (int32 I = 0, J = Sector.Boundary.Num() - 1; I < Sector.Boundary.Num(); J = I++)
        {
            const auto &A = Sector.Boundary[I], &B = Sector.Boundary[J];
            if ((A.Y > Position.Y) != (B.Y > Position.Y) &&
                Position.X < (B.X - A.X) * (Position.Y - A.Y) / (B.Y - A.Y) + A.X)
                Inside = !Inside;
        }
        if (Inside) return &Sector;
    }
    return nullptr;
}
const FCitySectorDefinition *UWroclawMapSubsystem::FindSector(FName SectorId) const
{
    if (!Definition || SectorId.IsNone()) return nullptr;
    return Definition->Sectors.FindByPredicate([SectorId](const FCitySectorDefinition &Sector) {
        return Sector.Id == SectorId;
    });
}
bool UWroclawMapSubsystem::SetWaypoint(FName SectorId)
{
    if (!FindSector(SectorId))
        return false;
    WaypointSector = SectorId;
    return true;
}
bool UWroclawMapSubsystem::CycleWaypoint()
{
    if (!Definition || Definition->Sectors.IsEmpty())
        return false;

    int32 Start = 0;
    if (!WaypointSector.IsNone())
    {
        const int32 Current = Definition->Sectors.IndexOfByPredicate([this](const FCitySectorDefinition &Sector) {
            return Sector.Id == WaypointSector;
        });
        if (Current != INDEX_NONE)
            Start = (Current + 1) % Definition->Sectors.Num();
    }

    for (int32 Offset = 0; Offset < Definition->Sectors.Num(); ++Offset)
    {
        const int32 Index = (Start + Offset) % Definition->Sectors.Num();
        const auto &Sector = Definition->Sectors[Index];
        if (Sector.Status != ECityCoverageStatus::Missing)
        {
            WaypointSector = Sector.Id;
            return true;
        }
    }
    return false;
}
FString UWroclawMapSubsystem::PhoneMapText() const
{
    if (!Definition) return TEXT("\n\nMAPA GIS niedostępna na tej mapie.\n");

    const APawn *Player = UGameplayStatics::GetPlayerPawn(this, 0);
    const FVector PlayerPosition = Player ? Player->GetActorLocation() : FVector::ZeroVector;
    const FCitySectorDefinition *CurrentSector = Player ? SectorAt(PlayerPosition) : nullptr;
    const FCitySectorDefinition *Waypoint = FindSector(WaypointSector);

    FString Text = TEXT("\n\nMAPA GIS WROCŁAWIA\n");
    if (CurrentSector)
        Text += FString::Printf(TEXT("Pozycja: %s [%s]\n"),
                               *CurrentSector->DisplayName, CoverageLabel(CurrentSector->Status));
    else if (Player)
        Text += TEXT("Pozycja: poza zarejestrowanym sektorem GIS\n");

    if (Waypoint)
    {
        const FVector2D Center = SectorCenter(*Waypoint);
        const double DistanceMeters = FVector2D::Distance(
            FVector2D(PlayerPosition.X, PlayerPosition.Y), Center) / 100.0;
        Text += FString::Printf(TEXT("Cel: %s — %.0f m w linii prostej\n"),
                               *Waypoint->DisplayName, DistanceMeters);
    }
    else
        Text += TEXT("Cel: brak\n");

    Text += TEXT("C — następny cel sektora | BACKSPACE — usuń cel\n\nOBSZARY\n");
    for (const auto &Sector : Definition->Sectors)
    {
        const bool bCurrent = CurrentSector && CurrentSector->Id == Sector.Id;
        const bool bWaypoint = Waypoint && Waypoint->Id == Sector.Id;
        Text += FString::Printf(TEXT("%s%s%s [%s] — %d bud. / %d dróg\n"),
                               bCurrent ? TEXT("> ") : TEXT("  "),
                               bWaypoint ? TEXT("* ") : TEXT(""),
                               *Sector.DisplayName,
                               CoverageLabel(Sector.Status),
                               Sector.BuildingCount, Sector.RoadCount);
    }
    Text += TEXT("\n* cel | > aktualny sektor\nRouting po ulicach pozostaje wyłączony do czasu integracji grafu runtime.\n");
    return Text;
}
