// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#include "SSlateLivePreviewTab.h"
#include "SlateParser.h"
#include "SlateWidgetBuilder.h"
#include "SlateWidgetSnapshotter.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "Styling/AppStyle.h"
#include "SlateLivePreviewStyle.h"
#include "Widgets/Images/SImage.h"
#include "DesktopPlatformModule.h"
#include "HAL/PlatformProcess.h"

void SSlateLivePreviewTab::Construct(const FArguments& InArgs)
{
	FileWatcher = MakeShared<FSlateFileWatcher>(0.25f);

	Viewport = SNew(SSlateLivePreviewViewport);

	CodeTextBox = SNew(SMultiLineEditableTextBox)
		.Font(FCoreStyle::GetDefaultFontStyle("Mono", 10))
		.AutoWrapText(false)
		.OnTextChanged(this, &SSlateLivePreviewTab::OnCodeTextChanged);

	FilePathTextBox = SNew(SEditableTextBox)
		.HintText(FText::FromString(TEXT("Path to .cpp or .slate file to watch...")))
		.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type CommitType)
		{
			if (bAutoReloadOnFileSave)
			{
				FString Path = Text.ToString();
				if (!Path.IsEmpty())
				{
					FileWatcher->StartWatching(Path, FOnSlateWatchedFileChanged::CreateSP(this, &SSlateLivePreviewTab::OnFileChanged));
				}
			}
		});

	StatusTextBlock = SNew(STextBlock)
		.Text(FText::FromString(TEXT("Ready")))
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f));

	ConsoleTextBox = SNew(SMultiLineEditableTextBox)
		.Font(FCoreStyle::GetDefaultFontStyle("Mono", 8))
		.IsReadOnly(true)
		.AutoWrapText(true);

	ChildSlot
	[
		SNew(SVerticalBox)

		// ---------------------------------------------------------------------
		// Top Toolbar
		// ---------------------------------------------------------------------
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			.Padding(4.0f)
			[
				SNew(SHorizontalBox)

				// File watcher input
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				.Padding(2.0f)
				[
					FilePathTextBox.ToSharedRef()
				]

				// Browse button
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.0f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Browse...")))
					.OnClicked(this, &SSlateLivePreviewTab::OnBrowseFileClicked)
				]

				// Auto-reload toggle
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(6.0f, 0.0f)
				[
					SNew(SCheckBox)
					.IsChecked(ECheckBoxState::Checked)
					.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
					{
						bAutoReloadOnFileSave = (NewState == ECheckBoxState::Checked);
						if (bAutoReloadOnFileSave)
						{
							FString Path = FilePathTextBox->GetText().ToString();
							if (!Path.IsEmpty())
							{
								FileWatcher->StartWatching(Path, FOnSlateWatchedFileChanged::CreateSP(this, &SSlateLivePreviewTab::OnFileChanged));
							}
						}
						else
						{
							FileWatcher->StopWatching();
						}
					})
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Auto Watch File")))
					]
				]

				// Auto-snapshot toggle
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(6.0f, 0.0f)
				[
					SNew(SCheckBox)
					.IsChecked(ECheckBoxState::Checked)
					.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
					{
						bAutoSnapshot = (NewState == ECheckBoxState::Checked);
					})
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Export preview.png")))
					]
				]

				// Refresh button
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.0f)
				[
					SNew(SButton)
					.ContentPadding(FMargin(6.0f, 2.0f))
					.OnClicked(this, &SSlateLivePreviewTab::OnRefreshClicked)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 4.0f, 0.0f)
						[
							SNew(SImage)
							.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.Refresh")))
							.DesiredSizeOverride(FVector2D(12.0f, 12.0f))
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("Refresh")))
						]
					]
				]

				// Take Snapshot button
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.0f)
				[
					SNew(SButton)
					.ContentPadding(FMargin(6.0f, 2.0f))
					.OnClicked(this, &SSlateLivePreviewTab::OnSnapshotClicked)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 4.0f, 0.0f)
						[
							SNew(SImage)
							.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.Snapshot")))
							.DesiredSizeOverride(FVector2D(12.0f, 12.0f))
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("Snapshot")))
						]
					]
				]

				// Template Presets
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.0f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Card Template")))
					.OnClicked_Lambda([this]() -> FReply
					{
						LoadPresetTemplate(0);
						return FReply::Handled();
					})
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.0f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Settings Template")))
					.OnClicked_Lambda([this]() -> FReply
					{
						LoadPresetTemplate(1);
						return FReply::Handled();
					})
				]
			]
		]

		// ---------------------------------------------------------------------
		// Main Splitter (Left: Code, Right: Live Viewport)
		// ---------------------------------------------------------------------
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(4.0f)
		[
			SNew(SSplitter)
			.Orientation(Orient_Horizontal)

			// Left: Code Editor
			+ SSplitter::Slot()
			.Value(0.45f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(2.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("C++ Slate Snippet:")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.Text(FText::FromString(TEXT("Clear")))
						.OnClicked_Lambda([this]() -> FReply
						{
							CodeTextBox->SetText(FText::GetEmpty());
							return FReply::Handled();
						})
					]
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(2.0f)
				[
					CodeTextBox.ToSharedRef()
				]
			]

			// Right: Live Viewport
			+ SSplitter::Slot()
			.Value(0.55f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(2.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Live Slate Viewport:")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					]
					// Background mode buttons
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2.0f, 0.0f)
					[
						SNew(SButton)
						.Text(FText::FromString(TEXT("Dark")))
						.OnClicked_Lambda([this]() -> FReply
						{
							Viewport->SetBackgroundMode(ESlatePreviewBackground::SlateDark);
							return FReply::Handled();
						})
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2.0f, 0.0f)
					[
						SNew(SButton)
						.Text(FText::FromString(TEXT("Light")))
						.OnClicked_Lambda([this]() -> FReply
						{
							Viewport->SetBackgroundMode(ESlatePreviewBackground::Light);
							return FReply::Handled();
						})
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2.0f, 0.0f)
					[
						SNew(SButton)
						.Text(FText::FromString(TEXT("400x300")))
						.OnClicked_Lambda([this]() -> FReply
						{
							Viewport->SetSizePreset(ESlatePreviewSizePreset::Fixed400x300);
							return FReply::Handled();
						})
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2.0f, 0.0f)
					[
						SNew(SButton)
						.Text(FText::FromString(TEXT("Auto Size")))
						.OnClicked_Lambda([this]() -> FReply
						{
							Viewport->SetSizePreset(ESlatePreviewSizePreset::Auto);
							return FReply::Handled();
						})
					]
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(2.0f)
				[
					Viewport.ToSharedRef()
				]
			]
		]

		// ---------------------------------------------------------------------
		// Bottom Status Bar & Mini Console
		// ---------------------------------------------------------------------
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f, 2.0f)
		[
			StatusTextBlock.ToSharedRef()
		]

		+ SVerticalBox::Slot()
		.MaxHeight(90.0f)
		.Padding(4.0f, 2.0f)
		[
			ConsoleTextBox.ToSharedRef()
		]
	];

	// Load initial template
	LoadPresetTemplate(0);
}

SSlateLivePreviewTab::~SSlateLivePreviewTab()
{
	if (FileWatcher.IsValid())
	{
		FileWatcher->StopWatching();
	}
}

void SSlateLivePreviewTab::SetSourceCode(const FString& InCode)
{
	if (CodeTextBox.IsValid())
	{
		CodeTextBox->SetText(FText::FromString(InCode));
	}
}

void SSlateLivePreviewTab::OnCodeTextChanged(const FText& InText)
{
	RebuildPreview();
}

void SSlateLivePreviewTab::OnFileChanged(const FString& FilePath, const FString& FileContent)
{
	OnLogReceived(FString::Printf(TEXT("[File Watcher] Detected change in: %s"), *FilePath));
	SetSourceCode(FileContent);
}

FReply SSlateLivePreviewTab::OnBrowseFileClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		TArray<FString> OutFiles;
		bool bOpened = DesktopPlatform->OpenFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			TEXT("Select C++ Slate File"),
			FPaths::ProjectDir(),
			TEXT(""),
			TEXT("Slate Source Files (*.cpp;*.h;*.slate)|*.cpp;*.h;*.slate|All Files (*.*)|*.*"),
			EFileDialogFlags::None,
			OutFiles
		);

		if (bOpened && OutFiles.Num() > 0)
		{
			FilePathTextBox->SetText(FText::FromString(OutFiles[0]));
			if (bAutoReloadOnFileSave)
			{
				FileWatcher->StartWatching(OutFiles[0], FOnSlateWatchedFileChanged::CreateSP(this, &SSlateLivePreviewTab::OnFileChanged));
			}

			FString Content;
			if (FFileHelper::LoadFileToString(Content, *OutFiles[0]))
			{
				SetSourceCode(Content);
			}
		}
	}
	return FReply::Handled();
}

FReply SSlateLivePreviewTab::OnRefreshClicked()
{
	RebuildPreview();
	return FReply::Handled();
}

FReply SSlateLivePreviewTab::OnSnapshotClicked()
{
	CaptureSnapshot();
	return FReply::Handled();
}

void SSlateLivePreviewTab::OnLogReceived(const FString& Message)
{
	ConsoleLogAccumulator.Append(Message);
	ConsoleLogAccumulator.Append(TEXT("\n"));

	// Keep last 1500 chars to prevent infinite memory growth
	if (ConsoleLogAccumulator.Len() > 2000)
	{
		ConsoleLogAccumulator = ConsoleLogAccumulator.Right(1500);
	}

	if (ConsoleTextBox.IsValid())
	{
		ConsoleTextBox->SetText(FText::FromString(ConsoleLogAccumulator));
	}
}

void SSlateLivePreviewTab::CaptureSnapshot()
{
	if (!Viewport.IsValid()) return;

	TSharedPtr<SWidget> PreviewWidget = Viewport->GetPreviewWidget();
	if (!PreviewWidget.IsValid()) return;

	FString SavedPath;
	if (FSlateWidgetSnapshotter::CaptureWidgetToPng(PreviewWidget.ToSharedRef(), SavedPath))
	{
		OnLogReceived(FString::Printf(TEXT("[Snapshot] Captured live widget -> %s"), *SavedPath));
	}
	else
	{
		OnLogReceived(TEXT("[Snapshot] Failed to capture snapshot."));
	}
}

void SSlateLivePreviewTab::RebuildPreview()
{
	if (!CodeTextBox.IsValid() || !Viewport.IsValid()) return;

	FString Code = CodeTextBox->GetText().ToString();
	if (Code.TrimStartAndEnd().IsEmpty())
	{
		Viewport->SetPreviewWidget(SNew(STextBlock).Text(FText::FromString(TEXT("Type Slate C++ code to preview..."))));
		StatusTextBlock->SetText(FText::FromString(TEXT("Empty code snippet.")));
		return;
	}

	double StartTime = FPlatformTime::Seconds();

	TSharedPtr<FSlateWidgetNode> RootAst;
	TArray<FString> Errors;
	TArray<FString> Warnings;

	bool bParsed = FSlateCodeParser::ParseSource(Code, RootAst, Errors, Warnings);
	double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;

	if (bParsed && RootAst.IsValid())
	{
		FSlateWidgetBuilder Builder;
		Builder.SetLogHandler(FOnSlatePreviewLog::CreateSP(this, &SSlateLivePreviewTab::OnLogReceived));
		TSharedRef<SWidget> LiveWidget = Builder.Build(RootAst);

		Viewport->SetPreviewWidget(LiveWidget);

		StatusTextBlock->SetText(FText::FromString(FString::Printf(TEXT("Parsed in %.2f ms | Root: <%s> | Properties: %d | Slots: %d"),
			ElapsedMs, *RootAst->WidgetType, RootAst->Properties.Num(), RootAst->Slots.Num())));

		if (bAutoSnapshot)
		{
			CaptureSnapshot();
		}
	}
	else
	{
		TSharedRef<SWidget> ErrorWidget = FSlateWidgetBuilder::BuildErrorWidget(Errors, Warnings);
		Viewport->SetPreviewWidget(ErrorWidget);

		StatusTextBlock->SetText(FText::FromString(FString::Printf(TEXT("Parsing error (%d error(s), %d warning(s))"),
			Errors.Num(), Warnings.Num())));
	}
}

void SSlateLivePreviewTab::LoadPresetTemplate(int32 TemplateIndex)
{
	FString Code;
	if (TemplateIndex == 0)
	{
		Code = TEXT("SNew(SBorder)\n")
			TEXT(".BorderBackgroundColor(FLinearColor(0.08f, 0.09f, 0.12f, 1.0f))\n")
			TEXT(".Padding(FMargin(16.0f))\n")
			TEXT("[\n")
			TEXT("    SNew(SVerticalBox)\n")
			TEXT("    + SVerticalBox::Slot()\n")
			TEXT("    .AutoHeight()\n")
			TEXT("    .Padding(FMargin(0, 0, 0, 10))\n")
			TEXT("    [\n")
			TEXT("        SNew(STextBlock)\n")
			TEXT("        .Text(FText::FromString(TEXT(\"Live Slate Preview\")))\n")
			TEXT("        .ColorAndOpacity(FLinearColor(0.2f, 0.8f, 1.0f, 1.0f))\n")
			TEXT("        .Font(FCoreStyle::GetDefaultFontStyle(TEXT(\"Bold\"), 14))\n")
			TEXT("    ]\n")
			TEXT("    + SVerticalBox::Slot()\n")
			TEXT("    .AutoHeight()\n")
			TEXT("    .Padding(FMargin(0, 0, 0, 8))\n")
			TEXT("    [\n")
			TEXT("        SNew(STextBlock)\n")
			TEXT("        .Text(FText::FromString(TEXT(\"Instant feedback with zero C++ compilation!\")))\n")
			TEXT("        .ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f))\n")
			TEXT("    ]\n")
			TEXT("    + SVerticalBox::Slot()\n")
			TEXT("    .AutoHeight()\n")
			TEXT("    [\n")
			TEXT("        SNew(SHorizontalBox)\n")
			TEXT("        + SHorizontalBox::Slot()\n")
			TEXT("        .AutoWidth()\n")
			TEXT("        .Padding(FMargin(0, 0, 8, 0))\n")
			TEXT("        [\n")
			TEXT("            SNew(SButton)\n")
			TEXT("            .ButtonColorAndOpacity(FLinearColor(0.1f, 0.6f, 0.2f, 1.0f))\n")
			TEXT("            .ContentPadding(FMargin(12.0f, 6.0f))\n")
			TEXT("            [\n")
			TEXT("                SNew(STextBlock)\n")
			TEXT("                .Text(FText::FromString(TEXT(\"Accept\")))\n")
			TEXT("            ]\n")
			TEXT("        ]\n")
			TEXT("        + SHorizontalBox::Slot()\n")
			TEXT("        .AutoWidth()\n")
			TEXT("        [\n")
			TEXT("            SNew(SButton)\n")
			TEXT("            .ButtonColorAndOpacity(FLinearColor(0.6f, 0.1f, 0.1f, 1.0f))\n")
			TEXT("            .ContentPadding(FMargin(12.0f, 6.0f))\n")
			TEXT("            [\n")
			TEXT("                SNew(STextBlock)\n")
			TEXT("                .Text(FText::FromString(TEXT(\"Cancel\")))\n")
			TEXT("            ]\n")
			TEXT("        ]\n")
			TEXT("    ]\n")
			TEXT("]\n");
	}
	else
	{
		Code = TEXT("SNew(SBorder)\n")
			TEXT(".BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.06f, 1.0f))\n")
			TEXT(".Padding(FMargin(20.0f))\n")
			TEXT("[\n")
			TEXT("    SNew(SVerticalBox)\n")
			TEXT("    + SVerticalBox::Slot()\n")
			TEXT("    .AutoHeight()\n")
			TEXT("    .Padding(FMargin(0, 0, 0, 12))\n")
			TEXT("    [\n")
			TEXT("        SNew(STextBlock)\n")
			TEXT("        .Text(FText::FromString(TEXT(\"System Configuration\")))\n")
			TEXT("        .Font(FCoreStyle::GetDefaultFontStyle(TEXT(\"Bold\"), 15))\n")
			TEXT("        .ColorAndOpacity(FLinearColor::White)\n")
			TEXT("    ]\n")
			TEXT("    + SVerticalBox::Slot()\n")
			TEXT("    .AutoHeight()\n")
			TEXT("    .Padding(FMargin(0, 0, 0, 6))\n")
			TEXT("    [\n")
			TEXT("        SNew(SSeparator)\n")
			TEXT("        .Thickness(1.5f)\n")
			TEXT("        .ColorAndOpacity(FLinearColor(0.2f, 0.2f, 0.2f, 1.0f))\n")
			TEXT("    ]\n")
			TEXT("    + SVerticalBox::Slot()\n")
			TEXT("    .AutoHeight()\n")
			TEXT("    .Padding(FMargin(0, 8, 0, 8))\n")
			TEXT("    [\n")
			TEXT("        SNew(SHorizontalBox)\n")
			TEXT("        + SHorizontalBox::Slot()\n")
			TEXT("        .FillWidth(1.0f)\n")
			TEXT("        .VAlign(VAlign_Center)\n")
			TEXT("        [\n")
			TEXT("            SNew(STextBlock).Text(FText::FromString(TEXT(\"Enable Wire Glow Effect\")))\n")
			TEXT("        ]\n")
			TEXT("        + SHorizontalBox::Slot()\n")
			TEXT("        .AutoWidth()\n")
			TEXT("        [\n")
			TEXT("            SNew(SCheckBox).IsChecked(true)\n")
			TEXT("        ]\n")
			TEXT("    ]\n")
			TEXT("    + SVerticalBox::Slot()\n")
			TEXT("    .AutoHeight()\n")
			TEXT("    .Padding(FMargin(0, 8, 0, 8))\n")
			TEXT("    [\n")
			TEXT("        SNew(SHorizontalBox)\n")
			TEXT("        + SHorizontalBox::Slot()\n")
			TEXT("        .FillWidth(1.0f)\n")
			TEXT("        .VAlign(VAlign_Center)\n")
			TEXT("        [\n")
			TEXT("            SNew(STextBlock).Text(FText::FromString(TEXT(\"Routing Smoothing Factor\")))\n")
			TEXT("        ]\n")
			TEXT("        + SHorizontalBox::Slot()\n")
			TEXT("        .AutoWidth()\n")
			TEXT("        [\n")
			TEXT("            SNew(SBox)\n")
			TEXT("            .WidthOverride(120.0f)\n")
			TEXT("            [\n")
			TEXT("                SNew(SSlider).Value(0.75f)\n")
			TEXT("            ]\n")
			TEXT("        ]\n")
			TEXT("    ]\n")
			TEXT("]\n");
	}

	SetSourceCode(Code);
}
