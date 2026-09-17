#include "World/CityInteriorDoor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "Systems/CityGameplaySubsystem.h"
ACityInteriorDoor::ACityInteriorDoor()
{
    auto *Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Door"));SetRootComponent(Mesh);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Mesh->SetStaticMesh(Cube.Object);Mesh->SetRelativeScale3D(FVector(.25,1,1.8));
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    Mesh->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
}
FText ACityInteriorDoor::Prompt(ASliceCharacter *Player) const { return FText::FromString(Label); }
void ACityInteriorDoor::Interact(ASliceCharacter *Player)
{
    GetWorld()->GetSubsystem<UCityGameplaySubsystem>()->Travel(Player,Destination);
}
