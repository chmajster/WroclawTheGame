#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WTGEconomySubsystem.generated.h"

USTRUCT(BlueprintType)
struct WROCLAWTHEGAME_API FWTGEconomyTransaction
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FString TransactionId;
    UPROPERTY(BlueprintReadOnly) int64 AmountCents = 0;
    UPROPERTY(BlueprintReadOnly) int64 BalanceAfterCents = 0;
    UPROPERTY(BlueprintReadOnly) FString Reason;
};

USTRUCT(BlueprintType)
struct WROCLAWTHEGAME_API FWTGEconomySaveState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) int64 BalanceCents = 25000;
    UPROPERTY(BlueprintReadWrite) TArray<FWTGEconomyTransaction> Ledger;
};

UCLASS()
class WROCLAWTHEGAME_API UWTGEconomySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UPROPERTY(BlueprintReadOnly, Category="Economy")
    int64 BalanceCents = 25000;

    UPROPERTY(BlueprintReadOnly, Category="Economy")
    TArray<FWTGEconomyTransaction> Ledger;

    UFUNCTION(BlueprintCallable, Category="Economy")
    bool ApplyTransaction(const FString& TransactionId, int64 AmountCents, const FString& Reason);

    UFUNCTION(BlueprintCallable, Category="Economy")
    bool Purchase(const FString& TransactionId, FName PriceKey, int32 Quantity = 1);

    UFUNCTION(BlueprintCallable, Category="Economy")
    bool Reward(const FString& TransactionId, FName RewardKey, int32 Quantity = 1);

    UFUNCTION(BlueprintPure, Category="Economy")
    float GetBalancePLN() const { return static_cast<float>(BalanceCents) / 100.0f; }

    UFUNCTION(BlueprintPure, Category="Economy")
    bool HasAppliedTransaction(const FString& TransactionId) const;

    UFUNCTION(BlueprintCallable, Category="Economy")
    FWTGEconomySaveState ExportSaveState() const;

    UFUNCTION(BlueprintCallable, Category="Economy")
    void ImportSaveState(const FWTGEconomySaveState& State);

    UFUNCTION(BlueprintCallable, Category="Economy")
    void ResetEconomy();

private:
    TSet<FString> AppliedTransactionIds;
    TMap<FName, int64> PriceCents;
    TMap<FName, int64> RewardCents;

    static constexpr int64 StartingBalanceCents = 25000;
    static constexpr int64 MinimumBalanceCents = -200000;
    static constexpr int64 MaximumTransactionAbsCents = 10000000;
};
