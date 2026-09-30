// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "CppAiAssistant.h"

DECLARE_DELEGATE_OneParam(FOnInsertCodeToEditor, const FString& /* CodeToInsert */);
DECLARE_DELEGATE_TwoParams(FOnApplyCodeToEditor, const FString& /* CodeToApply */, const FString& /* TargetFileName */);
DECLARE_DELEGATE_RetVal(FAiEditorContext, FOnGetEditorContext);

enum class EAiPermissionMode : uint8
{
	AskBeforeApply = 0,
	AutoApply
};

struct FAiChatMessage
{
	bool bIsUser = false;
	FString MessageText;
	FDateTime Timestamp;
};

struct FAiChatSession
{
	FGuid SessionId;
	FString Title;
	TArray<TSharedPtr<FAiChatMessage>> Messages;
	FDateTime CreatedAt;
};

template<typename OptionType>
class SComboBox;
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

	/** Starts a new chat session */
	void StartNewSession();

	/** Switches to a chat session by index */
	void SwitchSession(int32 NewIndex);

	/** Gets current permission mode */
	EAiPermissionMode GetPermissionMode() const { return PermissionMode; }
	void SetPermissionMode(EAiPermissionMode NewMode);

private:
	FOnInsertCodeToEditor OnInsertCodeToEditor;
	FOnApplyCodeToEditor OnApplyCodeToEditor;
	FOnGetEditorContext OnGetEditorContext;
	FSimpleDelegate OnCloseRequested;

	// Chat Sessions
	TArray<TSharedPtr<FAiChatSession>> Sessions;
	int32 ActiveSessionIndex = 0;
	EAiPermissionMode PermissionMode = EAiPermissionMode::AskBeforeApply;

	// UI Controls
	TSharedPtr<SScrollBox> ChatScrollBox;
	TSharedPtr<SMultiLineEditableTextBox> PromptInputBox;
	TSharedPtr<STextBlock> StatusIndicatorText;
	TSharedPtr<STextBlock> ContextBadgeText;
	TSharedPtr<STextBlock> PermissionButtonText;
	TSharedPtr<STextBlock> QuotaIndicatorText;

	TSharedPtr<SComboBox<TSharedPtr<FString>>> SessionComboBox;
	TArray<TSharedPtr<FString>> SessionTitleOptions;

	TSharedPtr<SComboBox<TSharedPtr<FString>>> ModelComboBox;
	TArray<TSharedPtr<FString>> ModelOptions;

	bool bIsThinking = false;
	int32 SessionRequestCount = 0;

	void SendCurrentPrompt();
	void AppendMessage(bool bIsUser, const FString& Content);
	void RebuildChatMessages();
	void RefreshSessionOptions();
	void AutoApplyCodeBlocksFromMessage(const FString& MessageText);

	TArray<TSharedPtr<FAiChatMessage>>& GetActiveMessages();
	const TArray<TSharedPtr<FAiChatMessage>>& GetActiveMessages() const;

	TSharedRef<SWidget> CreateMessageWidget(const TSharedPtr<FAiChatMessage>& Message);
	TSharedRef<SWidget> BuildMarkdownWidget(const FString& MarkdownText);
	FReply HandlePromptInputKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);
	void UpdateContextBadge();
	void UpdateQuotaDisplay();
};
