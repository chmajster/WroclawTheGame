#include "Systems/WroclawMapSubsystem.h"
#include "Engine/World.h"

void ACityRegistry::BeginPlay()
{
    Super::BeginPlay();
    GetWorld()->GetSubsystem<UWroclawMapSubsystem>()->RegisterCity(Definition);
}
void UWroclawMapSubsystem::RegisterCity(UCityDefinition *City)
{
    Definition = City;
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
FString UWroclawMapSubsystem::PhoneMapText() const
{
    if (!Definition) return FString();
    FString Text = TEXT("\n\nOBSZARY MIASTA\n");
    for (const auto &Sector : Definition->Sectors)
        Text += Sector.DisplayName +
                (Sector.Status >= ECityCoverageStatus::Playable ? TEXT("\n") : TEXT(" — w przygotowaniu\n"));
    return Text;
}
