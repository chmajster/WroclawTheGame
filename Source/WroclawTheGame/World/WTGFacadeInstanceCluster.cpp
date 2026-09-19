#include "World/WTGFacadeInstanceCluster.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"

AWTGFacadeInstanceCluster::AWTGFacadeInstanceCluster()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);
}

UHierarchicalInstancedStaticMeshComponent* AWTGFacadeInstanceCluster::FindOrCreateComponent(UStaticMesh* Mesh)
{
    if (!Mesh)
    {
        return nullptr;
    }

    for (UHierarchicalInstancedStaticMeshComponent* Component : InstanceComponents)
    {
        if (Component && Component->GetStaticMesh() == Mesh)
        {
            return Component;
        }
    }

    const FName ComponentName = MakeUniqueObjectName(
        this,
        UHierarchicalInstancedStaticMeshComponent::StaticClass(),
        FName(*FString::Printf(TEXT("FacadeInstances_%s"), *Mesh->GetName())));

    UHierarchicalInstancedStaticMeshComponent* Component =
        NewObject<UHierarchicalInstancedStaticMeshComponent>(this, ComponentName, RF_Transactional);

    if (!Component)
    {
        return nullptr;
    }

    Component->SetupAttachment(SceneRoot);
    Component->SetStaticMesh(Mesh);
    Component->SetMobility(EComponentMobility::Static);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetCollisionProfileName(TEXT("NoCollision"));
    Component->SetCastShadow(true);
    AddInstanceComponent(Component);
    Component->RegisterComponent();
    InstanceComponents.Add(Component);

    return Component;
}

int32 AWTGFacadeInstanceCluster::AddFacadeInstance(UStaticMesh* Mesh, FTransform WorldTransform)
{
    if (UHierarchicalInstancedStaticMeshComponent* Component = FindOrCreateComponent(Mesh))
    {
        return Component->AddInstance(WorldTransform, true);
    }

    return INDEX_NONE;
}

int32 AWTGFacadeInstanceCluster::GetFacadeInstanceCount() const
{
    int32 Count = 0;

    for (const UHierarchicalInstancedStaticMeshComponent* Component : InstanceComponents)
    {
        if (Component)
        {
            Count += Component->GetInstanceCount();
        }
    }

    return Count;
}
