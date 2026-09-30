// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#include "SSlateLivePreviewViewport.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "Styling/AppStyle.h"

void SSlateLivePreviewViewport::Construct(const FArguments& InArgs)
{
	ConstraintBox = SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center);

	ViewportBackgroundBorder = SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.06f, 0.06f, 0.06f, 1.0f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				ConstraintBox.ToSharedRef()
			]
		];

	ChildSlot
	[
		ViewportBackgroundBorder.ToSharedRef()
	];

	UpdateBackgroundStyle();
	UpdateSizeConstraint();
}

void SSlateLivePreviewViewport::SetPreviewWidget(const TSharedRef<SWidget>& InWidget)
{
	CurrentWidget = InWidget;
	if (ConstraintBox.IsValid())
	{
		ConstraintBox->SetContent(InWidget);
	}
}

void SSlateLivePreviewViewport::SetBackgroundMode(ESlatePreviewBackground InMode)
{
	CurrentBgMode = InMode;
	UpdateBackgroundStyle();
}

void SSlateLivePreviewViewport::SetSizePreset(ESlatePreviewSizePreset InPreset)
{
	CurrentSizePreset = InPreset;
	UpdateSizeConstraint();
}

void SSlateLivePreviewViewport::UpdateBackgroundStyle()
{
	if (!ViewportBackgroundBorder.IsValid()) return;

	switch (CurrentBgMode)
	{
	case ESlatePreviewBackground::SlateDark:
		ViewportBackgroundBorder->SetBorderBackgroundColor(FLinearColor(0.06f, 0.06f, 0.06f, 1.0f));
		break;
	case ESlatePreviewBackground::Checkerboard:
		ViewportBackgroundBorder->SetBorderBackgroundColor(FLinearColor(0.12f, 0.12f, 0.12f, 1.0f));
		break;
	case ESlatePreviewBackground::Light:
		ViewportBackgroundBorder->SetBorderBackgroundColor(FLinearColor(0.85f, 0.85f, 0.85f, 1.0f));
		break;
	case ESlatePreviewBackground::Black:
		ViewportBackgroundBorder->SetBorderBackgroundColor(FLinearColor::Black);
		break;
	}
}

void SSlateLivePreviewViewport::UpdateSizeConstraint()
{
	if (!ConstraintBox.IsValid()) return;

	switch (CurrentSizePreset)
	{
	case ESlatePreviewSizePreset::Auto:
		ConstraintBox->SetWidthOverride(FOptionalSize());
		ConstraintBox->SetHeightOverride(FOptionalSize());
		break;
	case ESlatePreviewSizePreset::Fixed400x300:
		ConstraintBox->SetWidthOverride(FOptionalSize(400.0f));
		ConstraintBox->SetHeightOverride(FOptionalSize(300.0f));
		break;
	case ESlatePreviewSizePreset::Fixed800x600:
		ConstraintBox->SetWidthOverride(FOptionalSize(800.0f));
		ConstraintBox->SetHeightOverride(FOptionalSize(600.0f));
		break;
	case ESlatePreviewSizePreset::Mobile390x844:
		ConstraintBox->SetWidthOverride(FOptionalSize(390.0f));
		ConstraintBox->SetHeightOverride(FOptionalSize(844.0f));
		break;
	}
}
