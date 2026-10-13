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
FIntVector UGeoMeshLibrary::GetAuthoredMeshBasisStats(UStaticMesh* Mesh,int32 LodIndex)
{
    FIntVector Result=FIntVector::ZeroValue;
#if WITH_EDITOR
    if(Mesh && LodIndex>=0 && LodIndex<Mesh->GetNumSourceModels())
        if(const FMeshDescription* Description=Mesh->GetMeshDescription(LodIndex)){
            const FStaticMeshConstAttributes Attributes(*Description);
            const auto Tangents=Attributes.GetVertexInstanceTangents();
            const auto Normals=Attributes.GetVertexInstanceNormals();
            for(const FVertexInstanceID ID:Description->VertexInstances().GetElementIDs()){
                ++Result.X;Result.Y+=Tangents[ID].IsNearlyZero(.0001f);Result.Z+=Normals[ID].IsNearlyZero(.0001f);
            }
        }
#endif
    return Result;
}
int32 UGeoMeshLibrary::RepairAuthoredMeshTangents(UStaticMesh* Mesh,int32 LodIndex)
{
    int32 Repaired=0;
#if WITH_EDITOR
    if(!Mesh || LodIndex<0 || LodIndex>=Mesh->GetNumSourceModels())return 0;
    FMeshDescription* Description=Mesh->GetMeshDescription(LodIndex);
    if(!Description)return 0;
    FStaticMeshAttributes Attributes(*Description);
    const auto Positions=Attributes.GetVertexPositions();
    const auto Normals=Attributes.GetVertexInstanceNormals();
    auto Tangents=Attributes.GetVertexInstanceTangents();
    auto Signs=Attributes.GetVertexInstanceBinormalSigns();
    const auto UVs=Attributes.GetVertexInstanceUVs();
    if(UVs.GetNumChannels()==0)return 0;
    for(const FTriangleID Triangle:Description->Triangles().GetElementIDs()){
        const auto IDs=Description->GetTriangleVertexInstances(Triangle);
        const FVector A(Positions[Description->GetVertexInstanceVertex(IDs[0])]);
        const FVector E1=FVector(Positions[Description->GetVertexInstanceVertex(IDs[1])])-A;
        const FVector E2=FVector(Positions[Description->GetVertexInstanceVertex(IDs[2])])-A;
        const FVector2D U1=FVector2D(UVs.Get(IDs[1],0))-FVector2D(UVs.Get(IDs[0],0));
        const FVector2D U2=FVector2D(UVs.Get(IDs[2],0))-FVector2D(UVs.Get(IDs[0],0));
        const double Det=U1.X*U2.Y-U1.Y*U2.X;
        for(const FVertexInstanceID ID:IDs){
            if(!Tangents[ID].IsNearlyZero(.0001f))continue;
            const FVector N=FVector(Normals[ID]).GetSafeNormal();
            if(N.IsNearlyZero())continue;
            FVector T=FMath::Abs(Det)>1.e-12 ? (E1*U2.Y-E2*U1.Y)/Det : E1;
            T=FVector::VectorPlaneProject(T,N).GetSafeNormal();
            if(T.IsNearlyZero()){
                const FVector Axis=FMath::Abs(N.X)<.8 ? FVector::ForwardVector : FVector::RightVector;
                T=FVector::VectorPlaneProject(Axis,N).GetSafeNormal();
            }
            const FVector B=FMath::Abs(Det)>1.e-12 ? (E2*U1.X-E1*U2.X)/Det : FVector::CrossProduct(N,T);
            Tangents[ID]=FVector3f(T);
            Signs[ID]=FVector::DotProduct(FVector::CrossProduct(N,T),B)>=0 ? 1.f : -1.f;
            ++Repaired;
        }
    }
    if(Repaired){
        Mesh->GetSourceModel(LodIndex).BuildSettings.bRecomputeTangents=false;
        Mesh->CommitMeshDescription(LodIndex);
        Mesh->Build(false);
        Mesh->MarkPackageDirty();
    }
#endif
    return Repaired;
}
UStaticMesh *UGeoMeshLibrary::BakeMesh(const FString &Path, const TArray<FVector> &Vertices,
                                       const TArray<int32> &Triangles,
                                       bool EnableNanite, bool GenerateDistanceField)
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
    // Sector-sized opaque GIS geometry can use Nanite's shadow path. Retain
    // the complete fallback for complex collision and software Lumen.
    auto& Nanite=Mesh->GetNaniteSettings();
    Nanite.bEnabled=EnableNanite;
    Nanite.bExplicitTangents=true;
    Nanite.FallbackTarget=ENaniteFallbackTarget::PercentTriangles;
    Nanite.FallbackPercentTriangles=1.f;
    Nanite.FallbackRelativeError=0.f;
    // Open, nearly planar ground is unsuitable for a thick voxelized mesh SDF.
    // Architectural volumes retain SDFs; this does not disable raster shadows.
    Mesh->GetSourceModel(0).BuildSettings.DistanceFieldResolutionScale=GenerateDistanceField?1.f:0.f;
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
