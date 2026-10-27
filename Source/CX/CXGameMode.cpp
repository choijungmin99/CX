// Copyright Epic Games, Inc. All Rights Reserved.

#include "CXGameMode.h"
#include "CXPlayerController.h"
#include "CXGameState.h"
#include "CXPlayerState.h"
#include "CXPawn.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Tests/CXNetworkTestDirector.h"
#include "CXMine.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

ACXGameMode::ACXGameMode()
{
	PlayerControllerClass = ACXPlayerController::StaticClass();
    PlayerStateClass=ACXPlayerState::StaticClass();
    GameStateClass=ACXGameState::StaticClass();
    static ConstructorHelpers::FClassFinder<APawn> Vehicle(TEXT("/Game/VehicleTemplate/Blueprints/SportsCar/BP_SportsCar_Pawn"));
    if(Vehicle.Class)DefaultPawnClass=Vehicle.Class;
}

void ACXGameMode::BeginPlay()
{
    Super::BeginPlay();
    if(auto* S=GetGameState<ACXGameState>())S->SetPhase(ECXMatchPhase::Waiting,WaitingDuration);
    if(FParse::Param(FCommandLine::Get(),TEXT("CXNetTest")))GetWorld()->SpawnActor<ACXNetworkTestDirector>();
}
void ACXGameMode::PreLogin(const FString& O,const FString& A,const FUniqueNetIdRepl& U,FString& Error)
{
    Super::PreLogin(O,A,U,Error);
    if(GetNumPlayers()>=2)Error=TEXT("이 경기는 2인 전용입니다. 빈 자리가 생긴 뒤 다시 접속하세요.");
}
void ACXGameMode::PostLogin(APlayerController* PC)
{
    Super::PostLogin(PC);
    if(auto* PS=PC->GetPlayerState<ACXPlayerState>())
    {PS->SetPlayerName(FString::Printf(TEXT("드라이버 %d"),GetNumPlayers()));PS->ResetRoundStats();}
    if(!bCountdownStarted)
    {
        bCountdownStarted=true;
        if(auto* S=GetGameState<ACXGameState>())S->SetPhase(ECXMatchPhase::Waiting,WaitingDuration);
        GetWorldTimerManager().SetTimer(RoundClock,this,&ACXGameMode::AdvanceClock,1.f,true);
    }
    UE_LOG(LogTemp,Display,TEXT("CX_LOGIN players=%d"),GetNumPlayers());
}
void ACXGameMode::Logout(AController* PC)
{
    Super::Logout(PC);
    if(GetNumPlayers()==0)
    {
        GetWorldTimerManager().ClearTimer(RoundClock);bCountdownStarted=false;
        if(auto* S=GetGameState<ACXGameState>()){S->Winners.Empty();S->SetPhase(ECXMatchPhase::Waiting,WaitingDuration);}
    }
}
void ACXGameMode::AdvanceClock()
{
    auto* S=GetGameState<ACXGameState>();if(!S||S->MatchPhase==ECXMatchPhase::Finished)return;
    S->RemainingSeconds=FMath::Max(0,S->RemainingSeconds-1);S->ForceNetUpdate();
    if(S->RemainingSeconds==0){if(S->MatchPhase==ECXMatchPhase::Waiting)StartPlaying();else FinishMatch();}
}
void ACXGameMode::StartPlaying()
{
    if(!HasAuthority())return;
    if(auto* S=GetGameState<ACXGameState>()){S->Winners.Empty();S->SetPhase(ECXMatchPhase::Playing,PlayingDuration);}
    UE_LOG(LogTemp,Display,TEXT("CX_PHASE Playing"));
}
void ACXGameMode::FinishMatch()
{
    if(!HasAuthority())return;
    auto* S=GetGameState<ACXGameState>();if(!S)return;
    S->Winners.Empty();int32 Best=-1;
    // 모든 잔액을 가진 서버에서만 판정한다. 상대의 개인 원화는 클라이언트에 없다.
    for(APlayerState* Base:S->PlayerArray)if(auto* PS=Cast<ACXPlayerState>(Base))
    {
        if(PS->IsOnlyASpectator())continue;
        if(PS->Coins>Best){Best=PS->Coins;S->Winners.Empty();S->Winners.Add(PS);}
        else if(PS->Coins==Best)S->Winners.Add(PS);
    }
    S->SetPhase(ECXMatchPhase::Finished,0);GetWorldTimerManager().ClearTimer(RoundClock);
    UE_LOG(LogTemp,Display,TEXT("CX_PHASE Finished winners=%d"),S->Winners.Num());
}
void ACXGameMode::RestartRound()
{
    if(!HasAuthority())return;
    auto* S=GetGameState<ACXGameState>();if(!S||S->MatchPhase!=ECXMatchPhase::Finished)return;
    ++S->RoundNumber;S->Winners.Empty();
    for(TActorIterator<ACXMine> It(GetWorld());It;++It)It->Destroy();
    for(TActorIterator<ACXPlayerState> It(GetWorld());It;++It)It->ResetRoundStats();
    for(FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)
    {
        auto* PC=It->Get();if(!PC)continue;
        if(APawn* Old=PC->GetPawn())Old->Destroy();RestartPlayer(PC);
    }
    S->SetPhase(ECXMatchPhase::Waiting,WaitingDuration);
    GetWorldTimerManager().SetTimer(RoundClock,this,&ACXGameMode::AdvanceClock,1.f,true);
}
AActor* ACXGameMode::ChoosePlayerStart_Implementation(AController* PC)
{
    TArray<APlayerStart*> Free;
    for(TActorIterator<APlayerStart> It(GetWorld());It;++It)if(IsPlayerStartAvailable(*It))Free.Add(*It);
    // 빈 스폰 지점 부족은 checkf 크래시 대신 엔진의 기본 스폰 복구에 맡긴다.
    return Free.IsEmpty()?Super::ChoosePlayerStart_Implementation(PC):Free[FMath::RandHelper(Free.Num())];
}
bool ACXGameMode::IsPlayerStartAvailable(const APlayerStart* S) const
{
    if(!IsValid(S)||!GetWorld())return false;
    FCollisionObjectQueryParams Objects;Objects.AddObjectTypesToQuery(ECC_Vehicle);
    return !GetWorld()->OverlapAnyTestByObjectType(S->GetActorTransform().TransformPosition(VehicleSpawnCheckOffset),S->GetActorQuat(),Objects,FCollisionShape::MakeBox(VehicleSpawnCheckHalfExtent),FCollisionQueryParams(SCENE_QUERY_STAT(CXSpawn),false));
}
