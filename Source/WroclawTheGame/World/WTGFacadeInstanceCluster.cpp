#include "World/WTGFacadeInstanceCluster.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "TimerManager.h"

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

    for (UHierarchicalInstancedStaticMeshComponent* Component : FacadeComponents)
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
    FacadeComponents.Add(Component);

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

void AWTGFacadeInstanceCluster::BeginPlay()
{
    Super::BeginPlay();
    // Editor commandlet worlds have no query scene. Check support after the
    // game world's baked mesh physics has registered, including loaded maps.
    if(GetActorNameOrLabel().StartsWith(TEXT("District_Facades_"))){
        FTimerHandle Handle;GetWorldTimerManager().SetTimer(Handle,this,&AWTGFacadeInstanceCluster::ValidateWallSupport,2,false);
    }
}

void AWTGFacadeInstanceCluster::ValidateWallSupport()
{
    int32 Kept=0,Removed=0;
    for(UHierarchicalInstancedStaticMeshComponent* Component:FacadeComponents){
        if(!Component || !Component->GetStaticMesh())continue;
        const auto Bounds=Component->GetStaticMesh()->GetBounds();
        Component->bAutoRebuildTreeOnInstanceChanges=false;
        for(int32 I=Component->GetInstanceCount()-1;I>=0;--I){
            FTransform Pose;Component->GetInstanceTransform(I,Pose,true);
            const FVector Centre=Pose.TransformPosition(Bounds.Origin);
            const FVector Normal=Pose.TransformVectorNoScale(Bounds.BoxExtent.Y>=Bounds.BoxExtent.X?FVector::ForwardVector:FVector::RightVector).GetSafeNormal();
            FHitResult Wall;FCollisionQueryParams Params(SCENE_QUERY_STAT(FacadeWallSupport),true,this);
            if(!GetWorld()->LineTraceSingleByChannel(Wall,Centre+Normal*150,Centre-Normal*300,ECC_Visibility,Params) || FMath::Abs(Wall.ImpactNormal.Z)>.3f){Component->RemoveInstance(I);++Removed;continue;}
            Pose.AddToTranslation(Wall.ImpactPoint+Normal*5-Centre);
            Component->UpdateInstanceTransform(I,Pose,true,false,true);++Kept;
        }
        Component->bAutoRebuildTreeOnInstanceChanges=true;
        Component->BuildTreeIfOutdated(false,true);Component->MarkRenderStateDirty();
    }
    UE_LOG(LogTemp,Display,TEXT("WTG_FACADE_SUPPORT kept=%d removed=%d"),Kept,Removed);
}

int32 AWTGFacadeInstanceCluster::GetFacadeInstanceCount() const
{
    int32 Count = 0;

    for (const UHierarchicalInstancedStaticMeshComponent* Component : FacadeComponents)
    {
        if (Component)
        {
            Count += Component->GetInstanceCount();
        }
    }

    return Count;
}
