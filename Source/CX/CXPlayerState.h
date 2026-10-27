#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CXPlayerState.generated.h"

/** 개별 원화와 속도 보너스의 유일한 저장소. 실제 값은 서버만 변경하고 소유자만 전달받는다. */
UCLASS()
class CX_API ACXPlayerState : public APlayerState
{
    GENERATED_BODY()
public:
    ACXPlayerState();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    virtual void Tick(float DeltaSeconds) override;

    /** 원화는 상대방에게 복제하지 않는다. 순위 판정은 모든 값을 보유한 서버가 담당한다. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_PrivateStats, Category="채굴|개인 기록", meta=(ToolTip="현재 보유한 원화입니다. 서버에서만 변경되며 이 플레이어의 화면에만 복제됩니다."))
    int32 Coins = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_PrivateStats, Category="채굴|개인 기록", meta=(ToolTip="아이템 보너스와 바퀴 피격 페널티를 합한 추가 최고 속도(km/h)입니다. 소유자에게만 복제됩니다."))
    float BonusMaxSpeed = 0.f;

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="채굴|서버", meta=(ToolTip="서버에서만 원화를 증가시킵니다. 0 이하 입력과 정수 범위 초과는 거부합니다."))
    bool AddCoins(int32 Amount);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="채굴|서버", meta=(ToolTip="잔액이 충분한 경우에만 서버에서 원화를 차감합니다. 잔액 부족 시 아무 값도 바꾸지 않습니다."))
    bool RemoveCoins(int32 Amount);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="채굴|서버", meta=(ToolTip="아이템으로 얻는 영구 추가 최고 속도를 증가시킵니다. 경기 초기화 시 사라집니다."))
    bool AddBonusMaxSpeed(float Amount);
    void ApplyWheelPenalty(float Amount);
    void ResetRoundStats();
    float GetWheelPenalty() const { return WheelPenalty; }
private:
    /** 획득 보너스와 일시 페널티를 분리해야 회복이 새 보너스로 변하는 오류를 막을 수 있다. */
    float CollectedSpeedBonus = 0.f;
    float WheelPenalty = 0.f;
    void RefreshBonus();
    UFUNCTION() void OnRep_PrivateStats();
};
