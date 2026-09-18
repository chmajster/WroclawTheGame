#include "World/CityInteriorDoor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Systems/CityGameplaySubsystem.h"
ACityInteriorDoor::ACityInteriorDoor()
{
    Collider=CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBounds"));SetRootComponent(Collider);
    Collider->SetBoxExtent(FVector(12.5,50,90));Collider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Collider->SetCollisionResponseToAllChannels(ECR_Ignore);Collider->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Door"));Mesh->SetupAttachment(Collider);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->SetCanEverAffectNavigation(false);
}
FText ACityInteriorDoor::Prompt(ASliceCharacter *Player) const { return FText::FromString(Label); }
void ACityInteriorDoor::Interact(ASliceCharacter *Player)
{
    GetWorld()->GetSubsystem<UCityGameplaySubsystem>()->Travel(Player,Destination);
}
