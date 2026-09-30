// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#include "SCppStudioTab.h"
#include "SlateLivePreviewStyle.h"
#include "SlateParser.h"
#include "SlateWidgetBuilder.h"
#include "ILiveCodingModule.h"
#include "SCreateClassDialog.h"
#include "SCppSettingsDialog.h"
#include "CppEditorSettings.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/SOverlay.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "HAL/FileManager.h"

void SCppStudioTab::Construct(const FArguments& InArgs)
{
	SlatePreviewViewport = SNew(SSlateLivePreviewViewport);

	FileHeaderTextBlock = SNew(STextBlock)
		.Text(FText::FromString(TEXT("No File Selected")))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
		.ColorAndOpacity(FLinearColor::White);

	StatusBadgeIcon = SNew(SImage)
		.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.Status.Ready")))
		.DesiredSizeOverride(FVector2D(12.0f, 12.0f));

	StatusBadgeTextBlock = SNew(STextBlock)
		.Text(FText::FromString(TEXT("Ready")))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
		.ColorAndOpacity(FLinearColor(0.4f, 0.8f, 0.4f, 1.0f));

	OutputConsoleTextBox = SNew(SMultiLineEditableTextBox)
		.Font(FCoreStyle::GetDefaultFontStyle("Mono", 8))
		.IsReadOnly(true)
		.AutoWrapText(true)
		.OnCursorMoved_Lambda([this](const FTextLocation&)
		{
			if (OutputConsoleTextBox.IsValid())
			{
				FString Line;
				OutputConsoleTextBox->GetCurrentTextLine(Line);
				CheckAndParseErrorLine(Line);
			}
		});

	ProjectTree = SNew(SCppProjectTree)
		.OnFileSelected(this, &SCppStudioTab::OnFileSelectedFromTree)
		.OnNewClassRequested(this, &SCppStudioTab::OpenNewClassWizard)
		.OnFilesRenamed(this, &SCppStudioTab::OnFilesRenamedInTree)
		.OnFileDeleted(this, &SCppStudioTab::OnFileDeletedInTree);

	// Construct Left and Right Editor Panes
	SAssignNew(LeftEditorPane, SCppEditorPane)
		.OnActiveDocumentChanged_Lambda([this](const FString& Path)
		{
			OnPaneActiveDocumentChanged(Path, LeftEditorPane);
		})
		.OnDocumentContentChanged(this, &SCppStudioTab::OnPaneDocumentContentChanged)
		.OnPaneLog(this, &SCppStudioTab::AppendLog)
		.OnQuickOpenRequested_Lambda([this]() { ToggleQuickOpen(true); })
		.OnNewClassRequested(this, &SCppStudioTab::OpenNewClassWizard)
		.OnPaneFocused(this, &SCppStudioTab::OnPaneFocusedChanged)
		.OnMoveDocumentRequested(this, &SCppStudioTab::OnMoveDocumentBetweenPanes);

	SAssignNew(RightEditorPane, SCppEditorPane)
		.OnActiveDocumentChanged_Lambda([this](const FString& Path)
		{
			OnPaneActiveDocumentChanged(Path, RightEditorPane);
		})
		.OnDocumentContentChanged(this, &SCppStudioTab::OnPaneDocumentContentChanged)
		.OnPaneLog(this, &SCppStudioTab::AppendLog)
		.OnQuickOpenRequested_Lambda([this]() { ToggleQuickOpen(true); })
		.OnNewClassRequested(this, &SCppStudioTab::OpenNewClassWizard)
		.OnPaneFocused(this, &SCppStudioTab::OnPaneFocusedChanged)
		.OnMoveDocumentRequested(this, &SCppStudioTab::OnMoveDocumentBetweenPanes);

	ActiveEditorPane = LeftEditorPane;
	LeftEditorPane->SetIsActivePane(true);
	RightEditorPane->SetIsActivePane(false);

	// Construct Editor Area Splitter (Starts with LeftEditorPane only)
	SAssignNew(EditorAreaSplitter, SSplitter)
		.Orientation(Orient_Horizontal)
		+ SSplitter::Slot()
		.Value(1.0f)
		[
			LeftEditorPane.ToSharedRef()
		];

	ChildSlot
	[
		SNew(SOverlay)

		// ---------------------------------------------------------------------
		// Layer 0: Main Work Area
		// ---------------------------------------------------------------------
		+ SOverlay::Slot()
		[
			SNew(SVerticalBox)

			// ---------------------------------------------------------------------
			// Top Toolbar
			// ---------------------------------------------------------------------
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2.0f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				.Padding(4.0f)
				[
					SNew(SHorizontalBox)

					// Active File Name / Path
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(4.0f, 0.0f)
					[
						FileHeaderTextBlock.ToSharedRef()
					]

					// Status Badge
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(12.0f, 0.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(0.0f, 0.0f, 5.0f, 0.0f)
						[
							StatusBadgeIcon.ToSharedRef()
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							StatusBadgeTextBlock.ToSharedRef()
						]
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(SSpacer)
					]

					// 1. New Class Wizard (Ctrl+N)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(2.0f)
					[
						SNew(SButton)
						.ButtonColorAndOpacity(FLinearColor(0.18f, 0.45f, 0.3f, 1.0f))
						.ToolTipText(FText::FromString(TEXT("Create a new Slate Widget, UObject, Actor, Component or Struct with full boilerplate (Ctrl+N)")))
						.OnClicked(this, &SCppStudioTab::OnNewClassClicked)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(0.0f, 0.0f, 5.0f, 0.0f)
							[
								SNew(SImage)
								.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.NewClass")))
								.DesiredSizeOverride(FVector2D(14.0f, 14.0f))
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("New Class (Ctrl+N)")))
							]
						]
					]

					// 2. Quick Open (Ctrl+P)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(2.0f)
					[
						SNew(SButton)
						.ToolTipText(FText::FromString(TEXT("Quickly search and jump to any project C++ file (Ctrl+P)")))
						.OnClicked(this, &SCppStudioTab::OnQuickOpenClicked)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(0.0f, 0.0f, 5.0f, 0.0f)
							[
								SNew(SImage)
								.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.QuickOpen")))
								.DesiredSizeOverride(FVector2D(14.0f, 14.0f))
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Quick Open (Ctrl+P)")))
							]
						]
					]

					// 3. Save Current File
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(2.0f)
					[
						SNew(SButton)
						.ToolTipText(FText::FromString(TEXT("Save currently active file to disk")))
						.OnClicked(this, &SCppStudioTab::OnSaveClicked)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(0.0f, 0.0f, 5.0f, 0.0f)
							[
								SNew(SImage)
								.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.Save")))
								.DesiredSizeOverride(FVector2D(14.0f, 14.0f))
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Save (Ctrl+S)")))
							]
						]
					]

					// 4. Save All Files
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(2.0f)
					[
						SNew(SButton)
						.ToolTipText(FText::FromString(TEXT("Save all open dirty files across all editor tabs")))
						.OnClicked(this, &SCppStudioTab::OnSaveAllClicked)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(0.0f, 0.0f, 5.0f, 0.0f)
							[
								SNew(SImage)
								.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.SaveAll")))
								.DesiredSizeOverride(FVector2D(14.0f, 14.0f))
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Save All")))
							]
						]
					]

					// 5. Live Coding Button
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(2.0f)
					[
						SNew(SButton)
						.ButtonColorAndOpacity(FLinearColor(0.12f, 0.38f, 0.72f, 1.0f))
						.ToolTipText(FText::FromString(TEXT("Save all files and hot-patch C++ code into the running editor")))
						.OnClicked(this, &SCppStudioTab::OnLiveCodingClicked)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(0.0f, 0.0f, 5.0f, 0.0f)
							[
								SNew(SImage)
								.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.LiveCoding")))
								.DesiredSizeOverride(FVector2D(14.0f, 14.0f))
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Live Coding (Ctrl+B)")))
							]
						]
					]

					// 6. Revert Button
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(2.0f)
					[
						SNew(SButton)
						.ToolTipText(FText::FromString(TEXT("Discard unsaved changes in active file and reload from disk")))
						.OnClicked(this, &SCppStudioTab::OnRevertClicked)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(0.0f, 0.0f, 5.0f, 0.0f)
							[
								SNew(SImage)
								.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.Revert")))
								.DesiredSizeOverride(FVector2D(14.0f, 14.0f))
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Revert")))
							]
						]
					]

					// 7. Split View Toggle
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(2.0f)
					[
						SNew(SButton)
						.ToolTipText(FText::FromString(TEXT("Toggle Dual-Pane Split View to view two files (.h and .cpp) side-by-side")))
						.OnClicked(this, &SCppStudioTab::OnToggleSplitClicked)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(0.0f, 0.0f, 5.0f, 0.0f)
							[
								SNew(SImage)
								.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.SplitView")))
								.DesiredSizeOverride(FVector2D(14.0f, 14.0f))
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Split View")))
							]
						]
					]

					// 8. Slate Preview Toggle
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(2.0f)
					[
						SNew(SButton)
						.ToolTipText(FText::FromString(TEXT("Toggle live Slate preview viewport on the right")))
						.OnClicked(this, &SCppStudioTab::OnTogglePreviewClicked)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(0.0f, 0.0f, 5.0f, 0.0f)
							[
								SNew(SImage)
								.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.SlatePreview")))
								.DesiredSizeOverride(FVector2D(14.0f, 14.0f))
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Slate Preview")))
							]
						]
					]

					// 9. Settings
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(2.0f)
					[
						SNew(SButton)
						.ToolTipText(FText::FromString(TEXT("Preferences: Theme, Font, Formatting, Behavior (Ctrl+,)")))
						.OnClicked(this, &SCppStudioTab::OnSettingsClicked)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(0.0f, 0.0f, 5.0f, 0.0f)
							[
								SNew(SImage)
								.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.Settings")))
								.DesiredSizeOverride(FVector2D(14.0f, 14.0f))
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Settings")))
							]
						]
					]
				]
			]

			// ---------------------------------------------------------------------
			// Resizable Vertical Splitter (Upper: Work Area | Lower: Output Console)
			// ---------------------------------------------------------------------
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2.0f)
			[
				SNew(SSplitter)
				.Orientation(Orient_Vertical)

				// 1. Upper: Main Horizontal Splitter (Tree | Editor Panes | Slate Preview)
				+ SSplitter::Slot()
				.Value(0.78f)
				[
					SAssignNew(MainHorizontalSplitter, SSplitter)
					.Orientation(Orient_Horizontal)

					// Left: Project Source Tree
					+ SSplitter::Slot()
					.Value(0.20f)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
						.Padding(2.0f)
						[
							ProjectTree.ToSharedRef()
						]
					]

					// Center: Editor Panes (Single or Split)
					+ SSplitter::Slot()
					.Value(0.52f)
					[
						EditorAreaSplitter.ToSharedRef()
					]

					// Right: Live Slate Preview
					+ SSplitter::Slot()
					.Value(0.28f)
					[
						SAssignNew(PreviewPanelBorder, SBorder)
						.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
						.Padding(2.0f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(2.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Live Slate Viewport:")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								.ColorAndOpacity(FLinearColor(0.2f, 0.8f, 1.0f, 1.0f))
							]
							+ SVerticalBox::Slot()
							.FillHeight(1.0f)
							[
								SlatePreviewViewport.ToSharedRef()
							]
						]
					]
				]

				// 2. Lower: Resizable Output Console Log
				+ SSplitter::Slot()
				.Value(0.22f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
					.Padding(3.0f)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(2.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Output / Live Coding Console:")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f))
							]

							// Jump to Compiler Error Button
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(8.0f, 0.0f)
							[
								SAssignNew(ErrorJumpButton, SButton)
								.ButtonStyle(FAppStyle::Get(), "PrimaryButton")
								.ButtonColorAndOpacity(FLinearColor(0.85f, 0.2f, 0.2f, 1.0f))
								.Visibility(EVisibility::Collapsed)
								.ToolTipText(FText::FromString(TEXT("Jump to compiler error in source code (F4)")))
								.OnClicked(this, &SCppStudioTab::OnErrorJumpClicked)
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									.AutoWidth()
									.VAlign(VAlign_Center)
									.Padding(0.0f, 0.0f, 4.0f, 0.0f)
									[
										SAssignNew(ErrorJumpIcon, SImage)
										.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.Status.ErrorJump")))
										.DesiredSizeOverride(FVector2D(12.0f, 12.0f))
									]
									+ SHorizontalBox::Slot()
									.AutoWidth()
									.VAlign(VAlign_Center)
									[
										SAssignNew(ErrorJumpTextBlock, STextBlock)
										.Text(FText::FromString(TEXT("Jump to Error (F4)")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
									]
								]
							]

							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							[
								SNew(SSpacer)
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SButton)
								.ButtonStyle(FAppStyle::Get(), "SimpleButton")
								.Text(FText::FromString(TEXT("Clear")))
								.ToolTipText(FText::FromString(TEXT("Clear console output")))
								.OnClicked_Lambda([this]() -> FReply
								{
									OutputConsoleAccumulator.Empty();
									if (OutputConsoleTextBox.IsValid())
									{
										OutputConsoleTextBox->SetText(FText::GetEmpty());
									}
									if (ErrorJumpButton.IsValid())
									{
										ErrorJumpButton->SetVisibility(EVisibility::Collapsed);
									}
									return FReply::Handled();
								})
							]
						]

						+ SVerticalBox::Slot()
						.FillHeight(1.0f)
						[
							OutputConsoleTextBox.ToSharedRef()
						]
					]
				]
			]
		]

		// ---------------------------------------------------------------------
		// Layer 1: Backdrop to dismiss Quick Open on outside click
		// ---------------------------------------------------------------------
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.Visibility_Lambda([this]() { return bQuickOpenVisible ? EVisibility::Visible : EVisibility::Collapsed; })
			.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.45f))
			.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent&) -> FReply
			{
				ToggleQuickOpen(false);
				return FReply::Handled();
			})
		]

		// ---------------------------------------------------------------------
		// Layer 2: Floating Quick Open Overlay (Ctrl+P)
		// ---------------------------------------------------------------------
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.Padding(0.0f, 40.0f, 0.0f, 0.0f)
		[
			SAssignNew(QuickOpenOverlayWidget, SBorder)
			.Visibility_Lambda([this]() { return bQuickOpenVisible ? EVisibility::Visible : EVisibility::Collapsed; })
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			.BorderBackgroundColor(FLinearColor(0.08f, 0.09f, 0.12f, 0.98f))
			.Padding(6.0f)
			[
				SNew(SBox)
				.WidthOverride(560.0f)
				.MaxDesiredHeight(380.0f)
				[
					SNew(SVerticalBox)

					// Search Box
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(2.0f)
					[
						SAssignNew(QuickOpenSearchBox, SSearchBox)
						.HintText(FText::FromString(TEXT("Type file name to open... (Esc to dismiss)")))
						.OnTextChanged(this, &SCppStudioTab::OnQuickOpenSearchTextChanged)
						.OnKeyDownHandler_Lambda([this](const FGeometry&, const FKeyEvent& InKeyEvent) -> FReply
						{
							if (InKeyEvent.GetKey() == EKeys::Escape || (InKeyEvent.IsControlDown() && InKeyEvent.GetKey() == EKeys::P))
							{
								ToggleQuickOpen(false);
								return FReply::Handled();
							}
							if (InKeyEvent.GetKey() == EKeys::Up)
							{
								QuickOpenNavigate(-1);
								return FReply::Handled();
							}
							if (InKeyEvent.GetKey() == EKeys::Down)
							{
								QuickOpenNavigate(1);
								return FReply::Handled();
							}
							return FReply::Unhandled();
						})
						.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type CommitType)
						{
							if (CommitType == ETextCommit::OnEnter)
							{
								CommitQuickOpenSelection();
							}
						})
					]

					// File list
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					.Padding(2.0f, 4.0f)
					[
						SAssignNew(QuickOpenListView, SListView<TSharedPtr<FString>>)
						.ListItemsSource(&FilteredQuickOpenFiles)
						.OnGenerateRow(this, &SCppStudioTab::OnGenerateQuickOpenRow)
						.OnMouseButtonDoubleClick_Lambda([this](TSharedPtr<FString> Item)
						{
							if (Item.IsValid())
							{
								OpenFile(*Item);
								ToggleQuickOpen(false);
							}
						})
					]

					// Footer help
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4.0f, 2.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Up/Down Navigate | Enter Open | Esc Close")))
						.Font(FCoreStyle::GetDefaultFontStyle("Italic", 8))
						.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f))
					]
				]
			]
		]
	];

	// Hook into Live Coding patch completion delegate
	ILiveCodingModule* LiveCoding = FModuleManager::GetModulePtr<ILiveCodingModule>(LIVE_CODING_MODULE_NAME);
	if (LiveCoding)
	{
		PatchCompleteDelegateHandle = LiveCoding->GetOnPatchCompleteDelegate().AddRaw(this, &SCppStudioTab::OnPatchComplete);
	}

	AppendLog(TEXT("[C++ Studio] Initialized with Syntax Highlighting, Find in File (Ctrl+F), Undo/Redo, Multi-Tabs & Split View."));

	// Automatically open sample files if present
	FString SampleCppPath = FPaths::Combine(FPaths::GameSourceDir(), TEXT("CircuitNodes"), TEXT("SampleSlateWidget.cpp"));
	FString SampleHPath = FPaths::Combine(FPaths::GameSourceDir(), TEXT("CircuitNodes"), TEXT("SampleSlateWidget.h"));

	if (IFileManager::Get().FileExists(*SampleCppPath))
	{
		OpenFile(SampleCppPath);
	}
	if (IFileManager::Get().FileExists(*SampleHPath))
	{
		// Open .h tab in Left pane as well so user can switch between them
		if (LeftEditorPane.IsValid())
		{
			LeftEditorPane->OpenFile(SampleHPath);
			// Switch back to .cpp as active
			LeftEditorPane->OpenFile(SampleCppPath);
		}
	}
}

SCppStudioTab::~SCppStudioTab()
{
	ILiveCodingModule* LiveCoding = FModuleManager::GetModulePtr<ILiveCodingModule>(LIVE_CODING_MODULE_NAME);
	if (LiveCoding && PatchCompleteDelegateHandle.IsValid())
	{
		LiveCoding->GetOnPatchCompleteDelegate().Remove(PatchCompleteDelegateHandle);
	}
}

void SCppStudioTab::OnFileSelectedFromTree(const FString& FilePath)
{
	OpenFile(FilePath);
	TSharedPtr<SCppEditorPane> ActivePane = ActiveEditorPane.Pin();
	if (ActivePane.IsValid())
	{
		ActivePane->FocusEditor();
	}
}

void SCppStudioTab::OpenFile(const FString& InFilePath)
{
	if (!IFileManager::Get().FileExists(*InFilePath))
	{
		return;
	}

	TSharedPtr<SCppEditorPane> TargetPane = ActiveEditorPane.Pin();
	if (!TargetPane.IsValid())
	{
		TargetPane = LeftEditorPane;
	}

	if (TargetPane.IsValid())
	{
		TargetPane->OpenFile(InFilePath);
		ActiveEditorPane = TargetPane;
		TargetPane->FocusEditor();
		UpdateFileHeader();
		UpdateSlatePreviewIfApplicable();
	}
}

void SCppStudioTab::OpenFileAtLine(const FString& InFilePath, int32 LineNumber, int32 ColumnNumber)
{
	OpenFile(InFilePath);
	TSharedPtr<SCppEditorPane> TargetPane = ActiveEditorPane.Pin();
	if (TargetPane.IsValid())
	{
		TargetPane->GoToLine(LineNumber, ColumnNumber);
	}
}

void SCppStudioTab::OnPaneActiveDocumentChanged(const FString& FilePath, TSharedPtr<SCppEditorPane> SourcePane)
{
	OnPaneFocusedChanged(SourcePane);
	UpdateFileHeader();
	UpdateSlatePreviewIfApplicable();
}

void SCppStudioTab::OnPaneFocusedChanged(TSharedPtr<SCppEditorPane> FocusedPane)
{
	if (FocusedPane.IsValid())
	{
		ActiveEditorPane = FocusedPane;
		if (LeftEditorPane.IsValid())
		{
			LeftEditorPane->SetIsActivePane(FocusedPane == LeftEditorPane);
		}
		if (RightEditorPane.IsValid())
		{
			RightEditorPane->SetIsActivePane(FocusedPane == RightEditorPane);
		}
		UpdateFileHeader();
	}
}

void SCppStudioTab::OnMoveDocumentBetweenPanes(const FString& FilePath, TSharedPtr<SCppEditorPane> SourcePane)
{
	if (!bIsSplitView)
	{
		ToggleSplitView();
	}

	TSharedPtr<SCppEditorPane> TargetPane = (SourcePane == LeftEditorPane) ? RightEditorPane : LeftEditorPane;
	if (SourcePane.IsValid() && TargetPane.IsValid())
	{
		SourcePane->CloseFile(FilePath);
		TargetPane->OpenFile(FilePath);
		OnPaneFocusedChanged(TargetPane);
	}
}

void SCppStudioTab::OnFilesRenamedInTree(const FString& OldPath, const FString& NewPath)
{
	RefreshProjectFileList();

	if (LeftEditorPane.IsValid())
	{
		if (LeftEditorPane->CloseFile(OldPath))
		{
			LeftEditorPane->OpenFile(NewPath);
		}
	}
	if (RightEditorPane.IsValid())
	{
		if (RightEditorPane->CloseFile(OldPath))
		{
			RightEditorPane->OpenFile(NewPath);
		}
	}
	SetStatus(TEXT("Renamed successfully"), FLinearColor(0.2f, 0.9f, 0.4f, 1.0f));
	AppendLog(FString::Printf(TEXT("[Refactor] Renamed %s -> %s"), *FPaths::GetCleanFilename(OldPath), *FPaths::GetCleanFilename(NewPath)));
	UpdateFileHeader();
}

void SCppStudioTab::OnFileDeletedInTree(const FString& DeletedPath)
{
	RefreshProjectFileList();
	if (LeftEditorPane.IsValid())
	{
		LeftEditorPane->CloseFile(DeletedPath);
	}
	if (RightEditorPane.IsValid())
	{
		RightEditorPane->CloseFile(DeletedPath);
	}
	SetStatus(TEXT("Deleted file"), FLinearColor(0.9f, 0.4f, 0.2f, 1.0f));
	UpdateFileHeader();
}

void SCppStudioTab::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	// Check Live Coding compile state transition
	ILiveCodingModule* LiveCoding = FModuleManager::GetModulePtr<ILiveCodingModule>(LIVE_CODING_MODULE_NAME);
	if (LiveCoding)
	{
		const bool bIsCompiling = LiveCoding->IsCompiling();
		if (bWasLiveCodingCompiling && !bIsCompiling)
		{
			// Compiling just finished!
			if (!bPatchSucceeded)
			{
				SetStatus(TEXT("Compile Failed"), FLinearColor(1.0f, 0.3f, 0.3f, 1.0f));
				AppendLog(TEXT("[Live Coding] Compilation failed. See Output Log for compiler errors."));
			}
			bPatchSucceeded = false;
		}
		bWasLiveCodingCompiling = bIsCompiling;
	}
}

void SCppStudioTab::OnPaneDocumentContentChanged(const FString& FilePath, const FString& Content)
{
	UpdateFileHeader();
	if (FCppEditorSettings::Get().bAutoReloadSlatePreview)
	{
		UpdateSlatePreviewIfApplicable();
	}
}

void SCppStudioTab::SaveCurrentFile()
{
	TSharedPtr<SCppEditorPane> ActivePane = ActiveEditorPane.Pin();
	if (ActivePane.IsValid())
	{
		if (ActivePane->SaveCurrentFile())
		{
			SetStatus(TEXT("Saved"), FLinearColor(0.2f, 0.8f, 0.4f, 1.0f));
		}
		else
		{
			SetStatus(TEXT("Save Failed"), FLinearColor(1.0f, 0.3f, 0.3f, 1.0f));
		}
		UpdateFileHeader();
		UpdateSlatePreviewIfApplicable();
	}
}

void SCppStudioTab::SaveAllFiles()
{
	bool bLeftOk = LeftEditorPane.IsValid() ? LeftEditorPane->SaveAll() : true;
	bool bRightOk = RightEditorPane.IsValid() ? RightEditorPane->SaveAll() : true;

	if (bLeftOk && bRightOk)
	{
		SetStatus(TEXT("All Saved"), FLinearColor(0.2f, 0.8f, 0.4f, 1.0f));
		AppendLog(TEXT("[File] All open documents saved."));
	}
	else
	{
		SetStatus(TEXT("Save All Failed"), FLinearColor(1.0f, 0.3f, 0.3f, 1.0f));
	}
	UpdateFileHeader();
	UpdateSlatePreviewIfApplicable();
}

void SCppStudioTab::TriggerLiveCoding()
{
	// 1. Save all files first (if enabled in settings)
	if (FCppEditorSettings::Get().bAutoSaveOnLiveCoding)
	{
		SaveAllFiles();
	}

	// 2. Trigger Live Coding module
	ILiveCodingModule* LiveCoding = FModuleManager::GetModulePtr<ILiveCodingModule>(LIVE_CODING_MODULE_NAME);
	if (LiveCoding)
	{
		if (LiveCoding->CanEnableForSession() && !LiveCoding->IsEnabledForSession())
		{
			LiveCoding->EnableForSession(true);
		}

		bPatchSucceeded = false;
		bWasLiveCodingCompiling = true;
		SetStatus(TEXT("Live Coding Compiling..."), FLinearColor(1.0f, 0.8f, 0.2f, 1.0f));
		AppendLog(TEXT("[Live Coding] Compile requested..."));
		LiveCoding->Compile();
	}
	else
	{
		SetStatus(TEXT("Live Coding Unavailable"), FLinearColor(1.0f, 0.3f, 0.3f, 1.0f));
		AppendLog(TEXT("[Live Coding] Module not found or inactive."));
	}
}

void SCppStudioTab::ToggleSplitView()
{
	if (!EditorAreaSplitter.IsValid() || !LeftEditorPane.IsValid() || !RightEditorPane.IsValid())
	{
		return;
	}

	bIsSplitView = !bIsSplitView;

	EditorAreaSplitter->ClearChildren();

	if (bIsSplitView)
	{
		EditorAreaSplitter->AddSlot()
			.Value(0.5f)
			[
				LeftEditorPane.ToSharedRef()
			];

		EditorAreaSplitter->AddSlot()
			.Value(0.5f)
			[
				RightEditorPane.ToSharedRef()
			];

		// If RightEditorPane has no open files, open corresponding .h or active file
		if (!RightEditorPane->HasOpenFiles())
		{
			FString ActiveLeftFile = LeftEditorPane->GetActiveFilePath();
			if (!ActiveLeftFile.IsEmpty())
			{
				FString Ext = FPaths::GetExtension(ActiveLeftFile).ToLower();
				FString AltFile;
				if (Ext == TEXT("cpp"))
				{
					AltFile = FPaths::ChangeExtension(ActiveLeftFile, TEXT("h"));
				}
				else if (Ext == TEXT("h"))
				{
					AltFile = FPaths::ChangeExtension(ActiveLeftFile, TEXT("cpp"));
				}

				if (!AltFile.IsEmpty() && IFileManager::Get().FileExists(*AltFile))
				{
					RightEditorPane->OpenFile(AltFile);
				}
				else
				{
					RightEditorPane->OpenFile(ActiveLeftFile);
				}
			}
		}

		AppendLog(TEXT("[C++ Studio] Split View activated (Dual Editor Panes)."));
	}
	else
	{
		EditorAreaSplitter->AddSlot()
			.Value(1.0f)
			[
				LeftEditorPane.ToSharedRef()
			];

		ActiveEditorPane = LeftEditorPane;
		AppendLog(TEXT("[C++ Studio] Single Pane mode restored."));
	}

	UpdateFileHeader();
}

void SCppStudioTab::OnPatchComplete()
{
	bPatchSucceeded = true;
	SetStatus(TEXT("Patch Applied!"), FLinearColor(0.2f, 0.9f, 0.4f, 1.0f));
	AppendLog(TEXT("[Live Coding] Patch successfully built and loaded!"));
}

void SCppStudioTab::UpdateFileHeader()
{
	if (!FileHeaderTextBlock.IsValid()) return;

	TSharedPtr<SCppEditorPane> ActivePane = ActiveEditorPane.Pin();
	if (!ActivePane.IsValid())
	{
		ActivePane = LeftEditorPane;
	}

	FString ActivePath = ActivePane.IsValid() ? ActivePane->GetActiveFilePath() : FString();

	if (ActivePath.IsEmpty())
	{
		FileHeaderTextBlock->SetText(FText::FromString(TEXT("No File Selected")));
	}
	else
	{
		FString CleanName = FPaths::GetCleanFilename(ActivePath);
		if (ActivePane->IsCurrentDirty())
		{
			CleanName.Append(TEXT(" *"));
		}
		FileHeaderTextBlock->SetText(FText::FromString(CleanName));
	}
}

void SCppStudioTab::SetStatus(const FString& StatusText, const FLinearColor& StatusColor)
{
	FString CleanText = StatusText;
	CleanText.ReplaceInline(TEXT("✓ "), TEXT(""));
	CleanText.ReplaceInline(TEXT("❌ "), TEXT(""));
	CleanText.ReplaceInline(TEXT("⚡ "), TEXT(""));
	CleanText.ReplaceInline(TEXT("🗑️ "), TEXT(""));
	CleanText.ReplaceInline(TEXT("● "), TEXT(""));

	if (StatusBadgeTextBlock.IsValid())
	{
		StatusBadgeTextBlock->SetText(FText::FromString(CleanText));
		StatusBadgeTextBlock->SetColorAndOpacity(StatusColor);
	}

	if (StatusBadgeIcon.IsValid())
	{
		FName IconBrushName = TEXT("SlateLivePreview.Status.Ready");
		if (StatusText.Contains(TEXT("Compil")) || StatusText.Contains(TEXT("Live Coding")))
		{
			IconBrushName = TEXT("SlateLivePreview.Status.Compiling");
		}
		else if (StatusText.Contains(TEXT("Fail")) || StatusText.Contains(TEXT("Unavailable")) || StatusText.Contains(TEXT("Error")))
		{
			IconBrushName = TEXT("SlateLivePreview.Status.Failed");
		}
		else if (StatusText.Contains(TEXT("Saved")) || StatusText.Contains(TEXT("Applied")) || StatusText.Contains(TEXT("Created")) || StatusText.Contains(TEXT("Renamed")) || StatusText.Contains(TEXT("Success")))
		{
			IconBrushName = TEXT("SlateLivePreview.Status.Success");
		}
		else if (StatusText.Contains(TEXT("Delete")))
		{
			IconBrushName = TEXT("SlateLivePreview.Delete");
		}
		StatusBadgeIcon->SetImage(FSlateLivePreviewStyle::GetBrush(IconBrushName));
	}
}

void SCppStudioTab::AppendLog(const FString& LogLine)
{
	OutputConsoleAccumulator.Append(LogLine);
	OutputConsoleAccumulator.Append(TEXT("\n"));

	if (OutputConsoleAccumulator.Len() > 8000)
	{
		OutputConsoleAccumulator = OutputConsoleAccumulator.Right(5000);
	}

	if (OutputConsoleTextBox.IsValid())
	{
		OutputConsoleTextBox->SetText(FText::FromString(OutputConsoleAccumulator));
	}

	CheckAndParseErrorLine(LogLine);
}

void SCppStudioTab::UpdateSlatePreviewIfApplicable()
{
	if (!bShowSlatePreview || !SlatePreviewViewport.IsValid())
	{
		return;
	}

	// Prefer active pane, or fallback to any pane with Slate code
	TSharedPtr<SCppEditorPane> TargetPane = ActiveEditorPane.Pin();
	if (!TargetPane.IsValid()) TargetPane = LeftEditorPane;

	FString Code = TargetPane.IsValid() ? TargetPane->GetActiveContent() : FString();

	// If current pane does not have SNew, check the other pane
	if (!Code.Contains(TEXT("SNew(")) && !Code.Contains(TEXT("SAssignNew(")))
	{
		if (TargetPane == LeftEditorPane && RightEditorPane.IsValid())
		{
			Code = RightEditorPane->GetActiveContent();
		}
		else if (TargetPane == RightEditorPane && LeftEditorPane.IsValid())
		{
			Code = LeftEditorPane->GetActiveContent();
		}
	}

	if (Code.Contains(TEXT("SNew(")) || Code.Contains(TEXT("SAssignNew(")))
	{
		TSharedPtr<FSlateWidgetNode> RootAst;
		TArray<FString> Errors;
		TArray<FString> Warnings;

		bool bParsed = FSlateCodeParser::ParseSource(Code, RootAst, Errors, Warnings);
		if (bParsed && RootAst.IsValid())
		{
			FSlateWidgetBuilder Builder;
			Builder.SetLogHandler(FOnSlatePreviewLog::CreateSP(this, &SCppStudioTab::AppendLog));
			TSharedRef<SWidget> LiveWidget = Builder.Build(RootAst);
			SlatePreviewViewport->SetPreviewWidget(LiveWidget);
		}
		else if (Errors.Num() > 0)
		{
			TSharedRef<SWidget> ErrorWidget = FSlateWidgetBuilder::BuildErrorWidget(Errors, Warnings);
			SlatePreviewViewport->SetPreviewWidget(ErrorWidget);
		}
	}
}

FReply SCppStudioTab::OnSaveClicked()
{
	SaveCurrentFile();
	return FReply::Handled();
}

FReply SCppStudioTab::OnSaveAllClicked()
{
	SaveAllFiles();
	return FReply::Handled();
}

FReply SCppStudioTab::OnLiveCodingClicked()
{
	TriggerLiveCoding();
	return FReply::Handled();
}

FReply SCppStudioTab::OnRevertClicked()
{
	TSharedPtr<SCppEditorPane> ActivePane = ActiveEditorPane.Pin();
	if (ActivePane.IsValid())
	{
		ActivePane->RevertCurrentFile();
		UpdateFileHeader();
		UpdateSlatePreviewIfApplicable();
	}
	return FReply::Handled();
}

FReply SCppStudioTab::OnToggleSplitClicked()
{
	ToggleSplitView();
	return FReply::Handled();
}

FReply SCppStudioTab::OnTogglePreviewClicked()
{
	bShowSlatePreview = !bShowSlatePreview;
	if (PreviewPanelBorder.IsValid())
	{
		PreviewPanelBorder->SetVisibility(bShowSlatePreview ? EVisibility::Visible : EVisibility::Collapsed);
	}
	return FReply::Handled();
}

void SCppStudioTab::ToggleSlatePreview(bool bShow)
{
	bShowSlatePreview = bShow;
	if (PreviewPanelBorder.IsValid())
	{
		PreviewPanelBorder->SetVisibility(bShow ? EVisibility::Visible : EVisibility::Collapsed);
	}
}

void SCppStudioTab::OpenNewClassWizard()
{
	FString TargetDir = ProjectTree.IsValid() ? ProjectTree->GetSelectedDirectory() : FString();
	SCreateClassDialog::OpenModal(TargetDir, FOnClassCreated::CreateSP(this, &SCppStudioTab::OnClassCreated));
}

void SCppStudioTab::OnClassCreated(const FString& HeaderPath, const FString& CppPath)
{
	if (ProjectTree.IsValid())
	{
		ProjectTree->RefreshTree();
	}
	RefreshProjectFileList();

	if (bIsSplitView && LeftEditorPane.IsValid() && RightEditorPane.IsValid())
	{
		if (IFileManager::Get().FileExists(*HeaderPath))
		{
			LeftEditorPane->OpenFile(HeaderPath);
		}
		if (IFileManager::Get().FileExists(*CppPath))
		{
			RightEditorPane->OpenFile(CppPath);
		}
		ActiveEditorPane = LeftEditorPane;
	}
	else
	{
		if (IFileManager::Get().FileExists(*CppPath))
		{
			OpenFile(CppPath);
		}
		if (IFileManager::Get().FileExists(*HeaderPath))
		{
			OpenFile(HeaderPath);
		}
	}

	SetStatus(FString::Printf(TEXT("Created: %s"), *FPaths::GetBaseFilename(HeaderPath)), FLinearColor(0.2f, 0.85f, 0.4f, 1.0f));
	AppendLog(FString::Printf(TEXT("[Wizard] Successfully created %s and %s"), *FPaths::GetCleanFilename(HeaderPath), *FPaths::GetCleanFilename(CppPath)));
}

void SCppStudioTab::ToggleQuickOpen(bool bShow)
{
	bQuickOpenVisible = bShow;
	if (bShow)
	{
		RefreshProjectFileList();
		if (QuickOpenSearchBox.IsValid())
		{
			QuickOpenSearchBox->SetText(FText::GetEmpty());
			FSlateApplication::Get().SetKeyboardFocus(QuickOpenSearchBox);
		}
		if (FilteredQuickOpenFiles.Num() > 0 && QuickOpenListView.IsValid())
		{
			QuickOpenListView->SetSelection(FilteredQuickOpenFiles[0]);
		}
	}
	else
	{
		TSharedPtr<SCppEditorPane> ActivePane = ActiveEditorPane.Pin();
		if (ActivePane.IsValid())
		{
			ActivePane->FocusEditor();
		}
	}
}

void SCppStudioTab::RefreshProjectFileList()
{
	AllProjectFiles.Empty();
	TArray<FString> FilePaths;
	if (ProjectTree.IsValid())
	{
		ProjectTree->GetAllFiles(FilePaths);
	}
	for (const FString& Path : FilePaths)
	{
		AllProjectFiles.Add(MakeShared<FString>(Path));
	}
	FilteredQuickOpenFiles = AllProjectFiles;
	if (QuickOpenListView.IsValid())
	{
		QuickOpenListView->RequestListRefresh();
	}
}

void SCppStudioTab::OnQuickOpenSearchTextChanged(const FText& SearchText)
{
	FString Query = SearchText.ToString().TrimStartAndEnd();
	if (Query.IsEmpty())
	{
		FilteredQuickOpenFiles = AllProjectFiles;
	}
	else
	{
		FilteredQuickOpenFiles.Empty();
		TArray<TSharedPtr<FString>> ExactMatches;
		TArray<TSharedPtr<FString>> PathMatches;

		for (const TSharedPtr<FString>& Item : AllProjectFiles)
		{
			if (!Item.IsValid()) continue;
			FString Filename = FPaths::GetCleanFilename(*Item);
			if (Filename.Contains(Query, ESearchCase::IgnoreCase))
			{
				ExactMatches.Add(Item);
			}
			else if (Item->Contains(Query, ESearchCase::IgnoreCase))
			{
				PathMatches.Add(Item);
			}
		}

		FilteredQuickOpenFiles.Append(ExactMatches);
		FilteredQuickOpenFiles.Append(PathMatches);
	}

	if (QuickOpenListView.IsValid())
	{
		QuickOpenListView->RequestListRefresh();
		if (FilteredQuickOpenFiles.Num() > 0)
		{
			QuickOpenListView->SetSelection(FilteredQuickOpenFiles[0]);
		}
	}
}

void SCppStudioTab::CommitQuickOpenSelection()
{
	if (FilteredQuickOpenFiles.Num() > 0 && QuickOpenListView.IsValid())
	{
		TArray<TSharedPtr<FString>> Selected = QuickOpenListView->GetSelectedItems();
		TSharedPtr<FString> Target = (Selected.Num() > 0) ? Selected[0] : FilteredQuickOpenFiles[0];
		if (Target.IsValid())
		{
			OpenFile(*Target);
		}
	}
	ToggleQuickOpen(false);
}

void SCppStudioTab::QuickOpenNavigate(int32 Direction)
{
	if (FilteredQuickOpenFiles.Num() == 0 || !QuickOpenListView.IsValid())
	{
		return;
	}

	TArray<TSharedPtr<FString>> Selected = QuickOpenListView->GetSelectedItems();
	int32 CurrentIndex = 0;
	if (Selected.Num() > 0)
	{
		CurrentIndex = FilteredQuickOpenFiles.Find(Selected[0]);
		if (CurrentIndex == INDEX_NONE) CurrentIndex = 0;
	}

	int32 NextIndex = FMath::Clamp(CurrentIndex + Direction, 0, FilteredQuickOpenFiles.Num() - 1);
	QuickOpenListView->SetSelection(FilteredQuickOpenFiles[NextIndex]);
	QuickOpenListView->RequestScrollIntoView(FilteredQuickOpenFiles[NextIndex]);
}

TSharedRef<ITableRow> SCppStudioTab::OnGenerateQuickOpenRow(TSharedPtr<FString> FileItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	if (!FileItem.IsValid())
	{
		return SNew(STableRow<TSharedPtr<FString>>, OwnerTable);
	}

	FString FilePath = *FileItem;
	FString CleanFilename = FPaths::GetCleanFilename(FilePath);
	FString DirPath = FPaths::GetPath(FilePath);
	FString Ext = FPaths::GetExtension(FilePath).ToLower();

	FLinearColor BadgeColor = (Ext == TEXT("h") || Ext == TEXT("hpp")) 
		? FLinearColor(0.2f, 0.75f, 0.95f, 1.0f)
		: FLinearColor(0.95f, 0.65f, 0.2f, 1.0f);

	FString BadgeText = (Ext == TEXT("h") || Ext == TEXT("hpp")) ? TEXT("H") : TEXT("CPP");

	return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
		.Padding(FMargin(4.0f, 3.0f))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				.BorderBackgroundColor(BadgeColor * 0.35f)
				.Padding(FMargin(4.0f, 1.0f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(BadgeText))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7.5f))
					.ColorAndOpacity(BadgeColor)
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(CleanFilename))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9.5f))
				.ColorAndOpacity(FLinearColor::White)
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(DirPath))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.0f))
				.ColorAndOpacity(FLinearColor(0.55f, 0.55f, 0.55f, 1.0f))
			]
		];
}

void SCppStudioTab::CheckAndParseErrorLine(const FString& InLine)
{
	FString Line = InLine.TrimStartAndEnd();
	if (Line.IsEmpty()) return;

	int32 ErrorIdx = Line.Find(TEXT("error"), ESearchCase::IgnoreCase);
	int32 FatalIdx = Line.Find(TEXT("fatal error"), ESearchCase::IgnoreCase);
	if (ErrorIdx == INDEX_NONE && FatalIdx == INDEX_NONE)
	{
		return;
	}

	FString ParsedFile;
	int32 ParsedLine = 0;
	int32 ParsedCol = 1;

	// Pattern 1: MSVC File(Line) or File(Line,Col)
	int32 ParenOpen = Line.Find(TEXT("("));
	int32 ParenClose = (ParenOpen != INDEX_NONE) ? Line.Find(TEXT(")"), ESearchCase::IgnoreCase, ESearchDir::FromStart, ParenOpen) : INDEX_NONE;
	if (ParenOpen != INDEX_NONE && ParenClose != INDEX_NONE && ParenClose > ParenOpen && ParenClose < ErrorIdx)
	{
		FString CandidateFile = Line.Left(ParenOpen).TrimEnd();
		int32 BracketClose = CandidateFile.Find(TEXT("] "));
		if (BracketClose != INDEX_NONE)
		{
			CandidateFile = CandidateFile.Mid(BracketClose + 2).TrimStart();
		}

		FString InsideParen = Line.Mid(ParenOpen + 1, ParenClose - ParenOpen - 1);
		int32 CommaIdx = InsideParen.Find(TEXT(","));
		if (CommaIdx != INDEX_NONE)
		{
			ParsedLine = FCString::Atoi(*InsideParen.Left(CommaIdx));
			ParsedCol = FCString::Atoi(*InsideParen.Mid(CommaIdx + 1));
		}
		else
		{
			ParsedLine = FCString::Atoi(*InsideParen);
		}

		ParsedFile = CandidateFile;
	}
	else
	{
		// Pattern 2: Clang/GCC File:Line:Col: error:
		int32 Colon1 = Line.Find(TEXT(":"));
		if (Colon1 != INDEX_NONE && Colon1 + 2 < Line.Len() && Line[Colon1 + 1] == TEXT('\\'))
		{
			Colon1 = Line.Find(TEXT(":"), ESearchCase::IgnoreCase, ESearchDir::FromStart, Colon1 + 2);
		}

		if (Colon1 != INDEX_NONE && Colon1 < ErrorIdx)
		{
			int32 Colon2 = Line.Find(TEXT(":"), ESearchCase::IgnoreCase, ESearchDir::FromStart, Colon1 + 1);
			if (Colon2 != INDEX_NONE && Colon2 < ErrorIdx)
			{
				ParsedFile = Line.Left(Colon1).TrimEnd();
				ParsedLine = FCString::Atoi(*Line.Mid(Colon1 + 1, Colon2 - Colon1 - 1));
				int32 Colon3 = Line.Find(TEXT(":"), ESearchCase::IgnoreCase, ESearchDir::FromStart, Colon2 + 1);
				if (Colon3 != INDEX_NONE && Colon3 <= ErrorIdx)
				{
					ParsedCol = FCString::Atoi(*Line.Mid(Colon2 + 1, Colon3 - Colon2 - 1));
				}
			}
		}
	}

	if (!ParsedFile.IsEmpty() && ParsedLine > 0)
	{
		if (FPaths::IsRelative(ParsedFile) || !IFileManager::Get().FileExists(*ParsedFile))
		{
			FString CleanName = FPaths::GetCleanFilename(ParsedFile);
			RefreshProjectFileList();
			for (const TSharedPtr<FString>& FileItem : AllProjectFiles)
			{
				if (FileItem.IsValid() && (FileItem->EndsWith(CleanName) || FileItem->EndsWith(ParsedFile)))
				{
					ParsedFile = *FileItem;
					break;
				}
			}
		}

		if (IFileManager::Get().FileExists(*ParsedFile))
		{
			LastParsedErrorFile = ParsedFile;
			LastParsedErrorLine = ParsedLine;
			LastParsedErrorCol = ParsedCol;

			if (ErrorJumpButton.IsValid())
			{
				ErrorJumpButton->SetVisibility(EVisibility::Visible);
			}
			if (ErrorJumpTextBlock.IsValid())
			{
				ErrorJumpTextBlock->SetText(FText::FromString(FString::Printf(TEXT("%s:%d (F4)"), *FPaths::GetCleanFilename(LastParsedErrorFile), LastParsedErrorLine)));
			}
		}
	}
}

FReply SCppStudioTab::OnErrorJumpClicked()
{
	if (!LastParsedErrorFile.IsEmpty())
	{
		OpenFileAtLine(LastParsedErrorFile, LastParsedErrorLine, LastParsedErrorCol);
		AppendLog(FString::Printf(TEXT("[Console] Jumped to %s:%d"), *FPaths::GetCleanFilename(LastParsedErrorFile), LastParsedErrorLine));
	}
	return FReply::Handled();
}

FReply SCppStudioTab::OnNewClassClicked()
{
	OpenNewClassWizard();
	return FReply::Handled();
}

FReply SCppStudioTab::OnQuickOpenClicked()
{
	ToggleQuickOpen(true);
	return FReply::Handled();
}

FReply SCppStudioTab::OnSettingsClicked()
{
	OpenSettingsDialog();
	return FReply::Handled();
}

void SCppStudioTab::OpenSettingsDialog()
{
	SCppSettingsDialog::OpenModal(AsShared());
}

FReply SCppStudioTab::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (bQuickOpenVisible)
	{
		if (Key == EKeys::Escape)
		{
			ToggleQuickOpen(false);
			return FReply::Handled();
		}
		if (Key == EKeys::Up)
		{
			QuickOpenNavigate(-1);
			return FReply::Handled();
		}
		if (Key == EKeys::Down)
		{
			QuickOpenNavigate(1);
			return FReply::Handled();
		}
		if (Key == EKeys::Enter)
		{
			CommitQuickOpenSelection();
			return FReply::Handled();
		}
	}

	if (Key == EKeys::F4)
	{
		OnErrorJumpClicked();
		return FReply::Handled();
	}

	if (InKeyEvent.IsControlDown() && Key == EKeys::Comma)
	{
		OpenSettingsDialog();
		return FReply::Handled();
	}

	if (InKeyEvent.IsControlDown() && Key == EKeys::P)
	{
		ToggleQuickOpen(!bQuickOpenVisible);
		return FReply::Handled();
	}
	if (InKeyEvent.IsControlDown() && Key == EKeys::N)
	{
		OpenNewClassWizard();
		return FReply::Handled();
	}

	if (InKeyEvent.IsControlDown() && Key == EKeys::S)
	{
		SaveCurrentFile();
		return FReply::Handled();
	}
	if (InKeyEvent.IsControlDown() && Key == EKeys::B)
	{
		TriggerLiveCoding();
		return FReply::Handled();
	}
	if (InKeyEvent.IsControlDown() && InKeyEvent.IsAltDown() && Key == EKeys::F11)
	{
		TriggerLiveCoding();
		return FReply::Handled();
	}

	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}
