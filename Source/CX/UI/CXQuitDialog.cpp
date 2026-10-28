#include "CXQuitDialog.h"
#include "CXPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Brushes/SlateColorBrush.h"
#include "Styling/CoreStyle.h"
#include "InputCoreTypes.h"

void UCXQuitDialog::NativeOnInitialized()
{
    Super::NativeOnInitialized();SetIsFocusable(true);
    // 화면 전체를 덮어 확인창 뒤의 채팅/버튼으로 클릭이 전달되지 않게 한다.
    auto* Shade=WidgetTree->ConstructWidget<UBorder>();WidgetTree->RootWidget=Shade;
    Shade->SetBrushColor(FLinearColor(.004f,.01f,.018f,.8f));
    Shade->SetHorizontalAlignment(HAlign_Center);Shade->SetVerticalAlignment(VAlign_Center);
    auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetWidthOverride(560.f);Shade->SetContent(Size);
    auto* Panel=WidgetTree->ConstructWidget<UBorder>();Panel->SetBrushColor(FLinearColor(.018f,.035f,.052f));Panel->SetPadding(FMargin(30));Size->SetContent(Panel);
    auto* Content=WidgetTree->ConstructWidget<UVerticalBox>();Panel->SetContent(Content);
    auto Label=[&](const TCHAR* Text,int32 FontSize,FLinearColor Color)
    {
        auto* TextBlock=WidgetTree->ConstructWidget<UTextBlock>();TextBlock->SetText(FText::FromString(Text));
        TextBlock->SetFont(FCoreStyle::GetDefaultFontStyle("Regular",FontSize));
        TextBlock->SetColorAndOpacity(FSlateColor(Color));TextBlock->SetJustification(ETextJustify::Center);TextBlock->SetAutoWrapText(true);
        return TextBlock;
    };
    Content->AddChildToVerticalBox(Label(TEXT("게임 종료"),28,FLinearColor(1,.75f,.35f)));
    Content->AddChildToVerticalBox(Label(TEXT("게임을 종료하시겠습니까?"),22,FLinearColor::White))->SetPadding(FMargin(0,18,0,0));
    const bool bHost=GetOwningPlayer()&&GetOwningPlayer()->GetNetMode()==NM_ListenServer;
    const TCHAR* Hint=bHost?TEXT("호스트가 종료하면 상대 플레이어와의 연결도 종료됩니다."):TEXT("종료를 선택하면 게임 창이 닫힙니다.");
    Content->AddChildToVerticalBox(Label(Hint,15,FLinearColor(.65f,.75f,.8f)))->SetPadding(FMargin(0,12,0,26));
    auto* Buttons=WidgetTree->ConstructWidget<UHorizontalBox>();Content->AddChildToVerticalBox(Buttons);
    auto Button=[&](const TCHAR* Caption,FLinearColor Color)
    {
        auto* Result=WidgetTree->ConstructWidget<UButton>();
        FButtonStyle Style=FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
        Style.SetNormal(FSlateColorBrush(Color));Style.SetHovered(FSlateColorBrush(Color*1.25f));
        Style.SetPressed(FSlateColorBrush(Color*.75f));
        Style.SetNormalPadding(FMargin(18,13));Style.SetPressedPadding(FMargin(18,13));Result->SetStyle(Style);
        Result->SetContent(Label(Caption,18,FLinearColor::White));
        auto* Slot=Buttons->AddChildToHorizontalBox(Result);Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));Slot->SetPadding(FMargin(6,0));
        return Result;
    };
    ContinueButton=Button(TEXT("계속하기  [Esc]"),FLinearColor(.045f,.3f,.24f));
    ContinueButton->SetToolTipText(FText::FromString(TEXT("종료 확인창을 닫고 게임으로 돌아갑니다.")));
    ContinueButton->OnClicked.AddDynamic(this,&UCXQuitDialog::ContinuePlaying);
    auto* QuitButton=Button(TEXT("게임 종료"),FLinearColor(.45f,.12f,.12f));
    QuitButton->SetToolTipText(FText::FromString(TEXT("현재 게임 창을 닫습니다. 호스트가 종료하면 상대의 접속도 끊깁니다.")));
    QuitButton->OnClicked.AddDynamic(this,&UCXQuitDialog::ConfirmQuit);
}
void UCXQuitDialog::FocusContinueButton()
{
    // 기본 선택은 계속하기다. Enter를 눌러도 실수로 즉시 종료되지 않는다.
    if(ContinueButton)ContinueButton->SetUserFocus(GetOwningPlayer());
}
FReply UCXQuitDialog::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    if(Event.GetKey()==EKeys::Escape)
    {
        if(!Event.IsRepeat())ContinuePlaying();
        return FReply::Handled();
    }
    return Super::NativeOnPreviewKeyDown(Geometry,Event);
}
void UCXQuitDialog::ContinuePlaying(){if(auto* PC=GetOwningPlayer<ACXPlayerController>())PC->CancelQuit();}
void UCXQuitDialog::ConfirmQuit(){if(auto* PC=GetOwningPlayer<ACXPlayerController>())PC->ConfirmQuit();}
