#include "Geography/GeoMeshLibrary.h"
#include "Engine/StaticMesh.h"
#if WITH_EDITOR
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshResources.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#endif
UStaticMesh *UGeoMeshLibrary::BakeMesh(const FString &Path, const TArray<FVector> &Vertices,
                                       const TArray<int32> &Triangles)
{
#if WITH_EDITOR
    if (!Path.StartsWith(TEXT("/Game/Generated/GIS/")) || Vertices.IsEmpty() || Triangles.Num() % 3)
        return nullptr;
    for (int32 Index : Triangles)
        if (!Vertices.IsValidIndex(Index))
            return nullptr;
    FMeshDescription Description;
    FStaticMeshAttributes Attributes(Description);
    Attributes.Register();
    auto Positions = Attributes.GetVertexPositions();
    auto Normals = Attributes.GetVertexInstanceNormals();
    auto UVs = Attributes.GetVertexInstanceUVs();
    UVs.SetNumChannels(1);
    TArray<FVertexID> IDs;
    for (const FVector &Position : Vertices)
    {
        FVertexID ID = Description.CreateVertex();
        Positions[ID] = FVector3f(Position);
        IDs.Add(ID);
    }
    const FPolygonGroupID Group = Description.CreatePolygonGroup();
    for (int32 I = 0; I < Triangles.Num(); I += 3)
    {
        const FVector Normal = FVector::CrossProduct(Vertices[Triangles[I + 1]] - Vertices[Triangles[I]],
                                                     Vertices[Triangles[I + 2]] - Vertices[Triangles[I]])
                                   .GetSafeNormal();
        TArray<FVertexInstanceID> Instances;
        for (int32 J = 0; J < 3; J++)
        {
            // GIS inputs use mathematical CCW winding. UE MeshDescription
            // uses clockwise fronts (its normal is Cross(C-A, B-A)).
            const int32 Index = Triangles[I + (J==1?2:(J==2?1:0))];
            const FVertexInstanceID Instance = Description.CreateVertexInstance(IDs[Index]);
            Normals[Instance] = FVector3f(Normal);
            UVs.Set(Instance, 0, FVector2f(Vertices[Index].X / 400, Vertices[Index].Y / 400));
            Instances.Add(Instance);
        }
        Description.CreatePolygon(Group, Instances);
    }
    UPackage *Package = CreatePackage(*Path);
    UStaticMesh *Mesh =
        NewObject<UStaticMesh>(Package, *FPackageName::GetShortName(Path), RF_Public | RF_Standalone);
    Mesh->GetStaticMaterials().Add(FStaticMaterial());
    UStaticMesh::FBuildMeshDescriptionsParams Params;
    Params.bBuildSimpleCollision = false;
    Params.bFastBuild = false;
    TArray<const FMeshDescription *> Descriptions;
    Descriptions.Add(&Description);
    Mesh->BuildFromMeshDescriptions(Descriptions, Params);
    Mesh->CreateBodySetup();
    // The editor build can cook a body before we switch to complex-as-simple.
    // Invalidate that cached body and explicitly retain collision on sections.
    for (int32 I=0;I<Mesh->GetRenderData()->LODResources[0].Sections.Num();++I)
    {
        auto Info=Mesh->GetSectionInfoMap().Get(0,I);
        Info.bEnableCollision=true;
        Mesh->GetSectionInfoMap().Set(0,I,Info);
        Mesh->GetRenderData()->LODResources[0].Sections[I].bEnableCollision=true;
    }
    Mesh->GetBodySetup()->CollisionTraceFlag = CTF_UseComplexAsSimple;
    Mesh->GetBodySetup()->bDoubleSidedGeometry=true;
    Mesh->GetBodySetup()->InvalidatePhysicsData();
    Mesh->GetBodySetup()->CreatePhysicsMeshes();
    FAssetRegistryModule::AssetCreated(Mesh);
    Mesh->MarkPackageDirty();
    return Mesh;
#else
    return nullptr;
#endif
}
