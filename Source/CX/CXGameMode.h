// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CXGameMode.generated.h"

class APlayerStart;

/** 서버에만 존재하는 경기 규칙. 시간과 승패 판정은 클라이언트에 위임하지 않는다. */
UCLASS()
class CX_API ACXGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACXGameMode();
    virtual void BeginPlay() override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
    virtual void PreLogin(const FString& Options,const FString& Address,const FUniqueNetIdRepl& UniqueId,FString& ErrorMessage) override;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="채굴|경기 규칙", meta=(ClampMin="3", ToolTip="첫 플레이어 접속 후 출발까지 대기하는 초입니다. 서버 타이머가 매초 감소시킵니다.")) int32 WaitingDuration=10;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="채굴|경기 규칙", meta=(ClampMin="10", ToolTip="실제 원화 채굴 경기 시간(초)입니다. 시간이 끝나면 서버가 모든 잔액을 비교합니다.")) int32 PlayingDuration=120;
    UPROPERTY(EditDefaultsOnly, Category="채굴|스폰", meta=(ToolTip="PlayerStart의 차량 겹침을 확인하는 검사 박스 절반 크기입니다.")) FVector VehicleSpawnCheckHalfExtent=FVector(250,120,80);
    UPROPERTY(EditDefaultsOnly, Category="채굴|스폰", meta=(ToolTip="스폰 검사 박스를 차량 차체 높이에 맞추는 오프셋입니다.")) FVector VehicleSpawnCheckOffset=FVector(0,0,80);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="채굴|경기") void StartPlaying();
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="채굴|경기") void FinishMatch();
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="채굴|경기") void RestartRound();
protected:
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
private:
    FTimerHandle RoundClock;
    bool bCountdownStarted=false;
    void AdvanceClock();
    bool IsPlayerStartAvailable(const APlayerStart* Start) const;
};
