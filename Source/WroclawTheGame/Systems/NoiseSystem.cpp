#include "Systems/NoiseSystem.h"
#include "Systems/GameplayEventBus.h"
#include "Content/WorldCatalog.h"
#include "Mission/SliceMission.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Perception/AISense_Hearing.h"
#include "DrawDebugHelpers.h"
void UNoiseSystem::Report(FName Type, const FVector &Location, APawn *Source)
{
    for (const auto &N : Wroclaw::NoiseProfiles())
        if (Type == FName(UTF8_TO_TCHAR(N.id.c_str())))
        {
            auto *M = GetWorld()->GetGameInstance()->GetSubsystem<USliceMission>();
            const auto *Weather = M->WorldState.Weather();
            const float Mult = Weather ? Weather->noise : 1;
            UAISense_Hearing::ReportNoiseEvent(GetWorld(), Location, N.intensity * Mult, Source, N.radius,
                                               Type);
            GetWorld()->GetSubsystem<UGameplayEventBus>()->Emit(TEXT("Event.Noise"), Type, N.heat, Location,
                                                                Source);
#if !UE_BUILD_SHIPPING
            if (bDebug)
                DrawDebugSphere(GetWorld(), Location, N.radius, 24, FColor::Yellow, false, 1.5);
#endif
            return;
        }
}
