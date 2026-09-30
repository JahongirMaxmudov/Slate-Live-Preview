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
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"

void SCppAiAssistantDrawer::Construct(const FArguments& InArgs)
{
	OnInsertCodeToEditor = InArgs._OnInsertCodeToEditor;
	OnApplyCodeToEditor = InArgs._OnApplyCodeToEditor;
	OnGetEditorContext = InArgs._OnGetEditorContext;
	OnCloseRequested = InArgs._OnCloseRequested;

	// Initial welcome message
	TSharedPtr<FAiChatMessage> Welcome = MakeShared<FAiChatMessage>();
	Welcome->bIsUser = false;
	Welcome->MessageText = TEXT(
		"### Welcome to C++ Studio AI Agent!\n"
		"I am your autonomous Unreal Engine 5.8 pair programmer and Slate architect.\n\n"
		"- **Direct Code Editing**: When I generate code, click `Apply to File` to patch your active file instantly.\n"
		"- **Context Aware**: I automatically inspect your open file, selection, and Live Coding compile errors.\n"
		"- **Refactor & Fix**: Highlight code in the editor and click `Refactor Selection`, or ask me to explain and fix compile errors.\n\n"
		"How can I help you build today?"
	);
	Welcome->Timestamp = FDateTime::Now();
	ChatHistory.Add(Welcome);

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.BorderBackgroundColor(FLinearColor(0.08f, 0.085f, 0.11f, 1.0f))
		.Padding(8.0f)
		[
			SNew(SVerticalBox)

			// -----------------------------------------------------------------
			// 1. Header (Icon + Title + Provider + Context Chip + Close Button)
			// -----------------------------------------------------------------
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				SNew(SHorizontalBox)

				// Agent Icon
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SNew(SImage)
					.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.AIAssistant")))
					.DesiredSizeOverride(FVector2D(16.0f, 16.0f))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("C++ Studio AI Agent")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11.0f))
					.ColorAndOpacity(FLinearColor(0.35f, 0.75f, 1.0f, 1.0f))
				]

				// Provider & Model Pill
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
					.BorderBackgroundColor(FLinearColor(0.05f, 0.25f, 0.45f, 0.7f))
					.Padding(FMargin(6.0f, 2.0f))
					[
						SNew(STextBlock)
						.Text_Lambda([]()
						{
							const FCppEditorSettings& Settings = FCppEditorSettings::Get();
							if (Settings.AiProvider == EAiProvider::GitHubCopilot)
							{
								return FText::FromString(TEXT("Copilot (") + (Settings.AiModel.IsEmpty() ? TEXT("gpt-4o") : Settings.AiModel) + TEXT(")"));
							}
							return FText::FromString(Settings.AiModel.IsEmpty() ? TEXT("Ollama") : Settings.AiModel);
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8.5f))
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
					.ContentPadding(FMargin(4.0f))
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
			// 2. Active Context Strip (File / Selection indicator)
			// -----------------------------------------------------------------
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SAssignNew(ContextBadgeText, STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.5f))
					.ColorAndOpacity(FLinearColor(0.65f, 0.70f, 0.80f, 1.0f))
				]
			]

			// -----------------------------------------------------------------
			// 3. Quick Action Chips Bar
			// -----------------------------------------------------------------
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				SNew(SHorizontalBox)

				// Refactor Selection
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.ContentPadding(FMargin(6.0f, 3.0f))
					.ToolTipText(FText::FromString(TEXT("Ask AI to refactor the currently selected code or function")))
					.OnClicked_Lambda([this]() -> FReply
					{
						RefactorSelection();
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("⚡ Refactor")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8.5f))
						.ColorAndOpacity(FLinearColor(0.35f, 0.85f, 1.0f, 1.0f))
					]
				]

				// Fix Error
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.ContentPadding(FMargin(6.0f, 3.0f))
					.ToolTipText(FText::FromString(TEXT("Analyze compiler errors from Live Coding and generate the fix")))
					.OnClicked_Lambda([this]() -> FReply
					{
						ExplainError();
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("🛠️ Fix Error")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8.5f))
						.ColorAndOpacity(FLinearColor(1.0f, 0.65f, 0.2f, 1.0f))
					]
				]

				// Generate Slate
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.ContentPadding(FMargin(6.0f, 3.0f))
					.ToolTipText(FText::FromString(TEXT("Ask AI to generate a complete custom Slate widget")))
					.OnClicked_Lambda([this]() -> FReply
					{
						PromptSlateWidgetGeneration();
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("+ Slate Widget")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8.5f))
						.ColorAndOpacity(FLinearColor(0.3f, 0.9f, 0.5f, 1.0f))
					]
				]

				// Document Code
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.ContentPadding(FMargin(6.0f, 3.0f))
					.ToolTipText(FText::FromString(TEXT("Generate Unreal Engine Doxygen doc comments for selected code")))
					.OnClicked_Lambda([this]() -> FReply
					{
						DocumentCode();
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("📝 Document")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8.5f))
						.ColorAndOpacity(FLinearColor(0.8f, 0.7f, 1.0f, 1.0f))
					]
				]

				+ SHorizontalBox::Slot().FillWidth(1.0f) [ SNew(SSpacer) ]

				// Clear Chat
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.ContentPadding(FMargin(6.0f, 3.0f))
					.OnClicked_Lambda([this]() -> FReply
					{
						ChatHistory.Empty();
						RebuildChatMessages();
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Clear")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.5f))
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
			// 4. Scrollable Message Feed
			// -----------------------------------------------------------------
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(0.0f, 2.0f)
			[
				SAssignNew(ChatScrollBox, SScrollBox)
			]

			// Thinking Status Indicator
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.0f, 3.0f)
			[
				SAssignNew(StatusIndicatorText, STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Italic", 9.0f))
				.ColorAndOpacity(FLinearColor(0.2f, 0.75f, 1.0f, 1.0f))
				.Visibility(EVisibility::Collapsed)
			]

			// -----------------------------------------------------------------
			// 5. Multi-line Input Area
			// -----------------------------------------------------------------
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 6.0f, 0.0f, 0.0f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
				.BorderBackgroundColor(FLinearColor(0.06f, 0.065f, 0.085f, 1.0f))
				.Padding(FMargin(6.0f, 6.0f))
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(PromptInputBox, SMultiLineEditableTextBox)
						.HintText(FText::FromString(TEXT("Ask AI agent to write code, refactor selection, or debug... (Enter to send, Shift+Enter for newline)")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10.0f))
						.AutoWrapText(true)
						.OnKeyDownHandler(this, &SCppAiAssistantDrawer::HandlePromptInputKeyDown)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 6.0f, 0.0f, 0.0f)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Enter ↵ to send  ·  Shift+Enter for newline")))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.0f))
							.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.55f, 1.0f))
						]

						+ SHorizontalBox::Slot().FillWidth(1.0f) [ SNew(SSpacer) ]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SButton)
							.ButtonStyle(FAppStyle::Get(), "PrimaryButton")
							.ContentPadding(FMargin(14.0f, 4.0f))
							.OnClicked_Lambda([this]() -> FReply
							{
								SendCurrentPrompt();
								return FReply::Handled();
							})
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Send ➔")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9.0f))
							]
						]
					]
				]
			]
		]
	];

	RebuildChatMessages();
	UpdateContextBadge();
}

FReply SCppAiAssistantDrawer::HandlePromptInputKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Enter)
	{
		if (InKeyEvent.IsShiftDown())
		{
			// Allow multi-line input on Shift+Enter
			return FReply::Unhandled();
		}
		else
		{
			// Submit prompt on Enter without Shift
			SendCurrentPrompt();
			return FReply::Handled();
		}
	}

	// Arrow keys navigate within multiline text box naturally without losing widget focus
	if (Key == EKeys::Up || Key == EKeys::Down || Key == EKeys::Left || Key == EKeys::Right)
	{
		return FReply::Unhandled();
	}

	return FReply::Unhandled();
}

void SCppAiAssistantDrawer::FocusInput()
{
	UpdateContextBadge();
	if (PromptInputBox.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(PromptInputBox.ToSharedRef(), EFocusCause::SetDirectly);
	}
}

void SCppAiAssistantDrawer::UpdateContextBadge()
{
	if (!ContextBadgeText.IsValid())
	{
		return;
	}

	FAiEditorContext Context;
	if (OnGetEditorContext.IsBound())
	{
		Context = OnGetEditorContext.Execute();
	}

	if (!Context.ActiveFilePath.IsEmpty())
	{
		FString Badge;
		FString FileName = FPaths::GetCleanFilename(Context.ActiveFilePath);
		if (!Context.SelectedText.IsEmpty())
		{
			TArray<FString> SelLines;
			Context.SelectedText.ParseIntoArrayLines(SelLines, false);
			Badge = FString::Printf(TEXT("📄 %s  ·  ✂️ %d lines selected"), *FileName, FMath::Max(1, SelLines.Num()));
		}
		else
		{
			Badge = FString::Printf(TEXT("📄 %s (Line %d)"), *FileName, Context.CursorLine);
		}
		ContextBadgeText->SetText(FText::FromString(Badge));
	}
	else
	{
		ContextBadgeText->SetText(FText::FromString(TEXT("No active file open")));
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

	FAiEditorContext Context;
	if (OnGetEditorContext.IsBound())
	{
		Context = OnGetEditorContext.Execute();
	}

	bIsThinking = true;
	if (StatusIndicatorText.IsValid())
	{
		StatusIndicatorText->SetText(FText::FromString(TEXT("AI Agent is analyzing context & generating solution...")));
		StatusIndicatorText->SetVisibility(EVisibility::Visible);
	}

	FCppAiAssistant::Get().SendChatMessage(
		Prompt,
		Context,
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

void SCppAiAssistantDrawer::RefactorSelection()
{
	FAiEditorContext Context;
	if (OnGetEditorContext.IsBound())
	{
		Context = OnGetEditorContext.Execute();
	}

	FString Prompt;
	if (!Context.SelectedText.IsEmpty())
	{
		Prompt = TEXT("Refactor this selected code for optimal Unreal Engine 5.8 performance, clean architecture, and modern C++20 conventions. Provide the complete replacement code block.");
	}
	else
	{
		Prompt = TEXT("Analyze the active file and suggest improvements for Unreal Engine 5.8 conventions, safety, and performance.");
	}

	if (PromptInputBox.IsValid())
	{
		PromptInputBox->SetText(FText::FromString(Prompt));
		FocusInput();
	}
}

void SCppAiAssistantDrawer::DocumentCode()
{
	FString Prompt = TEXT("Generate clean, comprehensive Unreal Engine Doxygen doc comments (/** ... */) with @param and @return descriptions for this code.");
	if (PromptInputBox.IsValid())
	{
		PromptInputBox->SetText(FText::FromString(Prompt));
		FocusInput();
	}
}

void SCppAiAssistantDrawer::ExplainError(const FString& ErrorLine)
{
	FString Prompt;
	if (!ErrorLine.IsEmpty())
	{
		Prompt = FString::Printf(TEXT("Explain this compiler error and provide the exact C++ fix:\n%s"), *ErrorLine);
	}
	else
	{
		Prompt = TEXT("Analyze the latest Live Coding compiler error and provide the exact C++ fix ready to apply.");
	}

	if (PromptInputBox.IsValid())
	{
		PromptInputBox->SetText(FText::FromString(Prompt));
		FocusInput();
	}
}

void SCppAiAssistantDrawer::PromptSlateWidgetGeneration()
{
	if (PromptInputBox.IsValid())
	{
		PromptInputBox->SetText(FText::FromString(TEXT("Create a custom Unreal Engine SCompoundWidget with declarative syntax, styling, and event delegates for: ")));
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
			.Padding(0.0f, 4.0f)
			[
				CreateMessageWidget(Msg)
			];
	}

	ChatScrollBox->ScrollToEnd();
}

TSharedRef<SWidget> SCppAiAssistantDrawer::CreateMessageWidget(const TSharedPtr<FAiChatMessage>& Message)
{
	if (Message->bIsUser)
	{
		// User Message Bubble (Right-aligned, deep blue)
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.15f) [ SNew(SSpacer) ]
			+ SHorizontalBox::Slot()
			.FillWidth(0.85f)
			.HAlign(HAlign_Right)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
				.BorderBackgroundColor(FLinearColor(0.12f, 0.35f, 0.65f, 0.95f))
				.Padding(FMargin(12.0f, 8.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(Message->MessageText))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10.0f))
						.ColorAndOpacity(FLinearColor::White)
						.AutoWrapText(true)
					]
				]
			];
	}

	// AI Message Bubble (Left-aligned, rich markdown renderer)
	return SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
		.BorderBackgroundColor(FLinearColor(0.11f, 0.12f, 0.15f, 0.98f))
		.Padding(FMargin(12.0f, 8.0f))
		[
			SNew(SVerticalBox)

			// Message Header
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SNew(SImage)
					.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.AIAssistant")))
					.DesiredSizeOverride(FVector2D(14.0f, 14.0f))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("AI Agent")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9.0f))
					.ColorAndOpacity(FLinearColor(0.35f, 0.75f, 1.0f, 1.0f))
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f) [ SNew(SSpacer) ]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Message->Timestamp.ToString(TEXT("%H:%M"))))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.0f))
					.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.55f, 0.8f))
				]
			]

			// Message Content (Parsed Markdown)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				BuildMarkdownWidget(Message->MessageText)
			]
		];
}

TSharedRef<SWidget> SCppAiAssistantDrawer::BuildMarkdownWidget(const FString& MarkdownText)
{
	TSharedRef<SVerticalBox> Container = SNew(SVerticalBox);

	TArray<FString> Lines;
	MarkdownText.ParseIntoArrayLines(Lines, false);

	bool bInCodeBlock = false;
	FString CodeLanguage;
	FString CurrentCodeSnippet;
	FString CurrentParagraph;

	auto FlushParagraph = [&]()
	{
		if (!CurrentParagraph.IsEmpty())
		{
			Container->AddSlot()
				.AutoHeight()
				.Padding(0.0f, 2.0f, 0.0f, 4.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(CurrentParagraph.TrimStartAndEnd()))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9.5f))
					.ColorAndOpacity(FLinearColor(0.88f, 0.90f, 0.95f, 1.0f))
					.AutoWrapText(true)
				];
			CurrentParagraph.Empty();
		}
	};

	for (int32 i = 0; i < Lines.Num(); ++i)
	{
		const FString& Line = Lines[i];
		FString Trimmed = Line.TrimStartAndEnd();

		if (Trimmed.StartsWith(TEXT("```")))
		{
			if (bInCodeBlock)
			{
				// End of code block: build interactive code card
				bInCodeBlock = false;
				FlushParagraph();

				FString CapturedCode = CurrentCodeSnippet.TrimStartAndEnd();
				FString CapturedLang = CodeLanguage.IsEmpty() ? TEXT("C++") : CodeLanguage.ToUpper();

				TSharedRef<SVerticalBox> CodeCard = SNew(SVerticalBox);

				// Code Card Header Bar
				CodeCard->AddSlot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 4.0f)
					[
						SNew(SHorizontalBox)

						// Language Badge
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(CapturedLang))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8.5f))
							.ColorAndOpacity(FLinearColor(0.35f, 0.75f, 1.0f, 1.0f))
						]

						+ SHorizontalBox::Slot().FillWidth(1.0f) [ SNew(SSpacer) ]

						// "Apply to File" Button (Direct AI Agent Edit)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(4.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(SButton)
							.ButtonStyle(FAppStyle::Get(), "PrimaryButton")
							.ContentPadding(FMargin(8.0f, 2.0f))
							.ToolTipText(FText::FromString(TEXT("Apply this code directly to your active editor file (replaces selection or function, with Ctrl+Z Undo support)")))
							.OnClicked_Lambda([this, CapturedCode]() -> FReply
							{
								if (OnApplyCodeToEditor.IsBound())
								{
									OnApplyCodeToEditor.Execute(CapturedCode);

									FNotificationInfo Info(FText::FromString(TEXT("✓ AI code applied to active editor (Ctrl+Z to Undo)")));
									Info.ExpireDuration = 3.0f;
									Info.bFireAndForget = true;
									FSlateNotificationManager::Get().AddNotification(Info);
								}
								return FReply::Handled();
							})
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("⚡ Apply to File")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8.5f))
							]
						]

						// "Insert at Cursor" Button
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(4.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(SButton)
							.ButtonStyle(FAppStyle::Get(), "SimpleButton")
							.ContentPadding(FMargin(6.0f, 2.0f))
							.ToolTipText(FText::FromString(TEXT("Insert snippet at active cursor position")))
							.OnClicked_Lambda([this, CapturedCode]() -> FReply
							{
								if (OnInsertCodeToEditor.IsBound())
								{
									OnInsertCodeToEditor.Execute(CapturedCode);
								}
								return FReply::Handled();
							})
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("📋 Insert")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8.5f))
								.ColorAndOpacity(FLinearColor(0.3f, 0.85f, 0.5f, 1.0f))
							]
						]

						// "Copy" Button
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(4.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(SButton)
							.ButtonStyle(FAppStyle::Get(), "SimpleButton")
							.ContentPadding(FMargin(6.0f, 2.0f))
							.ToolTipText(FText::FromString(TEXT("Copy code to clipboard")))
							.OnClicked_Lambda([CapturedCode]() -> FReply
							{
								FPlatformApplicationMisc::ClipboardCopy(*CapturedCode);
								FNotificationInfo Info(FText::FromString(TEXT("Code copied to clipboard!")));
								Info.ExpireDuration = 2.0f;
								Info.bFireAndForget = true;
								FSlateNotificationManager::Get().AddNotification(Info);
								return FReply::Handled();
							})
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("📄 Copy")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.5f))
								.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.75f, 1.0f))
							]
						]
					];

				// Code Card Body (Syntax-highlighted read-only editor)
				CodeCard->AddSlot()
					.AutoHeight()
					[
						SNew(SMultiLineEditableTextBox)
						.Text(FText::FromString(CapturedCode))
						.Marshaller(FCppSyntaxHighlighterMarshaller::Create(FCppEditorSettings::Get().GetSyntaxStyle()))
						.Font(FCppSyntaxHighlighterMarshaller::GetEditorFont(9.5f))
						.IsReadOnly(true)
						.AutoWrapText(false)
						.BackgroundColor(FLinearColor(0.06f, 0.065f, 0.09f, 1.0f))
					];

				Container->AddSlot()
					.AutoHeight()
					.Padding(0.0f, 6.0f, 0.0f, 6.0f)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
						.BorderBackgroundColor(FLinearColor(0.07f, 0.075f, 0.10f, 1.0f))
						.Padding(8.0f)
						[
							CodeCard
						]
					];

				CurrentCodeSnippet.Empty();
			}
			else
			{
				// Start of code block
				bInCodeBlock = true;
				FlushParagraph();
				CodeLanguage = Trimmed.Mid(3).TrimStartAndEnd();
				CurrentCodeSnippet.Empty();
			}
			continue;
		}

		if (bInCodeBlock)
		{
			CurrentCodeSnippet += Line + TEXT("\n");
			continue;
		}

		// Header 1 (#)
		if (Trimmed.StartsWith(TEXT("# ")))
		{
			FlushParagraph();
			Container->AddSlot()
				.AutoHeight()
				.Padding(0.0f, 6.0f, 0.0f, 2.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Trimmed.Mid(2)))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13.0f))
					.ColorAndOpacity(FLinearColor(0.35f, 0.8f, 1.0f, 1.0f))
				];
			continue;
		}

		// Header 2 (##)
		if (Trimmed.StartsWith(TEXT("## ")))
		{
			FlushParagraph();
			Container->AddSlot()
				.AutoHeight()
				.Padding(0.0f, 5.0f, 0.0f, 2.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Trimmed.Mid(3)))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11.5f))
					.ColorAndOpacity(FLinearColor(0.5f, 0.85f, 1.0f, 1.0f))
				];
			continue;
		}

		// Header 3 (###)
		if (Trimmed.StartsWith(TEXT("### ")))
		{
			FlushParagraph();
			Container->AddSlot()
				.AutoHeight()
				.Padding(0.0f, 4.0f, 0.0f, 2.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Trimmed.Mid(4)))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10.5f))
					.ColorAndOpacity(FLinearColor(0.7f, 0.9f, 1.0f, 1.0f))
				];
			continue;
		}

		// Bullet Items (- or * or +)
		if (Trimmed.StartsWith(TEXT("- ")) || Trimmed.StartsWith(TEXT("* ")) || Trimmed.StartsWith(TEXT("+ ")))
		{
			FlushParagraph();
			Container->AddSlot()
				.AutoHeight()
				.Padding(8.0f, 1.0f, 0.0f, 2.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.0f, 0.0f, 6.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("•")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10.0f))
						.ColorAndOpacity(FLinearColor(0.35f, 0.75f, 1.0f, 1.0f))
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Trimmed.Mid(2)))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9.5f))
						.ColorAndOpacity(FLinearColor(0.85f, 0.88f, 0.92f, 1.0f))
						.AutoWrapText(true)
					]
				];
			continue;
		}

		// Empty line flushes paragraph
		if (Trimmed.IsEmpty())
		{
			FlushParagraph();
			continue;
		}

		// Normal paragraph accumulation
		if (!CurrentParagraph.IsEmpty())
		{
			CurrentParagraph += TEXT(" ");
		}
		CurrentParagraph += Line;
	}

	FlushParagraph();
	return Container;
}
