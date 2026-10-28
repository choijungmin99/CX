#include "ChatUI.h"
#include "CXPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

void UChatUI::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    // 빈 위젯 BP가 기본 Canvas를 갖더라도 실제 채팅 컨트롤을 구성한다.
    if(EditableTextBox_ChatInput && ChatScrollBox)return;
    auto* Border=WidgetTree->ConstructWidget<UBorder>();Border->SetBrushColor(FLinearColor(.015f,.03f,.045f,.92f));Border->SetPadding(FMargin(16));WidgetTree->RootWidget=Border;
    auto* Box=WidgetTree->ConstructWidget<UVerticalBox>();Border->SetContent(Box);
    auto* Title=WidgetTree->ConstructWidget<UTextBlock>();Title->SetText(FText::FromString(TEXT("드라이버 채팅  /  Enter 또는 T")));Title->SetFont(FCoreStyle::GetDefaultFontStyle("Bold",14));Title->SetColorAndOpacity(FSlateColor(FLinearColor(.15f,.92f,.68f)));
    Box->AddChildToVerticalBox(Title)->SetPadding(FMargin(0,0,0,8));
    ChatScrollBox=WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("ChatScrollBox"));
    Box->AddChildToVerticalBox(ChatScrollBox)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    EditableTextBox_ChatInput=WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(),TEXT("EditableTextBox_ChatInput"));
    EditableTextBox_ChatInput->SetHintText(FText::FromString(TEXT("메시지 입력 후 Enter · 최대 200자")));
    EditableTextBox_ChatInput->WidgetStyle.SetFont(FCoreStyle::GetDefaultFontStyle("Regular",15));
    EditableTextBox_ChatInput->SetToolTipText(FText::FromString(TEXT("Enter로 전송합니다. Esc는 게임 종료 확인창을 열며, 계속하기를 선택하면 작성 중인 내용이 유지됩니다.")));
    Box->AddChildToVerticalBox(EditableTextBox_ChatInput)->SetPadding(FMargin(0,8,0,0));
}
void UChatUI::NativeConstruct()
{
    Super::NativeConstruct();
    if(EditableTextBox_ChatInput)EditableTextBox_ChatInput->OnTextCommitted.AddUniqueDynamic(this,&UChatUI::OnChatInputTextCommitted);
}
void UChatUI::NativeDestruct()
{
    if(EditableTextBox_ChatInput)EditableTextBox_ChatInput->OnTextCommitted.RemoveDynamic(this,&UChatUI::OnChatInputTextCommitted);
    Super::NativeDestruct();
}
void UChatUI::FocusInput(){if(EditableTextBox_ChatInput)EditableTextBox_ChatInput->SetKeyboardFocus();}
void UChatUI::OnChatInputTextCommitted(const FText& Text,ETextCommit::Type Method)
{
    if(Method!=ETextCommit::OnEnter)return;
    // 내용 전송은 소유 컨트롤러가 맡는다. 위젯 자체는 네트워크 액터가 아니므로 RPC를 선언하지 않는다.
    if(auto* PC=GetOwningPlayer<ACXPlayerController>())PC->SubmitChat(Text.ToString());
    if(EditableTextBox_ChatInput)EditableTextBox_ChatInput->SetText(FText::GetEmpty());
}
void UChatUI::OnNewChatMessage(const FString& Sender,const FString& Message)
{
    if(!ChatScrollBox)return;
    while(ChatScrollBox->GetChildrenCount()>=80)ChatScrollBox->RemoveChildAt(0);
    auto* Row=WidgetTree->ConstructWidget<UTextBlock>();
    Row->SetText(FText::FromString(FString::Printf(TEXT("%s  ›  %s"),*Sender,*Message)));
    Row->SetFont(FCoreStyle::GetDefaultFontStyle("Regular",14));Row->SetColorAndOpacity(FSlateColor(FLinearColor(.85f,.92f,.94f)));Row->SetAutoWrapText(true);
    ChatScrollBox->AddChild(Row);ChatScrollBox->ScrollToEnd();
}
FReply UChatUI::NativeOnKeyDown(const FGeometry& G,const FKeyEvent& E)
{
    if(E.GetKey()==EKeys::Escape)
    {
        if(!E.IsRepeat())if(auto* PC=GetOwningPlayer<ACXPlayerController>())PC->ToggleQuitConfirmation();
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(G,E);
}
