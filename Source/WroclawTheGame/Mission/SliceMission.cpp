#include "Mission/SliceMission.h"
#include "Character/CharacterCreatorSubsystem.h"
#include "Systems/PhoneSystem.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "AI/SliceEnemy.h"
#include "World/ResidentNPC.h"
#include "Components/GameplayComponents.h"
#include "EngineUtils.h"
#include "Systems/GameplayEventBus.h"
#include "Core/WTGLog.h"
#include "Data/ChapterDefinition.h"
#include "Data/CampaignMigrationDefinition.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Save/SliceSave.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Character/SliceCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Core/SliceGameMode.h"
namespace
{
const TCHAR *SaveSlotName = TEXT("Przebudzenie_v3");
FString U(const std::string &S)
{
    return UTF8_TO_TCHAR(S.c_str());
}
bool ValidCampaignWorldPosition(const FVector &Position)
{
    constexpr double MaxHorizontal = 5000000.0; // 50 km from project origin.
    constexpr double MinVertical = -200000.0;
    constexpr double MaxVertical = 200000.0;
    return !Position.ContainsNaN() &&
           FMath::Abs(Position.X) <= MaxHorizontal &&
           FMath::Abs(Position.Y) <= MaxHorizontal &&
           Position.Z >= MinVertical && Position.Z <= MaxVertical;
}
const UCampaignMigrationDefinition *ActiveCampaignMigration(const UWorld *World)
{
    if (!World || !World->GetMapName().Contains(TEXT("Nadodrze_GIS")))
        return nullptr;
    return LoadObject<UCampaignMigrationDefinition>(
        nullptr, TEXT("/Game/Generated/CampaignMigrationDefinition.CampaignMigrationDefinition"));
}
FString CurrentCoordinateSpace(const UWorld *World)
{
    if (const auto *Migration = ActiveCampaignMigration(World))
        return Migration->TargetSpace;
    return TEXT("BlockoutV1");
}

} // namespace
void USliceMission::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);
    if (auto *Definition = LoadObject<UChapterDefinition>(
            nullptr, TEXT("/Game/Generated/ChapterDefinition.ChapterDefinition")))
        Definition->Apply();
}
void USliceMission::NewGame()
{
    NPCs.Empty();
    WorldState = Wroclaw::WorldState();
    WorldState.seed = static_cast<uint32>(FMath::RandRange(1, MAX_int32));
    bDebugSession = false;
    State = Wroclaw::Progress(FMath::RandRange(0, static_cast<int32>(Wroclaw::LightVariants().size()) - 1));
    if (const auto *Awake = Wroclaw::Progress::Find("awake"))
        Anchor = FVector(Awake->x, Awake->y, Awake->z);
    else
        Anchor = FVector(250, 400, 456);
    bInGame = true;
    bShowMenu = false;
    bDead = false;
    if (auto *Profile = Cast<USliceProfile>(UGameplayStatics::LoadGameFromSlot(TEXT("LocalAchievements"), 0)))
        LifetimeAchievements = Profile->Achievements & 63;
    SaveCheckpoint();
}
bool USliceMission::Threat() const
{
    auto *GM = GetWorld() ? Cast<ASliceGameMode>(GetWorld()->GetAuthGameMode()) : nullptr;
    return GM && GM->HasThreat();
}
bool USliceMission::HasSave() const
{
    if (!bSaveChecked)
    {
        bHasValidSave = const_cast<USliceMission *>(this)->LoadState(false);
        bSaveChecked = true;
    }
    return bHasValidSave;
}
bool USliceMission::LoadState(bool bApply)
{
    const bool Legacy = !UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0);
    auto *S =
        Cast<USliceSave>(
            UGameplayStatics::LoadGameFromSlot(Legacy ? TEXT("Przebudzenie_v2") : SaveSlotName, 0));
    if (!S || (S->Version != Wroclaw::Progress::Version && !(Legacy && S->Version == 2)) ||
        S->History.Num() > static_cast<int32>(Wroclaw::Catalog().size()) ||
        !ValidCampaignWorldPosition(S->Anchor))
        return false;

    FVector LoadedAnchor = S->Anchor;
    TMap<FString, FWTGNPCSnapshot> LoadedNPCs = S->NPCs;
    FString SaveSpace = S->CoordinateSpace.IsEmpty() ? TEXT("BlockoutV1") : S->CoordinateSpace;
    const FString ActiveSpace = CurrentCoordinateSpace(GetWorld());
    if (SaveSpace != ActiveSpace)
    {
        const auto *Migration = ActiveCampaignMigration(GetWorld());
        if (SaveSpace != TEXT("BlockoutV1") || ActiveSpace != TEXT("WroclawGISV1") || !Migration ||
            !Migration->TransformLegacyPosition(LoadedAnchor, LoadedAnchor))
            return false;
        for (auto &Pair : LoadedNPCs)
        {
            FTransform Migrated;
            if (!Migration->TransformLegacyTransform(Pair.Value.Transform, Migrated))
                return false;
            Pair.Value.Transform = Migrated;
        }
        if (!ValidCampaignWorldPosition(LoadedAnchor))
            return false;
    }

    Wroclaw::Progress Candidate(S->Variant);
    if (Legacy)
    {
        for (const auto &Id : S->History)
        {
            const auto *A = Wroclaw::Progress::Find(TCHAR_TO_UTF8(*Id));
            if (!A)
                return false;
            const bool Threat = S->GarageUnderThreat && std::find(A->tags.begin(), A->tags.end(),
                                                                  "Achievement.UnderThreat") != A->tags.end();
            if (Candidate.Apply(A->id, Threat) != Wroclaw::Result::Applied)
                return false;
        }
        if (S->MedkitsUsed < 0 || S->MedkitsUsed > 10 || S->DistractionsUsed < 0 || S->DistractionsUsed > 10)
            return false;
        for (int I = 0; I < S->MedkitsUsed; ++I)
            if (!Candidate.Use("medkit"))
                return false;
        for (int I = 0; I < S->DistractionsUsed; ++I)
            if (!Candidate.Use("distraction"))
                return false;
    }
    else
    {
        if (S->Timeline.Num() > static_cast<int32>(Wroclaw::Catalog().size()) + 1000 ||
            S->ObjectiveCounters.Num() > 1000)
            return false;
        for (const auto &Pair : S->ObjectiveCounters)
            Candidate.counters.emplace(TCHAR_TO_UTF8(*Pair.Key), Pair.Value);
        std::vector<std::string> Timeline;
        for (const auto &Event : S->Timeline)
            Timeline.emplace_back(TCHAR_TO_UTF8(*Event));
        if (!Wroclaw::Progress::ReplayTimeline(Timeline, Candidate, S->GarageUnderThreat))
            return false;
        if (S->History.Num() != static_cast<int32>(Candidate.history.size()) ||
            S->UsedItems.Num() != static_cast<int32>(Candidate.used.size()))
            return false;
        for (int I = 0; I < S->History.Num(); ++I)
            if (Candidate.history[I] != TCHAR_TO_UTF8(*S->History[I]))
                return false;
        for (const auto &Pair : S->UsedItems)
        {
            auto It = Candidate.used.find(TCHAR_TO_UTF8(*Pair.Key));
            if (It == Candidate.used.end() || It->second != Pair.Value)
                return false;
        }
    }
    Wroclaw::WorldState World;
    if (!Legacy && !Wroclaw::WorldState::Deserialize(TCHAR_TO_UTF8(*S->WorldPayload), World))
        return false;
    Candidate.kills = S->Kills;
    Candidate.courtyardDetected = S->CourtyardDetected;
    Candidate.elapsed = S->Elapsed;
    Candidate.questSince = S->QuestSince;
    for (const auto &Id : S->Neutralized)
        Candidate.neutralized.insert(TCHAR_TO_UTF8(*Id));
    if (S->Failures.Num() != S->LockUntil.Num())
        return false;
    for (const auto &Pair : S->Failures)
    {
        const auto *Until = S->LockUntil.Find(Pair.Key);
        if (!Until)
            return false;
        Candidate.locks[TCHAR_TO_UTF8(*Pair.Key)] = {Pair.Value, *Until};
    }
    if (!Candidate.Valid())
        return false;
    if (LoadedNPCs.Num() > 128 || S->Settings.Num() > 64)
        return false;
    for (const auto &Pair : LoadedNPCs)
    {
        const auto &Snap = Pair.Value;
        const FVector Pos = Snap.Transform.GetLocation();
        if (Snap.Transform.ContainsNaN() || !FMath::IsFinite(Snap.Health) || Snap.Health < 0 ||
            Snap.Health > 10000 || !ValidCampaignWorldPosition(Pos))
            return false;
        const std::string Id = TCHAR_TO_UTF8(*Pair.Key);
        if (std::none_of(Wroclaw::Guards().begin(), Wroclaw::Guards().end(),
                         [&](const auto &N) { return N.id == Id; }) &&
            std::none_of(Wroclaw::NPCs().begin(), Wroclaw::NPCs().end(),
                         [&](const auto &N) { return N.id == Id; }))
            return false;
    }
    if (bApply)
    {
        State = Candidate;
        WorldState = World;
        Anchor = LoadedAnchor;
        NPCs = MoveTemp(LoadedNPCs);
        Settings = S->Settings;
        GetGameInstance()->GetSubsystem<UCharacterCreatorSubsystem>()->Restore(S->CharacterCustomization);
    }
    return true;
}
bool USliceMission::ContinueGame()
{
    if (!LoadState(true))
    {
        Notify(TEXT("Brak poprawnego zapisu. Obsługiwany jest zapis v3 oraz migracja v2."));
        return false;
    }
    bDebugSession = false;
    bInGame = true;
    bShowMenu = false;
    bDead = false;
    bLastSaveSucceeded = true;
    if (auto *Profile = Cast<USliceProfile>(UGameplayStatics::LoadGameFromSlot(TEXT("LocalAchievements"), 0)))
        LifetimeAchievements = Profile->Achievements & 63;
    return true;
}
bool USliceMission::SaveCheckpoint()
{
    if (bDebugSession)
    {
        Notify(TEXT("Sesja debug: zapis wyłączony, aby zachować checkpoint gracza."));
        return false;
    }
    if (!State.Valid() || !WorldState.Valid())
    {
        bLastSaveSucceeded = false;
        Notify(TEXT("Zapis odrzucony: niespójny stan rozdziału."));
        return false;
    }
    auto *S = Cast<USliceSave>(UGameplayStatics::CreateSaveGameObject(USliceSave::StaticClass()));
    S->CoordinateSpace = CurrentCoordinateSpace(GetWorld());
    S->CharacterCustomization = GetGameInstance()->GetSubsystem<UCharacterCreatorSubsystem>()->Committed;
    if (!State.history.empty())
        for (TActorIterator<ASliceEnemy> It(GetWorld()); It; ++It)
        {
            FWTGNPCSnapshot Snap;
            Snap.Transform = It->GetActorTransform();
            Snap.Health = It->HealthState->Value;
            NPCs.Add(It->GuardId, Snap);
        }
    if (!State.history.empty())
        for (TActorIterator<AResidentNPC> It(GetWorld()); It; ++It)
        {
            FWTGNPCSnapshot Snap;
            Snap.Transform = It->GetActorTransform();
            NPCs.Add(It->DefinitionId.ToString(), Snap);
        }
    for (const auto &Event : State.timeline)
        S->Timeline.Add(U(Event));
    for (const auto &Pair : State.counters)
        S->ObjectiveCounters.Add(U(Pair.first), Pair.second);
    S->NPCs = NPCs;
    S->Settings = Settings;
    S->WorldPayload = UTF8_TO_TCHAR(WorldState.Serialize().c_str());
    for (const auto &Pair : State.used)
        S->UsedItems.Add(U(Pair.first), Pair.second);
    S->Variant = State.variant;
    S->Anchor = Anchor;
    S->Elapsed = State.elapsed;
    S->QuestSince = State.questSince;
    S->Kills = State.kills;
    S->MedkitsUsed = State.medkitsUsed;
    S->DistractionsUsed = State.distractionsUsed;
    S->CourtyardDetected = State.courtyardDetected;
    S->GarageUnderThreat = State.garageUnderThreat;
    for (const auto &Id : State.history)
        S->History.Add(U(Id));
    for (const auto &Id : State.neutralized)
        S->Neutralized.Add(U(Id));
    for (const auto &Pair : State.locks)
    {
        S->Failures.Add(U(Pair.first), Pair.second.failures);
        S->LockUntil.Add(U(Pair.first), Pair.second.until);
    }
    bLastSaveSucceeded = UGameplayStatics::SaveGameToSlot(S, SaveSlotName, 0);
    bSaveChecked = false;
    Notify(bLastSaveSucceeded ? TEXT("Zapisano checkpoint.")
                              : TEXT("Nie udało się zapisać checkpointu. Sprawdź miejsce na dysku."));
    return bLastSaveSucceeded;
}
void USliceMission::After(const FString &Id, Wroclaw::Result Result)
{
    if (Result == Wroclaw::Result::AlreadyDone)
        for (const auto &Location : Wroclaw::Locations())
            if (Location.safehouse && Id == UTF8_TO_TCHAR(Location.action.c_str()))
            {
                if (!WorldState.Rest(Location, Threat()))
                {
                    Notify(TEXT("Najpierw zgub zagrożenie."));
                    return;
                }
                if (auto *P = Cast<ASliceCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
                {
                    P->HealthState->Heal(P->HealthState->Maximum);
                    P->StaminaState->Value = P->StaminaState->Maximum;
                    Anchor = P->GetActorLocation();
                    Anchor.Z += 90.f - P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
                    SaveCheckpoint();
                }
                return;
            }
    if (Result == Wroclaw::Result::Applied)
    {
        const auto *A = Wroclaw::Progress::Find(TCHAR_TO_UTF8(*Id));
        WorldState.flags.insert(A->setTags.begin(), A->setTags.end());
        bool Rested = false;
        for (const auto &Location : Wroclaw::Locations())
            if (Location.safehouse && Location.action == A->id)
            {
                WorldState.Discover(Location.id, true);
                Rested = WorldState.Rest(Location, Threat());
                if (Rested)
                    if (auto *P = Cast<ASliceCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
                    {
                        P->HealthState->Heal(P->HealthState->Maximum);
                        P->StaminaState->Value = P->StaminaState->Maximum;
                    }
            }
        if (!Rested)
            WorldState.AddHeat(A->heat);
        GetWorld()->GetSubsystem<UGameplayEventBus>()->Emit(TEXT("Event.Action.Completed"), FName(*Id));
        UE_LOG(LogWTGQuest, Log, TEXT("Action completed: %s"), *Id);
        UpdateAchievements();
        if (A && A->checkpoint)
        {
            if (auto *P = Cast<ASliceCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
            {
                Anchor = P->GetActorLocation();
                // Checkpoints restore a standing capsule, even when activated while crouching.
                Anchor.Z += 90.f - P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
            }
            SaveCheckpoint();
        }
    }
    else
    {
        switch (Result)
        {
        case Wroclaw::Result::Wrong:
            Notify(TEXT("Błędne rozwiązanie. Po trzech pomyłkach panel blokuje się na 8 sekund."));
            break;
        case Wroclaw::Result::Cooldown:
            Notify(TEXT("Panel czasowo zablokowany. Poczekaj, aż zgaśnie ostrzeżenie."));
            break;
        case Wroclaw::Result::MissingClue:
            Notify(TEXT("Brakuje wskazówek do tego rozwiązania. Sprawdź śledztwo [J]."));
            break;
        case Wroclaw::Result::Unsafe:
            Notify(TEXT("Napastnik nadal cię szuka. Najpierw zgub pościg."));
            break;
        case Wroclaw::Result::Locked:
            Notify(TEXT("Brakuje przedmiotu lub wcześniejszego etapu. Sprawdź cel i podpowiedź [H]."));
            break;
        default:
            break;
        }
    }
}
Wroclaw::Result USliceMission::Act(const FString &Id)
{
    auto R = State.Apply(TCHAR_TO_UTF8(*Id), Threat());
    After(Id, R);
    return R;
}
Wroclaw::Result USliceMission::Submit(const FString &Id, const FString &Code)
{
    auto R = State.Submit(TCHAR_TO_UTF8(*Id), TCHAR_TO_UTF8(*Code), Threat());
    After(Id, R);
    return R;
}
void USliceMission::Notify(const FString &Message)
{
    Notification = Message;
    NotificationUntil = FPlatformTime::Seconds() + 7;
}
FString USliceMission::ObjectiveText() const
{
    const int Index = State.Current();
    if (State.Finished())
        return TEXT("Rozdział 1 ukończony. Odblokowano rozdział 2.");
    return FString::Printf(TEXT("%d / %d — %s"), Index + 1, static_cast<int32>(Wroclaw::Quests().size()),
                           *U(Wroclaw::Quests()[Index].title));
}
FString USliceMission::InventoryText() const
{
    FString Text;
    const auto& Appearance=GetGameInstance()->GetSubsystem<UCharacterCreatorSubsystem>()->Committed;
    Text+=TEXT("UBRANIA (garderoba)\n");
    for (FName ID:Appearance.OwnedClothing) Text+=ID.ToString()+TEXT("\n");
    Text+=TEXT("\nPRZEDMIOTY\n");
    for (const auto &Pair : State.inventory)
        if (Pair.second > 0)
        {
            const auto *Item = Wroclaw::Progress::Item(Pair.first);
            Text += FString::Printf(TEXT("%s × %d\n"), Item ? *U(Item->name) : *U(Pair.first), Pair.second);
        }
    return Text.IsEmpty() ? TEXT("Brak przedmiotów") : Text;
}
FString USliceMission::InvestigationText(int32 CategoryIndex) const
{
    const char *Categories[] = {"Person",    "Location", "Document", "Photo",
                                "Recording", "Message",  "Object",   "Vehicle"};
    FString Text = TEXT("1 Ludzie | 2 Miejsca | 3 Dokumenty | 4 Zdjęcia | 5 Nagrania | 6 Wiadomości | 7 "
                        "Obiekty | 8 Pojazdy\nC — połącz zebrane dowody o samochodzie\n\n");
    for (const auto &E : Wroclaw::EvidenceDefinitions())
        if (State.evidence.count(E.id) && E.category == Categories[FMath::Clamp(CategoryIndex, 0, 7)])
            for (const auto &A : Wroclaw::Catalog())
                if (A.evidence == E.id)
                    Text += U(A.label) + TEXT(":\n") + U(A.body) + TEXT("\n\n");
    for (const auto &Combination : Wroclaw::Combinations())
        if (WorldState.flags.count("Evidence." + Combination.id + ".Confirmed"))
            Text += TEXT("Połączony trop: ") + U(Combination.title) + TEXT("\n");
    return Text;
}
void USliceMission::OnPhonePage(int32 Page)
{
    if (auto *PC = UGameplayStatics::GetPlayerController(this, 0))
        if (auto *LP = PC->GetLocalPlayer())
            LP->GetSubsystem<UPhoneSystem>()->Open(Page);
}
FString USliceMission::WorldMapText() const
{
    FString Text = TEXT("ODKRYTE MIEJSCA — brak szybkiej podróży\n\n");
    for (const auto &L : Wroclaw::Locations())
        if (WorldState.discoveries.count(L.id))
            Text += U(L.name) + FString::Printf(TEXT(" — %.0f m X / %.0f m Y\n"), L.position[0] / 100,
                                                L.position[1] / 100);
    return Text;
}
FString USliceMission::PhoneText(int32 Page) const
{
    if (auto *PC = UGameplayStatics::GetPlayerController(this, 0))
        if (auto *LP = PC->GetLocalPlayer())
            return LP->GetSubsystem<UPhoneSystem>()->Render(Page);
    return TEXT("");
}
bool USliceMission::CombineEvidence(int32 Index)
{
    if (Index < 0 || Index >= static_cast<int32>(Wroclaw::Combinations().size()))
        return false;
    const auto &C = Wroclaw::Combinations()[Index];
    for (const auto &E : C.evidence)
        if (!State.evidence.count(E))
        {
            Notify(TEXT("Brakuje dowodów do połączenia."));
            return false;
        }
    if (Act(U(C.action)) == Wroclaw::Result::Applied)
    {
        WorldState.flags.insert("Evidence." + C.id + ".Confirmed");
        Notify(TEXT("Odkryto nowy trop."));
        return true;
    }
    return false;
}
bool USliceMission::CapturePhoto()
{
    auto *P = Cast<ASliceCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
    if (!P)
        return false;
    const auto *Best = static_cast<const Wroclaw::ActionDef *>(nullptr);
    float BestDot = .92f;
    for (const auto &A : Wroclaw::Catalog())
    {
        if (A.evidence.empty())
            continue;
        const FVector Location(A.x, A.y, A.z), Delta = Location - P->Camera->GetComponentLocation();
        const float Dot = FVector::DotProduct(Delta.GetSafeNormal(), P->Camera->GetForwardVector());
        if (Delta.Size() > 2000 || Dot < BestDot)
            continue;
        FHitResult Hit;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(Photo), false, P);
        GetWorld()->LineTraceSingleByChannel(Hit, P->Camera->GetComponentLocation(), Location, ECC_Visibility,
                                             Params);
        if (Hit.bBlockingHit && FVector::Dist(Hit.ImpactPoint, Location) > 60)
            continue;
        Best = &A;
        BestDot = Dot;
    }
    if (!Best)
    {
        Notify(TEXT("Brak czytelnego obiektu w kadrze."));
        return false;
    }
    constexpr int32 PhotoWidth = 1280;
    constexpr int32 PhotoHeight = 720;
    auto *Target = NewObject<UTextureRenderTarget2D>(P);
    if (!Target)
    {
        Notify(TEXT("Nie udało się utworzyć bufora zdjęcia."));
        return false;
    }
    Target->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
    Target->InitAutoFormat(PhotoWidth, PhotoHeight);
    Target->UpdateResourceImmediate(true);

    auto *Capture = NewObject<USceneCaptureComponent2D>(P);
    if (!Capture)
    {
        Notify(TEXT("Nie udało się uruchomić aparatu."));
        return false;
    }
    Capture->RegisterComponent();
    Capture->AttachToComponent(P->Camera, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    Capture->TextureTarget = Target;
    Capture->FOVAngle = P->Camera->FieldOfView;
    Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    Capture->bCaptureEveryFrame = false;
    Capture->bCaptureOnMovement = false;
    Capture->CaptureScene();

    FTextureRenderTargetResource *Resource = Target->GameThread_GetRenderTargetResource();
    TArray<FColor> Pixels;
    if (!Resource || !Resource->ReadPixels(Pixels) || Pixels.Num() != PhotoWidth * PhotoHeight)
    {
        Capture->DestroyComponent();
        Notify(TEXT("Nie udało się odczytać obrazu aparatu."));
        return false;
    }

    TArray<uint8> Png;
    FImageUtils::CompressImageArray(PhotoWidth, PhotoHeight, Pixels, Png);
    if (Png.IsEmpty())
    {
        Capture->DestroyComponent();
        Notify(TEXT("Nie udało się zakodować zdjęcia."));
        return false;
    }

    FString SafeId = U(Best->id);
    for (const TCHAR Invalid : FString(TEXT("/\\:*?\"<>|")))
        SafeId.ReplaceCharInline(Invalid, TEXT('_'));
    const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Photos"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    const FString Filename = FString::Printf(
        TEXT("%s_%lld.png"), *SafeId, FDateTime::UtcNow().ToUnixTimestamp());
    const FString FullPath = FPaths::Combine(Directory, Filename);
    if (!FFileHelper::SaveArrayToFile(Png, *FullPath))
    {
        Capture->DestroyComponent();
        Notify(TEXT("Nie udało się zapisać zdjęcia na dysku."));
        return false;
    }

    Capture->DestroyComponent();
    WorldState.photos.insert(Best->id);
    Notify(TEXT("Zapisano zdjęcie PNG w galerii."));
    return true;
}
FString USliceMission::HintText() const
{
    const int L = State.HintLevel();
    if (!L)
        return TEXT("Podpowiedź poziomu 1 pojawi się po 90 sekundach bez postępu.");

    const auto &Q =
        Wroclaw::Quests()[FMath::Min(State.Current(), static_cast<int>(Wroclaw::Quests().size()) - 1)];
    const Wroclaw::ActionDef *A = nullptr;
    for (const auto &Id : Q.all)
        if (!State.Done(Id))
        {
            A = Wroclaw::Progress::Find(Id);
            break;
        }
    if (!A && !Q.any.empty())
        A = Wroclaw::Progress::Find(Q.any.front());
    if (!A)
        return TEXT("Wróć do ostatniej wiadomości i sprawdź śledztwo.");
    // Follow unmet prerequisites so a late quest points at a reachable clue, never reveals its answer.
    for (int I = 0; I < 30; ++I)
    {
        const Wroclaw::ActionDef *Next = nullptr;
        for (const auto &Id : A->prerequisites)
            if (!State.Done(Id))
            {
                Next = Wroclaw::Progress::Find(Id);
                break;
            }
        if (!Next)
            for (const auto &Item : A->items)
                if (!State.Has(Item))
                {
                    for (const auto &Producer : Wroclaw::Catalog())
                        if (!State.Done(Producer.id) &&
                            std::find(Producer.reward.begin(), Producer.reward.end(), Item) !=
                                Producer.reward.end())
                        {
                            Next = &Producer;
                            break;
                        }
                    if (Next)
                        break;
                }
        if (!Next)
            break;
        A = Next;
    }
    FString Text = U(Wroclaw::PuzzleFramework::Hint(*A, State));
    if (Text.IsEmpty())
        Text = U(A->label);
    if (L >= 2)
        for (const auto &Id : A->clues)
        {
            const auto *Clue = Wroclaw::Progress::Find(Id);
            if (Clue)
                Text += TEXT("\nPorównaj: ") + U(Clue->label);
        }
    if (L >= 3)
        Text += TEXT("\nSprawdź kolejność liczb, dat i symboli w znalezionych materiałach. Podpowiedź nie "
                     "podaje kodu.");
    return Text;
}
void USliceMission::UpdateAchievements()
{
    if (bDebugSession)
        return;
    const int32 Combined = LifetimeAchievements | State.Achievements();
    if (Combined == LifetimeAchievements)
        return;
    auto *Profile = Cast<USliceProfile>(UGameplayStatics::CreateSaveGameObject(USliceProfile::StaticClass()));
    Profile->Achievements = Combined;
    if (UGameplayStatics::SaveGameToSlot(Profile, TEXT("LocalAchievements"), 0))
        LifetimeAchievements = Combined;
    else
        Notify(TEXT("Nie udało się zapisać lokalnego osiągnięcia."));
}
FString USliceMission::AchievementsText() const
{
    const TCHAR *Names[] = {TEXT("Pierwsze kroki"), TEXT("Escape Artist"), TEXT("Bez śladu"),
                            TEXT("Detektyw"),       TEXT("Pacyfista"),     TEXT("Szybkie myślenie")};
    FString Text;
    for (int I = 0; I < 6; ++I)
        Text += (LifetimeAchievements & (1 << I) ? TEXT("[zdobyte] ") : TEXT("[zamknięte] ")) +
                FString(Names[I]) + TEXT("\n");
    return Text;
}

FString USliceMission::QuestLogText() const
{
    FString Text = TEXT("GŁÓWNE ETAPY\n");
    int I = 0;
    for (const auto &Q : Wroclaw::Quests())
    {
        const bool Done = State.QuestComplete(Q);
        Text += FString::Printf(TEXT("%d. [%s] %s\n"), ++I, Done ? TEXT("ukończony") : TEXT("do wykonania"),
                                *U(Q.title));
    }
    Text += TEXT("\nPOBOCZNE — NIE BLOKUJĄ KAMPANII\n");
    Wroclaw::QuestFramework Framework;
    for (const auto &Q : Wroclaw::SideQuests())
        Text += (Framework.Complete(Q, State) ? TEXT("[ukończony] ") : TEXT("[do wykonania] ")) + U(Q.title) +
                TEXT("\n");
    return Text;
}
