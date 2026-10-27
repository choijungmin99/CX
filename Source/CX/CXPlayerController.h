#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CXPlayerController.generated.h"

class UInputMappingContext;
class ACXPawn;
class UCXGameWidget;

/** 로컬 입력과 UI, 소유권을 이용한 채팅 Server/Client RPC를 연결한다. */
UCLASS()
class CX_API ACXPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    ACXPlayerController();
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void Tick(float Delta) override;
    virtual void OnPossess(APawn* Pawn) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="채굴|입력", meta=(ToolTip="차량 기본 주행용 Enhanced Input 매핑 컨텍스트입니다.")) TObjectPtr<UInputMappingContext> InputMappingContext;
    UPROPERTY(EditAnywhere, Category="채굴|입력") bool bUseSteeringWheelControls=false;
    UPROPERTY(EditAnywhere, Category="채굴|입력") TObjectPtr<UInputMappingContext> SteeringWheelInputMappingContext;
    UPROPERTY(EditDefaultsOnly, Category="채굴|입력", meta=(ToolTip="좌클릭 공격과 F키 지뢰 설치를 연결하는 추가 입력 매핑입니다.")) TObjectPtr<UInputMappingContext> CombatMapping;
    UPROPERTY(Replicated, BlueprintReadOnly, Category="채굴|차량", meta=(ToolTip="이 컨트롤러가 조종하는 차량입니다. 클라이언트에도 참조가 복제됩니다.")) TObjectPtr<ACXPawn> VehiclePawn;
    UPROPERTY(EditDefaultsOnly, Category="채굴|UI", meta=(ToolTip="원화·속도·타이머·결과·채팅을 표시하는 위젯 블루프린트입니다.")) TSubclassOf<UCXGameWidget> GameWidgetClass;
    UPROPERTY(Transient) TObjectPtr<UCXGameWidget> GameWidget;

    void FocusChat();
    void CloseChat();
    void SubmitChat(const FString& Message);
    UFUNCTION(Server,Reliable) void ServerSendChat(const FString& Message);
    UFUNCTION(Client,Reliable) void ClientReceiveChat(const FString& Sender,const FString& Message);
    UFUNCTION(Client,Reliable) void ClientNotify(const FString& Message);
    UFUNCTION(Server,Reliable) void ServerRequestRematch();
    void Rematch();
    bool IsChatOpen() const {return bChatOpen;}
    int32 ReceivedChatMessages=0;
    /** 실제 별도 클라이언트에서만 실행하는 개발용 네트워크 검증. 일반 경기에서는 호출하지 않는다. */
    UFUNCTION(Client,Reliable) void ClientTestAction(int32 Action);
    UFUNCTION(Client,Reliable) void ClientTestProbe(const FString& Label,int32 ExpectedCoins,float ExpectedBonus,int32 Phase);
    UFUNCTION(Server,Reliable) void ServerTestResult(const FString& Label,bool bPassed);
    UFUNCTION(Client,Reliable) void ClientStartDemoCapture();
    UFUNCTION(Client,Reliable) void ClientTestPlaceCar(FVector Location);
private:
    bool bChatOpen=false;
    double LastChatTime=-100;
    bool bCapturingDemo=false;
    double DemoStartTime=0;
    double LastDemoFrame=-100;
    int32 DemoFrameIndex=0;
    FString DemoFrameFolder;
};
