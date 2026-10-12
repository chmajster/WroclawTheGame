#include "Geography/DistrictRegistry.h"
#include "Components/SceneComponent.h"

AWTGDistrictRegistry::AWTGDistrictRegistry()
{
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    PrimaryActorTick.bCanEverTick = false;
    bIsSpatiallyLoaded = false;
}
