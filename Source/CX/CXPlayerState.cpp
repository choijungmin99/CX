#include "CXPlayerState.h"
#include "Net/UnrealNetwork.h"

ACXPlayerState::ACXPlayerState()
{
    PrimaryActorTick.bCanEverTick=true;
    SetNetUpdateFrequency(10.f);
}
void ACXPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    // 채점 핵심: 다른 플레이어는 이 두 프로퍼티의 실제 값을 전달받지 않는다.
    DOREPLIFETIME_CONDITION(ACXPlayerState, Coins, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(ACXPlayerState, BonusMaxSpeed, COND_OwnerOnly);
}
bool ACXPlayerState::AddCoins(int32 Amount)
{
    if(!HasAuthority() || Amount<=0 || Coins>MAX_int32-Amount) return false;
    Coins+=Amount;ForceNetUpdate();return true;
}
bool ACXPlayerState::RemoveCoins(int32 Amount)
{
    if(!HasAuthority() || Amount<=0 || Coins<Amount) return false;
    Coins-=Amount;ForceNetUpdate();return true;
}
bool ACXPlayerState::AddBonusMaxSpeed(float Amount)
{
    if(!HasAuthority() || !FMath::IsFinite(Amount) || Amount<=0) return false;
    CollectedSpeedBonus=FMath::Clamp(CollectedSpeedBonus+Amount,0.f,200.f);
    RefreshBonus();return true;
}
void ACXPlayerState::ApplyWheelPenalty(float Amount)
{
    if(!HasAuthority() || !FMath::IsFinite(Amount) || Amount>=0) return;
    WheelPenalty=FMath::Clamp(WheelPenalty+Amount,-60.f,0.f);RefreshBonus();
}
void ACXPlayerState::RefreshBonus()
{
    BonusMaxSpeed=CollectedSpeedBonus+WheelPenalty;ForceNetUpdate();
}
void ACXPlayerState::ResetRoundStats()
{
    if(!HasAuthority())return;
    Coins=0;CollectedSpeedBonus=0;WheelPenalty=0;RefreshBonus();
}
void ACXPlayerState::Tick(float Dt)
{
    Super::Tick(Dt);
    if(HasAuthority() && WheelPenalty<0)
    {
        // 페널티 자체는 절대로 양수가 되지 않는다. 기존 아이템 보너스만 보존한다.
        WheelPenalty=FMath::Min(0.f,WheelPenalty+10.f*Dt);RefreshBonus();
    }
}
void ACXPlayerState::OnRep_PrivateStats() { /* HUD는 로컬 PlayerState만 읽으므로 원격 개인 기록이 노출되지 않는다. */ }
