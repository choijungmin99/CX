# 과제 요구사항 체크리스트

출처: 사용자가 전달한 **[CH4 2번] 분노의 질주 : 원화 채굴** 본문. 페이지 최상단의 강의 진도 안내에 채팅/판정 예시가 섞여 있으나 실제 채점 대상은 아래 필수/도전 체크리스트로 정리했습니다. 구현 위치와 자동 검증 결과를 함께 확인할 수 있습니다.

## 사전 준비

- [x] UE 5.5.4 정식 Vehicle C++ 템플릿을 사용한 별도 `CX` 프로젝트
- [x] Desktop / Scalable 설정 및 Starter Content 포함
- [x] 리슨 서버와 별도 게스트 프로세스 실행 스크립트
- [x] 맵에 PlayerStart 3개 배치 및 차량 겹침 검사
- [x] PlayerController의 VehiclePawn 프로퍼티 복제
- [x] 로컬 컨트롤러만 UI 생성
- [x] 백그라운드 에디터 CPU 제한을 프로젝트 기본 설정에서 해제

## 필수 1. 채팅 시스템

| 완료 | 요구사항 | 구현 위치 |
|---|---|---|
| [x] | 채팅창 위젯 C++ 클래스 생성 | `UI/ChatUI.h`, `ChatUI.cpp` |
| [x] | 위젯 블루프린트 애셋 생성 | `Content/UI/WBP_ChatUI` |
| [x] | 위젯 객체 생성 및 연결 | `CXPlayerController::BeginPlay`, `CXGameWidget::NativeOnInitialized` |
| [x] | 입력창 Enter 이벤트 | `OnChatInputTextCommitted`, `ETextCommit::OnEnter` |
| [x] | 작성한 메시지를 화면에 표시 | `OnNewChatMessage`, ScrollBox에 TextBlock 추가 |
| [x] | 모든 플레이어의 채팅 동기화 | `ServerSendChat` → 각 `ClientReceiveChat` |

추가 방어: 발신자 위조 방지, 공백/초과 길이/제어 문자 검사, 전송 간격 제한, 채팅 기록 80개 제한, 이벤트 바인딩 해제.

## 필수 2. PlayerState

| 완료 | 요구사항 | 구현 위치 |
|---|---|---|
| [x] | Coins 속성과 프로퍼티 복제 | `CXPlayerState::Coins` |
| [x] | 원화 증가·차감 함수 | `AddCoins`, `RemoveCoins` |
| [x] | 원화 값은 서버에서만 변경 | 두 함수의 `HasAuthority()` 방어 |
| [x] | Coins는 COND_OwnerOnly 복제 | `GetLifetimeReplicatedProps` |
| [x] | BonusMaxSpeed 속성과 복제 | `BonusMaxSpeed` |
| [x] | 추가 속도 증가 함수 | `AddBonusMaxSpeed` |
| [x] | 추가 속도도 서버만 변경 | 증가/페널티/회복 함수의 권한 검사 |
| [x] | BonusMaxSpeed도 COND_OwnerOnly | `DOREPLIFETIME_CONDITION` |
| [x] | 자신의 원화·추가 속도 UI | `CXGameWidget::NativeTick`에서 자신의 PS만 참조 |

## 필수 3. Vehicle·아이템·스포너

| 완료 | 요구사항 | 구현 위치 |
|---|---|---|
| [x] | 좌클릭 InputAction 애셋 및 차량 연결 | `IA_Fire`, `IMC_Combat`, `SetupPlayerInputComponent` |
| [x] | 좌클릭에서 차량 Server RPC 호출 | `Fire` → `ServerFire` |
| [x] | 앞 범퍼 중앙에서 서버 라인트레이스 | 서버 차량 좌표 + 전방 250cm + 높이 55cm |
| [x] | 상대 감지 후 피해 함수 호출 | 도전 기능을 포함한 `ApplyPointDamage` 사용 |
| [x] | 피해자 5원 이상이면 5원 이동 | `CXPawn::TakeDamage` |
| [x] | 피해자 5원 미만이면 금액 유지 | 잔액 조건 검사 및 부족 잔액 테스트 |
| [x] | DebugLine과 명중 파티클 | `MulticastAttackFX` |
| [x] | 코인 액터 및 오버랩 +10원 | `CXCoin`, 서버 `OnOverlap` |
| [x] | 코인 NetUpdateFrequency=1 | `CXPickup` 생성자 |
| [x] | 획득한 코인 파괴 | 중복 지급 가드 후 `Destroy()` |
| [x] | 코인 파괴 파티클·사운드 | `GameState::MulticastWorldFX` |
| [x] | 스피드 버프 및 추가 속도 +10 | `CXSpeedBuff::Grant` |
| [x] | 버프 NetUpdateFrequency=1 | 공통 아이템 생성자 |
| [x] | 획득한 버프 파괴 | 공통 서버 오버랩 처리 |
| [x] | 버프 파괴 파티클·사운드 | 공통 효과 RPC, 속도 아이템 피치 구분 |
| [x] | 스포너 클래스와 생성 함수 | `CXItemSpawner::SpawnItem` |
| [x] | 생성 아이템 참조와 OnDestroyed 구독 | `SpawnedItem`, `AddDynamic` |
| [x] | 파괴 2초 뒤 동일 종류 재생성 | `OnItemDestroyed` → 타이머 → 같은 `ItemClass` |
| [x] | 코인·버프 스포너 레벨 배치 | `/Game/Maps/CXMiningArena` |

## 필수 4. GameMode·GameState

| 완료 | 요구사항 | 구현 위치 |
|---|---|---|
| [x] | GameState 클래스 | `ACXGameState` |
| [x] | Waiting·Playing·Finished Enum | `ECXMatchPhase` |
| [x] | 현재 상태와 남은 시간 속성 | `MatchPhase`, `RemainingSeconds` |
| [x] | 우승자 목록 속성 | 복제되는 `Winners` 배열 |
| [x] | 시작 상태 Waiting | GameMode `BeginPlay` |
| [x] | 첫 접속 후 대기 카운트다운 | `PostLogin`, 중복 시작 방지 |
| [x] | 서버의 1초 대기 타이머와 UI | `AdvanceClock`, 복제 시간 표시 |
| [x] | 대기 종료 후 Playing | `StartPlaying` |
| [x] | 서버의 1초 경기 타이머와 UI | 같은 서버 타이머, 120초 설정 |
| [x] | 경기 시간 종료 후 Finished | `FinishMatch` |
| [x] | 가장 많은 원화 보유자 비교 | 서버 PlayerArray 순회 |
| [x] | 최고 금액 동점자는 모두 우승 | Winners 전체 동점자 포함 |
| [x] | 자신의 승리·패배 UI | 우승 / 공동 우승 / 패배 결과 패널 |

## 도전 1. 부위 공격·지뢰

| 완료 | 요구사항 | 구현 위치 |
|---|---|---|
| [x] | ApplyDamage 대신 ApplyPointDamage | `ServerFire` |
| [x] | 피격 Bone의 바퀴 여부 판별 | `FPointDamageEvent::HitInfo.BoneName` |
| [x] | 바퀴 피격 시 음수 속도 페널티 | `ApplyWheelPenalty(-30)` |
| [x] | 시간에 따른 0까지 회복·상한 | PS Tick, `FMath::Min(0.f, ...)` |
| [x] | F키 InputAction 애셋·차량 연결 | `IA_Mine`, `IMC_Combat` |
| [x] | F 입력의 Server RPC | `PlaceMine` → `ServerPlaceMine` |
| [x] | 차량 뒤쪽에 지뢰 생성 | 서버 차량 후방 360cm |
| [x] | 지뢰 Owner를 설치 차량으로 지정 | `FActorSpawnParameters.Owner=this` |
| [x] | 오버랩 차량의 가속 즉시 0 | `BreakAccelerator`, RepNotify, 매 Tick 유지 |
| [x] | 2초 뒤 가속 고장 해제 | 덮어쓰기 가능한 서버 타이머 |
| [x] | 폭발 파티클·사운드·파괴 | 공용 효과 RPC 후 지뢰 파괴 |

## 도전 2. 경기 상태별 제약

| 완료 | 요구사항 | 구현 위치 |
|---|---|---|
| [x] | Playing에서만 가속·조향 | `CanDrive`, 입력 콜백 |
| [x] | Waiting/Finished 가속·조향 0 | `ApplyMatchInputLock`, 서버 Tick 재확인 |
| [x] | Waiting/Finished 핸드브레이크 true | 같은 함수, 수동 해제 요청도 차단 |
| [x] | Waiting/Finished 차량 초기화 금지 | 입력과 `ServerResetVehicle` 양쪽 검사 |
| [x] | Playing 전환 시 상태 잠금 해제 | 상태 잠금 플래그의 전환 처리 |
| [x] | Playing에서만 좌클릭 공격 | 로컬 `CanFire`와 서버 재검사 |
| [x] | Playing에서만 F 지뢰 설치 | 로컬 입력과 서버 재검사 |

## 제출 산출물

- [x] 필수·도전 기능을 포함한 하나의 로컬 Git 저장소
- [x] 한글 주석·UFUNCTION/UPROPERTY 메타데이터 툴팁
- [x] 실행 방법, 구조 설명, 검증 결과 문서
- [x] 실제 호스트·게스트 화면을 포함하는 MP4 시연 영상, 5GB 이하
- GitHub 원격 저장소 생성·업로드 및 LMS 최종 제출은 사용자 계정에서 진행합니다. 제출하지 않은 원격 주소나 시연 결과를 임의로 만들어 기재하지 않습니다.

자동 검증의 최종 결과는 [NetworkReport.json](NetworkReport.json)에 기록합니다. 단순 소스 문자열 검사로 성공을 판단하지 않고 실제 두 프로세스에서 RPC, 복제, 오버랩, 타이머와 경기 상태를 확인합니다. 바퀴 검사는 실제 뒷바퀴 외측을 향해 게스트의 공격 RPC를 보내고, 서버 라인트레이스의 BoneName과 음수 페널티를 함께 확인합니다. 바퀴 피격 범위는 실제 스켈레톤 Bone에 부착된 쿼리 전용 구체이므로 차량 주행 물리에는 영향을 주지 않습니다.
