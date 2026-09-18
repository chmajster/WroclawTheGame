#include "World/WorldInteraction.h"
#include "Components/GameplayComponents.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Character/SliceCharacter.h"
#include "Mission/SliceMission.h"
#include "UI/SliceController.h"
#include "World/SurveillanceCamera.h"
#include "EngineUtils.h"
AWorldInteraction::AWorldInteraction()
{
    Collider = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBounds"));
    RootComponent = Collider;
    Collider->SetBoxExtent(FVector(32.5,25,50));
    Collider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Collider->SetCollisionResponseToAllChannels(ECR_Ignore);
    Collider->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Collider);Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->SetCanEverAffectNavigation(false);
    Hideable = CreateDefaultSubobject<UHideableComponent>(TEXT("Hideable"));
}
void AWorldInteraction::BeginPlay()
{
    Super::BeginPlay();
    for (const auto &H : Wroclaw::Hides())
        if (DefinitionId == FName(UTF8_TO_TCHAR(H.id.c_str())))
        {
            Hideable->HideId = DefinitionId;
            Hideable->ExitLocation = FVector(H.exit[0], H.exit[1], H.exit[2]);
        }
}
FText AWorldInteraction::Prompt(ASliceCharacter *Player) const
{
    return FText::FromString(Kind == TEXT("Hide")
                                 ? TEXT("Ukryj się / E — wyjście")
                                 : (Kind == TEXT("CCTV") ? TEXT("Podgląd kamery") : TEXT("Zbadaj")));
}
void AWorldInteraction::Interact(ASliceCharacter *Player)
{
    if (Kind == TEXT("Hide"))
    {
        Hideable->Enter(Player);
        return;
    }
    if (Kind == TEXT("CCTV"))
    {
        for (TActorIterator<ASurveillanceCamera> It(GetWorld()); It; ++It)
            if (It->DefinitionId == DefinitionId)
            {
                CastChecked<ASliceController>(Player->GetController())->OpenCCTV(It->Feed());
                return;
            }
    }
}
