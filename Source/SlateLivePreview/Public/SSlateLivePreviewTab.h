// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "SlateFileWatcher.h"
#include "SSlateLivePreviewViewport.h"

class SMultiLineEditableTextBox;
class SEditableTextBox;
class STextBlock;

class SLATELIVEPREVIEW_API SSlateLivePreviewTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSlateLivePreviewTab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SSlateLivePreviewTab() override;

	/** Load code into editor and trigger preview update */
	void SetSourceCode(const FString& InCode);

	/** Trigger full re-evaluation */
	void RebuildPreview();

	/** Trigger screenshot export */
	void CaptureSnapshot();

private:
	TSharedPtr<SSlateLivePreviewViewport> Viewport;
	TSharedPtr<SMultiLineEditableTextBox> CodeTextBox;
	TSharedPtr<SEditableTextBox> FilePathTextBox;
	TSharedPtr<STextBlock> StatusTextBlock;
	TSharedPtr<SMultiLineEditableTextBox> ConsoleTextBox;

	TSharedPtr<FSlateFileWatcher> FileWatcher;

	bool bAutoReloadOnFileSave = true;
	bool bAutoSnapshot = true;
	FString ConsoleLogAccumulator;

	void OnCodeTextChanged(const FText& InText);
	void OnFileChanged(const FString& FilePath, const FString& FileContent);
	FReply OnBrowseFileClicked();
	FReply OnRefreshClicked();
	FReply OnSnapshotClicked();
	void OnLogReceived(const FString& Message);

	void LoadPresetTemplate(int32 TemplateIndex);
};
