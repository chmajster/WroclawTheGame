#pragma once
#include "DynamicRHI.h"
#include "RHI.h"
#include "RHIStats.h"
#include "RenderTimer.h"
#include "HAL/PlatformMemory.h"
#include "Misc/CoreDelegates.h"
#include "Scalability.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "LegacyScreenPercentageDriver.h"

// Development-only real movement, public engine timing and on-screen warnings.
// No save writes, teleports, shadow budget changes or hidden diagnostics.
static FAutoConsoleCommandWithWorld GRealismPerformance(TEXT("WTG.RealismPerformance"),
 TEXT("Measure six 4m square laps on the start map, warm assets, capture; exit."),
 FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World){
 if(!World || !World->IsGameWorld())return;
 const FString Folder=FPaths::ProjectSavedDir()/TEXT("Realism/Performance");
 IFileManager::Get().MakeDirectory(*Folder,true);
 IFileManager::Get().Delete(*(Folder/TEXT("report.json")));
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
 [Weak=TWeakObjectPtr<UWorld>(World),Folder,Elapsed=0.f,Started=-1.f,Last=FPlatformTime::Seconds(),
  Initial=FVector::ZeroVector,Previous=FVector::ZeroVector,Forward=FVector::ZeroVector,
  Right=FVector::ZeroVector,TargetIndex=0,Distance=0.,FloorOK=true,CameraOK=true,
  Raw=TArray<TSharedPtr<FJsonValue>>(),WarnFrames=0,Warnings=TSet<FString>(),Shots=0](float Dt) mutable {
 const double Now=FPlatformTime::Seconds(); const double FrameMs=(Now-Last)*1000.;Last=Now;
 auto* W=Weak.Get();if(!W)return false;Elapsed+=Dt;WTGRealismCaptureTime(W);
 auto* P=Cast<ASliceCharacter>(UGameplayStatics::GetPlayerPawn(W,0));
 auto* PC=UGameplayStatics::GetPlayerController(W,0);
 AWTGDistrictRegistry* R=nullptr;for(TActorIterator<AWTGDistrictRegistry> It(W);It;++It){R=*It;break;}
 FCoreDelegates::FSeverityMessageMap Messages;FCoreDelegates::OnGetOnScreenMessages.Broadcast(Messages);
 bool HasVSM=false;for(const auto& Entry:Messages){const FString S=Entry.Value.ToString();
   if(S.Contains(TEXT("[VSM]"))){HasVSM=true;Warnings.Add(S.Left(S.Find(TEXT(". See"))>=0?S.Find(TEXT(". See")):S.Len()));}}
 if(HasVSM)++WarnFrames;
 if(Started<0 && Elapsed>60 && P && PC && R && P->Appearance && !P->Appearance->bLoading
   && !P->Appearance->bUsingPlaceholder && WTGRealismCaptureReady() && P->GetCharacterMovement()->IsMovingOnGround()){
   Initial=Previous=P->GetActorLocation();Forward=(R->MeetingPoint-R->PlayerStart).GetSafeNormal2D();
   Right=FVector(-Forward.Y,Forward.X,0);P->GetCharacterMovement()->MaxWalkSpeed=180;
   PC->SetViewTarget(P);Started=Elapsed;
   UE_LOG(LogTemp,Display,TEXT("WTG_PERF_START origin=%s"),*Initial.ToString());
 }
 const float Since=Started>=0?Elapsed-Started:0;
 if(Started>=0 && P && PC){
   Distance+=FVector::Dist2D(Previous,P->GetActorLocation());Previous=P->GetActorLocation();
   const FVector Offsets[]={Forward*400,Forward*400+Right*400,Right*400,FVector::ZeroVector};
   if(TargetIndex<24){FVector Delta=Initial+Offsets[TargetIndex%4]-P->GetActorLocation();Delta.Z=0;
     if(Delta.Size()<25)++TargetIndex;
     else {PC->SetControlRotation(Delta.Rotation());P->AddMovementInput(Delta.GetSafeNormal(),1,true);}}
   FloorOK&=P->GetCharacterMovement()->IsMovingOnGround();
   FCollisionQueryParams Params(SCENE_QUERY_STAT(RealismPerformanceCamera),false,P);
   CameraOK&=P->Camera && P->Boom && !W->OverlapBlockingTestByChannel(P->Camera->GetComponentLocation(),
     FQuat::Identity,ECC_Camera,FCollisionShape::MakeSphere(8),Params);
   TArray<TSharedPtr<FJsonValue>> Values;
   const double Timings[]={double(Since),FrameMs,FPlatformTime::ToMilliseconds(GGameThreadTime),
      FPlatformTime::ToMilliseconds(GRenderThreadTime),FPlatformTime::ToMilliseconds(GRHIThreadTime),
      FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles())};
   for(double V:Timings)Values.Add(MakeShared<FJsonValueNumber>(V));
   Raw.Add(MakeShared<FJsonValueArray>(Values));
   if(Since>10+Shots*15 && Shots<4){++Shots;FScreenshotRequest::RequestScreenshot(
      Folder/FString::Printf(TEXT("route_%d.png"),Shots),true,false);}
 }
 if((Started>=0 && Since>=65) || Elapsed>1200){
   const bool OK=Started>=0 && TargetIndex==24 && Distance>8500 && Raw.Num()>300 && FloorOK && CameraOK;
   auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("status"),OK?TEXT("PASS"):TEXT("FAIL"));
   Report->SetStringField(TEXT("captured_utc"),FDateTime::UtcNow().ToIso8601());
   Report->SetStringField(TEXT("map"),W->GetOutermost()->GetName());
   Report->SetStringField(TEXT("cpu"),FPlatformMisc::GetCPUBrand());Report->SetStringField(TEXT("gpu"),GRHIAdapterName);
   Report->SetNumberField(TEXT("logical_cores"),FPlatformMisc::NumberOfCoresIncludingHyperthreads());
   const auto Memory=FPlatformMemory::GetStats();FTextureMemoryStats Textures;RHIGetTextureMemoryStats(Textures);
   Report->SetNumberField(TEXT("system_ram_bytes"),Memory.TotalPhysical);
   Report->SetNumberField(TEXT("process_ram_bytes"),Memory.UsedPhysical);
   Report->SetNumberField(TEXT("process_peak_ram_bytes"),Memory.PeakUsedPhysical);
   Report->SetNumberField(TEXT("dedicated_vram_bytes"),Textures.DedicatedVideoMemory);
   Report->SetNumberField(TEXT("streaming_texture_bytes"),Textures.StreamingMemorySize);
   Report->SetNumberField(TEXT("nonstreaming_texture_bytes"),Textures.NonStreamingMemorySize);
   Report->SetNumberField(TEXT("texture_pool_bytes"),Textures.TexturePoolSize);
   Report->SetStringField(TEXT("memory_scope"),TEXT("Process working set; RHI texture allocation, not total GPU device memory."));
   Report->SetNumberField(TEXT("distance_cm"),Distance);Report->SetNumberField(TEXT("completed_segments"),TargetIndex);
   Report->SetNumberField(TEXT("warmup_elapsed_s"),Started);
   Report->SetBoolField(TEXT("floor_valid"),FloorOK);Report->SetBoolField(TEXT("camera_clear"),CameraOK);
   Report->SetStringField(TEXT("route"),TEXT("Six physical 4m square laps, forward toward meeting, right perpendicular; 180cm/s; 60s asset warmup then 65s sample."));
   TArray<TSharedPtr<FJsonValue>> Coordinates;
   for(const FVector& V:{Initial,Forward,Right}){TArray<TSharedPtr<FJsonValue>> A;
     for(double C:{V.X,V.Y,V.Z})A.Add(MakeShared<FJsonValueNumber>(C));Coordinates.Add(MakeShared<FJsonValueArray>(A));}
   Report->SetArrayField(TEXT("route_origin_forward_right"),Coordinates);
   Report->SetNumberField(TEXT("vsm_warning_frames_including_warmup"),WarnFrames);
   TArray<TSharedPtr<FJsonValue>> Strings;for(const FString& S:Warnings)Strings.Add(MakeShared<FJsonValueString>(S));
   Report->SetArrayField(TEXT("vsm_on_screen_messages"),Strings);
   const auto Q=Scalability::GetQualityLevels();
   for(const auto& Pair:TMap<FString,int32>{{TEXT("view_distance"),Q.ViewDistanceQuality},{TEXT("shadow"),Q.ShadowQuality},
     {TEXT("gi"),Q.GlobalIlluminationQuality},{TEXT("reflection"),Q.ReflectionQuality},{TEXT("texture"),Q.TextureQuality},
     {TEXT("effects"),Q.EffectsQuality},{TEXT("foliage"),Q.FoliageQuality},{TEXT("shading"),Q.ShadingQuality}})
     Report->SetNumberField(Pair.Key,Pair.Value);
   for(const TCHAR* Name:{TEXT("r.ScreenPercentage"),TEXT("sg.ResolutionQuality"),TEXT("r.DynamicRes.OperationMode"),
     TEXT("r.VSync"),TEXT("t.MaxFPS"),TEXT("r.Shadow.Virtual.Enable"),TEXT("r.Shadow.Virtual.AllowScreenOverflowMessages"),
     TEXT("r.Shadow.Virtual.MaxPhysicalPages"),TEXT("r.Nanite"),TEXT("r.TemporalAA.Upsampling"),
     TEXT("r.ScreenPercentage.Default"),TEXT("r.ScreenPercentage.Default.Desktop.Mode"),
     TEXT("r.ScreenPercentage.MinResolution"),TEXT("r.ScreenPercentage.MaxResolution"),
     TEXT("r.SecondaryScreenPercentage.GameViewport")})
     if(auto* V=IConsoleManager::Get().FindConsoleVariable(Name))Report->SetNumberField(Name,V->GetFloat());
   const auto Size=W->GetGameViewport()->Viewport->GetSizeXY();Report->SetNumberField(TEXT("width"),Size.X);Report->SetNumberField(TEXT("height"),Size.Y);
   FStaticResolutionFractionHeuristic Heuristic;
   Heuristic.Settings.PullRunTimeRenderingSettings(EViewStatusForScreenPercentage::Desktop);
   Heuristic.TotalDisplayedPixelCount=Size.X*Size.Y;
   Heuristic.DPIScale=W->GetGameViewport()->GetDPIScale();
   // Match GameViewportClient's static driver with dynamic resolution off.
   const auto* Secondary=IConsoleManager::Get().FindConsoleVariable(TEXT("r.SecondaryScreenPercentage.GameViewport"));
   Heuristic.SecondaryViewFraction=Secondary && Secondary->GetFloat()>0 ? Secondary->GetFloat()/100.f : FMath::Min(1.f,1.f/Heuristic.DPIScale);
   Report->SetNumberField(TEXT("resolved_static_primary_fraction"),Heuristic.ResolveResolutionFraction());
   Report->SetNumberField(TEXT("secondary_fraction"),Heuristic.SecondaryViewFraction);
   TArray<TSharedPtr<FJsonValue>> Inventory;
   for(TActorIterator<AActor> It(W);It;++It){TArray<UStaticMeshComponent*> Components;It->GetComponents(Components);
     for(auto* C:Components){UStaticMesh* M=C->GetStaticMesh();if(!M || !C->IsRegistered())continue;
       auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("actor"),It->GetActorNameOrLabel());
       Item->SetStringField(TEXT("mesh"),M->GetPathName());Item->SetBoolField(TEXT("nanite_enabled"),M->IsNaniteEnabled());
       Item->SetBoolField(TEXT("valid_nanite_data"),M->HasValidNaniteData());Item->SetBoolField(TEXT("cast_shadow"),C->CastShadow);
       Item->SetNumberField(TEXT("max_bounds_extent_cm"),C->Bounds.BoxExtent.GetMax());
       auto* I=Cast<UInstancedStaticMeshComponent>(C);Item->SetNumberField(TEXT("instances"),I?I->GetInstanceCount():1);
       if(I){Item->SetNumberField(TEXT("cull_start_cm"),I->InstanceStartCullDistance);Item->SetNumberField(TEXT("cull_end_cm"),I->InstanceEndCullDistance);}
       Inventory.Add(MakeShared<FJsonValueObject>(Item));}}
   Report->SetArrayField(TEXT("static_mesh_components"),Inventory);
   Report->SetStringField(TEXT("sample_columns"),TEXT("elapsed_s,wall_frame_ms,game_thread_active_ms,render_thread_active_ms,rhi_thread_active_ms,gpu_frame_ms; zero GPU samples unavailable; captures included; cap retained; other apps may share GPU."));
   Report->SetArrayField(TEXT("samples"),Raw);
   FString Json;auto Writer=TJsonWriterFactory<>::Create(&Json);FJsonSerializer::Serialize(Report,Writer);
   FFileHelper::SaveStringToFile(Json,*(Folder/TEXT("report.json")));
   UE_LOG(LogTemp,Display,TEXT("WTG_PERF_DONE %s segments=%d distance=%.2f warnings=%d"),OK?TEXT("PASS"):TEXT("FAIL"),TargetIndex,Distance,WarnFrames);
   FPlatformMisc::RequestExit(false);return false;
 }
 return true;
 }));
 }));
