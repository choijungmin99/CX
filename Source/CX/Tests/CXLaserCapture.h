#pragma once
class AActor;
class ACXLaserShotEffect;
class UAudioComponent;

/** -CXNetTest에서만 재생/자동 정리를 기록하고, -CXLaserCapture에서 렌더 화면도 저장한다. */
void CXRecordLaserFXForTest(AActor* Context, ACXLaserShotEffect* Beam, UAudioComponent* Audio, bool bHit);
