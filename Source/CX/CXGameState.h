#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CXGameState.generated.h"

class ACXPlayerState;

UENUM(BlueprintType)
enum class ECXMatchPhase : uint8
{
    Waiting UMETA(DisplayName="대기", ToolTip="출발 카운트다운 중입니다. 운전과 공격을 잠급니다."),
    Playing UMETA(DisplayName="진행", ToolTip="원화를 모으고 상대 차량과 경쟁하는 상태입니다."),
    Finished UMETA(DisplayName="종료", ToolTip="시간이 종료되어 우승자가 결정된 상태입니다.")
};

/** 서버 GameMode의 판정 결과만 모두에게 복제한다. 개인 잔액은 여기에 저장하지 않는다. */
UCLASS()
class CX_API ACXGameState : public AGameStateBase
{
    GENERATED_BODY()
public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_Phase, Category="채굴|경기", meta=(ToolTip="서버가 결정한 경기 상태입니다. 모든 클라이언트가 같은 상태를 표시합니다."))
    ECXMatchPhase MatchPhase=ECXMatchPhase::Waiting;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="채굴|경기", meta=(ToolTip="서버의 1초 타이머가 갱신하는 남은 초입니다. 로컬 타이머로 별도 감소시키지 않습니다."))
    int32 RemainingSeconds=0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="채굴|경기", meta=(ToolTip="최고 원화 보유자 목록입니다. 동점이면 여러 명을 저장하여 공동 우승으로 처리합니다."))
    TArray<TObjectPtr<ACXPlayerState>> Winners;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="채굴|경기") int32 RoundNumber=1;
    bool IsPlaying() const {return MatchPhase==ECXMatchPhase::Playing;}
    void SetPhase(ECXMatchPhase Phase,int32 Seconds);
    UFUNCTION() void OnRep_Phase();
    /** 생성 직후 사라지는 아이템도 모든 화면에서 효과를 재생하도록 항상 존재하는 GameState를 경유한다. */
    UFUNCTION(NetMulticast,Reliable) void MulticastWorldFX(FVector Location,bool bExplosion,bool bSpeed,class UParticleSystem* Effect=nullptr,class USoundBase* Sound=nullptr);
};
