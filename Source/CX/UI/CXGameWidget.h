#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CXGameWidget.generated.h"
class UTextBlock;
class UBorder;
class UButton;
class UEditableTextBox;
class UChatUI;

/** WBP_CXHUD의 부모. 개인 기록은 반드시 자신의 PlayerState에서만 읽는다. */
UCLASS()
class CX_API UCXGameWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry,float DeltaSeconds) override;
    void FocusChatInput();
    void AddChatMessage(const FString& Sender,const FString& Message);
    void ShowNotice(const FString& Message);
    UPROPERTY(EditDefaultsOnly, Category="채굴|UI", meta=(ToolTip="실제로 생성할 채팅 위젯 블루프린트입니다. 기본은 WBP_ChatUI입니다.")) TSubclassOf<UChatUI> ChatWidgetClass;
    UPROPERTY(BlueprintReadOnly, Transient) TObjectPtr<UChatUI> Chat;
    UFUNCTION() void HostGame();
    UFUNCTION() void JoinGame();
    UFUNCTION() void Rematch();
private:
    UPROPERTY() TObjectPtr<UTextBlock> CoinsText;
    UPROPERTY() TObjectPtr<UTextBlock> BonusText;
    UPROPERTY() TObjectPtr<UTextBlock> SpeedText;
    UPROPERTY() TObjectPtr<UTextBlock> PhaseText;
    UPROPERTY() TObjectPtr<UTextBlock> ClockText;
    UPROPERTY() TObjectPtr<UTextBlock> ConnectionText;
    UPROPERTY() TObjectPtr<UTextBlock> ConnectionHintText;
    UPROPERTY() TObjectPtr<UTextBlock> ResultText;
    UPROPERTY() TObjectPtr<UTextBlock> NoticeText;
    UPROPERTY() TObjectPtr<UTextBlock> DamageText;
    UPROPERTY() TObjectPtr<UBorder> ResultPanel;
    UPROPERTY() TObjectPtr<UBorder> NetworkPanel;
    UPROPERTY() TObjectPtr<UEditableTextBox> IPInput;
    float NoticeRemaining=0;
    UTextBlock* Label(const FString& Text,int32 Size,FLinearColor Color=FLinearColor::White);
};
