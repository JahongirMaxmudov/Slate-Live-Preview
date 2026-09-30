// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

enum class ESlatePreviewBackground : uint8
{
	SlateDark,
	Checkerboard,
	Light,
	Black
};

enum class ESlatePreviewSizePreset : uint8
{
	Auto,
	Fixed400x300,
	Fixed800x600,
	Mobile390x844
};

class SLATELIVEPREVIEW_API SSlateLivePreviewViewport : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSlateLivePreviewViewport) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetPreviewWidget(const TSharedRef<SWidget>& InWidget);
	void SetBackgroundMode(ESlatePreviewBackground InMode);
	void SetSizePreset(ESlatePreviewSizePreset InPreset);

	TSharedPtr<SWidget> GetPreviewWidget() const { return CurrentWidget; }

private:
	TSharedPtr<SBorder> ViewportBackgroundBorder;
	TSharedPtr<class SBox> ConstraintBox;
	TSharedPtr<SWidget> CurrentWidget;

	ESlatePreviewBackground CurrentBgMode = ESlatePreviewBackground::SlateDark;
	ESlatePreviewSizePreset CurrentSizePreset = ESlatePreviewSizePreset::Auto;

	void UpdateBackgroundStyle();
	void UpdateSizeConstraint();
};
