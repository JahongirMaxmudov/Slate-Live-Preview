// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SlateAst.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

DECLARE_DELEGATE_OneParam(FOnSlatePreviewLog, const FString& /* LogMessage */);

class SLATELIVEPREVIEW_API FSlateWidgetBuilder
{
public:
	FSlateWidgetBuilder();

	void SetLogHandler(const FOnSlatePreviewLog& InHandler) { OnLogHandler = InHandler; }

	/** Converts parsed AST into real native Slate widget hierarchy */
	TSharedRef<SWidget> Build(const TSharedPtr<FSlateWidgetNode>& RootNode);

	/** Helper to build a styled error widget */
	static TSharedRef<SWidget> BuildErrorWidget(const TArray<FString>& Errors, const TArray<FString>& Warnings);

private:
	FOnSlatePreviewLog OnLogHandler;

	TSharedRef<SWidget> BuildWidget(const TSharedPtr<FSlateWidgetNode>& Node);

	// Dedicated builders for supported widget classes
	TSharedRef<SWidget> BuildBorder(const TSharedPtr<FSlateWidgetNode>& Node);
	TSharedRef<SWidget> BuildVerticalBox(const TSharedPtr<FSlateWidgetNode>& Node);
	TSharedRef<SWidget> BuildHorizontalBox(const TSharedPtr<FSlateWidgetNode>& Node);
	TSharedRef<SWidget> BuildOverlay(const TSharedPtr<FSlateWidgetNode>& Node);
	TSharedRef<SWidget> BuildBox(const TSharedPtr<FSlateWidgetNode>& Node);
	TSharedRef<SWidget> BuildSpacer(const TSharedPtr<FSlateWidgetNode>& Node);
	TSharedRef<SWidget> BuildScrollBox(const TSharedPtr<FSlateWidgetNode>& Node);
	TSharedRef<SWidget> BuildSeparator(const TSharedPtr<FSlateWidgetNode>& Node);
	TSharedRef<SWidget> BuildTextBlock(const TSharedPtr<FSlateWidgetNode>& Node);
	TSharedRef<SWidget> BuildButton(const TSharedPtr<FSlateWidgetNode>& Node);
	TSharedRef<SWidget> BuildImage(const TSharedPtr<FSlateWidgetNode>& Node);
	TSharedRef<SWidget> BuildCheckBox(const TSharedPtr<FSlateWidgetNode>& Node);
	TSharedRef<SWidget> BuildEditableTextBox(const TSharedPtr<FSlateWidgetNode>& Node);
	TSharedRef<SWidget> BuildMultiLineEditableTextBox(const TSharedPtr<FSlateWidgetNode>& Node);
	TSharedRef<SWidget> BuildProgressBar(const TSharedPtr<FSlateWidgetNode>& Node);
	TSharedRef<SWidget> BuildSlider(const TSharedPtr<FSlateWidgetNode>& Node);
	TSharedRef<SWidget> BuildColorBlock(const TSharedPtr<FSlateWidgetNode>& Node);
	TSharedRef<SWidget> BuildFallbackWidget(const TSharedPtr<FSlateWidgetNode>& Node);

	void Log(const FString& Message);
};
