#include "CXGameState.h"
#include "CXPawn.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"

void ACXGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACXGameState,MatchPhase);
    DOREPLIFETIME(ACXGameState,RemainingSeconds);
    DOREPLIFETIME(ACXGameState,Winners);
    DOREPLIFETIME(ACXGameState,RoundNumber);
}
void ACXGameState::SetPhase(ECXMatchPhase Phase,int32 Seconds)
{
    if(!HasAuthority())return;
    MatchPhase=Phase;RemainingSeconds=FMath::Max(Seconds,0);
    OnRep_Phase();ForceNetUpdate();
}
void ACXGameState::OnRep_Phase()
{
    // 리슨 서버는 RepNotify가 자동 실행되지 않으므로 SetPhase에서 직접 한 번 호출한다.
    for(TActorIterator<ACXPawn> It(GetWorld());It;++It) It->ApplyMatchInputLock();
}
void ACXGameState::MulticastWorldFX_Implementation(FVector P,bool Explosion,bool Speed,UParticleSystem* Effect,USoundBase* OverrideSound)
{
    if(GetNetMode()==NM_DedicatedServer)return;
    // RPC에 애셋이 생략되어도 코인/속도 아이템의 전용 효과를 선택한다.
    const TCHAR* DefaultParticlePath=Explosion
        ? TEXT("/Game/StarterContent/Particles/P_Explosion.P_Explosion")
        : Speed ? TEXT("/Game/Mining/Particles/P_SpeedPickup.P_SpeedPickup")
        : TEXT("/Game/Mining/Particles/P_CoinPickup.P_CoinPickup");
    auto* FX=Effect?Effect:LoadObject<UParticleSystem>(nullptr,DefaultParticlePath);
    const TCHAR* DefaultSoundPath = Explosion
        ? TEXT("/Game/StarterContent/Audio/Explosion01.Explosion01")
        : Speed ? TEXT("/Game/Mining/Audio/S_SpeedPickup.S_SpeedPickup")
        : TEXT("/Game/Mining/Audio/S_CoinPickup.S_CoinPickup");
    auto* Sound=OverrideSound?OverrideSound:LoadObject<USoundBase>(nullptr,DefaultSoundPath);
    if(FX)UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),FX,P);
    // 전용 획득음의 음색과 상승 곡선을 유지하도록 원래 피치(1.0)로 재생한다.
    const float Pitch = Explosion ? 1.7f : 1.f;
    if(Sound)UGameplayStatics::PlaySoundAtLocation(this,Sound,P,.5f,Pitch);
}
