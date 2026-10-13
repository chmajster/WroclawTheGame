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
FBox UGeoMeshLibrary::GetAuthoredMeshBounds(UStaticMesh* Mesh,int32 LodIndex)
{
    FBox Bounds(ForceInit);
#if WITH_EDITOR
    if(Mesh && LodIndex>=0 && LodIndex<Mesh->GetNumSourceModels())
        if(const FMeshDescription* Description=Mesh->GetMeshDescription(LodIndex)){
            const auto Positions=Description->GetVertexPositions();
            for(const FVertexID Vertex:Description->Vertices().GetElementIDs())Bounds+=FVector(Positions[Vertex]);
        }
#endif
    return Bounds;
}
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
    auto Tangents = Attributes.GetVertexInstanceTangents();
    auto BinormalSigns = Attributes.GetVertexInstanceBinormalSigns();
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
        // Project walls in their dominant plane, not XY (which collapses
        // their UVs). Explicit orthogonal tangents also remain stable on
        // tiny triangles produced at exact sector boundaries.
        const FVector UAxis = FMath::Abs(Normal.Z)>.5f || FMath::Abs(Normal.Y)>=FMath::Abs(Normal.X)
            ? FVector::ForwardVector : FVector::RightVector;
        const FVector VAxis = FMath::Abs(Normal.Z)>.5f ? FVector::RightVector : FVector::UpVector;
        const FVector Tangent=FVector::VectorPlaneProject(UAxis,Normal).GetSafeNormal();
        const float Sign=FVector::DotProduct(FVector::CrossProduct(Normal,Tangent),VAxis)>=0 ? 1.f : -1.f;
        TArray<FVertexInstanceID> Instances;
        for (int32 J = 0; J < 3; J++)
        {
            // GIS inputs use mathematical CCW winding. UE MeshDescription
            // uses clockwise fronts (its normal is Cross(C-A, B-A)).
            const int32 Index = Triangles[I + (J==1?2:(J==2?1:0))];
            const FVertexInstanceID Instance = Description.CreateVertexInstance(IDs[Index]);
            Normals[Instance] = FVector3f(Normal);
            Tangents[Instance] = FVector3f(Tangent);
            BinormalSigns[Instance] = Sign;
            UVs.Set(Instance, 0, FVector2f(FVector::DotProduct(Vertices[Index],UAxis)/400,
                                         FVector::DotProduct(Vertices[Index],VAxis)/400));
            Instances.Add(Instance);
        }
        Description.CreatePolygon(Group, Instances);
    }
    UPackage *Package = CreatePackage(*Path);
    UStaticMesh *Mesh =
        NewObject<UStaticMesh>(Package, *FPackageName::GetShortName(Path), RF_Public | RF_Standalone);
    Mesh->GetStaticMaterials().Add(FStaticMaterial());
    // GIS supplies physical outward/upward normals explicitly. Preserve them
    // independently of the raster winding conversion during editor rebuilds.
    Mesh->SetNumSourceModels(1);
    Mesh->GetSourceModel(0).BuildSettings.bRecomputeNormals = false;
    Mesh->GetSourceModel(0).BuildSettings.bRecomputeTangents = false;
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
