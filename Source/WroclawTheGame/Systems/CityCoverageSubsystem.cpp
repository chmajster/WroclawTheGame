#include "Systems/CityCoverageSubsystem.h"
#include "Systems/WroclawMapSubsystem.h"
#include "Engine/World.h"
FLinearColor UCityCoverageSubsystem::Color(ECityCoverageStatus Status)
{
    switch (Status)
    {
    case ECityCoverageStatus::GISOnly: return FLinearColor(.22f, .55f, .83f);
    case ECityCoverageStatus::Blockout: return FLinearColor(.90f, .54f, .20f);
    case ECityCoverageStatus::Playable: return FLinearColor(.89f, .78f, .28f);
    case ECityCoverageStatus::Detailed: return FLinearColor(.32f, .68f, .40f);
    case ECityCoverageStatus::Final: return FLinearColor(.68f, .46f, .87f);
    default: return FLinearColor(.45f, .45f, .45f);
    }
}
FString UCityCoverageSubsystem::Label(ECityCoverageStatus Status)
{
    switch (Status)
    {
    case ECityCoverageStatus::GISOnly: return TEXT("GISOnly");
    case ECityCoverageStatus::Blockout: return TEXT("Blockout");
    case ECityCoverageStatus::Playable: return TEXT("Playable");
    case ECityCoverageStatus::Detailed: return TEXT("Detailed");
    case ECityCoverageStatus::Final: return TEXT("Final");
    default: return TEXT("Missing");
    }
}
FString UCityCoverageSubsystem::ReportText() const
{
    const auto *City = GetWorld()->GetSubsystem<UWroclawMapSubsystem>()->GetCity();
    if (!City) return TEXT("Brak katalogu GIS w tej mapie.");
    int32 Playable = 0, Detailed = 0;
    for (const auto &Sector : City->Sectors)
    {
        Playable += Sector.Status >= ECityCoverageStatus::Playable ? 1 : 0;
        Detailed += Sector.Status >= ECityCoverageStatus::Detailed ? 1 : 0;
    }
    return FString::Printf(TEXT("Sektory: %d | Playable: %d | Detailed: %d"), City->Sectors.Num(), Playable, Detailed);
}
