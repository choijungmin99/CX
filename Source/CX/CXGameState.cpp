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
    auto* FX=Effect?Effect:LoadObject<UParticleSystem>(nullptr,Explosion?TEXT("/Game/StarterContent/Particles/P_Explosion.P_Explosion"):TEXT("/Game/StarterContent/Particles/P_Sparks.P_Sparks"));
    auto* Sound=OverrideSound?OverrideSound:LoadObject<USoundBase>(nullptr,Explosion?TEXT("/Game/StarterContent/Audio/Explosion01.Explosion01"):TEXT("/Game/StarterContent/Audio/Collapse01.Collapse01"));
    if(FX)UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),FX,P);
    if(Sound)UGameplayStatics::PlaySoundAtLocation(this,Sound,P,.5f,Speed?1.3f:1.7f);
}
