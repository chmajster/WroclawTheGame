#include "Character/CivilianAnimInstance.h"
#include "Character/SliceCharacter.h"
#include "Character/CharacterAppearanceComponent.h"
#include "Character/CharacterCreatorSubsystem.h"
#include "Animation/AnimSingleNodeInstanceProxy.h"
#include "Animation/BlendSpace1D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TwoBoneIK.h"

struct FCivilianAnimProxy final : FAnimSingleNodeInstanceProxy
{
    explicit FCivilianAnimProxy(UAnimInstance* Instance):FAnimSingleNodeInstanceProxy(Instance){}
    FTransform MeshToWorld=FTransform::Identity;
    FVector Ground[2]={FVector::ZeroVector,FVector::ZeroVector};
    FVector Normal[2]={FVector::UpVector,FVector::UpVector};
    bool Contact[2]={false,false},Grounded=false;
    FVector Forward=FVector::ForwardVector;
    float Dt=0,TransitionAge=1,Offsets[2]={0,0},Floor=0;
    int32 Serial=0,LastSerial=0;
    TArray<FTransform> PreviousPose;

    virtual void PreUpdate(UAnimInstance* Instance,float DeltaSeconds) override
    {
        FAnimSingleNodeInstanceProxy::PreUpdate(Instance,DeltaSeconds);
        const auto* Civilian=CastChecked<UCivilianAnimInstance>(Instance);
        const auto* Mesh=Instance->GetSkelMeshComponent();
        MeshToWorld=Mesh->GetComponentTransform();
        Ground[0]=Civilian->LeftGround;Ground[1]=Civilian->RightGround;
        Normal[0]=Civilian->LeftNormal;Normal[1]=Civilian->RightNormal;
        Contact[0]=Civilian->bLeftContact;Contact[1]=Civilian->bRightContact;
        Grounded=Civilian->bGrounded;Floor=Civilian->FloorZ;Serial=Civilian->TransitionSerial;Dt=DeltaSeconds;
        Forward=MeshToWorld.InverseTransformVectorNoScale(Mesh->GetOwner()->GetActorForwardVector()).GetSafeNormal();
    }
    virtual void PostUpdate(UAnimInstance* Instance) const override
    {
        FAnimSingleNodeInstanceProxy::PostUpdate(Instance);
        CastChecked<UCivilianAnimInstance>(Instance)->FootOffsets=FVector2D(Offsets[0],Offsets[1]);
    }
    virtual bool Evaluate(FPoseContext& Output) override
    {
        if(!FAnimSingleNodeInstanceProxy::Evaluate(Output))return false;
        const int32 Count=Output.Pose.GetNumBones();if(!Count)return true;
        if(Serial!=LastSerial){LastSerial=Serial;TransitionAge=0;}
        TransitionAge+=Dt;
        const float Blend=FMath::Clamp(TransitionAge/.18f,0.f,1.f);
        if(PreviousPose.Num()==Count && Blend<1)
            for(auto Bone:Output.Pose.ForEachBoneIndex())
            {FTransform Current=Output.Pose[Bone];Output.Pose[Bone].Blend(PreviousPose[Bone.GetInt()],Current,Blend);}
        PreviousPose.SetNum(Count);
        for(auto Bone:Output.Pose.ForEachBoneIndex())PreviousPose[Bone.GetInt()]=Output.Pose[Bone];
        const auto& Bones=Output.Pose.GetBoneContainer();
        auto Index=[&](const TCHAR* Name){return Bones.GetCompactPoseIndexFromSkeletonIndex(Bones.GetReferenceSkeleton().FindBoneIndex(Name));};
        const auto Pelvis=Index(TEXT("pelvis"));
        if(!Pelvis.IsValid())return true;
        TArray<FTransform> Component;Component.SetNum(Count);
        auto BuildComponent=[&](){for(auto Bone:Output.Pose.ForEachBoneIndex()){
            const auto Parent=Bones.GetParentBoneIndex(Bone);
            Component[Bone.GetInt()]=Parent.IsValid()?Output.Pose[Bone]*Component[Parent.GetInt()]:Output.Pose[Bone];
        }};
        BuildComponent();
        const TCHAR* ThighNames[]={TEXT("thigh_l"),TEXT("thigh_r")};
        const TCHAR* CalfNames[]={TEXT("calf_l"),TEXT("calf_r")};
        const TCHAR* FootNames[]={TEXT("foot_l"),TEXT("foot_r")};
        float Sole[2]={0,0},Weight[2]={0,0};FVector SoleAxis[2];
        TArray<FTransform> Reference;Reference.SetNum(Count);
        const auto& RefPose=Bones.GetRefPoseCompactArray();
        for(auto Bone:Output.Pose.ForEachBoneIndex()){
            auto Parent=Bones.GetParentBoneIndex(Bone);
            Reference[Bone.GetInt()]=Parent.IsValid()?RefPose[Bone.GetInt()]*Reference[Parent.GetInt()]:RefPose[Bone.GetInt()];
        }
        const float Scale=FMath::Max(.1f,MeshToWorld.GetScale3D().Z);
        for(int32 Side=0;Side<2;++Side){
            const auto Foot=Index(FootNames[Side]);if(!Foot.IsValid())continue;
            Sole[Side]=Reference[Foot.GetInt()].GetLocation().Z*Scale;
            SoleAxis[Side]=Reference[Foot.GetInt()].GetRotation().Inverse().RotateVector(FVector::UpVector);
            const auto Raw=MeshToWorld.TransformPosition(Component[Foot.GetInt()].GetLocation());
            // Fade correction during the swing phase; never pin an airborne foot.
            Weight[Side]=Grounded && Contact[Side]?1-FMath::Clamp((Raw.Z-Floor-Sole[Side]-4)/10,0.f,1.f):0;
            const float Target=Weight[Side]*FMath::Clamp(Ground[Side].Z+Sole[Side]-Raw.Z,-22.f,22.f);
            Offsets[Side]=FMath::FInterpTo(Offsets[Side],Target,Dt,18);
        }
        if(Grounded){
            const float PelvisOffset=FMath::Clamp(FMath::Min(Offsets[0],Offsets[1]),-10.f,0.f)/Scale;
            Output.Pose[Pelvis].AddToTranslation(FVector(0,0,PelvisOffset));BuildComponent();
        }
        for(int32 Side=0;Side<2;++Side){
            if(Weight[Side]<=.01f)continue;
            auto Thigh=Index(ThighNames[Side]),Calf=Index(CalfNames[Side]),Foot=Index(FootNames[Side]);
            if(!Thigh.IsValid() || !Calf.IsValid() || !Foot.IsValid())continue;
            auto Root=Component[Thigh.GetInt()],Joint=Component[Calf.GetInt()],End=Component[Foot.GetInt()];
            FVector Target=MeshToWorld.TransformPosition(End.GetLocation());Target.Z=Ground[Side].Z+Sole[Side];
            Target=MeshToWorld.InverseTransformPosition(Target);
            // Clamp reach to ordinary curb-sized corrections; no leg stretching.
            const float Correction=FMath::Clamp(Target.Z-End.GetLocation().Z,-22.f/Scale,22.f/Scale);
            Target.Z=End.GetLocation().Z+Correction*Weight[Side];
            AnimationCore::SolveTwoBoneIK(Root,Joint,End,Joint.GetLocation()+Forward*40,Target,false,1,1);
            FVector SurfaceNormal=MeshToWorld.InverseTransformVectorNoScale(Normal[Side]).GetSafeNormal();
            const FQuat Tilt=FQuat::FindBetweenNormals(End.GetRotation().RotateVector(SoleAxis[Side]),SurfaceNormal);
            End.SetRotation(FQuat::Slerp(FQuat::Identity,Tilt,Weight[Side]*.8f)*End.GetRotation());
            auto Parent=Bones.GetParentBoneIndex(Thigh);
            Output.Pose[Thigh]=Root.GetRelativeTransform(Component[Parent.GetInt()]);
            Output.Pose[Calf]=Joint.GetRelativeTransform(Root);Output.Pose[Foot]=End.GetRelativeTransform(Joint);
        }
        Output.Pose.NormalizeRotations();return true;
    }
};

FAnimInstanceProxy* UCivilianAnimInstance::CreateAnimInstanceProxy(){return new FCivilianAnimProxy(this);}

void UCivilianAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    auto* Player=Cast<ASliceCharacter>(TryGetPawnOwner());auto* Mesh=GetSkelMeshComponent();
    if(!Player || !Mesh || !Player->Appearance)return;
    auto* Creator=GetWorld()->GetGameInstance()->GetSubsystem<UCharacterCreatorSubsystem>();
    const auto* Body=Creator->Catalog->Body(Player->Appearance->Appearance.BodyPreset);if(!Body)return;
    GroundSpeed=Player->GetVelocity().Size2D();bGrounded=Player->GetCharacterMovement()->IsMovingOnGround();
    const bool Falling=Player->GetCharacterMovement()->IsFalling(),Crouched=Player->bIsCrouched;
    MovementState=Falling?TEXT("Jump"):(Crouched?TEXT("Crouch"):(GroundSpeed<8?TEXT("Idle"):(GroundSpeed>420?TEXT("Sprint"):(GroundSpeed>230?TEXT("Run"):TEXT("Walk")))));
    auto* Asset=Falling?Body->JumpLoop.Get():(Crouched?Body->Crouch.Get():Body->Locomotion.Get());
    if(!Asset)Asset=Body->CreatorIdle.Get();
    if(Asset!=GetAnimationAsset()){++TransitionSerial;SetAnimationAsset(Asset,true,1);}
    const float Ratio=FMath::Max(.5f,Player->Appearance->HeightRatio);
    if(Cast<UBlendSpace1D>(Asset))SetBlendSpacePosition(FVector(GroundSpeed/Ratio,0,0));
    else SetPlayRate(Crouched && GroundSpeed>8?FMath::Clamp(GroundSpeed/(140*Ratio),.5f,1.7f):1);
    FloorZ=Player->GetActorLocation().Z-Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    auto Trace=[&](FName Foot,FVector& Point,FVector& Normal){
        FVector Location=Mesh->GetSocketLocation(Foot);Location.Z=FloorZ;
        FHitResult Hit;FCollisionQueryParams Params(SCENE_QUERY_STAT(CivilianFoot),false,Player);
        const bool Found=bGrounded && GetWorld()->LineTraceSingleByChannel(Hit,Location+FVector(0,0,50),Location-FVector(0,0,70),ECC_Visibility,Params)
            && Hit.ImpactNormal.Z>=Player->GetCharacterMovement()->GetWalkableFloorZ();
        Point=Found?Hit.ImpactPoint:Location;Normal=Found?Hit.ImpactNormal:FVector::UpVector;return Found;
    };
    bLeftContact=Trace(TEXT("foot_l"),LeftGround,LeftNormal);bRightContact=Trace(TEXT("foot_r"),RightGround,RightNormal);
}
