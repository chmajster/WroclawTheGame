#include "World/CityStreamingProbe.h"
#include "Components/SceneComponent.h"
#include "WorldPartition/WorldPartitionStreamingSourceComponent.h"
ACityStreamingProbe::ACityStreamingProbe()
{
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    Source=CreateDefaultSubobject<UWorldPartitionStreamingSourceComponent>(TEXT("PredictiveSource"));
    Source->DisableStreamingSource();
}
