#include "World/CityActivity.h"
#include "Systems/CityGameplaySubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
ACityActivity::ACityActivity()
{
    auto *Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
    SetRootComponent(Mesh);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Mesh->SetStaticMesh(Cube.Object);
    Mesh->SetRelativeScale3D(FVector(.4, .4, .6));
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}
void ACityActivity::BeginPlay()
{
    Super::BeginPlay();
    GetWorld()->GetSubsystem<UCityGameplaySubsystem>()->Register(this);
}
FText ACityActivity::Prompt(ASliceCharacter *Player) const
{
    const auto *Action = Wroclaw::CityProgress::Find(TCHAR_TO_UTF8(*ActionId));
    if (!Action) return FText::GetEmpty();
    const bool Done = GetWorld()->GetSubsystem<UCityGameplaySubsystem>()->IsComplete(ActionId);
    return FText::FromString(FString(Done ? TEXT("Zapisano: ") : TEXT("Zbadaj: ")) + UTF8_TO_TCHAR(Action->title.c_str()));
}
void ACityActivity::Interact(ASliceCharacter *Player)
{
    GetWorld()->GetSubsystem<UCityGameplaySubsystem>()->Interact(this, Player);
}
