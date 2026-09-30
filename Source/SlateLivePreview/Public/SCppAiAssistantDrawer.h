// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "CppAiAssistant.h"

DECLARE_DELEGATE_OneParam(FOnInsertCodeToEditor, const FString& /* CodeToInsert */);
DECLARE_DELEGATE_OneParam(FOnApplyCodeToEditor, const FString& /* CodeToApply */);
DECLARE_DELEGATE_RetVal(FAiEditorContext, FOnGetEditorContext);

struct FAiChatMessage
{
	bool bIsUser = false;
	FString MessageText;
	FDateTime Timestamp;
};

class SScrollBox;
class SMultiLineEditableTextBox;
class STextBlock;
class SVerticalBox;

class SLATELIVEPREVIEW_API SCppAiAssistantDrawer : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCppAiAssistantDrawer) {}
		SLATE_EVENT(FOnInsertCodeToEditor, OnInsertCodeToEditor)
		SLATE_EVENT(FOnApplyCodeToEditor, OnApplyCodeToEditor)
		SLATE_EVENT(FOnGetEditorContext, OnGetEditorContext)
		SLATE_EVENT(FSimpleDelegate, OnCloseRequested)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Asks the assistant to explain a compiler error */
	void ExplainError(const FString& ErrorLine = TEXT(""));

	/** Asks the assistant to refactor the current selection or file */
	void RefactorSelection();

	/** Asks the assistant to document the current function or class */
	void DocumentCode();

	/** Asks the assistant to generate a Slate widget */
	void PromptSlateWidgetGeneration();

	/** Focuses user input */
	void FocusInput();

private:
	FOnInsertCodeToEditor OnInsertCodeToEditor;
	FOnApplyCodeToEditor OnApplyCodeToEditor;
	FOnGetEditorContext OnGetEditorContext;
	FSimpleDelegate OnCloseRequested;

	TArray<TSharedPtr<FAiChatMessage>> ChatHistory;
	TSharedPtr<SScrollBox> ChatScrollBox;
	TSharedPtr<SMultiLineEditableTextBox> PromptInputBox;
	TSharedPtr<STextBlock> StatusIndicatorText;
	TSharedPtr<STextBlock> ContextBadgeText;

	bool bIsThinking = false;

	void SendCurrentPrompt();
	void AppendMessage(bool bIsUser, const FString& Content);
	void RebuildChatMessages();
	TSharedRef<SWidget> CreateMessageWidget(const TSharedPtr<FAiChatMessage>& Message);
	TSharedRef<SWidget> BuildMarkdownWidget(const FString& MarkdownText);
	FReply HandlePromptInputKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);
	void UpdateContextBadge();
};
