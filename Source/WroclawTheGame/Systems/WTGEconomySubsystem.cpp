#include "Systems/WTGEconomySubsystem.h"

void UWTGEconomySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    PriceCents = {
        {TEXT("fuel_per_litre"), 650},
        {TEXT("basic_repair"), 18000},
        {TEXT("major_repair"), 90000},
        {TEXT("parking_hour"), 600},
        {TEXT("parking_ticket"), 12000},
        {TEXT("tow_release"), 35000},
        {TEXT("medkit"), 3500}
    };

    RewardCents = {
        {TEXT("minor_quest"), 15000},
        {TEXT("major_quest"), 60000},
        {TEXT("race"), 30000},
        {TEXT("investigation"), 45000}
    };

    ResetEconomy();
}

bool UWTGEconomySubsystem::ApplyTransaction(const FString& TransactionId, int64 AmountCents, const FString& Reason)
{
    if (TransactionId.IsEmpty())
    {
        return false;
    }

    if (AppliedTransactionIds.Contains(TransactionId))
    {
        return true;
    }

    if (FMath::Abs(AmountCents) > MaximumTransactionAbsCents)
    {
        return false;
    }

    const int64 NewBalance = BalanceCents + AmountCents;
    if (NewBalance < MinimumBalanceCents)
    {
        return false;
    }

    BalanceCents = NewBalance;
    AppliedTransactionIds.Add(TransactionId);

    FWTGEconomyTransaction Entry;
    Entry.TransactionId = TransactionId;
    Entry.AmountCents = AmountCents;
    Entry.BalanceAfterCents = BalanceCents;
    Entry.Reason = Reason;
    Ledger.Add(MoveTemp(Entry));

    return true;
}

bool UWTGEconomySubsystem::Purchase(const FString& TransactionId, FName PriceKey, int32 Quantity)
{
    const int64* UnitPrice = PriceCents.Find(PriceKey);
    if (!UnitPrice || Quantity <= 0)
    {
        return false;
    }

    return ApplyTransaction(TransactionId, -(*UnitPrice * static_cast<int64>(Quantity)), PriceKey.ToString());
}

bool UWTGEconomySubsystem::Reward(const FString& TransactionId, FName RewardKey, int32 Quantity)
{
    const int64* UnitReward = RewardCents.Find(RewardKey);
    if (!UnitReward || Quantity <= 0)
    {
        return false;
    }

    return ApplyTransaction(TransactionId, *UnitReward * static_cast<int64>(Quantity), RewardKey.ToString());
}

bool UWTGEconomySubsystem::HasAppliedTransaction(const FString& TransactionId) const
{
    return AppliedTransactionIds.Contains(TransactionId);
}

FWTGEconomySaveState UWTGEconomySubsystem::ExportSaveState() const
{
    FWTGEconomySaveState State;
    State.BalanceCents = BalanceCents;
    State.Ledger = Ledger;
    return State;
}

void UWTGEconomySubsystem::ImportSaveState(const FWTGEconomySaveState& State)
{
    BalanceCents = FMath::Max(State.BalanceCents, MinimumBalanceCents);
    Ledger = State.Ledger;
    AppliedTransactionIds.Reset();

    for (const FWTGEconomyTransaction& Entry : Ledger)
    {
        if (!Entry.TransactionId.IsEmpty())
        {
            AppliedTransactionIds.Add(Entry.TransactionId);
        }
    }
}

void UWTGEconomySubsystem::ResetEconomy()
{
    BalanceCents = StartingBalanceCents;
    Ledger.Reset();
    AppliedTransactionIds.Reset();
}
