// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "CppEditorSettings.h"

class SWindow;
class SMultiLineEditableTextBox;
class FCppSyntaxHighlighterMarshaller;

/**
 * Settings and Preferences Dialog for C++ Studio & Slate Live Preview
 */
class SLATELIVEPREVIEW_API SCppSettingsDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCppSettingsDialog)
		: _ParentWindow()
	{}
		SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	static void OpenModal(TSharedPtr<SWidget> ParentWidget);

private:
	TWeakPtr<SWindow> ParentWindow;

	// Working copies of settings
	ECppEditorTheme TempTheme;
	ECppEditorFont TempFontFamily;
	FString TempCustomFontPath;
	int32 TempFontSize;
	ECppTabSize TempTabSize;
	bool bTempAutoIndent;
	bool bTempAutoCloseBrackets;
	bool bTempShowLineNumbers;
	bool bTempEnableIntelliSense;
	bool bTempWordWrap;
	bool bTempHighlightActiveLine;
	bool bTempAutoSaveOnLiveCoding;
	bool bTempAutoReloadSlatePreview;

	// Dropdown item containers
	TArray<TSharedPtr<FString>> ThemeOptions;
	TArray<TSharedPtr<FString>> FontOptions;
	TArray<TSharedPtr<FString>> FontSizeOptions;

	TSharedPtr<FString> SelectedThemeOption;
	TSharedPtr<FString> SelectedFontOption;
	TSharedPtr<FString> SelectedFontSizeOption;

	// Preview Box
	TSharedPtr<SMultiLineEditableTextBox> PreviewTextBox;
	TSharedPtr<FCppSyntaxHighlighterMarshaller> PreviewMarshaller;

	// AI Copilot Settings
	bool bTempEnableAiInlineCompletion;
	EAiProvider TempAiProvider;
	FString TempAiEndpoint;
	FString TempAiModel;
	FString TempAiApiKey;
	int32 TempAiGhostTextDelayMs;

	TArray<TSharedPtr<FString>> AiProviderOptions;
	TSharedPtr<FString> SelectedAiProviderOption;
	TSharedPtr<STextBlock> AiTestStatusTextBlock;

	void UpdatePreview();
	void UpdateAiProviderDefaults();
	FReply OnTestAiConnectionClicked();
	FReply OnApplyClicked();
	FReply OnResetClicked();
	FReply OnCancelClicked();
	FReply OnBrowseFontClicked();
};
