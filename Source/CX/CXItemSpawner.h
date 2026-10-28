#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXItemSpawner.generated.h"
class ACXPickup;

/** 파괴 델리게이트를 구독하고 동일 종류 아이템을 2초 후 재생성하는 서버 전용 스포너. */
UCLASS()
class CX_API ACXItemSpawner : public AActor
{
    GENERATED_BODY()
public:
    ACXItemSpawner();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="채굴|스포너", meta=(ToolTip="이 위치에 생성할 아이템 클래스입니다. CXCoin 또는 CXSpeedBuff를 선택하세요.")) TSubclassOf<ACXPickup> ItemClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="채굴|스포너", meta=(ClampMin="0.1", ToolTip="아이템 파괴 후 같은 종류를 다시 만들기까지의 초입니다. 기본값은 2초입니다. 재생성 순간 영역 안의 차량은 완전히 나갔다 들어와야 다시 획득할 수 있습니다.")) float RespawnDelay=2.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="채굴|스포너", meta=(ToolTip="이 스포너가 생성한 현재 아이템입니다. 파괴되면 null로 바꾸고 타이머를 시작합니다.")) TObjectPtr<ACXPickup> SpawnedItem;
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="채굴|스포너") void SpawnItem();
    UFUNCTION() void OnItemDestroyed(AActor* DestroyedActor);
private:
    FTimerHandle RespawnTimer;
    bool bHasSpawnedItem=false;
};
