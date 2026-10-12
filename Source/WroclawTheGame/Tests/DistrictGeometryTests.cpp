#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Geography/GeoMeshLibrary.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "PhysicsEngine/BodySetup.h"
#include "Interfaces/Interface_CollisionDataProvider.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDistrictMeshCollisionTest,"WTG.District.GeneratedMeshCollision",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDistrictMeshCollisionTest::RunTest(const FString&)
{
    auto* Mesh=UGeoMeshLibrary::BakeMesh(TEXT("/Game/Generated/GIS/Tests/DistrictCollision"),{FVector(0,0,0),FVector(1000,0,0),FVector(1000,1000,0),FVector(0,1000,0)},{0,1,2,0,2,3});
    if (!TestNotNull(TEXT("Mesh created"),Mesh)) return false;
    TestEqual(TEXT("Render triangles retained"),Mesh->GetNumTriangles(0),2);
    const auto& LOD=Mesh->GetRenderData()->LODResources[0];
    TestTrue(TEXT("Floor normal points up after UE winding conversion"),LOD.VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(0).Z>.99f);
    FTriMeshCollisionData Data;
    TestTrue(TEXT("Collision triangles available"),Mesh->GetPhysicsTriMeshData(&Data,false));
    TestEqual(TEXT("Two collision triangles"),Data.Indices.Num(),2);
    TestTrue(TEXT("Complex-as-simple cooked"),Mesh->GetBodySetup()->CollisionTraceFlag==CTF_UseComplexAsSimple);
    TestTrue(TEXT("Cooked triangle mesh exists"),!Mesh->GetBodySetup()->TriMeshGeometries.IsEmpty());
    return true;
}
#endif
