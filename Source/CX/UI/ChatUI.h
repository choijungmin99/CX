#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/EditableTextBox.h"
#include "ChatUI.generated.h"
class UScrollBox;

/** WBP_ChatUI의 C++ 부모. Enter 입력 → 소유 컨트롤러 Server RPC → 각 Client RPC → 채팅 로그. */
UCLASS()
class CX_API UChatUI : public UUserWidget
{
    GENERATED_BODY()
public:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
    UFUNCTION(BlueprintCallable, Category="채굴|채팅") void OnNewChatMessage(const FString& Sender,const FString& Message);
    UFUNCTION() void OnChatInputTextCommitted(const FText& Text,ETextCommit::Type Method);
    void FocusInput();
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="Enter 커밋 이벤트를 바인딩하는 채팅 입력창입니다.")) TObjectPtr<UEditableTextBox> EditableTextBox_ChatInput;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="수신된 채팅을 시간 순서대로 쌓는 스크롤 영역입니다.")) TObjectPtr<UScrollBox> ChatScrollBox;
};
