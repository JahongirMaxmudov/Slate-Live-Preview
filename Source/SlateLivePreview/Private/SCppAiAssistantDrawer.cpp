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
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Internationalization/Regex.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"

static FString ExtractFileNameFromString(const FString& Text)
{
	static const FRegexPattern FilePattern(TEXT("([A-Za-z0-9_]+\\.(?:h|hpp|cpp|inl|cs))"));
	FRegexMatcher Matcher(FilePattern, Text);
	FString BestMatch;
	while (Matcher.FindNext())
	{
		FString Candidate = Matcher.GetCaptureGroup(1);
		if (!Candidate.EndsWith(TEXT(".generated.h"), ESearchCase::IgnoreCase))
		{
			BestMatch = Candidate;
		}
	}
	return BestMatch;
}

static FString DetectTargetFile(const TArray<FString>& PrecedingLines, const FString& CodeSnippet, const FString& ActiveFilePath)
{
	// 1. Scan preceding markdown lines backwards (closest line first)
	for (int32 Idx = PrecedingLines.Num() - 1; Idx >= FMath::Max(0, PrecedingLines.Num() - 8); --Idx)
	{
		FString Found = ExtractFileNameFromString(PrecedingLines[Idx]);
		if (!Found.IsEmpty())
		{
			return Found;
		}
	}

	// 2. Scan snippet top comments
	TArray<FString> SnippetLines;
	CodeSnippet.ParseIntoArrayLines(SnippetLines, false);
	for (int32 Idx = 0; Idx < FMath::Min(SnippetLines.Num(), 5); ++Idx)
	{
		if (SnippetLines[Idx].TrimStart().StartsWith(TEXT("//")))
		{
			FString Found = ExtractFileNameFromString(SnippetLines[Idx]);
			if (!Found.IsEmpty())
			{
				return Found;
			}
		}
	}

	// 3. Heuristic: generated header in snippet -> Header file
	static const FRegexPattern GenHeaderPattern(TEXT("#include\\s+[\"<]([A-Za-z0-9_]+)\\.generated\\.h[\">]"));
	FRegexMatcher GenMatcher(GenHeaderPattern, CodeSnippet);
	if (GenMatcher.FindNext())
	{
		return GenMatcher.GetCaptureGroup(1) + TEXT(".h");
	}

	// 4. Heuristic: #pragma once or class/struct declaration -> Header file
	const bool bHasPragmaOnce = CodeSnippet.Contains(TEXT("#pragma once"));
	const bool bHasUClass = CodeSnippet.Contains(TEXT("UCLASS(")) || CodeSnippet.Contains(TEXT("GENERATED_BODY()"));
	const bool bHasMethodImpl = CodeSnippet.Contains(TEXT("::")) && CodeSnippet.Contains(TEXT("{"));

	FString ActiveBase = FPaths::GetBaseFilename(ActiveFilePath);
	FString ActiveClean = FPaths::GetCleanFilename(ActiveFilePath);

	if (bHasPragmaOnce || bHasUClass)
	{
		if (!ActiveBase.IsEmpty())
		{
			return ActiveBase + TEXT(".h");
		}
		return TEXT("Header.h");
	}

	// 5. Heuristic: Method implementations (ClassName::Method) -> Source file
	if (bHasMethodImpl)
	{
		static const FRegexPattern MethodPattern(TEXT("(?:void|bool|int32|float|double|auto|[A-Za-z0-9_]+[*&]?)\\s+([A-Za-z0-9_]+)::"));
		FRegexMatcher MethodMatcher(MethodPattern, CodeSnippet);
		if (MethodMatcher.FindNext())
		{
			FString ClassName = MethodMatcher.GetCaptureGroup(1);
			if (ClassName.StartsWith(TEXT("A")) || ClassName.StartsWith(TEXT("U")) || ClassName.StartsWith(TEXT("F")) || ClassName.StartsWith(TEXT("S")))
			{
				FString Base = ClassName.Mid(1);
				return Base + TEXT(".cpp");
			}
			return ClassName + TEXT(".cpp");
		}

		if (!ActiveBase.IsEmpty())
		{
			return ActiveBase + TEXT(".cpp");
		}
		return TEXT("Source.cpp");
	}

	// 6. Default to active editor file if known
	if (!ActiveClean.IsEmpty())
	{
		return ActiveClean;
	}

	return FString();
}

void SCppAiAssistantDrawer::Construct(const FArguments& InArgs)
{
	OnInsertCodeToEditor = InArgs._OnInsertCodeToEditor;
	OnApplyCodeToEditor = InArgs._OnApplyCodeToEditor;
	OnGetEditorContext = InArgs._OnGetEditorContext;
	OnCloseRequested = InArgs._OnCloseRequested;

	// Populate model options
	ModelOptions.Empty();
	ModelOptions.Add(MakeShared<FString>(TEXT("gpt-4o")));
	ModelOptions.Add(MakeShared<FString>(TEXT("claude-3.5-sonnet")));
	ModelOptions.Add(MakeShared<FString>(TEXT("o1-preview")));
	ModelOptions.Add(MakeShared<FString>(TEXT("o1-mini")));
	ModelOptions.Add(MakeShared<FString>(TEXT("gpt-4o-mini")));
	ModelOptions.Add(MakeShared<FString>(TEXT("deepseek-coder")));

	// Create initial session
	StartNewSession();

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.BorderBackgroundColor(FLinearColor(0.08f, 0.085f, 0.11f, 1.0f))
		.Padding(8.0f)
		[
			SNew(SVerticalBox)

			// -----------------------------------------------------------------
			// 1. Header (Icon + Title + Provider/Model + Close Button)
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

				// Model Selector Dropdown
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SAssignNew(ModelComboBox, SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&ModelOptions)
					.OnGenerateWidget_Lambda([](TSharedPtr<FString> Option)
					{
						return SNew(STextBlock)
							.Text(FText::FromString(*Option))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.5f));
					})
					.OnSelectionChanged_Lambda([](TSharedPtr<FString> Selected, ESelectInfo::Type)
					{
						if (Selected.IsValid())
						{
							FCppEditorSettings& Settings = FCppEditorSettings::Get();
							Settings.AiModel = *Selected;
							Settings.Save();
						}
					})
					[
						SNew(STextBlock)
						.Text_Lambda([]()
						{
							const FCppEditorSettings& Settings = FCppEditorSettings::Get();
							return FText::FromString(Settings.AiModel.IsEmpty() ? TEXT("gpt-4o") : Settings.AiModel);
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8.5f))
						.ColorAndOpacity(FLinearColor(0.6f, 0.85f, 1.0f, 1.0f))
					]
				]

				// Quota / Limit Badge
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(6.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
					.BorderBackgroundColor(FLinearColor(0.04f, 0.20f, 0.12f, 0.85f))
					.Padding(FMargin(5.0f, 2.0f))
					.ToolTipText_Lambda([this]()
					{
						const FCppEditorSettings& Settings = FCppEditorSettings::Get();
						return FText::FromString(FString::Printf(
							TEXT("Provider: %s\nModel: %s\nSession Requests: %d\nQuota Limits: Unlimited (Copilot Subscription Active)"),
							*FCppEditorSettings::GetAiProviderDisplayName(Settings.AiProvider),
							*Settings.AiModel,
							SessionRequestCount
						));
					})
					[
						SAssignNew(QuotaIndicatorText, STextBlock)
						.Text(FText::FromString(TEXT("● Quota OK")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8.0f))
						.ColorAndOpacity(FLinearColor(0.25f, 0.90f, 0.50f, 1.0f))
					]
				]

				+ SHorizontalBox::Slot().FillWidth(1.0f) [ SNew(SSpacer) ]

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
			// 2. Chat Session Switcher & Autonomous Permission Bar
			// -----------------------------------------------------------------
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				SNew(SHorizontalBox)

				// "+ New Chat" Button
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "PrimaryButton")
					.ContentPadding(FMargin(6.0f, 2.0f))
					.ToolTipText(FText::FromString(TEXT("Start a fresh chat conversation")))
					.OnClicked_Lambda([this]() -> FReply
					{
						StartNewSession();
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("+ New Chat")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8.5f))
					]
				]

				// Session History Dropdown
				+ SHorizontalBox::Slot()
				.FillWidth(0.55f)
				.VAlign(VAlign_Center)
				.Padding(2.0f, 0.0f)
				[
					SAssignNew(SessionComboBox, SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&SessionTitleOptions)
					.OnGenerateWidget_Lambda([](TSharedPtr<FString> Option)
					{
						return SNew(STextBlock)
							.Text(FText::FromString(*Option))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.5f));
					})
					.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Selected, ESelectInfo::Type)
					{
						if (Selected.IsValid())
						{
							for (int32 i = 0; i < SessionTitleOptions.Num(); ++i)
							{
								if (SessionTitleOptions[i] == Selected)
								{
									SwitchSession(i);
									break;
								}
							}
						}
					})
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							if (Sessions.IsValidIndex(ActiveSessionIndex))
							{
								return FText::FromString(Sessions[ActiveSessionIndex]->Title);
							}
							return FText::FromString(TEXT("Chat History"));
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.5f))
						.ColorAndOpacity(FLinearColor(0.8f, 0.85f, 0.95f, 1.0f))
					]
				]

				// Permission Mode Toggle Button
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.ContentPadding(FMargin(6.0f, 2.0f))
					.ToolTipText(FText::FromString(TEXT("Click to toggle between 'Ask Before Apply' and 'Auto-Apply (Full Freedom Autonomous Agent)'")))
					.OnClicked_Lambda([this]() -> FReply
					{
						SetPermissionMode(PermissionMode == EAiPermissionMode::AskBeforeApply ? EAiPermissionMode::AutoApply : EAiPermissionMode::AskBeforeApply);
						return FReply::Handled();
					})
					[
						SAssignNew(PermissionButtonText, STextBlock)
						.Text(FText::FromString(TEXT("🛡️ Ask before apply ▾")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8.0f))
						.ColorAndOpacity(FLinearColor(0.85f, 0.85f, 0.85f, 1.0f))
					]
				]
			]

			// -----------------------------------------------------------------
			// 3. Active Context Strip (File / Selection indicator)
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
			// 4. Quick Action Chips Bar
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

				// Clear Current Chat
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.ContentPadding(FMargin(6.0f, 3.0f))
					.OnClicked_Lambda([this]() -> FReply
					{
						GetActiveMessages().Empty();
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
			// 5. Scrollable Message Feed
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
			// 6. Multi-line Input Area
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

void SCppAiAssistantDrawer::StartNewSession()
{
	TSharedPtr<FAiChatSession> NewSession = MakeShared<FAiChatSession>();
	NewSession->SessionId = FGuid::NewGuid();
	NewSession->Title = FString::Printf(TEXT("Chat %d"), Sessions.Num() + 1);
	NewSession->CreatedAt = FDateTime::Now();

	TSharedPtr<FAiChatMessage> Welcome = MakeShared<FAiChatMessage>();
	Welcome->bIsUser = false;
	Welcome->MessageText = TEXT(
		"### Welcome to C++ Studio AI Agent!\n"
		"I am your autonomous Unreal Engine 5.8 pair programmer and Slate architect.\n\n"
		"- **Multi-File Code Editing**: When I generate code for `.h` and `.cpp` files, each card routes directly to the correct file!\n"
		"- **Autonomous Mode**: Switch permission to `⚡ Auto-Apply (Autonomous)` to let me automatically apply multi-file changes.\n"
		"- **Selectable Text & Copy**: Select any explanation or click `📋 Copy` in the message header.\n\n"
		"How can I help you build today?"
	);
	Welcome->Timestamp = FDateTime::Now();
	NewSession->Messages.Add(Welcome);

	Sessions.Add(NewSession);
	ActiveSessionIndex = Sessions.Num() - 1;

	RefreshSessionOptions();
	RebuildChatMessages();
}

void SCppAiAssistantDrawer::SwitchSession(int32 NewIndex)
{
	if (Sessions.IsValidIndex(NewIndex))
	{
		ActiveSessionIndex = NewIndex;
		RebuildChatMessages();
	}
}

void SCppAiAssistantDrawer::RefreshSessionOptions()
{
	SessionTitleOptions.Empty();
	for (const TSharedPtr<FAiChatSession>& Sess : Sessions)
	{
		if (Sess.IsValid())
		{
			SessionTitleOptions.Add(MakeShared<FString>(Sess->Title));
		}
	}
	if (SessionComboBox.IsValid())
	{
		SessionComboBox->RefreshOptions();
	}
}

void SCppAiAssistantDrawer::SetPermissionMode(EAiPermissionMode NewMode)
{
	PermissionMode = NewMode;
	if (PermissionButtonText.IsValid())
	{
		if (PermissionMode == EAiPermissionMode::AutoApply)
		{
			PermissionButtonText->SetText(FText::FromString(TEXT("⚡ Auto-Apply (Autonomous) ▾")));
			PermissionButtonText->SetColorAndOpacity(FLinearColor(1.0f, 0.8f, 0.2f, 1.0f));
		}
		else
		{
			PermissionButtonText->SetText(FText::FromString(TEXT("🛡️ Ask before apply ▾")));
			PermissionButtonText->SetColorAndOpacity(FLinearColor(0.85f, 0.85f, 0.85f, 1.0f));
		}
	}
}

TArray<TSharedPtr<FAiChatMessage>>& SCppAiAssistantDrawer::GetActiveMessages()
{
	static TArray<TSharedPtr<FAiChatMessage>> EmptyMessages;
	if (Sessions.IsValidIndex(ActiveSessionIndex) && Sessions[ActiveSessionIndex].IsValid())
	{
		return Sessions[ActiveSessionIndex]->Messages;
	}
	return EmptyMessages;
}

const TArray<TSharedPtr<FAiChatMessage>>& SCppAiAssistantDrawer::GetActiveMessages() const
{
	static TArray<TSharedPtr<FAiChatMessage>> EmptyMessages;
	if (Sessions.IsValidIndex(ActiveSessionIndex) && Sessions[ActiveSessionIndex].IsValid())
	{
		return Sessions[ActiveSessionIndex]->Messages;
	}
	return EmptyMessages;
}

FReply SCppAiAssistantDrawer::HandlePromptInputKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Enter)
	{
		if (InKeyEvent.IsShiftDown())
		{
			return FReply::Unhandled();
		}
		else
		{
			SendCurrentPrompt();
			return FReply::Handled();
		}
	}

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

	// Update session title on first user prompt
	if (Sessions.IsValidIndex(ActiveSessionIndex) && Sessions[ActiveSessionIndex].IsValid())
	{
		if (Sessions[ActiveSessionIndex]->Title.StartsWith(TEXT("Chat ")))
		{
			FString ShortTitle = Prompt.Left(28).TrimStartAndEnd();
			if (Prompt.Len() > 28) ShortTitle += TEXT("...");
			Sessions[ActiveSessionIndex]->Title = ShortTitle;
			RefreshSessionOptions();
		}
	}

	AppendMessage(true, Prompt);
	SessionRequestCount++;

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

			// Autonomous Auto-Apply
			if (PermissionMode == EAiPermissionMode::AutoApply)
			{
				AutoApplyCodeBlocksFromMessage(Response);
			}
		})
	);
}

void SCppAiAssistantDrawer::AutoApplyCodeBlocksFromMessage(const FString& MessageText)
{
	TArray<FString> Lines;
	MessageText.ParseIntoArrayLines(Lines, false);

	bool bInBlock = false;
	FString CurrentSnippet;
	TArray<FString> PrecedingLines;
	TArray<TPair<FString, FString>> BlocksToApply;

	FString ActiveFile;
	if (OnGetEditorContext.IsBound())
	{
		ActiveFile = OnGetEditorContext.Execute().ActiveFilePath;
	}

	for (int32 i = 0; i < Lines.Num(); ++i)
	{
		const FString& Line = Lines[i];
		FString Trimmed = Line.TrimStartAndEnd();

		if (Trimmed.StartsWith(TEXT("```")))
		{
			if (bInBlock)
			{
				bInBlock = false;
				FString Target = DetectTargetFile(PrecedingLines, CurrentSnippet, ActiveFile);
				if (!CurrentSnippet.IsEmpty())
				{
					BlocksToApply.Add(TPair<FString, FString>(CurrentSnippet, Target));
				}
				CurrentSnippet.Empty();
				PrecedingLines.Empty();
			}
			else
			{
				bInBlock = true;
				CurrentSnippet.Empty();
			}
		}
		else if (bInBlock)
		{
			CurrentSnippet += Line + TEXT("\n");
		}
		else
		{
			PrecedingLines.Add(Line);
			if (PrecedingLines.Num() > 8)
			{
				PrecedingLines.RemoveAt(0);
			}
		}
	}

	if (BlocksToApply.Num() > 0 && OnApplyCodeToEditor.IsBound())
	{
		TArray<FString> AppliedFiles;
		for (const auto& Block : BlocksToApply)
		{
			OnApplyCodeToEditor.Execute(Block.Key, Block.Value);
			FString DisplayName = Block.Value.IsEmpty() ? TEXT("Active File") : Block.Value;
			AppliedFiles.AddUnique(DisplayName);
		}

		FString Summary = FString::Printf(TEXT("⚡ Autonomous Agent: Auto-applied code across %s (Ctrl+Z to Undo)"), *FString::Join(AppliedFiles, TEXT(", ")));
		FNotificationInfo Info(FText::FromString(Summary));
		Info.ExpireDuration = 4.0f;
		Info.bFireAndForget = true;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
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

	GetActiveMessages().Add(Msg);
	RebuildChatMessages();
}

void SCppAiAssistantDrawer::RebuildChatMessages()
{
	if (!ChatScrollBox.IsValid())
	{
		return;
	}

	ChatScrollBox->ClearChildren();

	for (const TSharedPtr<FAiChatMessage>& Msg : GetActiveMessages())
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
			+ SHorizontalBox::Slot().FillWidth(0.12f) [ SNew(SSpacer) ]
			+ SHorizontalBox::Slot()
			.FillWidth(0.88f)
			.HAlign(HAlign_Right)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
				.BorderBackgroundColor(FLinearColor(0.12f, 0.35f, 0.65f, 0.95f))
				.Padding(FMargin(10.0f, 6.0f))
				[
					SNew(SVerticalBox)

					// Header with Timestamp & Copy Button
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 4.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f) [ SNew(SSpacer) ]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(Message->Timestamp.ToString(TEXT("%H:%M"))))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.0f))
							.ColorAndOpacity(FLinearColor(0.75f, 0.85f, 1.0f, 0.8f))
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(6.0f, 0.0f, 0.0f, 0.0f)
						.VAlign(VAlign_Center)
						[
							SNew(SButton)
							.ButtonStyle(FAppStyle::Get(), "SimpleButton")
							.ContentPadding(FMargin(4.0f, 1.0f))
							.ToolTipText(FText::FromString(TEXT("Copy prompt to clipboard")))
							.OnClicked_Lambda([Message]() -> FReply
							{
								if (Message.IsValid() && !Message->MessageText.IsEmpty())
								{
									FPlatformApplicationMisc::ClipboardCopy(*Message->MessageText);
									FNotificationInfo Info(FText::FromString(TEXT("✓ Prompt copied to clipboard")));
									Info.ExpireDuration = 2.0f;
									Info.bFireAndForget = true;
									FSlateNotificationManager::Get().AddNotification(Info);
								}
								return FReply::Handled();
							})
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("📋 Copy")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7.5f))
								.ColorAndOpacity(FLinearColor(0.8f, 0.9f, 1.0f, 0.9f))
							]
						]
					]

					// Content (Selectable multi-line text)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SMultiLineEditableTextBox)
						.Text(FText::FromString(Message->MessageText))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9.5f))
						.ForegroundColor(FLinearColor::White)
						.BackgroundColor(FLinearColor::Transparent)
						.IsReadOnly(true)
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

				// Copy Entire Message Button
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.ContentPadding(FMargin(4.0f, 1.0f))
					.ToolTipText(FText::FromString(TEXT("Copy full explanation to clipboard")))
					.OnClicked_Lambda([Message]() -> FReply
					{
						if (Message.IsValid() && !Message->MessageText.IsEmpty())
						{
							FPlatformApplicationMisc::ClipboardCopy(*Message->MessageText);
							FNotificationInfo Info(FText::FromString(TEXT("✓ Message copied to clipboard")));
							Info.ExpireDuration = 2.0f;
							Info.bFireAndForget = true;
							FSlateNotificationManager::Get().AddNotification(Info);
						}
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("📋 Copy Message")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7.5f))
						.ColorAndOpacity(FLinearColor(0.6f, 0.8f, 1.0f, 0.9f))
					]
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
	TArray<FString> PrecedingLines;

	FString ActiveFile;
	if (OnGetEditorContext.IsBound())
	{
		ActiveFile = OnGetEditorContext.Execute().ActiveFilePath;
	}

	auto FlushParagraph = [&]()
	{
		if (!CurrentParagraph.IsEmpty())
		{
			FString TextContent = CurrentParagraph.TrimStartAndEnd();
			Container->AddSlot()
				.AutoHeight()
				.Padding(0.0f, 2.0f, 0.0f, 4.0f)
				[
					SNew(SMultiLineEditableTextBox)
					.Text(FText::FromString(TextContent))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9.5f))
					.ForegroundColor(FLinearColor(0.88f, 0.90f, 0.95f, 1.0f))
					.BackgroundColor(FLinearColor::Transparent)
					.IsReadOnly(true)
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
				FString DetectedTarget = DetectTargetFile(PrecedingLines, CapturedCode, ActiveFile);

				TSharedRef<SVerticalBox> CodeCard = SNew(SVerticalBox);

				// Determine Action Button Text and Badge
				FString ApplyButtonText = DetectedTarget.IsEmpty() ? TEXT("⚡ Apply to File") : FString::Printf(TEXT("⚡ Apply to %s"), *DetectedTarget);
				FString BadgeText = DetectedTarget.IsEmpty() ? CapturedLang : FString::Printf(TEXT("%s | %s"), *CapturedLang, *DetectedTarget);
				FLinearColor BadgeColor = DetectedTarget.EndsWith(TEXT(".h"), ESearchCase::IgnoreCase) ? 
					FLinearColor(0.35f, 0.75f, 1.0f, 1.0f) : FLinearColor(0.3f, 0.85f, 0.5f, 1.0f);

				// Code Card Header Bar
				CodeCard->AddSlot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 4.0f)
					[
						SNew(SHorizontalBox)

						// Language & Target File Badge
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(BadgeText))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8.5f))
							.ColorAndOpacity(BadgeColor)
						]

						+ SHorizontalBox::Slot().FillWidth(1.0f) [ SNew(SSpacer) ]

						// "Apply to File" Button (Smart Intelligent Routing)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(4.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(SButton)
							.ButtonStyle(FAppStyle::Get(), "PrimaryButton")
							.ContentPadding(FMargin(8.0f, 2.0f))
							.ToolTipText(FText::FromString(FString::Printf(
								TEXT("Apply changes to %s (auto-opens tab if needed, prevents duplicate insertions, Ctrl+Z to Undo)"),
								DetectedTarget.IsEmpty() ? TEXT("active file") : *DetectedTarget
							)))
							.OnClicked_Lambda([this, CapturedCode, DetectedTarget]() -> FReply
							{
								if (OnApplyCodeToEditor.IsBound())
								{
									OnApplyCodeToEditor.Execute(CapturedCode, DetectedTarget);

									FString Notification = DetectedTarget.IsEmpty() ? 
										TEXT("✓ AI code applied to active editor (Ctrl+Z to Undo)") :
										FString::Printf(TEXT("✓ AI code applied to %s (Ctrl+Z to Undo)"), *DetectedTarget);

									FNotificationInfo Info(FText::FromString(Notification));
									Info.ExpireDuration = 3.0f;
									Info.bFireAndForget = true;
									FSlateNotificationManager::Get().AddNotification(Info);
								}
								return FReply::Handled();
							})
							[
								SNew(STextBlock)
								.Text(FText::FromString(ApplyButtonText))
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
				PrecedingLines.Empty();
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

		// Keep track of preceding lines for target file detection
		PrecedingLines.Add(Line);
		if (PrecedingLines.Num() > 8)
		{
			PrecedingLines.RemoveAt(0);
		}

		// Header 1 (#)
		if (Trimmed.StartsWith(TEXT("# ")))
		{
			FlushParagraph();
			Container->AddSlot()
				.AutoHeight()
				.Padding(0.0f, 6.0f, 0.0f, 2.0f)
				[
					SNew(SMultiLineEditableTextBox)
					.Text(FText::FromString(Trimmed.Mid(2)))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13.0f))
					.ForegroundColor(FLinearColor(0.35f, 0.8f, 1.0f, 1.0f))
					.BackgroundColor(FLinearColor::Transparent)
					.IsReadOnly(true)
					.AutoWrapText(true)
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
					SNew(SMultiLineEditableTextBox)
					.Text(FText::FromString(Trimmed.Mid(3)))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11.5f))
					.ForegroundColor(FLinearColor(0.5f, 0.85f, 1.0f, 1.0f))
					.BackgroundColor(FLinearColor::Transparent)
					.IsReadOnly(true)
					.AutoWrapText(true)
				];
			continue;
		}

		// Header 3 (###) or Bold Header (**...**)
		if (Trimmed.StartsWith(TEXT("### ")))
		{
			FlushParagraph();
			Container->AddSlot()
				.AutoHeight()
				.Padding(0.0f, 4.0f, 0.0f, 2.0f)
				[
					SNew(SMultiLineEditableTextBox)
					.Text(FText::FromString(Trimmed.Mid(4)))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10.5f))
					.ForegroundColor(FLinearColor(0.7f, 0.9f, 1.0f, 1.0f))
					.BackgroundColor(FLinearColor::Transparent)
					.IsReadOnly(true)
					.AutoWrapText(true)
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
						SNew(SMultiLineEditableTextBox)
						.Text(FText::FromString(Trimmed.Mid(2)))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9.5f))
						.ForegroundColor(FLinearColor(0.85f, 0.88f, 0.92f, 1.0f))
						.BackgroundColor(FLinearColor::Transparent)
						.IsReadOnly(true)
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
