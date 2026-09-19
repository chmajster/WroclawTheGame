#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WTGFacadeInstanceCluster.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class USceneComponent;
class UStaticMesh;

UCLASS()
class WROCLAWTHEGAME_API AWTGFacadeInstanceCluster : public AActor
{
    GENERATED_BODY()

public:
    AWTGFacadeInstanceCluster();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Facade")
    TObjectPtr<USceneComponent> SceneRoot;

    UFUNCTION(BlueprintCallable, CallInEditor, Category="Facade")
    int32 AddFacadeInstance(UStaticMesh* Mesh, FTransform WorldTransform);

    UFUNCTION(BlueprintPure, Category="Facade")
    int32 GetFacadeInstanceCount() const;

private:
    UPROPERTY()
    TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> InstanceComponents;

    UHierarchicalInstancedStaticMeshComponent* FindOrCreateComponent(UStaticMesh* Mesh);
};
