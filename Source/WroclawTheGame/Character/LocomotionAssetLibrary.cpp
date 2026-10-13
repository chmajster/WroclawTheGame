#include "Character/LocomotionAssetLibrary.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace1D.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimData/IAnimationDataController.h"
#if WITH_EDITOR
#include "PhysicsAssetUtils.h"
#include "PhysicsEngine/PhysicsAsset.h"
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

float ULocomotionAssetLibrary::MeasureCivilianStride(UAnimSequence* Animation,USkeletalMesh* Mesh)
{
#if WITH_EDITOR
    if(!Animation || !Mesh || Animation->GetPlayLength()<=0)return 0;
    const auto Model=Animation->GetDataModelInterface();if(!Model)return 0;
    const auto& Ref=Mesh->GetRefSkeleton();
    TArray<TArray<FTransform>> Tracks;Tracks.SetNum(Ref.GetNum());int32 Frames=0;
    for(int32 Bone=0;Bone<Ref.GetNum();++Bone){if(Model->IsValidBoneTrackName(Ref.GetBoneName(Bone)))Model->GetBoneTrackTransforms(Ref.GetBoneName(Bone),Tracks[Bone]);Frames=FMath::Max(Frames,Tracks[Bone].Num());}
    if(Frames<3)return 0;
    TArray<FVector> Feet[2];const int32 FootIndex[]={Ref.FindBoneIndex(TEXT("foot_l")),Ref.FindBoneIndex(TEXT("foot_r"))};
    if(FootIndex[0]==INDEX_NONE || FootIndex[1]==INDEX_NONE)return 0;
    for(int32 Frame=0;Frame<Frames;++Frame){
        TArray<FTransform> Component;Component.SetNum(Ref.GetNum());
        for(int32 Bone=0;Bone<Ref.GetNum();++Bone){
            const auto Local=Tracks[Bone].IsEmpty()?Ref.GetRefBonePose()[Bone]:Tracks[Bone][FMath::Min(Frame,Tracks[Bone].Num()-1)];
            const int32 Parent=Ref.GetParentIndex(Bone);Component[Bone]=Parent==INDEX_NONE?Local:Local*Component[Parent];
        }
        for(int32 Side=0;Side<2;++Side)Feet[Side].Add(Component[FootIndex[Side]].GetLocation());
    }
    const float Dt=Animation->GetPlayLength()/(Frames-1);TArray<float> Speeds;
    for(int32 Side=0;Side<2;++Side){
        float Floor=MAX_flt;for(const auto& Foot:Feet[Side])Floor=FMath::Min(Floor,float(Foot.Z));
        for(int32 Frame=1;Frame<Frames;++Frame){
            const float Backward=-(Feet[Side][Frame].Y-Feet[Side][Frame-1].Y)/Dt;
            if(Feet[Side][Frame].Z<Floor+6 && Feet[Side][Frame-1].Z<Floor+6 && Backward>20 && Backward<1200)Speeds.Add(Backward);
        }
    }
    if(Speeds.Num()<4)return 0;Speeds.Sort();return Speeds[Speeds.Num()/2];
#else
    return 0;
#endif
}

USkeleton* ULocomotionAssetLibrary::CreateCivilianSkeleton(USkeletalMesh* Mesh)
{
#if WITH_EDITOR
    if(!Mesh || !Mesh->GetRefSkeleton().GetNum())return nullptr;
    const FString Path=TEXT("/Game/Wroclaw/Hero/SK_CivilianSkeleton");
    auto* Skeleton=LoadObject<USkeleton>(nullptr,*Path);
    if(!Skeleton)
    {
        Skeleton=NewObject<USkeleton>(CreatePackage(*Path),TEXT("SK_CivilianSkeleton"),RF_Public|RF_Standalone);
        if(!Skeleton->MergeAllBonesToBoneTree(Mesh))return nullptr;
        FAssetRegistryModule::AssetCreated(Skeleton);
    }
    Mesh->SetSkeleton(Skeleton);Skeleton->MarkPackageDirty();Mesh->MarkPackageDirty();return Skeleton;
#else
    return nullptr;
#endif
}

UPhysicsAsset* ULocomotionAssetLibrary::CreateCivilianPhysics(USkeletalMesh* Mesh)
{
#if WITH_EDITOR
    if(!Mesh)return nullptr;
    if(Mesh->GetPhysicsAsset())return Mesh->GetPhysicsAsset();
    const FString Path=TEXT("/Game/Wroclaw/Hero/PHYS_Civilian");
    auto* Physics=LoadObject<UPhysicsAsset>(nullptr,*Path);
    if(!Physics)
    {
        Physics=NewObject<UPhysicsAsset>(CreatePackage(*Path),TEXT("PHYS_Civilian"),RF_Public|RF_Standalone);
        FPhysAssetCreateParams Params;FText Error;
        if(!FPhysicsAssetUtils::CreateFromSkeletalMesh(Physics,Mesh,Params,Error,true))
        {UE_LOG(LogTemp,Error,TEXT("Civilian physics: %s"),*Error.ToString());return nullptr;}
        FAssetRegistryModule::AssetCreated(Physics);
    }
    Mesh->SetPhysicsAsset(Physics);Physics->MarkPackageDirty();Mesh->MarkPackageDirty();return Physics;
#else
    return nullptr;
#endif
}

bool ULocomotionAssetLibrary::ConfigureCivilianLodSettings(USkeletalMesh* Mesh,USkeleton* Skeleton)
{
#if WITH_EDITOR
    if (!Mesh || !Skeleton || !Mesh->GetLODInfo(0)) return false;
    Mesh->Modify();
    Mesh->SetSkeleton(Skeleton);
    if(!Skeleton->MergeAllBonesToBoneTree(Mesh))return false;
    Skeleton->UpdateReferencePoseFromMesh(Mesh);
    Skeleton->MarkPackageDirty();
    while (Mesh->GetLODNum()<3) Mesh->AddLODInfo();
    for (int32 Index=0;Index<3;++Index)
    {
        auto* Info=Mesh->GetLODInfo(Index);
        Info->ScreenSize=Index==0?1.f:(Index==1?.20f:.065f);
        Info->ReductionSettings.BaseLOD=0;
        Info->ReductionSettings.TerminationCriterion=SMTC_NumOfTriangles;
        Info->ReductionSettings.NumOfTrianglesPercentage=Index==0?1.f:(Index==1?.65f:.35f);
        Info->ReductionSettings.bRemapMorphTargets=true;
        Info->ReductionSettings.bRecalcNormals=false;
        Info->ReductionSettings.MaxBonesPerVertex=4;
    }
    Mesh->MarkPackageDirty();
    return true;
#else
    return false;
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
        {
            UE_LOG(LogTemp,Error,TEXT("Civilian retarget: source animation/skeleton missing"));
            return false;
        }
        const auto Model = Input->GetDataModelInterface();
        if (!Model)
        {
            UE_LOG(LogTemp,Error,TEXT("Civilian retarget: source data model missing"));
            return false;
        }
        const auto &SourceRef = Input->GetSkeleton()->GetReferenceSkeleton();
        const auto &TargetRef = Mesh->GetRefSkeleton();
        for (int32 Bone=0;Bone<TargetRef.GetNum();++Bone)
            if (SourceRef.FindBoneIndex(TargetRef.GetBoneName(Bone))==INDEX_NONE && TargetRef.GetParentIndex(Bone)!=INDEX_NONE)
            {
                UE_LOG(LogTemp,Error,TEXT("Civilian retarget source lacks bone %s"),*TargetRef.GetBoneName(Bone).ToString());
                return false;
            }
        Sequence->SetSkeleton(Mesh->GetSkeleton());
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
            const int32 Parent = TargetRef.GetParentIndex(Bone);
            TargetBind[Bone] = Parent == INDEX_NONE ? TargetRef.GetRefBonePose()[Bone]
                                                    : TargetRef.GetRefBonePose()[Bone] * TargetBind[Parent];
            if (SourceBone == INDEX_NONE)
            {
                SourceBind[Bone]=FTransform::Identity;
                continue;
            }
            if(Model->IsValidBoneTrackName(Name))Model->GetBoneTrackTransforms(Name, Tracks[Bone]);
            FrameCount = FMath::Max(FrameCount, Tracks[Bone].Num());
            const FTransform Authoring = Bone == 0 && !Tracks[Bone].IsEmpty()
                                             ? Tracks[Bone][0]
                                             : SourceRef.GetRefBonePose()[SourceBone];
            SourceBind[Bone] = Parent == INDEX_NONE ? Authoring : Authoring * SourceBind[Parent];
        }
        TArray<TArray<FTransform>> Converted;
        // FBX/glTF tracks can store metre translations on a centimetre skeleton.
        // Infer that import unit from a rigid leg segment, not the moving pelvis.
        float TrackUnit=1;
        const int32 Probe=TargetRef.FindBoneIndex(TEXT("calf_l"));
        if(Probe!=INDEX_NONE && !Tracks[Probe].IsEmpty()){
            const int32 SourceProbe=SourceRef.FindBoneIndex(TEXT("calf_l"));
            const float ReferenceLength=SourceRef.GetRefBonePose()[SourceProbe].GetTranslation().Size();
            if(ReferenceLength>KINDA_SMALL_NUMBER)TrackUnit=Tracks[Probe][0].GetTranslation().Size()/ReferenceLength;
        }
        TrackUnit=FMath::Max(TrackUnit,.0001f);
        UE_LOG(LogTemp,Display,TEXT("Civilian retarget source=%s frames=%d bones=%d"),*Input->GetPathName(),FrameCount,TargetRef.GetNum());
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
                if(SourceBone==INDEX_NONE)
                {
                    SourcePose[Bone]=FTransform::Identity;TargetPose[Bone]=TargetBind[Bone];
                    Converted[Bone].Add(TargetRef.GetRefBonePose()[Bone]);continue;
                }
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
                // Component-space rotation conversion must not transfer source
                // limb lengths. Preserve the civilian's local joint offsets;
                // otherwise flexing a different hand rig stretches its fingers.
                FVector Translation=TargetRef.GetRefBonePose()[Bone].GetTranslation();
                if(TargetRef.GetBoneName(Bone)==TEXT("pelvis")){
                    const auto SourceRest=SourceRef.GetRefBonePose()[SourceBone].GetTranslation();
                    const float LengthRatio=Translation.Size()/FMath::Max(SourceRest.Size(),.0001);
                    FVector Delta=(Local.GetTranslation()/TrackUnit-SourceRest)*LengthRatio;
                    if(Parent!=INDEX_NONE)Delta=TargetBind[Parent].GetRotation().Inverse().RotateVector(SourceBind[Parent].GetRotation().RotateVector(Delta));
                    Translation+=Delta;
                }
                Result.SetTranslation(Translation);
                Result.SetScale3D(TargetRef.GetRefBonePose()[Bone].GetScale3D());
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
                Controller.AddBoneCurve(Name, false);
            const bool TrackChanged=Controller.SetBoneTrackKeys(Name, Positions, Rotations, Scales, false);
            if(!TrackChanged)UE_LOG(LogTemp,Error,TEXT("Civilian retarget cannot set %s (%d frames)"),*Name.ToString(),Positions.Num());
            Changed &= TrackChanged;
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
