#include "World/SurfaceQualityLibrary.h"
#include "Materials/MaterialInterface.h"
#if WITH_EDITOR
#include "Materials/Material.h"
#include "MaterialShared.h"
#include "ShaderCompiler.h"
#endif

TArray<FString> USurfaceQualityLibrary::ValidateSurfaceShaders(const TArray<UMaterialInterface *> &Materials)
{
    TArray<FString> Errors;
#if WITH_EDITOR
    if (GShaderCompilingManager)
        GShaderCompilingManager->FinishAllCompilation();
    for (auto *Material : Materials)
    {
        if (!Material)
        {
            Errors.Add(TEXT("Missing material"));
            continue;
        }
        const auto *Resource = Material->GetMaterial()->GetMaterialResource(SP_PCD3D_SM6);
        if (!Resource)
        {
            Errors.Add(Material->GetPathName() + TEXT(": no SM6 resource"));
            continue;
        }
        for (const auto &Error : Resource->GetCompileErrors())
            Errors.Add(Material->GetPathName() + TEXT(": ") + Error);
        if (!Resource->GetGameThreadShaderMap())
            Errors.Add(Material->GetPathName() + TEXT(": no compiled shader map"));
    }
#else
    Errors.Add(TEXT("Shader validation requires an editor build"));
#endif
    return Errors;
}
