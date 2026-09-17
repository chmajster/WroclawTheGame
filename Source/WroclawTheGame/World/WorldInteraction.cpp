#include "World/WorldInteraction.h"
#include "Components/GameplayComponents.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Character/SliceCharacter.h"
#include "Mission/SliceMission.h"
#include "UI/SliceController.h"
#include "World/SurveillanceCamera.h"
#include "EngineUtils.h"
AWorldInteraction::AWorldInteraction()
{
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Mesh->SetStaticMesh(Cube.Object);
    Mesh->SetRelativeScale3D(FVector(.65, .5, 1));
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
