#include "Character/LocomotionAssetLibrary.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace1D.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimData/IAnimationDataController.h"
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
    // Same hierarchy does not imply matching authored bone lengths/units.
    // These clips are in-place: retain the body's reference translations and
    // animate rotations, avoiding a collapsed body from source unit mismatch.
    for(int32 I=0;I<B.GetNum();++I)Target->SetBoneTranslationRetargetingMode(I,EBoneTranslationRetargetingMode::Skeleton);
    for(int32 I=0;I<A.GetNum();++I)Source->SetBoneTranslationRetargetingMode(I,EBoneTranslationRetargetingMode::Skeleton);
    Target->MarkPackageDirty();Source->MarkPackageDirty();return true;
#else
    return false;
#endif
}
UBlendSpace1D* ULocomotionAssetLibrary::BakeLocomotion(const FString& Path,USkeletalMesh* Mesh,UAnimSequence* Idle,UAnimSequence* Walk,UAnimSequence* Jog,UAnimSequence* Sprint)
{
#if WITH_EDITOR
    if ((!Path.StartsWith(TEXT("/Game/CharacterCreator/")) && !Path.StartsWith(TEXT("/Game/Wroclaw/Hero/"))) || !LinkCompatibleRig(Mesh,Idle) || !LinkCompatibleRig(Mesh,Walk) || !LinkCompatibleRig(Mesh,Jog) || !LinkCompatibleRig(Mesh,Sprint)) return nullptr;
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

bool ULocomotionAssetLibrary::RetargetCivilianAnimation(UAnimationAsset *Animation, USkeletalMesh *Mesh,
                                                             UAnimationAsset *Source)
{
#if WITH_EDITOR
    if (!Animation || !Mesh || !Mesh->GetSkeleton() || Mesh->GetRefSkeleton().GetNum() == 0)
        return false;
    if(Animation==Source)return false;
    Animation->Modify();
    if (auto *Sequence = Cast<UAnimSequence>(Animation))
    {
        auto *Input = Cast<UAnimSequence>(Source ? Source : Animation);
        if (!Input || !Input->GetSkeleton())
            return false;
        const auto Model = Input->GetDataModelInterface();
        if (!Model)
            return false;
        const auto &SourceRef = Input->GetSkeleton()->GetReferenceSkeleton();
        const auto &TargetRef = Mesh->GetRefSkeleton();
        auto &Controller = Sequence->GetController();
        Controller.OpenBracket(FText::FromString(TEXT("Civilian component-space reference conversion")), false);
        bool Changed = true;
        TArray<TArray<FTransform>> Tracks;
        Tracks.SetNum(TargetRef.GetNum());
        TArray<FTransform> SourceBind, TargetBind;
        SourceBind.SetNum(TargetRef.GetNum());
        TargetBind.SetNum(TargetRef.GetNum());
        int32 FrameCount = 0;
        for (int32 Bone = 0; Bone < TargetRef.GetNum(); ++Bone)
        {
            const FName Name = TargetRef.GetBoneName(Bone);
            const int32 SourceBone = SourceRef.FindBoneIndex(Name);
            if (SourceBone == INDEX_NONE)
                continue;
            Model->GetBoneTrackTransforms(Name, Tracks[Bone]);
            FrameCount = FMath::Max(FrameCount, Tracks[Bone].Num());
            const int32 Parent = TargetRef.GetParentIndex(Bone);
            const FTransform Authoring = Bone == 0 && !Tracks[Bone].IsEmpty()
                                             ? Tracks[Bone][0]
                                             : SourceRef.GetRefBonePose()[SourceBone];
            SourceBind[Bone] = Parent == INDEX_NONE ? Authoring : Authoring * SourceBind[Parent];
            TargetBind[Bone] = Parent == INDEX_NONE ? TargetRef.GetRefBonePose()[Bone]
                                                    : TargetRef.GetRefBonePose()[Bone] * TargetBind[Parent];
        }
        TArray<TArray<FTransform>> Converted;
        Converted.SetNum(TargetRef.GetNum());
        for (int32 Frame = 0; Frame < FrameCount; ++Frame)
        {
            TArray<FTransform> SourcePose, TargetPose;
            SourcePose.SetNum(TargetRef.GetNum());
            TargetPose.SetNum(TargetRef.GetNum());
            for (int32 Bone = 0; Bone < TargetRef.GetNum(); ++Bone)
            {
                const int32 Parent = TargetRef.GetParentIndex(Bone),
                            SourceBone = SourceRef.FindBoneIndex(TargetRef.GetBoneName(Bone));
                if (SourceBone == INDEX_NONE)
                    return false;
                const FTransform Local = Tracks[Bone].IsEmpty()
                                             ? SourceRef.GetRefBonePose()[SourceBone]
                                             : Tracks[Bone][FMath::Min(Frame, Tracks[Bone].Num() - 1)];
                SourcePose[Bone] = Parent == INDEX_NONE ? Local : Local * SourcePose[Parent];
                // Convert component-space skinning motion. Local-only conversion fails
                // when FBX and glTF importers choose different child bone rolls.
                TargetPose[Bone] = TargetBind[Bone] * SourceBind[Bone].Inverse() * SourcePose[Bone];
                FTransform Result = Parent == INDEX_NONE
                                        ? TargetPose[Bone]
                                        : TargetPose[Bone].GetRelativeTransform(TargetPose[Parent]);
                Result.NormalizeRotation();
                Converted[Bone].Add(Result);
            }
        }
        for (int32 Bone = 0; Bone < TargetRef.GetNum(); ++Bone)
        {
            TArray<FVector3f> Positions, Scales;
            TArray<FQuat4f> Rotations;
            for (const auto &Frame : Converted[Bone])
            {
                Positions.Add(FVector3f(Frame.GetTranslation()));
                Rotations.Add(FQuat4f(Frame.GetRotation()));
                Scales.Add(FVector3f(Frame.GetScale3D()));
            }
            const FName Name = TargetRef.GetBoneName(Bone);
            if (Tracks[Bone].IsEmpty())
                Controller.AddBoneTrack(Name, false);
            Changed &= Controller.SetBoneTrackKeys(Name, Positions, Rotations, Scales, false);
        }
        Controller.CloseBracket(false);
        if (!Changed)
            return false;
    }
    Animation->SetSkeleton(Mesh->GetSkeleton());
    Animation->MarkPackageDirty();
    return true;
#else
    return false;
#endif
}
