// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#include "SCppAiAssistantDrawer.h"
#include "CppAiAssistant.h"
#include "CppEditorSettings.h"
#include "SlateLivePreviewStyle.h"
#include "CppSyntaxHighlighter.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"

void SCppAiAssistantDrawer::Construct(const FArguments& InArgs)
{
	OnInsertCodeToEditor = InArgs._OnInsertCodeToEditor;
	OnCloseRequested = InArgs._OnCloseRequested;

	// Initial welcome message
	TSharedPtr<FAiChatMessage> Welcome = MakeShared<FAiChatMessage>();
	Welcome->bIsUser = false;
	Welcome->MessageText = TEXT(
		"Hello! I am your C++ Studio AI Assistant.\n"
		"I can write modern Slate widgets, generate boilerplate, explain compile errors, or refactor functions.\n"
		"Ask a question or click one of the quick chips above!"
	);
	Welcome->Timestamp = FDateTime::Now();
	ChatHistory.Add(Welcome);

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.BorderBackgroundColor(FLinearColor(0.08f, 0.085f, 0.10f, 1.0f))
		.Padding(8.0f)
		[
			SNew(SVerticalBox)

			// -----------------------------------------------------------------
			// 1. Header (Icon + Title + Provider + Close Button)
			// -----------------------------------------------------------------
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				SNew(SHorizontalBox)

				// Title
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SNew(SImage)
					.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.Settings")))
					.DesiredSizeOverride(FVector2D(16.0f, 16.0f))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("C++ Studio AI Assistant")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10.5f))
					.ColorAndOpacity(FLinearColor(0.35f, 0.75f, 1.0f, 1.0f))
				]

				// Provider Pill
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
					.BorderBackgroundColor(FLinearColor(0.05f, 0.25f, 0.45f, 0.6f))
					.Padding(FMargin(5.0f, 1.0f))
					[
						SNew(STextBlock)
						.Text_Lambda([]()
						{
							const FCppEditorSettings& Settings = FCppEditorSettings::Get();
							return FText::FromString(Settings.AiModel.IsEmpty() ? TEXT("Ollama") : Settings.AiModel);
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						.ColorAndOpacity(FLinearColor(0.6f, 0.85f, 1.0f, 1.0f))
					]
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SSpacer)
				]

				// Close Button
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.ContentPadding(FMargin(2.0f))
					.ToolTipText(FText::FromString(TEXT("Close AI Assistant")))
					.OnClicked_Lambda([this]() -> FReply
					{
						OnCloseRequested.ExecuteIfBound();
						return FReply::Handled();
					})
					[
						SNew(SImage)
						.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.CloseTab")))
						.DesiredSizeOverride(FVector2D(12.0f, 12.0f))
					]
				]
			]

			// -----------------------------------------------------------------
			// 2. Quick Action Chips
			// -----------------------------------------------------------------
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				SNew(SHorizontalBox)

				// Generate Slate
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.ContentPadding(FMargin(6.0f, 2.0f))
					.OnClicked_Lambda([this]() -> FReply
					{
						PromptSlateWidgetGeneration();
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("+ New Slate Widget")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8.0f))
						.ColorAndOpacity(FLinearColor(0.2f, 0.8f, 0.4f, 1.0f))
					]
				]

				// Explain Error
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.ContentPadding(FMargin(6.0f, 2.0f))
					.OnClicked_Lambda([this]() -> FReply
					{
						ExplainError(TEXT(""));
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Explain Error")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8.0f))
						.ColorAndOpacity(FLinearColor(1.0f, 0.6f, 0.2f, 1.0f))
					]
				]

				// Clear Chat
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.ContentPadding(FMargin(6.0f, 2.0f))
					.OnClicked_Lambda([this]() -> FReply
					{
						ChatHistory.Empty();
						RebuildChatMessages();
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Clear Chat")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.0f))
						.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f))
					]
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 4.0f)
			[
				SNew(SSeparator)
			]

			// -----------------------------------------------------------------
			// 3. Scrollable Message Feed
			// -----------------------------------------------------------------
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(0.0f, 2.0f)
			[
				SAssignNew(ChatScrollBox, SScrollBox)
			]

			// Status Indicator (Thinking...)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 2.0f)
			[
				SAssignNew(StatusIndicatorText, STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Italic", 8.5f))
				.ColorAndOpacity(FLinearColor(0.2f, 0.7f, 1.0f, 1.0f))
				.Visibility(EVisibility::Collapsed)
			]

			// -----------------------------------------------------------------
			// 4. Input Area
			// -----------------------------------------------------------------
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					SAssignNew(PromptInputBox, SEditableTextBox)
					.HintText(FText::FromString(TEXT("Ask AI to write Slate code, explain syntax, or debug... (Press Enter)")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type CommitType)
					{
						if (CommitType == ETextCommit::OnEnter)
						{
							SendCurrentPrompt();
						}
					})
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "PrimaryButton")
					.ContentPadding(FMargin(10.0f, 3.0f))
					.Text(FText::FromString(TEXT("Send")))
					.OnClicked_Lambda([this]() -> FReply
					{
						SendCurrentPrompt();
						return FReply::Handled();
					})
				]
			]
		]
	];

	RebuildChatMessages();
}

void SCppAiAssistantDrawer::FocusInput()
{
	if (PromptInputBox.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(PromptInputBox, EFocusCause::SetDirectly);
	}
}

void SCppAiAssistantDrawer::SendCurrentPrompt()
{
	if (!PromptInputBox.IsValid() || bIsThinking)
	{
		return;
	}

	FString Prompt = PromptInputBox->GetText().ToString().TrimStartAndEnd();
	if (Prompt.IsEmpty())
	{
		return;
	}

	PromptInputBox->SetText(FText::GetEmpty());

	AppendMessage(true, Prompt);

	bIsThinking = true;
	if (StatusIndicatorText.IsValid())
	{
		StatusIndicatorText->SetText(FText::FromString(TEXT("AI is thinking & generating code...")));
		StatusIndicatorText->SetVisibility(EVisibility::Visible);
	}

	FCppAiAssistant::Get().SendChatMessage(
		Prompt,
		TEXT(""),
		TEXT(""),
		FOnAiChatReceived::CreateLambda([this](const FString& Response, bool bSuccess)
		{
			bIsThinking = false;
			if (StatusIndicatorText.IsValid())
			{
				StatusIndicatorText->SetVisibility(EVisibility::Collapsed);
			}

			AppendMessage(false, Response);
		})
	);
}

void SCppAiAssistantDrawer::ExplainError(const FString& ErrorLine)
{
	FString Prompt = ErrorLine.IsEmpty()
		? TEXT("Explain the latest compiler error and suggest the exact C++ fix.")
		: FString::Printf(TEXT("Explain this compiler error and how to fix it:\n%s"), *ErrorLine);

	AppendMessage(true, Prompt);

	bIsThinking = true;
	if (StatusIndicatorText.IsValid())
	{
		StatusIndicatorText->SetText(FText::FromString(TEXT("Analyzing compiler error...")));
		StatusIndicatorText->SetVisibility(EVisibility::Visible);
	}

	FCppAiAssistant::Get().SendChatMessage(
		Prompt,
		TEXT(""),
		ErrorLine,
		FOnAiChatReceived::CreateLambda([this](const FString& Response, bool bSuccess)
		{
			bIsThinking = false;
			if (StatusIndicatorText.IsValid())
			{
				StatusIndicatorText->SetVisibility(EVisibility::Collapsed);
			}

			AppendMessage(false, Response);
		})
	);
}

void SCppAiAssistantDrawer::PromptSlateWidgetGeneration()
{
	if (PromptInputBox.IsValid())
	{
		PromptInputBox->SetText(FText::FromString(TEXT("Create a custom Slate compound widget for ")));
		FocusInput();
	}
}

void SCppAiAssistantDrawer::AppendMessage(bool bIsUser, const FString& Content)
{
	TSharedPtr<FAiChatMessage> Msg = MakeShared<FAiChatMessage>();
	Msg->bIsUser = bIsUser;
	Msg->MessageText = Content;
	Msg->Timestamp = FDateTime::Now();
	ChatHistory.Add(Msg);

	RebuildChatMessages();
}

void SCppAiAssistantDrawer::RebuildChatMessages()
{
	if (!ChatScrollBox.IsValid())
	{
		return;
	}

	ChatScrollBox->ClearChildren();

	for (const TSharedPtr<FAiChatMessage>& Msg : ChatHistory)
	{
		ChatScrollBox->AddSlot()
			.Padding(0.0f, 3.0f)
			[
				CreateMessageWidget(Msg)
			];
	}

	ChatScrollBox->ScrollToEnd();
}

FString SCppAiAssistantDrawer::ExtractCodeBlock(const FString& FullText) const
{
	int32 StartIdx = FullText.Find(TEXT("```"));
	if (StartIdx == INDEX_NONE)
	{
		return FString();
	}

	int32 LineEnd = FullText.Find(TEXT("\n"), ESearchCase::IgnoreCase, ESearchDir::FromStart, StartIdx);
	if (LineEnd == INDEX_NONE)
	{
		return FString();
	}

	int32 EndIdx = FullText.Find(TEXT("```"), ESearchCase::IgnoreCase, ESearchDir::FromStart, LineEnd);
	if (EndIdx == INDEX_NONE)
	{
		EndIdx = FullText.Len();
	}

	return FullText.Mid(LineEnd + 1, EndIdx - LineEnd - 1).TrimStartAndEnd();
}

TSharedRef<SWidget> SCppAiAssistantDrawer::CreateMessageWidget(const TSharedPtr<FAiChatMessage>& Message)
{
	if (Message->bIsUser)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f) [ SNew(SSpacer) ]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				.BorderBackgroundColor(FLinearColor(0.08f, 0.32f, 0.65f, 0.9f))
				.Padding(FMargin(10.0f, 6.0f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(Message->MessageText))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FLinearColor::White)
					.AutoWrapText(true)
				]
			];
	}

	// AI Message
	FString CodeSnippet = ExtractCodeBlock(Message->MessageText);

	TSharedPtr<SVerticalBox> MsgBox = SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Message->MessageText))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			.ColorAndOpacity(FLinearColor(0.9f, 0.9f, 0.9f, 1.0f))
			.AutoWrapText(true)
		];

	if (!CodeSnippet.IsEmpty())
	{
		MsgBox->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
				.BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.07f, 1.0f))
				.Padding(6.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 4.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("C++ Snippet")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
							.ColorAndOpacity(FLinearColor(0.35f, 0.75f, 1.0f, 1.0f))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f) [ SNew(SSpacer) ]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.ButtonStyle(FAppStyle::Get(), "SimpleButton")
							.ContentPadding(FMargin(4.0f, 1.0f))
							.ToolTipText(FText::FromString(TEXT("Insert this code block into active editor at cursor")))
							.OnClicked_Lambda([this, CodeSnippet]() -> FReply
							{
								OnInsertCodeToEditor.ExecuteIfBound(CodeSnippet);
								return FReply::Handled();
							})
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("-> Insert at Cursor")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8.0f))
								.ColorAndOpacity(FLinearColor(0.2f, 0.85f, 0.45f, 1.0f))
							]
						]
					]
				]
			];
	}

	return SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
		.BorderBackgroundColor(FLinearColor(0.12f, 0.12f, 0.15f, 0.95f))
		.Padding(FMargin(10.0f, 6.0f))
		[
			MsgBox.ToSharedRef()
		];
}
