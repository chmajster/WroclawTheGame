#include "Character/CharacterCreator.h"
#include "Character/CharacterAppearanceComponent.h"
#include "Character/CharacterCreatorSubsystem.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "HAL/PlatformTime.h"

AWTG_CharacterCreator::AWTG_CharacterCreator()
{
    PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.bTickEvenWhenPaused=true;
    ModelRoot=CreateDefaultSubobject<USceneComponent>(TEXT("ModelRoot")); RootComponent=ModelRoot;
    Appearance=CreateDefaultSubobject<UCharacterAppearanceComponent>(TEXT("Appearance")); Appearance->bPreview=true;
    Capture=CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture")); Capture->SetupAttachment(ModelRoot);
    Capture->PrimitiveRenderMode=ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
    Capture->CaptureSource=ESceneCaptureSource::SCS_FinalColorLDR; Capture->FOVAngle=42;
    Capture->bCaptureEveryFrame=false; Capture->bCaptureOnMovement=false; Capture->bAlwaysPersistRenderingState=true;
    Capture->PrimaryComponentTick.bTickEvenWhenPaused=true;
    KeyLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("Key")); KeyLight->SetupAttachment(ModelRoot); KeyLight->SetRelativeLocation(FVector(150,-150,220)); KeyLight->SetAttenuationRadius(600);
    FillLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("Fill")); FillLight->SetupAttachment(ModelRoot); FillLight->SetRelativeLocation(FVector(80,130,150)); FillLight->SetAttenuationRadius(500);
    RimLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("Rim")); RimLight->SetupAttachment(ModelRoot); RimLight->SetRelativeLocation(FVector(-120,150,195)); RimLight->SetAttenuationRadius(650);
    Capture->PostProcessSettings.bOverride_AutoExposureMethod=true; Capture->PostProcessSettings.AutoExposureMethod=EAutoExposureMethod::AEM_Manual;
    Capture->PostProcessSettings.bOverride_AutoExposureBias=true; Capture->PostProcessSettings.AutoExposureBias=0;
    Capture->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;
    Capture->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure=true;
    Capture->PostProcessSettings.bOverride_CameraISO=true; Capture->PostProcessSettings.CameraISO=100;
    Capture->PostProcessSettings.bOverride_CameraShutterSpeed=true; Capture->PostProcessSettings.CameraShutterSpeed=60;
    Capture->PostProcessSettings.bOverride_DepthOfFieldFstop=true; Capture->PostProcessSettings.DepthOfFieldFstop=2.8f;
    Capture->ShowFlags.SetAtmosphere(false); Capture->ShowFlags.SetFog(false);
    Capture->ShowFlags.SetVolumetricFog(false); Capture->ShowFlags.SetSkyLighting(false); Capture->ShowFlags.SetMotionBlur(false);
    KeyLight->SetMobility(EComponentMobility::Movable);
    FillLight->SetMobility(EComponentMobility::Movable);
    RimLight->SetMobility(EComponentMobility::Movable);
    KeyLight->SetIntensityUnits(ELightUnits::Lumens);
    FillLight->SetIntensityUnits(ELightUnits::Lumens);
    RimLight->SetIntensityUnits(ELightUnits::Lumens);
    for (auto* Light:{KeyLight.Get(),FillLight.Get(),RimLight.Get()})
    { Light->LightingChannels.bChannel0=false; Light->LightingChannels.bChannel1=true; }
    StudioFloor=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StudioFloor")); StudioFloor->SetupAttachment(ModelRoot);
    StudioWall=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StudioWall")); StudioWall->SetupAttachment(ModelRoot);
}
void AWTG_CharacterCreator::BeginPlay()
{
    Super::BeginPlay();
    RenderTarget = NewObject<UTextureRenderTarget2D>(this);
    RenderTarget->ClearColor = FLinearColor(.025f, .03f, .04f, 1.0f);
    RenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
    RenderTarget->InitAutoFormat(720, 1000);
    RenderTarget->UpdateResourceImmediate(true);

    Capture->TextureTarget = RenderTarget;
    Capture->ShowOnlyActors.Reset();
    Capture->ShowOnlyActors.AddUnique(this);
    auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SurfaceQuality/Instances/MI_Concrete.MI_Concrete"));
    if (!Material) Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    for (auto* Mesh:{StudioFloor.Get(),StudioWall.Get()})
    { Mesh->SetStaticMesh(Cube); Mesh->SetMaterial(0,Material); Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); Mesh->SetLightingChannels(false,true,false); }
    StudioFloor->SetRelativeLocation(FVector(0,0,-3)); StudioFloor->SetRelativeScale3D(FVector(16,16,.05));
    StudioWall->SetRelativeLocation(FVector(-360,0,250)); StudioWall->SetRelativeScale3D(FVector(.05,16,5));
    GetGameInstance()->GetSubsystem<UCharacterCreatorSubsystem>()->OnChanged.AddDynamic(this,&AWTG_CharacterCreator::RefreshAppearance);
    SetLighting(TEXT("Modern")); RefreshAppearance();
}
void AWTG_CharacterCreator::RefreshAppearance()
{ Appearance->ApplyAppearance(GetGameInstance()->GetSubsystem<UCharacterCreatorSubsystem>()->Draft); }
void AWTG_CharacterCreator::ResizePreview(FIntPoint Size)
{
    Size.X=FMath::Clamp(Size.X,256,1280); Size.Y=FMath::Clamp(Size.Y,256,1440);
    if (RenderTarget && (RenderTarget->SizeX!=Size.X || RenderTarget->SizeY!=Size.Y)) RenderTarget->ResizeTarget(Size.X,Size.Y);
}
void AWTG_CharacterCreator::SetAngle(float Degrees) { TargetYaw=FMath::UnwindDegrees(Degrees); }
void AWTG_CharacterCreator::SetView(FName View)
{ TargetDistance=View==TEXT("Face")?65:(View==TEXT("UpperBody")?160:290); TargetHeight=View==TEXT("Face")?165:(View==TEXT("UpperBody")?133:92); }
void AWTG_CharacterCreator::SetLighting(FName P)
{
    const bool bModern = P == TEXT("Modern");
    const bool bNight = P == TEXT("Night");
    const bool bDaylight = P == TEXT("Daylight");

    KeyLight->SetIntensity(bNight ? 1800 : (bModern ? 8200 : 7000));
    FillLight->SetIntensity(bNight ? 600 : (bModern ? 2400 : 3000));
    RimLight->SetIntensity(bNight ? 1800 : (bModern ? 4300 : 2200));

    KeyLight->SetLightColor(
        bDaylight ? FLinearColor(1.0f,.93f,.80f) :
        (bNight ? FLinearColor(.50f,.65f,1.0f) :
        (bModern ? FLinearColor(1.0f,.88f,.76f) : FLinearColor::White)));
    FillLight->SetLightColor(bModern ? FLinearColor(.62f,.76f,1.0f) : FLinearColor(.80f,.88f,1.0f));
    RimLight->SetLightColor(bModern ? FLinearColor(.30f,.52f,1.0f) :
                            (bNight ? FLinearColor(.25f,.42f,1.0f) : FLinearColor(.72f,.82f,1.0f)));
}
void AWTG_CharacterCreator::Rotate(float Degrees) { TargetYaw=FMath::UnwindDegrees(TargetYaw+Degrees); }
void AWTG_CharacterCreator::ResetPresentation()
{
    TargetYaw = 0.0f;
    TargetDistance = 290.0f;
    TargetHeight = 92.0f;
    SetLighting(TEXT("Modern"));
}
void AWTG_CharacterCreator::Zoom(float Amount) { TargetDistance=FMath::Clamp(TargetDistance+Amount,45.f,380.f); }
void AWTG_CharacterCreator::Tick(float Dt)
{
    Super::Tick(Dt);
    const double Now=FPlatformTime::Seconds(); Dt=LastRealTime>0?FMath::Clamp(float(Now-LastRealTime),0.f,.05f):0.f; LastRealTime=Now;
    Distance=FMath::FInterpTo(Distance,TargetDistance,Dt,7); CameraHeight=FMath::FInterpTo(CameraHeight,TargetHeight,Dt,7);
    Yaw=FMath::UnwindDegrees(Yaw+FMath::FindDeltaAngleDegrees(Yaw,TargetYaw)*FMath::Min(Dt*10,1.f));
    TArray<UPrimitiveComponent*> Parts; GetComponents(Parts);
    for (auto* Part:Parts) Part->SetLightingChannels(false,true,false);
    const float Angle=FMath::DegreesToRadians(Yaw); FVector Position=FVector(FMath::Cos(Angle)*Distance,FMath::Sin(Angle)*Distance,CameraHeight*Appearance->HeightRatio);
    Capture->SetRelativeLocation(Position); Capture->SetRelativeRotation((FVector(0,0,CameraHeight*Appearance->HeightRatio)-Position).Rotation());
    Capture->CaptureScene();
}
