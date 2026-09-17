#include "Data/ChapterDefinition.h"
#include "Content/ChapterCatalog.h"
#include "Core/WTGLog.h"
#include <functional>
#include <set>
namespace
{
FGameplayTagContainer Tags(const std::vector<std::string> &Names)
{
    FGameplayTagContainer Out;
    for (const auto &N : Names)
    {
        const auto T = FGameplayTag::RequestGameplayTag(FName(UTF8_TO_TCHAR(N.c_str())), false);
        if (T.IsValid())
            Out.AddTag(T);
    }
    return Out;
}
TArray<FName> Names(const std::vector<std::string> &Values)
{
    TArray<FName> Out;
    for (const auto &V : Values)
        Out.Add(FName(UTF8_TO_TCHAR(V.c_str())));
    return Out;
}
std::vector<std::string> Values(const TArray<FName> &Names)
{
    std::vector<std::string> Out;
    for (const auto &N : Names)
        Out.emplace_back(TCHAR_TO_UTF8(*N.ToString()));
    return Out;
}
std::vector<std::string> Values(const FGameplayTagContainer &Tags)
{
    std::vector<std::string> Out;
    for (const auto &T : Tags)
        Out.emplace_back(TCHAR_TO_UTF8(*T.ToString()));
    return Out;
}
} // namespace
void UChapterDefinition::ImportGeneratedCatalog()
{
    Actions.Empty();
    SchemaVersion = 3;
    for (const auto &A : Wroclaw::Catalog())
    {
        FWTGActionRecord R;
        R.Id = FName(UTF8_TO_TCHAR(A.id.c_str()));
        R.Label = FText::FromString(UTF8_TO_TCHAR(A.label.c_str()));
        R.Body = FText::FromString(UTF8_TO_TCHAR(A.body.c_str()));
        R.Answer = UTF8_TO_TCHAR(A.answer.c_str());
        R.Prerequisites = Names(A.prerequisites);
        R.Items = Names(A.items);
        R.Rewards = Names(A.reward);
        R.Clues = Names(A.clues);
        R.Consumes = Names(A.consume);
        R.Tags = Tags(A.tags);
        R.RequiredTags = Tags(A.requireTags);
        R.GrantedTags = Tags(A.setTags);
        R.SafeOnly = A.safe;
        R.Checkpoint = A.checkpoint;
        R.Heat = A.heat;
        R.HintDelay = A.hintDelay;
        for (const auto &Hint : A.hints)
            R.Hints.Add(FText::FromString(UTF8_TO_TCHAR(Hint.c_str())));
        Actions.Add(R);
    }
}
bool UChapterDefinition::Apply() const
{
    if (SchemaVersion != 3 || Actions.Num() != static_cast<int32>(Wroclaw::Catalog().size()))
    {
        UE_LOG(LogWTGWorld, Error,
               TEXT("Chapter asset schema/count mismatch; generated definitions retained"));
        return false;
    }
    auto Candidate = Wroclaw::Catalog();
    std::set<std::string> Seen;
    for (const auto &R : Actions)
    {
        const std::string Id = TCHAR_TO_UTF8(*R.Id.ToString());
        auto It = std::find_if(Candidate.begin(), Candidate.end(), [&](const auto &A) { return A.id == Id; });
        if (It == Candidate.end() || !Seen.insert(Id).second || R.Hints.Num() != 3 ||
            !FMath::IsFinite(R.HintDelay) || R.HintDelay < 1 || R.HintDelay > 600 ||
            !FMath::IsFinite(R.Heat) || FMath::Abs(R.Heat) > 100 || R.Answer.Len() > 8)
            return false;
        It->label = TCHAR_TO_UTF8(*R.Label.ToString());
        It->body = TCHAR_TO_UTF8(*R.Body.ToString());
        It->answer = TCHAR_TO_UTF8(*R.Answer);
        It->prerequisites = Values(R.Prerequisites);
        It->items = Values(R.Items);
        It->reward = Values(R.Rewards);
        It->clues = Values(R.Clues);
        It->consume = Values(R.Consumes);
        It->tags = Values(R.Tags);
        It->requireTags = Values(R.RequiredTags);
        It->setTags = Values(R.GrantedTags);
        It->safe = R.SafeOnly;
        It->checkpoint = R.Checkpoint;
        It->heat = R.Heat;
        It->hintDelay = R.HintDelay;
        It->hints.clear();
        for (const auto &H : R.Hints)
            It->hints.emplace_back(TCHAR_TO_UTF8(*H.ToString()));
    }
    for (const auto &A : Candidate)
    {
        for (const auto *List : {&A.prerequisites, &A.clues, &A.anyOf})
            for (const auto &Ref : *List)
                if (!Seen.count(Ref))
                    return false;
        for (const auto *List : {&A.items, &A.reward, &A.consume})
            for (const auto &Ref : *List)
                if (std::none_of(Wroclaw::Items().begin(), Wroclaw::Items().end(),
                                 [&](const auto &I) { return I.id == Ref; }))
                    return false;
    }
    std::set<std::string> Active, Done;
    std::function<bool(const std::string &)> Visit = [&](const std::string &Id) {
        if (Active.count(Id))
            return false;
        if (Done.count(Id))
            return true;
        Active.insert(Id);
        auto It = std::find_if(Candidate.begin(), Candidate.end(), [&](const auto &A) { return A.id == Id; });
        if (It == Candidate.end())
            return false;
        for (const auto *List : {&It->prerequisites, &It->clues, &It->anyOf})
            for (const auto &Ref : *List)
                if (!Visit(Ref))
                    return false;
        Active.erase(Id);
        Done.insert(Id);
        return true;
    };
    for (const auto &A : Candidate)
        if (!Visit(A.id))
            return false;
    Wroclaw::MutableCatalog() = std::move(Candidate);
    return true;
}
