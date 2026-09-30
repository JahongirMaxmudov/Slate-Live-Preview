// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#include "SlateWidgetBuilder.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"

FSlateWidgetBuilder::FSlateWidgetBuilder()
{
}

void FSlateWidgetBuilder::Log(const FString& Message)
{
	if (OnLogHandler.IsBound())
	{
		OnLogHandler.Execute(Message);
	}
}

TSharedRef<SWidget> FSlateWidgetBuilder::Build(const TSharedPtr<FSlateWidgetNode>& RootNode)
{
	if (!RootNode.IsValid())
	{
		return SNew(STextBlock)
			.Text(FText::FromString(TEXT("No widget to display")));
	}

	return BuildWidget(RootNode);
}

TSharedRef<SWidget> FSlateWidgetBuilder::BuildWidget(const TSharedPtr<FSlateWidgetNode>& Node)
{
	if (!Node.IsValid())
	{
		return SNullWidget::NullWidget;
	}

	const FString& Type = Node->WidgetType;

	if (Type.Equals(TEXT("SBorder"), ESearchCase::IgnoreCase))
	{
		return BuildBorder(Node);
	}
	if (Type.Equals(TEXT("SVerticalBox"), ESearchCase::IgnoreCase))
	{
		return BuildVerticalBox(Node);
	}
	if (Type.Equals(TEXT("SHorizontalBox"), ESearchCase::IgnoreCase))
	{
		return BuildHorizontalBox(Node);
	}
	if (Type.Equals(TEXT("SOverlay"), ESearchCase::IgnoreCase))
	{
		return BuildOverlay(Node);
	}
	if (Type.Equals(TEXT("SBox"), ESearchCase::IgnoreCase))
	{
		return BuildBox(Node);
	}
	if (Type.Equals(TEXT("SSpacer"), ESearchCase::IgnoreCase))
	{
		return BuildSpacer(Node);
	}
	if (Type.Equals(TEXT("SScrollBox"), ESearchCase::IgnoreCase))
	{
		return BuildScrollBox(Node);
	}
	if (Type.Equals(TEXT("SSeparator"), ESearchCase::IgnoreCase))
	{
		return BuildSeparator(Node);
	}
	if (Type.Equals(TEXT("STextBlock"), ESearchCase::IgnoreCase))
	{
		return BuildTextBlock(Node);
	}
	if (Type.Equals(TEXT("SButton"), ESearchCase::IgnoreCase))
	{
		return BuildButton(Node);
	}
	if (Type.Equals(TEXT("SImage"), ESearchCase::IgnoreCase))
	{
		return BuildImage(Node);
	}
	if (Type.Equals(TEXT("SCheckBox"), ESearchCase::IgnoreCase))
	{
		return BuildCheckBox(Node);
	}
	if (Type.Equals(TEXT("SEditableTextBox"), ESearchCase::IgnoreCase))
	{
		return BuildEditableTextBox(Node);
	}
	if (Type.Equals(TEXT("SMultiLineEditableTextBox"), ESearchCase::IgnoreCase))
	{
		return BuildMultiLineEditableTextBox(Node);
	}
	if (Type.Equals(TEXT("SProgressBar"), ESearchCase::IgnoreCase))
	{
		return BuildProgressBar(Node);
	}
	if (Type.Equals(TEXT("SSlider"), ESearchCase::IgnoreCase))
	{
		return BuildSlider(Node);
	}
	if (Type.Equals(TEXT("SColorBlock"), ESearchCase::IgnoreCase))
	{
		return BuildColorBlock(Node);
	}

	return BuildFallbackWidget(Node);
}

// ----------------------------------------------------------------------------
// SBorder
// ----------------------------------------------------------------------------
TSharedRef<SWidget> FSlateWidgetBuilder::BuildBorder(const TSharedPtr<FSlateWidgetNode>& Node)
{
	FLinearColor BgColor = FLinearColor(0.04f, 0.04f, 0.04f, 0.9f);
	FMargin Padding = FMargin(4.0f);
	EHorizontalAlignment HAlign = HAlign_Fill;
	EVerticalAlignment VAlign = VAlign_Fill;

	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("BorderBackgroundColor")))
	{
		if (Prop->HasArgs()) BgColor = Prop->GetFirstArg().AsColor(BgColor);
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("Padding")))
	{
		if (Prop->HasArgs()) Padding = Prop->GetFirstArg().AsMargin(Padding);
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("HAlign")))
	{
		if (Prop->HasArgs()) HAlign = Prop->GetFirstArg().AsHAlign(HAlign);
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("VAlign")))
	{
		if (Prop->HasArgs()) VAlign = Prop->GetFirstArg().AsVAlign(VAlign);
	}

	TSharedPtr<SWidget> ChildWidget = SNullWidget::NullWidget;
	if (Node->DirectChild.IsValid())
	{
		ChildWidget = BuildWidget(Node->DirectChild);
	}
	else if (Node->Slots.Num() > 0 && Node->Slots[0].ChildWidget.IsValid())
	{
		ChildWidget = BuildWidget(Node->Slots[0].ChildWidget);
	}

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(BgColor)
		.Padding(Padding)
		.HAlign(HAlign)
		.VAlign(VAlign)
		[
			ChildWidget.ToSharedRef()
		];
}

// ----------------------------------------------------------------------------
// SVerticalBox
// ----------------------------------------------------------------------------
TSharedRef<SWidget> FSlateWidgetBuilder::BuildVerticalBox(const TSharedPtr<FSlateWidgetNode>& Node)
{
	TSharedRef<SVerticalBox> VBox = SNew(SVerticalBox);

	for (const FSlateSlotNode& SlotNode : Node->Slots)
	{
		SVerticalBox::FScopedWidgetSlotArguments SlotArg = VBox->AddSlot();

		if (const FSlatePropertyNode* Prop = SlotNode.FindProperty(TEXT("AutoHeight")))
		{
			SlotArg.AutoHeight();
		}
		if (const FSlatePropertyNode* Prop = SlotNode.FindProperty(TEXT("FillHeight")))
		{
			float Weight = Prop->HasArgs() ? Prop->GetFirstArg().AsFloat(1.0f) : 1.0f;
			SlotArg.FillHeight(Weight);
		}
		if (const FSlatePropertyNode* Prop = SlotNode.FindProperty(TEXT("Padding")))
		{
			if (Prop->HasArgs()) SlotArg.Padding(Prop->GetFirstArg().AsMargin(FMargin(0.0f)));
		}
		if (const FSlatePropertyNode* Prop = SlotNode.FindProperty(TEXT("HAlign")))
		{
			if (Prop->HasArgs()) SlotArg.HAlign(Prop->GetFirstArg().AsHAlign(HAlign_Fill));
		}
		if (const FSlatePropertyNode* Prop = SlotNode.FindProperty(TEXT("VAlign")))
		{
			if (Prop->HasArgs()) SlotArg.VAlign(Prop->GetFirstArg().AsVAlign(VAlign_Fill));
		}
		if (const FSlatePropertyNode* Prop = SlotNode.FindProperty(TEXT("MaxHeight")))
		{
			if (Prop->HasArgs()) SlotArg.MaxHeight(Prop->GetFirstArg().AsFloat(1000.0f));
		}

		TSharedRef<SWidget> Child = SlotNode.ChildWidget.IsValid() ? BuildWidget(SlotNode.ChildWidget) : SNullWidget::NullWidget;
		SlotArg[ Child ];
	}

	return VBox;
}

// ----------------------------------------------------------------------------
// SHorizontalBox
// ----------------------------------------------------------------------------
TSharedRef<SWidget> FSlateWidgetBuilder::BuildHorizontalBox(const TSharedPtr<FSlateWidgetNode>& Node)
{
	TSharedRef<SHorizontalBox> HBox = SNew(SHorizontalBox);

	for (const FSlateSlotNode& SlotNode : Node->Slots)
	{
		SHorizontalBox::FScopedWidgetSlotArguments SlotArg = HBox->AddSlot();

		if (const FSlatePropertyNode* Prop = SlotNode.FindProperty(TEXT("AutoWidth")))
		{
			SlotArg.AutoWidth();
		}
		if (const FSlatePropertyNode* Prop = SlotNode.FindProperty(TEXT("FillWidth")))
		{
			float Weight = Prop->HasArgs() ? Prop->GetFirstArg().AsFloat(1.0f) : 1.0f;
			SlotArg.FillWidth(Weight);
		}
		if (const FSlatePropertyNode* Prop = SlotNode.FindProperty(TEXT("Padding")))
		{
			if (Prop->HasArgs()) SlotArg.Padding(Prop->GetFirstArg().AsMargin(FMargin(0.0f)));
		}
		if (const FSlatePropertyNode* Prop = SlotNode.FindProperty(TEXT("HAlign")))
		{
			if (Prop->HasArgs()) SlotArg.HAlign(Prop->GetFirstArg().AsHAlign(HAlign_Fill));
		}
		if (const FSlatePropertyNode* Prop = SlotNode.FindProperty(TEXT("VAlign")))
		{
			if (Prop->HasArgs()) SlotArg.VAlign(Prop->GetFirstArg().AsVAlign(VAlign_Fill));
		}
		if (const FSlatePropertyNode* Prop = SlotNode.FindProperty(TEXT("MaxWidth")))
		{
			if (Prop->HasArgs()) SlotArg.MaxWidth(Prop->GetFirstArg().AsFloat(1000.0f));
		}

		TSharedRef<SWidget> Child = SlotNode.ChildWidget.IsValid() ? BuildWidget(SlotNode.ChildWidget) : SNullWidget::NullWidget;
		SlotArg[ Child ];
	}

	return HBox;
}

// ----------------------------------------------------------------------------
// SOverlay
// ----------------------------------------------------------------------------
TSharedRef<SWidget> FSlateWidgetBuilder::BuildOverlay(const TSharedPtr<FSlateWidgetNode>& Node)
{
	TSharedRef<SOverlay> Overlay = SNew(SOverlay);

	for (const FSlateSlotNode& SlotNode : Node->Slots)
	{
		SOverlay::FScopedWidgetSlotArguments SlotArg = Overlay->AddSlot();

		if (const FSlatePropertyNode* Prop = SlotNode.FindProperty(TEXT("Padding")))
		{
			if (Prop->HasArgs()) SlotArg.Padding(Prop->GetFirstArg().AsMargin(FMargin(0.0f)));
		}
		if (const FSlatePropertyNode* Prop = SlotNode.FindProperty(TEXT("HAlign")))
		{
			if (Prop->HasArgs()) SlotArg.HAlign(Prop->GetFirstArg().AsHAlign(HAlign_Fill));
		}
		if (const FSlatePropertyNode* Prop = SlotNode.FindProperty(TEXT("VAlign")))
		{
			if (Prop->HasArgs()) SlotArg.VAlign(Prop->GetFirstArg().AsVAlign(VAlign_Fill));
		}

		TSharedRef<SWidget> Child = SlotNode.ChildWidget.IsValid() ? BuildWidget(SlotNode.ChildWidget) : SNullWidget::NullWidget;
		SlotArg[ Child ];
	}

	return Overlay;
}

// ----------------------------------------------------------------------------
// SBox
// ----------------------------------------------------------------------------
TSharedRef<SWidget> FSlateWidgetBuilder::BuildBox(const TSharedPtr<FSlateWidgetNode>& Node)
{
	TSharedRef<SBox> Box = SNew(SBox);

	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("WidthOverride")))
	{
		if (Prop->HasArgs()) Box->SetWidthOverride(FOptionalSize(Prop->GetFirstArg().AsFloat(100.0f)));
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("HeightOverride")))
	{
		if (Prop->HasArgs()) Box->SetHeightOverride(FOptionalSize(Prop->GetFirstArg().AsFloat(100.0f)));
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("MinDesiredWidth")))
	{
		if (Prop->HasArgs()) Box->SetMinDesiredWidth(FOptionalSize(Prop->GetFirstArg().AsFloat(0.0f)));
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("MinDesiredHeight")))
	{
		if (Prop->HasArgs()) Box->SetMinDesiredHeight(FOptionalSize(Prop->GetFirstArg().AsFloat(0.0f)));
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("MaxDesiredWidth")))
	{
		if (Prop->HasArgs()) Box->SetMaxDesiredWidth(FOptionalSize(Prop->GetFirstArg().AsFloat(1000.0f)));
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("MaxDesiredHeight")))
	{
		if (Prop->HasArgs()) Box->SetMaxDesiredHeight(FOptionalSize(Prop->GetFirstArg().AsFloat(1000.0f)));
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("Padding")))
	{
		if (Prop->HasArgs()) Box->SetPadding(Prop->GetFirstArg().AsMargin(FMargin(0.0f)));
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("HAlign")))
	{
		if (Prop->HasArgs()) Box->SetHAlign(Prop->GetFirstArg().AsHAlign(HAlign_Fill));
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("VAlign")))
	{
		if (Prop->HasArgs()) Box->SetVAlign(Prop->GetFirstArg().AsVAlign(VAlign_Fill));
	}

	if (Node->DirectChild.IsValid())
	{
		Box->SetContent(BuildWidget(Node->DirectChild));
	}
	else if (Node->Slots.Num() > 0 && Node->Slots[0].ChildWidget.IsValid())
	{
		Box->SetContent(BuildWidget(Node->Slots[0].ChildWidget));
	}

	return Box;
}

// ----------------------------------------------------------------------------
// SSpacer
// ----------------------------------------------------------------------------
TSharedRef<SWidget> FSlateWidgetBuilder::BuildSpacer(const TSharedPtr<FSlateWidgetNode>& Node)
{
	FVector2D SpacerSize = FVector2D(16.0f, 16.0f);

	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("Size")))
	{
		if (Prop->Arguments.Num() >= 2)
		{
			SpacerSize = FVector2D(Prop->Arguments[0].AsFloat(16.0f), Prop->Arguments[1].AsFloat(16.0f));
		}
		else if (Prop->Arguments.Num() == 1)
		{
			float Val = Prop->Arguments[0].AsFloat(16.0f);
			SpacerSize = FVector2D(Val, Val);
		}
	}

	return SNew(SSpacer).Size(SpacerSize);
}

// ----------------------------------------------------------------------------
// SScrollBox
// ----------------------------------------------------------------------------
TSharedRef<SWidget> FSlateWidgetBuilder::BuildScrollBox(const TSharedPtr<FSlateWidgetNode>& Node)
{
	TSharedRef<SScrollBox> Scroll = SNew(SScrollBox);

	for (const FSlateSlotNode& SlotNode : Node->Slots)
	{
		if (SlotNode.ChildWidget.IsValid())
		{
			Scroll->AddSlot()
				.Padding(FMargin(0.0f))
				[
					BuildWidget(SlotNode.ChildWidget)
				];
		}
	}

	if (Node->DirectChild.IsValid())
	{
		Scroll->AddSlot()
			[
				BuildWidget(Node->DirectChild)
			];
	}

	return Scroll;
}

// ----------------------------------------------------------------------------
// SSeparator
// ----------------------------------------------------------------------------
TSharedRef<SWidget> FSlateWidgetBuilder::BuildSeparator(const TSharedPtr<FSlateWidgetNode>& Node)
{
	EOrientation Orientation = Orient_Horizontal;
	float Thickness = 1.0f;
	FLinearColor Color = FLinearColor(0.2f, 0.2f, 0.2f, 1.0f);

	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("Orientation")))
	{
		if (Prop->HasArgs() && Prop->GetFirstArg().StringValue.Contains(TEXT("Vertical")))
		{
			Orientation = Orient_Vertical;
		}
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("Thickness")))
	{
		if (Prop->HasArgs()) Thickness = Prop->GetFirstArg().AsFloat(1.0f);
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("ColorAndOpacity")))
	{
		if (Prop->HasArgs()) Color = Prop->GetFirstArg().AsColor(Color);
	}

	return SNew(SSeparator)
		.Orientation(Orientation)
		.Thickness(Thickness)
		.ColorAndOpacity(Color);
}

// ----------------------------------------------------------------------------
// STextBlock
// ----------------------------------------------------------------------------
TSharedRef<SWidget> FSlateWidgetBuilder::BuildTextBlock(const TSharedPtr<FSlateWidgetNode>& Node)
{
	FText Text = FText::FromString(TEXT("Text"));
	FSlateColor Color = FSlateColor(FLinearColor::White);
	int32 FontSize = 10;
	FName FontStyle = TEXT("Regular");
	ETextJustify::Type Justification = ETextJustify::Left;
	bool bAutoWrap = false;

	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("Text")))
	{
		if (Prop->HasArgs()) Text = Prop->GetFirstArg().AsText(Text);
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("ColorAndOpacity")))
	{
		if (Prop->HasArgs()) Color = FSlateColor(Prop->GetFirstArg().AsColor(FLinearColor::White));
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("Font")))
	{
		if (Prop->Arguments.Num() >= 2)
		{
			FontStyle = FName(*Prop->Arguments[0].StringValue);
			FontSize = Prop->Arguments[1].AsInt(10);
		}
		else if (Prop->Arguments.Num() == 1)
		{
			FontSize = Prop->Arguments[0].AsInt(10);
		}
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("AutoWrapText")))
	{
		if (Prop->HasArgs()) bAutoWrap = Prop->GetFirstArg().AsBool(false);
	}

	FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle(FontStyle, FontSize);

	return SNew(STextBlock)
		.Text(Text)
		.ColorAndOpacity(Color)
		.Font(FontInfo)
		.Justification(Justification)
		.AutoWrapText(bAutoWrap);
}

// ----------------------------------------------------------------------------
// SButton
// ----------------------------------------------------------------------------
TSharedRef<SWidget> FSlateWidgetBuilder::BuildButton(const TSharedPtr<FSlateWidgetNode>& Node)
{
	FLinearColor ButtonColor = FLinearColor(0.2f, 0.2f, 0.22f, 1.0f);
	FMargin ContentPadding = FMargin(8.0f, 4.0f);
	EHorizontalAlignment HAlign = HAlign_Center;
	EVerticalAlignment VAlign = VAlign_Center;
	bool bIsEnabled = true;

	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("ButtonColorAndOpacity")))
	{
		if (Prop->HasArgs()) ButtonColor = Prop->GetFirstArg().AsColor(ButtonColor);
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("ContentPadding")))
	{
		if (Prop->HasArgs()) ContentPadding = Prop->GetFirstArg().AsMargin(ContentPadding);
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("HAlign")))
	{
		if (Prop->HasArgs()) HAlign = Prop->GetFirstArg().AsHAlign(HAlign);
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("VAlign")))
	{
		if (Prop->HasArgs()) VAlign = Prop->GetFirstArg().AsVAlign(VAlign);
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("IsEnabled")))
	{
		if (Prop->HasArgs()) bIsEnabled = Prop->GetFirstArg().AsBool(true);
	}

	FString ButtonLabel = Node->VariableName.IsEmpty() ? TEXT("Button") : Node->VariableName;

	TSharedPtr<SWidget> Content = SNullWidget::NullWidget;
	if (Node->DirectChild.IsValid())
	{
		Content = BuildWidget(Node->DirectChild);
	}
	else if (Node->Slots.Num() > 0 && Node->Slots[0].ChildWidget.IsValid())
	{
		Content = BuildWidget(Node->Slots[0].ChildWidget);
	}
	else
	{
		Content = SNew(STextBlock)
			.Text(FText::FromString(ButtonLabel))
			.ColorAndOpacity(FLinearColor::White);
	}

	return SNew(SButton)
		.ButtonColorAndOpacity(ButtonColor)
		.ContentPadding(ContentPadding)
		.HAlign(HAlign)
		.VAlign(VAlign)
		.IsEnabled(bIsEnabled)
		.OnClicked_Lambda([LocalLog = OnLogHandler, ButtonLabel]() -> FReply
		{
			if (LocalLog.IsBound())
			{
				LocalLog.Execute(FString::Printf(TEXT("[Live Preview] Button '%s' clicked!"), *ButtonLabel));
			}
			return FReply::Handled();
		})
		[
			Content.ToSharedRef()
		];
}

// ----------------------------------------------------------------------------
// SImage
// ----------------------------------------------------------------------------
TSharedRef<SWidget> FSlateWidgetBuilder::BuildImage(const TSharedPtr<FSlateWidgetNode>& Node)
{
	FLinearColor Tint = FLinearColor::White;
	FVector2D DesiredSize = FVector2D(32.0f, 32.0f);

	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("ColorAndOpacity")))
	{
		if (Prop->HasArgs()) Tint = Prop->GetFirstArg().AsColor(Tint);
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("DesiredSizeOverride")))
	{
		if (Prop->Arguments.Num() >= 2)
		{
			DesiredSize = FVector2D(Prop->Arguments[0].AsFloat(32.0f), Prop->Arguments[1].AsFloat(32.0f));
		}
	}

	return SNew(SBox)
		.WidthOverride(DesiredSize.X)
		.HeightOverride(DesiredSize.Y)
		[
			SNew(SImage)
			.ColorAndOpacity(Tint)
			.Image(FAppStyle::Get().GetBrush("Icons.Default"))
		];
}

// ----------------------------------------------------------------------------
// SCheckBox
// ----------------------------------------------------------------------------
TSharedRef<SWidget> FSlateWidgetBuilder::BuildCheckBox(const TSharedPtr<FSlateWidgetNode>& Node)
{
	ECheckBoxState InitialState = ECheckBoxState::Unchecked;
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("IsChecked")))
	{
		if (Prop->HasArgs())
		{
			InitialState = Prop->GetFirstArg().AsBool(false) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		}
	}

	TSharedPtr<SWidget> Content = SNullWidget::NullWidget;
	if (Node->DirectChild.IsValid())
	{
		Content = BuildWidget(Node->DirectChild);
	}
	else if (Node->Slots.Num() > 0 && Node->Slots[0].ChildWidget.IsValid())
	{
		Content = BuildWidget(Node->Slots[0].ChildWidget);
	}
	else
	{
		Content = SNew(STextBlock).Text(FText::FromString(TEXT("CheckBox")));
	}

	TSharedRef<ECheckBoxState> CurrentState = MakeShared<ECheckBoxState>(InitialState);
	FString CheckBoxName = Node->VariableName.IsEmpty() ? TEXT("Option") : Node->VariableName;

	return SNew(SCheckBox)
		.IsChecked_Lambda([CurrentState]() { return *CurrentState; })
		.OnCheckStateChanged_Lambda([LocalLog = OnLogHandler, CurrentState, CheckBoxName](ECheckBoxState NewState)
		{
			*CurrentState = NewState;
			if (LocalLog.IsBound())
			{
				LocalLog.Execute(FString::Printf(TEXT("[Live Preview] '%s' is now %s"), *CheckBoxName,
					NewState == ECheckBoxState::Checked ? TEXT("Checked") : TEXT("Unchecked")));
			}
		})
		[
			Content.ToSharedRef()
		];
}

// ----------------------------------------------------------------------------
// SEditableTextBox
// ----------------------------------------------------------------------------
TSharedRef<SWidget> FSlateWidgetBuilder::BuildEditableTextBox(const TSharedPtr<FSlateWidgetNode>& Node)
{
	FText InitialText = FText::GetEmpty();
	FText HintText = FText::FromString(TEXT("Type text here..."));
	bool bIsReadOnly = false;

	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("Text")))
	{
		if (Prop->HasArgs()) InitialText = Prop->GetFirstArg().AsText(InitialText);
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("HintText")))
	{
		if (Prop->HasArgs()) HintText = Prop->GetFirstArg().AsText(HintText);
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("IsReadOnly")))
	{
		if (Prop->HasArgs()) bIsReadOnly = Prop->GetFirstArg().AsBool(false);
	}

	return SNew(SEditableTextBox)
		.Text(InitialText)
		.HintText(HintText)
		.IsReadOnly(bIsReadOnly);
}

// ----------------------------------------------------------------------------
// SMultiLineEditableTextBox
// ----------------------------------------------------------------------------
TSharedRef<SWidget> FSlateWidgetBuilder::BuildMultiLineEditableTextBox(const TSharedPtr<FSlateWidgetNode>& Node)
{
	FText InitialText = FText::GetEmpty();
	FText HintText = FText::FromString(TEXT("Multi-line text..."));
	bool bIsReadOnly = false;

	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("Text")))
	{
		if (Prop->HasArgs()) InitialText = Prop->GetFirstArg().AsText(InitialText);
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("HintText")))
	{
		if (Prop->HasArgs()) HintText = Prop->GetFirstArg().AsText(HintText);
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("IsReadOnly")))
	{
		if (Prop->HasArgs()) bIsReadOnly = Prop->GetFirstArg().AsBool(false);
	}

	return SNew(SMultiLineEditableTextBox)
		.Text(InitialText)
		.HintText(HintText)
		.IsReadOnly(bIsReadOnly);
}

// ----------------------------------------------------------------------------
// SProgressBar
// ----------------------------------------------------------------------------
TSharedRef<SWidget> FSlateWidgetBuilder::BuildProgressBar(const TSharedPtr<FSlateWidgetNode>& Node)
{
	float Percent = 0.5f;
	FLinearColor FillColor = FLinearColor(0.1f, 0.6f, 1.0f, 1.0f);

	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("Percent")))
	{
		if (Prop->HasArgs()) Percent = Prop->GetFirstArg().AsFloat(0.5f);
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("FillColorAndOpacity")))
	{
		if (Prop->HasArgs()) FillColor = Prop->GetFirstArg().AsColor(FillColor);
	}

	return SNew(SProgressBar)
		.Percent(Percent)
		.FillColorAndOpacity(FillColor);
}

// ----------------------------------------------------------------------------
// SSlider
// ----------------------------------------------------------------------------
TSharedRef<SWidget> FSlateWidgetBuilder::BuildSlider(const TSharedPtr<FSlateWidgetNode>& Node)
{
	float Val = 0.5f;
	float MinVal = 0.0f;
	float MaxVal = 1.0f;

	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("Value")))
	{
		if (Prop->HasArgs()) Val = Prop->GetFirstArg().AsFloat(0.5f);
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("MinValue")))
	{
		if (Prop->HasArgs()) MinVal = Prop->GetFirstArg().AsFloat(0.0f);
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("MaxValue")))
	{
		if (Prop->HasArgs()) MaxVal = Prop->GetFirstArg().AsFloat(1.0f);
	}

	TSharedRef<float> CurVal = MakeShared<float>(Val);

	return SNew(SSlider)
		.Value_Lambda([CurVal]() { return *CurVal; })
		.MinValue(MinVal)
		.MaxValue(MaxVal)
		.OnValueChanged_Lambda([LocalLog = OnLogHandler, CurVal](float NewVal)
		{
			*CurVal = NewVal;
			if (LocalLog.IsBound())
			{
				LocalLog.Execute(FString::Printf(TEXT("[Live Preview] Slider: %.2f"), NewVal));
			}
		});
}

// ----------------------------------------------------------------------------
// SColorBlock
// ----------------------------------------------------------------------------
TSharedRef<SWidget> FSlateWidgetBuilder::BuildColorBlock(const TSharedPtr<FSlateWidgetNode>& Node)
{
	FLinearColor Color = FLinearColor::White;
	FVector2D Size = FVector2D(24.0f, 24.0f);

	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("Color")))
	{
		if (Prop->HasArgs()) Color = Prop->GetFirstArg().AsColor(Color);
	}
	if (const FSlatePropertyNode* Prop = Node->FindProperty(TEXT("Size")))
	{
		if (Prop->Arguments.Num() >= 2)
		{
			Size = FVector2D(Prop->Arguments[0].AsFloat(24.0f), Prop->Arguments[1].AsFloat(24.0f));
		}
	}

	return SNew(SColorBlock)
		.Color(Color)
		.Size(Size);
}

// ----------------------------------------------------------------------------
// Fallback & Custom Widget Placeholder
// ----------------------------------------------------------------------------
TSharedRef<SWidget> FSlateWidgetBuilder::BuildFallbackWidget(const TSharedPtr<FSlateWidgetNode>& Node)
{
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

	// Header displaying custom widget name
	Box->AddSlot()
		.AutoHeight()
		.Padding(2.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("<%s>"), *Node->WidgetType)))
			.ColorAndOpacity(FLinearColor(0.4f, 0.8f, 1.0f, 0.8f))
			.Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
		];

	// Direct child
	if (Node->DirectChild.IsValid())
	{
		Box->AddSlot()
			.AutoHeight()
			.Padding(4.0f)
			[
				BuildWidget(Node->DirectChild)
			];
	}

	// Slots
	for (const FSlateSlotNode& Slot : Node->Slots)
	{
		if (Slot.ChildWidget.IsValid())
		{
			Box->AddSlot()
				.AutoHeight()
				.Padding(2.0f)
				[
					BuildWidget(Slot.ChildWidget)
				];
		}
	}

	return SNew(SBorder)
		.BorderBackgroundColor(FLinearColor(0.15f, 0.18f, 0.22f, 0.8f))
		.Padding(FMargin(6.0f))
		[
			Box
		];
}

// ----------------------------------------------------------------------------
// Error Widget
// ----------------------------------------------------------------------------
TSharedRef<SWidget> FSlateWidgetBuilder::BuildErrorWidget(const TArray<FString>& Errors, const TArray<FString>& Warnings)
{
	TSharedRef<SVerticalBox> Content = SNew(SVerticalBox);

	Content->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("⚠ Syntax / Parsing Diagnostics")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
			.ColorAndOpacity(FLinearColor(1.0f, 0.3f, 0.3f, 1.0f))
		];

	for (const FString& Err : Errors)
	{
		Content->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 2.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Err))
				.ColorAndOpacity(FLinearColor(1.0f, 0.4f, 0.4f, 1.0f))
				.AutoWrapText(true)
			];
	}

	for (const FString& Warn : Warnings)
	{
		Content->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 2.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("[Warning] %s"), *Warn)))
				.ColorAndOpacity(FLinearColor(1.0f, 0.8f, 0.2f, 1.0f))
				.AutoWrapText(true)
			];
	}

	return SNew(SBorder)
		.BorderBackgroundColor(FLinearColor(0.2f, 0.05f, 0.05f, 0.95f))
		.Padding(FMargin(16.0f))
		[
			Content
		];
}
