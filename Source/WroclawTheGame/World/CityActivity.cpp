#include "World/CityActivity.h"
#include "Systems/CityGameplaySubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
ACityActivity::ACityActivity()
{
    Collider = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBounds"));
    SetRootComponent(Collider);
    Collider->SetBoxExtent(FVector(40,40,60));
    Collider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Collider->SetCollisionResponseToAllChannels(ECR_Ignore);
    Collider->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
    Mesh->SetupAttachment(Collider);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetCanEverAffectNavigation(false);
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
