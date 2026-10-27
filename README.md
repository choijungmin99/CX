# 분노의 질주 : 원화 채굴 — CX

**Unreal Engine 5.5.4 / Vehicle C++ / 2인 리슨 서버 멀티플레이** 과제입니다. 필수 기능과 도전 기능을 하나의 프로젝트에 구성했습니다. 제한 시간에 더 많은 원화를 보유한 플레이어가 우승하며 동점은 공동 우승입니다.

## 빠른 실행

1. **UE 5.5.4**로 `CX.uproject`를 엽니다. 다른 엔진 버전으로 변환하지 마세요.
2. 기본 맵은 `/Game/Maps/CXMiningArena`입니다.
3. 에디터 Play 설정에서 **Number of Players = 2**, **Net Mode = Play As Listen Server**로 실행하거나, `PlayMultiplayer.cmd`를 실행합니다.
4. 첫 접속 후 10초 동안 출발을 기다립니다. 실제 경기는 120초입니다.
5. 종료 후 R을 누르면 개인 기록과 차량을 초기화하고 다음 경기를 시작합니다.

Standalone으로 실행하면 화면 오른쪽의 **리슨 서버 열기 / 서버에 접속** 버튼도 사용할 수 있습니다. 같은 PC의 두 번째 창에서는 `127.0.0.1`을 입력합니다. 다른 PC에서는 호스트의 로컬 IP를 입력하며, 별도 방화벽 설정은 이 프로젝트가 변경하지 않습니다.

| 조작 | 키 |
|---|---|
| 가속 / 제동·후진 | W / S |
| 조향 | A / D |
| 핸드브레이크 | Space |
| 앞 범퍼 라인트레이스 공격 | 좌클릭 |
| 뒤에 지뢰 설치 | F |
| 차량 자세 복구 | Backspace |
| 채팅 입력 / 전송 | Enter 또는 T / Enter |
| 채팅 취소 | Esc |
| 종료 후 다시 경기 | R |

금색 코인은 **10원**, 민트색 속도 아이템은 **추가 최고 속도 10 km/h**를 줍니다. 아이템은 획득 후 사라지고 같은 스포너에서 파괴 시점부터 2초 뒤 재생성됩니다.

## 구현한 규칙

- 공격은 **서버가 앞 범퍼에서 1,800cm 라인트레이스**를 수행합니다. 피해 차량이 5원 이상 보유하면 정확히 5원을 옮기고, 5원 미만이면 양쪽 잔액을 그대로 둡니다. 공격 간격은 서버에서 0.45초로 제한합니다.
- `ApplyPointDamage`의 `BoneName`에 `Wheel`이 포함되면 바퀴 피격입니다. 일시 속도 페널티 -30이 적용되고 초당 10씩 회복합니다. **페널티 자체는 0을 넘지 않으며 기존에 획득한 속도 보너스는 보존**됩니다.
- F키로 생성한 지뢰의 `Owner`는 설치 차량입니다. 설치자는 자기 지뢰에 걸리지 않습니다. 다른 차량이 닿으면 2초 동안 가속 입력을 0으로 유지하고 폭발 후 파괴합니다. 재설치 대기는 3초, 미폭발 지뢰 수명은 25초입니다.
- `Waiting`과 `Finished`에서 가속·조향은 0, 핸드브레이크는 true입니다. 공격·지뢰·차량 복구 요청도 서버에서 거부합니다. `Playing` 전환 시 상태 잠금만 해제하여 실제 핸드브레이크 입력을 방해하지 않습니다.
- 채팅은 Enter 이벤트에서 소유 컨트롤러의 Server RPC로 보냅니다. 발신자는 서버 PlayerState로 결정하고, 빈 문자열·200자 초과·제어 문자·0.35초 이내 연속 요청을 거부합니다. 모든 플레이어에게 Client RPC로 전달합니다.

## 서버와 클라이언트의 역할

| 대상 | 서버 | 클라이언트 |
|---|---|---|
| PlayerState.Coins / BonusMaxSpeed | 실제 값 변경, 우승 판정에 사용 | `COND_OwnerOnly`로 **자신의 값만** 수신 |
| GameState | 상태·남은 시간·우승자 목록 갱신 | 복제된 결과로 UMG 갱신 |
| GameMode | 첫 접속, 1초 타이머, 단독·공동 우승, 재경기 | 존재하지 않음 |
| 공격 / 지뢰 | RPC 소유권·경기 상태·쿨다운 확인 후 판정 | 입력 요청과 시각 효과 |
| 코인 / 속도 버프 | 오버랩, 한 번만 지급, 파괴·재생성 | 같은 액터의 생성·파괴와 효과 표시 |
| 채팅 | 발신자 확정, 입력 검증, 대상별 전달 | UMG 입력과 수신 로그 표시 |

리슨 서버의 호스트는 **서버이기도 하므로** 모든 실제 잔액을 메모리에 가지고 있습니다. `OwnerOnly`의 비공개 보장은 원격 클라이언트에 적용됩니다. UI는 호스트를 포함해 항상 자신의 PlayerState만 표시합니다.

## 파일 안내

- `Source/CX/CXPlayerState.*`: 소유자 전용 복제, 원화 증감, 속도 보너스·페널티 회복
- `Source/CX/CXGameState.*`, `CXGameMode.*`: 경기 상태, 서버 타이머, 공동 우승, 재경기
- `Source/CX/CXPawn.*`: Vehicle 템플릿 차량, Enhanced Input, 공격·지뢰·입력 제한
- `Source/CX/CXPickup.*`, `CXItemSpawner.*`, `CXMine.*`: 아이템과 파괴 델리게이트 기반 재생성
- `Source/CX/UI/`: 한글 UMG 상태창과 채팅 Enter 이벤트
- `Content/UI/WBP_ChatUI`, `WBP_CXHUD`: 실제 사용하는 위젯 블루프린트 애셋
- `Content/Input/IA_Fire`, `IA_Mine`, `IMC_Combat`: 좌클릭·F키 Enhanced Input 애셋
- `Source/CX/Tests/`: `-CXNetTest`일 때만 실행하는 실제 2프로세스 검증 시나리오
- [요구사항 체크리스트](Docs/Requirements.md), [설계와 데이터 흐름](Docs/Architecture.md)
- [제출본 검증과 시연 영상 안내](Docs/DeliveryVerification.md)

## 빌드와 검증

```powershell
.\Build.cmd
.\PlayMultiplayer.cmd
.\VerifyMultiplayer.cmd
```

Visual Studio C++ 도구와 Windows SDK가 필요합니다. 일반 게임 실행에는 테스트 시나리오가 켜지지 않습니다.

새 시연 영상을 만들려면 `VerifyMultiplayer.cmd --record-demo`를 실행합니다. 인코딩용 FFmpeg가 PATH에 없으면 `--ffmpeg "실행 파일 경로"`를 추가합니다. `Scripts/cx.py`는 UE에 포함된 Python으로 실행되므로 별도 Python 설치나 PowerShell 실행 정책 변경이 필요하지 않습니다.

`Saved/Tests/NetworkReport.json`과 Host/Client 로그에서 실제 결과를 확인합니다. 검증용 임시 코인·차량 고정은 테스트 옵션에서만 적용되고 일반 경기에는 없습니다. 네트워크 테스트는 별도 프로세스 연결, 개인 정보 비공개 복제, 채팅, 서버 판정, 아이템 재생성, 지뢰 회복, 우승과 재경기를 확인합니다.

## 제출 준비

- 이 폴더 전체가 **하나의 Git 저장소**입니다. 필수/도전 기능을 나누어 제출하지 않습니다.
- `Binaries`, `Intermediate`, `Saved`, `DerivedDataCache`, `.Tools`는 커밋 대상에서 제외합니다.
- 시연 영상은 [Deliverables/CX_Multiplayer_Demo.mp4](Deliverables/CX_Multiplayer_Demo.mp4)이며 **29.4초, 1920×600**입니다. 공동 우승과 재경기까지 포함하며 5GB보다 작습니다.
- GitHub 저장소 링크와 MP4를 과제 제출란에 제출합니다. GitHub 공개 업로드와 LMS 제출은 자동으로 실행하지 않습니다.
- 과제에 기재된 제출 기한: **2026-10-28 13:00**.

Epic Games의 정식 UE 5.5.4 Vehicle 템플릿과 Starter Content를 사용했습니다. 에셋 사용에는 Unreal Engine 라이선스가 적용됩니다.
