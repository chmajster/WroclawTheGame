#include "Character/CharacterCreator.h"
#include "Character/CharacterAppearanceComponent.h"
#include "Character/CharacterCreatorSubsystem.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

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
    Capture->PostProcessSettings.bOverride_AutoExposureMethod=true; Capture->PostProcessSettings.AutoExposureMethod=EAutoExposureMethod::AEM_Manual;
    Capture->PostProcessSettings.bOverride_AutoExposureBias=true; Capture->PostProcessSettings.AutoExposureBias=-1.5f;
    Capture->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;
    Capture->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure=false;
    KeyLight->SetMobility(EComponentMobility::Movable);
    FillLight->SetMobility(EComponentMobility::Movable);
    KeyLight->SetIntensityUnits(ELightUnits::Lumens);
    FillLight->SetIntensityUnits(ELightUnits::Lumens);
}
void AWTG_CharacterCreator::BeginPlay()
{
    Super::BeginPlay(); RenderTarget=NewObject<UTextureRenderTarget2D>(this); RenderTarget->InitAutoFormat(720,1000); RenderTarget->ClearColor=FLinearColor(.025f,.03f,.04f);
    Capture->TextureTarget=RenderTarget; Capture->ShowOnlyActors.Add(this); Capture->ShowOnlyActorComponents(this);
    GetGameInstance()->GetSubsystem<UCharacterCreatorSubsystem>()->OnChanged.AddDynamic(this,&AWTG_CharacterCreator::RefreshAppearance);
    SetLighting(TEXT("Neutral")); RefreshAppearance();
}
void AWTG_CharacterCreator::RefreshAppearance()
{ Appearance->ApplyAppearance(GetGameInstance()->GetSubsystem<UCharacterCreatorSubsystem>()->Draft); }
void AWTG_CharacterCreator::SetView(FName View)
{ TargetDistance=View==TEXT("Face")?65:(View==TEXT("UpperBody")?160:290); TargetHeight=View==TEXT("Face")?165:(View==TEXT("UpperBody")?133:92); }
void AWTG_CharacterCreator::SetLighting(FName P)
{
    KeyLight->SetIntensity(P==TEXT("Night")?1800:7000); FillLight->SetIntensity(P==TEXT("Night")?600:3000);
    KeyLight->SetLightColor(P==TEXT("Daylight")?FLinearColor(1,.93f,.8f):(P==TEXT("Night")?FLinearColor(.5f,.65f,1):FLinearColor::White));
    FillLight->SetLightColor(FLinearColor(.8f,.88f,1));
}
void AWTG_CharacterCreator::Rotate(float Degrees) { Yaw=FMath::UnwindDegrees(Yaw+Degrees); }
void AWTG_CharacterCreator::Zoom(float Amount) { TargetDistance=FMath::Clamp(TargetDistance+Amount,45.f,380.f); }
void AWTG_CharacterCreator::Tick(float Dt)
{
    Super::Tick(Dt); Distance=FMath::FInterpTo(Distance,TargetDistance,Dt,7); CameraHeight=FMath::FInterpTo(CameraHeight,TargetHeight,Dt,7);
    const float Angle=FMath::DegreesToRadians(Yaw); FVector Position=FVector(FMath::Cos(Angle)*Distance,FMath::Sin(Angle)*Distance,CameraHeight*Appearance->HeightRatio);
    Capture->SetRelativeLocation(Position); Capture->SetRelativeRotation((FVector(0,0,CameraHeight*Appearance->HeightRatio)-Position).Rotation());
    Capture->ShowOnlyActorComponents(this);
    Capture->CaptureScene();
}
