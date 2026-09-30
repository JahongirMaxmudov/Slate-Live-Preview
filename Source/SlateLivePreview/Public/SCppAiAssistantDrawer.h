// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

DECLARE_DELEGATE_OneParam(FOnInsertCodeToEditor, const FString& /* CodeToInsert */);

struct FAiChatMessage
{
	bool bIsUser = false;
	FString MessageText;
	FDateTime Timestamp;
};

class SScrollBox;
class SEditableTextBox;
class STextBlock;
class SVerticalBox;

class SLATELIVEPREVIEW_API SCppAiAssistantDrawer : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCppAiAssistantDrawer) {}
		SLATE_EVENT(FOnInsertCodeToEditor, OnInsertCodeToEditor)
		SLATE_EVENT(FSimpleDelegate, OnCloseRequested)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Asks the assistant to explain a compiler error */
	void ExplainError(const FString& ErrorLine);

	/** Asks the assistant to generate a Slate widget */
	void PromptSlateWidgetGeneration();

	/** Focuses user input */
	void FocusInput();

private:
	FOnInsertCodeToEditor OnInsertCodeToEditor;
	FSimpleDelegate OnCloseRequested;

	TArray<TSharedPtr<FAiChatMessage>> ChatHistory;
	TSharedPtr<SScrollBox> ChatScrollBox;
	TSharedPtr<SEditableTextBox> PromptInputBox;
	TSharedPtr<STextBlock> StatusIndicatorText;

	bool bIsThinking = false;

	void SendCurrentPrompt();
	void AppendMessage(bool bIsUser, const FString& Content);
	void RebuildChatMessages();
	TSharedRef<SWidget> CreateMessageWidget(const TSharedPtr<FAiChatMessage>& Message);
	FString ExtractCodeBlock(const FString& FullText) const;
};
