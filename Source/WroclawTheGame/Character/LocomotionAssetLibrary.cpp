#include "Character/LocomotionAssetLibrary.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace1D.h"
#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#endif
bool ULocomotionAssetLibrary::LinkCompatibleRig(USkeletalMesh* Mesh,UAnimSequence* Animation)
{
#if WITH_EDITOR
    if (!Mesh || !Animation || !Mesh->GetSkeleton() || !Animation->GetSkeleton()) return false;
    auto* Target=Mesh->GetSkeleton();auto* Source=Animation->GetSkeleton();
    const auto& A=Source->GetReferenceSkeleton();const auto& B=Target->GetReferenceSkeleton();
    // Compatibility is only allowed for the same named hierarchy. KayKit and
    // other rigs require a real retarget; they must never silently pass here.
    for (int32 I=0;I<A.GetNum();++I)
    {
        const int32 J=B.FindBoneIndex(A.GetBoneName(I));
        if (J==INDEX_NONE) return false;
        const int32 ParentA=A.GetParentIndex(I),ParentB=B.GetParentIndex(J);
        if ((ParentA==INDEX_NONE)!=(ParentB==INDEX_NONE)) return false;
        if (ParentA!=INDEX_NONE && A.GetBoneName(ParentA)!=B.GetBoneName(ParentB)) return false;
    }
    Target->AddCompatibleSkeleton(Source);Source->AddCompatibleSkeleton(Target);
    Target->MarkPackageDirty();Source->MarkPackageDirty();return true;
#else
    return false;
#endif
}
UBlendSpace1D* ULocomotionAssetLibrary::BakeLocomotion(const FString& Path,USkeletalMesh* Mesh,UAnimSequence* Idle,UAnimSequence* Walk,UAnimSequence* Jog,UAnimSequence* Sprint)
{
#if WITH_EDITOR
    if (!Path.StartsWith(TEXT("/Game/CharacterCreator/")) || !LinkCompatibleRig(Mesh,Idle) || !LinkCompatibleRig(Mesh,Walk) || !LinkCompatibleRig(Mesh,Jog) || !LinkCompatibleRig(Mesh,Sprint)) return nullptr;
    auto* Space=LoadObject<UBlendSpace1D>(nullptr,*Path);
    if (!Space)
    {
        auto* Package=CreatePackage(*Path);
        Space=NewObject<UBlendSpace1D>(Package,*FPackageName::GetShortName(Path),RF_Public|RF_Standalone);
        FAssetRegistryModule::AssetCreated(Space);
    }
    Space->SetSkeleton(Mesh->GetSkeleton());
    while (Space->GetNumberOfBlendSamples()) Space->DeleteSample(0);
    auto& Axis=const_cast<FBlendParameter&>(Space->GetBlendParameter(0));
    Axis.DisplayName=TEXT("Speed");Axis.Min=0;Axis.Max=530;Axis.GridNum=8;
    Space->InterpolationParam[0].InterpolationTime=.18f;
    Space->AddSample(Idle,FVector(0,0,0));Space->AddSample(Walk,FVector(180,0,0));
    Space->AddSample(Jog,FVector(350,0,0));Space->AddSample(Sprint,FVector(530,0,0));
    Space->ValidateSampleData();Space->ResampleData();Space->MarkPackageDirty();return Space;
#else
    return nullptr;
#endif
}
