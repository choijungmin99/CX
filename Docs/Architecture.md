# 설계 및 데이터 흐름

## 입력부터 결과까지

1. 플레이어의 Enhanced Input이 `CXPawn::Fire()` 또는 `PlaceMine()`을 호출합니다.
2. 로컬 상태를 먼저 확인한 뒤 **소유 차량**의 `ServerFire` / `ServerPlaceMine`을 요청합니다.
3. 서버는 RPC의 네트워크 소유권에 더해 현재 경기 상태와 쿨다운을 다시 확인합니다.
4. 서버가 직접 트레이스하거나 오버랩을 판정하고, `CXPlayerState`의 증감 함수를 호출합니다.
5. 원화와 추가 속도는 `DOREPLIFETIME_CONDITION(..., COND_OwnerOnly)`로 소유자에게만 전달합니다.
6. 각 화면의 UMG는 자신의 PlayerState와 공용 GameState를 읽어 갱신합니다.

클라이언트는 금액, 피해 대상, 공격 시작 좌표, 명중 Bone, 우승자 또는 남은 시간을 서버에 지정하지 않습니다. 라인트레이스의 시작·끝은 서버 차량의 위치와 전방 방향에서 계산합니다.

바퀴에는 실제 `Phys_Wheel_*` Bone에 부착된 쿼리 전용 피격 구체가 있습니다. Chaos 주행 물리를 위해 템플릿의 바퀴 바디 충돌이 꺼져 있는 경우에도 타이어를 조준할 수 있게 하는 장치입니다. 이 컴포넌트가 맞으면 실제 부착 Bone을 HitResult에 전달한 뒤 같은 `ApplyPointDamage` 경로로 처리합니다.

## 채팅

`WBP_ChatUI 입력창 OnTextCommitted(OnEnter)` → `CXPlayerController::SubmitChat` → `ServerSendChat` → 서버 입력 검증 및 발신자 확정 → 접속 컨트롤러별 `ClientReceiveChat` → `UChatUI::OnNewChatMessage`.

위젯은 네트워크 액터가 아니므로 자체에 Server RPC를 선언하지 않습니다. 신뢰할 수 있는 발신자는 서버가 소유한 PlayerState이며, 클라이언트가 임의 이름을 제출하는 API는 없습니다.

## 경기 상태와 잠금

`Waiting(10초)` → `Playing(120초)` → `Finished(0초)` → R 재경기 요청 → `Waiting`.

- 첫 플레이어가 접속하면 서버 타이머를 한 번 시작합니다.
- 매 1초 서버만 남은 초를 감소시킵니다. 클라이언트가 별도 로컬 초시계를 감소시키지 않습니다.
- GameState의 RepNotify가 차량 상태 잠금을 갱신하며 서버 Setter도 직접 같은 함수를 호출하여 리슨 호스트를 처리합니다.
- 경기 중에는 요청한 차량 복구만 허용하고, 경기 종료 후 재경기는 별도 GameMode 함수에서 허용합니다.
- 재경기는 개인 기록, 우승자 목록, 차량과 남은 지뢰를 초기화합니다.
- 접속자가 전부 나가면 타이머를 정리하고 다음 접속을 기다립니다.

## 원화와 속도

- `AddCoins` / `RemoveCoins`는 C++ 내부에서도 `HasAuthority()`를 확인합니다. `BlueprintAuthorityOnly` 메타데이터만 믿지 않습니다.
- 잔액 부족, 음수·0 증가량, 정수 오버플로, 비정상 부동소수점 입력을 거부합니다.
- 바퀴 페널티와 획득 보너스를 따로 보관하여 음수 회복이 의도하지 않은 영구 가속으로 바뀌지 않게 합니다.
- 실제 최고 속도는 `max(20, 기본 110 + 추가 속도)` km/h입니다. 서버와 소유자가 수평 물리 속도를 제한하고 다른 프록시는 복제된 물리를 표시합니다.

## 아이템 생명주기

`서버 스포너 SpawnActorDeferred` → `SpawnedItem 저장 + OnDestroyed 구독` → 생성 완료 → 서버 오버랩 → 중복 지급 가드 → 보상 → 공용 GameState 효과 Multicast → 아이템 파괴 → 스포너 델리게이트 → 2초 타이머 → 동일 ItemClass 재생성.

아이템의 NetUpdateFrequency는 1입니다. 애니메이션은 로컬 시각 효과이므로 매 프레임 위치를 네트워크로 전송하지 않습니다. 생성 직후 바로 획득된 액터는 아직 원격 채널이 없을 수 있어, 즉시 파괴되는 아이템 대신 이미 복제되어 있는 GameState가 파티클·사운드 RPC를 전달합니다.

## 우승과 정보 공개 범위

서버가 접속한 플레이어의 원화를 비교하고 최고 금액인 모든 PlayerState를 `Winners`에 넣습니다. 최고 금액이 0원인 동점도 공동 우승입니다. GameState는 우승자 참조만 공유하며 상대의 원화와 추가 속도 자체를 공개하지 않습니다.

## 재현한 네트워크 타이밍 문제

서로 다른 액터 채널의 RPC와 프로퍼티 복제는 전역 순서를 보장하지 않습니다. 자동 시험에서 컨트롤러의 단발 가속 명령이 GameState의 `Playing` 복제보다 먼저 도착했고, 대기 중 요청을 거부하는 정상 방어 경로가 실행됐습니다. 실제 키 입력은 매 프레임 처리되지만 검증용 단발 입력은 짧게 동기화를 기다린 후 같은 입력 콜백으로 전달하도록 수정했습니다. 디버거의 중단 위치와 실제 `CanDrive=false`, `bStateInputLock=true`, `RawBrakeInput=1` 값을 확인해 분리한 문제입니다.
