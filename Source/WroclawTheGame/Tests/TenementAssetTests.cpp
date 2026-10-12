#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTenementAssetTest,"WTG.Assets.TenementKit",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)

bool FTenementAssetTest::RunTest(const FString&)
{
    struct FExpected { const TCHAR* Name; FVector Size; bool Nanite; int32 Hulls; int32 Slots; };
    const FExpected Assets[] = {
        {TEXT("ornate_balcony_railing"),FVector(250,110,110.685),false,3,1},
        {TEXT("stone_entrance_surround"),FVector(280,48,389),true,0,1},
        {TEXT("dentil_cornice"),FVector(300,49,44),true,0,1},
        {TEXT("balcony_slab"),FVector(270,120,60),true,1,1},
        {TEXT("paneled_entrance_door"),FVector(179.4,21.868,230),false,0,3}
    };
    for(const FExpected& Expected:Assets)
    {
        const FString Path=FString::Printf(TEXT("/Game/Wroclaw/TenementKit/SM_wtg_tenement_%s"),Expected.Name);
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,*Path);
        if(!TestNotNull(*Path,Mesh)) continue;
        TestTrue(TEXT("Metre source converted to centimetres"),Mesh->GetBounds().BoxExtent.Equals(Expected.Size*.5,1));
        TestEqual(TEXT("Three integrated LODs"),Mesh->GetNumLODs(),3);
        for(int32 LOD=0;LOD<Mesh->GetNumLODs();++LOD)
        {
            TestTrue(TEXT("UV layer retained"),Mesh->GetNumUVChannels(LOD)>0);
            TestTrue(TEXT("Render geometry retained"),Mesh->GetNumTriangles(LOD)>0);
            // Nanite can simplify its separate LOD0 fallback below LOD1.
            // Verify imported source LODs rather than that derived fallback.
            const auto* Source=Mesh->GetMeshDescription(LOD);
            if(TestNotNull(TEXT("Imported source LOD retained"),Source) && LOD)
            {
                const auto* Previous=Mesh->GetMeshDescription(LOD-1);
                if(Previous) TestTrue(TEXT("Imported LOD triangle count decreases"),Source->Triangles().Num()<Previous->Triangles().Num());
            }
        }
        TestEqual(TEXT("Manifest Nanite policy retained"),Mesh->NaniteSettings.bEnabled,Expected.Nanite);
        if(Expected.Nanite)
        {
            TestEqual(TEXT("Preserve full stone fallback"),Mesh->NaniteSettings.FallbackPercentTriangles,1.f);
            if(const auto* Source=Mesh->GetMeshDescription(0))
                TestEqual(TEXT("Small ornament retained in fallback"),Mesh->GetNumTriangles(0),Source->Triangles().Num());
        }
        TestEqual(TEXT("Authored material slots"),Mesh->GetStaticMaterials().Num(),Expected.Slots);
        for(int32 Slot=0;Slot<Expected.Slots;++Slot)
        {
            auto* Material=Mesh->GetMaterial(Slot);
            const FString Prefix=Expected.Slots==3 ? TEXT("/Game/SurfaceQuality/Instances/MI_") : TEXT("/Game/Wroclaw/TenementKit/M_");
            TestTrue(TEXT("Authored PBR material bound"),Material && Material->GetPathName().StartsWith(Prefix));
        }
        auto* Body=Mesh->GetBodySetup();
        if(!TestNotNull(TEXT("Body setup exists"),Body)) continue;
        TestEqual(TEXT("Expected authored collision hull count"),Body->AggGeom.ConvexElems.Num(),Expected.Hulls);
        TestEqual(TEXT("No automatic enclosing primitive"),Body->AggGeom.SphereElems.Num()+Body->AggGeom.BoxElems.Num()+Body->AggGeom.SphylElems.Num(),0);
        for(const auto& Hull:Body->AggGeom.ConvexElems)
        {
            FBox Bounds(ForceInit);
            for(const FVector& Vertex:Hull.VertexData) Bounds+=Vertex;
            if(Expected.Hulls==3)
            {
                TestFalse(TEXT("Balcony central space remains open"),Bounds.IsInsideOrOn(FVector(0,0,50)));
                TestFalse(TEXT("Back towards doorway remains open"),Bounds.IsInsideOrOn(FVector(0,50,50)));
            }
        }
    }
    return !HasAnyErrors();
}
#endif
