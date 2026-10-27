#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXNetworkTestDirector.generated.h"
class ACXPlayerController;
class ACXPawn;

/** -CXNetTest 옵션에서만 작동하는 실제 2프로세스 네트워크 기능 검증. 일반 경기에는 생성되지 않는다. */
UCLASS(NotBlueprintable)
class CX_API ACXNetworkTestDirector : public AActor
{
    GENERATED_BODY()
public:
    ACXNetworkTestDirector();
    virtual void Tick(float Delta) override;
    void Record(const FString& Name,bool bPassed);
private:
    TMap<FString,bool> Results;
    UPROPERTY() TArray<TObjectPtr<ACXPlayerController>> Players;
    int32 Stage=-1;
    double NextStep=0;
    int32 PreviousClock=0;
    int32 BeforeAttacks=0;
    int32 BeforeMines=0;
    FVector BeforeLocation=FVector::ZeroVector;
    UPROPERTY() TObjectPtr<class ACXItemSpawner> TestedSpawner;
    TWeakObjectPtr<class ACXPickup> OriginalItem;
    void RunStage();
    void Probe(const FString& Label,int32 Phase);
    void FinishReport();
};
