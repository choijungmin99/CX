#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CXQuitDialog.generated.h"

class UButton;

/** 로컬 플레이어에게만 표시하는 종료 확인창. 경기 전체를 일시 정지하지 않는다. */
UCLASS()
class CX_API UCXQuitDialog : public UUserWidget
{
    GENERATED_BODY()
public:
    virtual void NativeOnInitialized() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
    void FocusContinueButton();
private:
    UPROPERTY(Transient) TObjectPtr<UButton> ContinueButton;
    UFUNCTION() void ContinuePlaying();
    UFUNCTION() void ConfirmQuit();
};
