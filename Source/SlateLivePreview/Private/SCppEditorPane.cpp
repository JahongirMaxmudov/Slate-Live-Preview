// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#include "SCppEditorPane.h"
#include "SlateLivePreviewStyle.h"
#include "CppSyntaxHighlighter.h"
#include "CppEditorSettings.h"
#include "CppAiAssistant.h"
#include "Fonts/FontMeasure.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SOverlay.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Rendering/DrawElements.h"

class SCppEditorGutter : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCppEditorGutter) {}
		SLATE_ARGUMENT(TSharedPtr<SMultiLineEditableTextBox>, TargetTextBox)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, SCppEditorPane* InOwnerPane)
	{
		OwnerPane = InOwnerPane;
		TargetTextBox = InArgs._TargetTextBox;
		SetClipping(EWidgetClipping::ClipToBounds);
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D(42.0f, 100.0f);
	}

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (OwnerPane && TargetTextBox.IsValid())
		{
			FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
			const float LineHeight = 18.0f;
			int32 TotalLines = OwnerPane->GetTotalLineCount();
			int32 FirstLine = 0;
			if (TargetTextBox.Pin()->GetVScrollBar().IsValid())
			{
				float ScrollFraction = TargetTextBox.Pin()->GetVScrollBar()->DistanceFromTop();
				FirstLine = FMath::Clamp(FMath::FloorToInt(ScrollFraction * TotalLines), 0, FMath::Max(0, TotalLines - 1));
			}
			int32 ClickedLine = FirstLine + FMath::FloorToInt((LocalPos.Y - 4.0f) / LineHeight);
			if (ClickedLine >= 0 && ClickedLine < TotalLines)
			{
				TargetTextBox.Pin()->GoTo(FTextLocation(ClickedLine, 0));
				OwnerPane->FocusEditor();
				return FReply::Handled();
			}
		}
		return FReply::Unhandled();
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		// 1. Dark Gutter Background
		const FSlateBrush* BackgroundBrush = FAppStyle::Get().GetBrush("ToolPanel.GroupBorder");
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			BackgroundBrush,
			ESlateDrawEffect::None,
			FCppEditorSettings::Get().GetGutterBackgroundColor()
		);

		// 2. Right dividing border line
		float Width = AllottedGeometry.GetLocalSize().X;
		float Height = AllottedGeometry.GetLocalSize().Y;
		TArray<FVector2D> BorderPoints;
		BorderPoints.Add(FVector2D(Width - 1.0f, 0.0f));
		BorderPoints.Add(FVector2D(Width - 1.0f, Height));
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToPaintGeometry(),
			BorderPoints,
			ESlateDrawEffect::None,
			FLinearColor(0.20f, 0.20f, 0.22f, 1.0f),
			true,
			1.0f
		);

		if (!OwnerPane || !TargetTextBox.IsValid())
		{
			return LayerId + 2;
		}

		int32 TotalLines = OwnerPane->GetTotalLineCount();
		if (TotalLines <= 0)
		{
			return LayerId + 2;
		}

		int32 CurrentLine = OwnerPane->GetCurrentLineIndex();
		const float LineHeight = 18.0f;

		int32 FirstLine = 0;
		if (TargetTextBox.Pin()->GetVScrollBar().IsValid())
		{
			float ScrollFraction = TargetTextBox.Pin()->GetVScrollBar()->DistanceFromTop();
			FirstLine = FMath::Clamp(FMath::FloorToInt(ScrollFraction * TotalLines), 0, FMath::Max(0, TotalLines - 1));
		}

		int32 VisibleLines = FMath::CeilToInt(Height / LineHeight) + 2;
		int32 LastLine = FMath::Min(TotalLines - 1, FirstLine + VisibleLines);

		FSlateFontInfo FontInfo = FCppEditorSettings::Get().GetFont(9.0f);

		for (int32 LineIdx = FirstLine; LineIdx <= LastLine; ++LineIdx)
		{
			float Y = (LineIdx - FirstLine) * LineHeight + 4.0f;
			if (Y + LineHeight > Height)
			{
				break;
			}
			if (Y < 0.0f)
			{
				continue;
			}
			FString LineNumberStr = FString::FromInt(LineIdx + 1);

			bool bIsCurrent = (LineIdx == CurrentLine);
			if (bIsCurrent && FCppEditorSettings::Get().bHighlightActiveLine)
			{
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					LayerId + 1,
					AllottedGeometry.ToPaintGeometry(FVector2f(Width, LineHeight), FSlateLayoutTransform(FVector2f(0.0f, Y))),
					FAppStyle::Get().GetBrush("WhiteBrush"),
					ESlateDrawEffect::None,
					FLinearColor(0.20f, 0.35f, 0.55f, 0.30f)
				);
			}

			FLinearColor TextColor = bIsCurrent ? FLinearColor(0.95f, 0.95f, 1.0f, 1.0f) : FLinearColor(0.40f, 0.42f, 0.46f, 0.85f);

			float TextX = Width - 8.0f - (LineNumberStr.Len() * 6.5f);

			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId + 2,
				AllottedGeometry.ToPaintGeometry(FVector2f(Width, LineHeight), FSlateLayoutTransform(FVector2f(TextX, Y))),
				LineNumberStr,
				FontInfo,
				ESlateDrawEffect::None,
				TextColor
			);
		}

		return LayerId + 3;
	}

private:
	SCppEditorPane* OwnerPane = nullptr;
	TWeakPtr<SMultiLineEditableTextBox> TargetTextBox;
};

class SCppEditorGhostOverlay : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCppEditorGhostOverlay) {}
		SLATE_ARGUMENT(TSharedPtr<SMultiLineEditableTextBox>, TargetTextBox)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, SCppEditorPane* InOwnerPane)
	{
		OwnerPane = InOwnerPane;
		TargetTextBox = InArgs._TargetTextBox;
		SetVisibility(EVisibility::HitTestInvisible);
		SetClipping(EWidgetClipping::ClipToBounds);
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		if (!OwnerPane || !OwnerPane->HasGhostText() || !TargetTextBox.IsValid())
		{
			return LayerId;
		}

		FString GhostText = OwnerPane->GetActiveGhostText();
		FTextLocation GhostLoc = OwnerPane->GetGhostTextLocation();
		int32 GhostLine = GhostLoc.GetLineIndex();

		int32 TotalLines = OwnerPane->GetTotalLineCount();
		if (TotalLines <= 0)
		{
			return LayerId;
		}

		TSharedPtr<SMultiLineEditableTextBox> TextBox = TargetTextBox.Pin();
		const float LineHeight = 18.0f;
		int32 FirstLine = 0;
		if (TextBox->GetVScrollBar().IsValid())
		{
			float ScrollFraction = TextBox->GetVScrollBar()->DistanceFromTop();
			FirstLine = FMath::Clamp(FMath::FloorToInt(ScrollFraction * TotalLines), 0, FMath::Max(0, TotalLines - 1));
		}

		float Y = (GhostLine - FirstLine) * LineHeight + 4.0f;
		float Height = AllottedGeometry.GetLocalSize().Y;
		if (Y < 0.0f || Y + LineHeight > Height)
		{
			return LayerId;
		}

		FSlateFontInfo FontInfo = FCppEditorSettings::Get().GetFont();
		TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

		FString ActiveContent = OwnerPane->GetActiveContent();
		TArray<FString> ContentLines;
		ActiveContent.ParseIntoArrayLines(ContentLines, false);

		float PrefixWidth = 0.0f;
		if (ContentLines.IsValidIndex(GhostLine))
		{
			FString LineText = ContentLines[GhostLine];
			int32 Offset = FMath::Clamp(GhostLoc.GetOffset(), 0, LineText.Len());
			FString Prefix = LineText.Left(Offset);
			Prefix.ReplaceInline(TEXT("\t"), TEXT("    "));
			PrefixWidth = (float)FontMeasure->Measure(Prefix, FontInfo).X;
		}

		float StartX = PrefixWidth + 4.0f;

		TArray<FString> GhostLines;
		GhostText.ParseIntoArrayLines(GhostLines, false);
		if (GhostLines.Num() == 0)
		{
			GhostLines.Add(GhostText);
		}

		const FLinearColor GhostColor(0.52f, 0.55f, 0.62f, 0.70f);

		for (int32 i = 0; i < GhostLines.Num(); ++i)
		{
			float LineY = Y + i * LineHeight;
			if (LineY + LineHeight > Height)
			{
				break;
			}
			float LineX = (i == 0) ? StartX : 4.0f;
			FString DisplayLine = GhostLines[i];
			DisplayLine.ReplaceInline(TEXT("\t"), TEXT("    "));

			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId + 5,
				AllottedGeometry.ToPaintGeometry(FVector2f(AllottedGeometry.GetLocalSize().X - LineX, LineHeight), FSlateLayoutTransform(FVector2f(LineX, LineY))),
				DisplayLine,
				FontInfo,
				ESlateDrawEffect::None,
				GhostColor
			);
		}

		return LayerId + 6;
	}

private:
	SCppEditorPane* OwnerPane = nullptr;
	TWeakPtr<SMultiLineEditableTextBox> TargetTextBox;
};

void SCppEditorPane::Construct(const FArguments& InArgs)
{
	OnActiveDocumentChanged = InArgs._OnActiveDocumentChanged;
	OnDocumentContentChanged = InArgs._OnDocumentContentChanged;
	OnPaneLog = InArgs._OnPaneLog;
	OnQuickOpenRequested = InArgs._OnQuickOpenRequested;
	OnNewClassRequested = InArgs._OnNewClassRequested;
	OnPaneFocused = InArgs._OnPaneFocused;
	OnMoveDocumentRequested = InArgs._OnMoveDocumentRequested;

	const FCppEditorSettings& Settings = FCppEditorSettings::Get();
	SyntaxMarshaller = FCppSyntaxHighlighterMarshaller::Create(Settings.GetSyntaxStyle());

	CodeTextBox = SNew(SMultiLineEditableTextBox)
		.Marshaller(SyntaxMarshaller)
		.Font_Lambda([]() { return FCppEditorSettings::Get().GetFont(); })
		.AutoWrapText(Settings.bWordWrap)
		.ClearKeyboardFocusOnCommit(false)
		.ModiferKeyForNewLine(EModifierKey::None)
		.ClearTextSelectionOnFocusLoss(false)
		.SelectAllTextWhenFocused(false)
		.OnIsTypedCharValid_Lambda([](const TCHAR) { return true; })
		.OnKeyDownHandler(this, &SCppEditorPane::HandleCodeTextBoxKeyDown)
		.OnKeyCharHandler(this, &SCppEditorPane::HandleCodeTextBoxKeyChar)
		.OnContextMenuOpening(this, &SCppEditorPane::OnEditorContextMenuOpening)
		.OnCursorMoved_Lambda([this](const FTextLocation&)
		{
			if (bIntelliSenseActive)
			{
				UpdateIntelliSense();
			}
		})
		.OnTextChanged(this, &SCppEditorPane::OnCodeTextChanged);

	SAssignNew(GutterWidget, SCppEditorGutter, this)
		.TargetTextBox(CodeTextBox);
	GutterWidget->SetVisibility(Settings.bShowLineNumbers ? EVisibility::Visible : EVisibility::Collapsed);

	SettingsChangedHandle = FCppEditorSettings::Get().OnSettingsChanged.AddSP(this, &SCppEditorPane::ApplySettings);

	ChildSlot
	[
		SAssignNew(PaneContainerBorder, SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.BorderBackgroundColor_Lambda([this]()
		{
			return bIsActivePane ? FLinearColor(0.0f, 0.48f, 0.8f, 0.85f) : FLinearColor(0.12f, 0.12f, 0.14f, 0.35f);
		})
		.Padding(1.5f)
		[
			SNew(SVerticalBox)

			// ---------------------------------------------------------------------
			// 1. Tab Bar & Pane Controls
			// ---------------------------------------------------------------------
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
				.Padding(FMargin(2.0f, 2.0f, 2.0f, 0.0f))
				[
					SNew(SHorizontalBox)

				// Scrollable Tab Strip
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(SScrollBox)
					.Orientation(Orient_Horizontal)
					+ SScrollBox::Slot()
					[
						SAssignNew(TabStripBox, SHorizontalBox)
					]
				]

				// Mini Pane Action Buttons (Undo, Redo, Find)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(4.0f, 1.0f)
				[
					SNew(SHorizontalBox)

					// Undo
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(1.0f, 0.0f)
					[
						SNew(SButton)
						.ButtonStyle(FAppStyle::Get(), "SimpleButton")
						.ContentPadding(FMargin(2.0f))
						.ToolTipText(FText::FromString(TEXT("Undo (Ctrl+Z)")))
						.OnClicked_Lambda([this]() -> FReply
						{
							Undo();
							return FReply::Handled();
						})
						[
							SNew(SImage)
							.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.Undo")))
							.DesiredSizeOverride(FVector2D(12.0f, 12.0f))
						]
					]

					// Redo
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(1.0f, 0.0f)
					[
						SNew(SButton)
						.ButtonStyle(FAppStyle::Get(), "SimpleButton")
						.ContentPadding(FMargin(2.0f))
						.ToolTipText(FText::FromString(TEXT("Redo (Ctrl+Y)")))
						.OnClicked_Lambda([this]() -> FReply
						{
							Redo();
							return FReply::Handled();
						})
						[
							SNew(SImage)
							.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.Redo")))
							.DesiredSizeOverride(FVector2D(12.0f, 12.0f))
						]
					]

					// Find
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(1.0f, 0.0f)
					[
						SNew(SButton)
						.ButtonStyle(FAppStyle::Get(), "SimpleButton")
						.ContentPadding(FMargin(2.0f))
						.ToolTipText(FText::FromString(TEXT("Find in file (Ctrl+F)")))
						.OnClicked_Lambda([this]() -> FReply
						{
							ToggleFindBar(!bFindBarVisible);
							return FReply::Handled();
						})
						[
							SNew(SImage)
							.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.Find")))
							.DesiredSizeOverride(FVector2D(12.0f, 12.0f))
						]
					]
				]
			]
		]

		// ---------------------------------------------------------------------
		// 2. Collapsible Find Bar (Ctrl+F)
		// ---------------------------------------------------------------------
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(FindBarBorder, SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			.Visibility(EVisibility::Collapsed)
			.Padding(4.0f)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(4.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.0f, 0.0f, 4.0f, 0.0f)
					[
						SNew(SImage)
						.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.Find")))
						.DesiredSizeOverride(FVector2D(12.0f, 12.0f))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Find:")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					]
				]

				+ SHorizontalBox::Slot()
				.FillWidth(0.5f)
				.VAlign(VAlign_Center)
				.Padding(4.0f, 0.0f)
				[
					SAssignNew(FindSearchBox, SSearchBox)
					.HintText(FText::FromString(TEXT("Search text...")))
					.OnTextChanged_Lambda([this](const FText& InText)
					{
						CurrentSearchQuery = InText.ToString();
						PerformSearch(false);
					})
					.OnTextCommitted_Lambda([this](const FText& InText, ETextCommit::Type CommitType)
					{
						if (CommitType == ETextCommit::OnEnter)
						{
							FindNext();
						}
					})
				]

				// Previous Match
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.0f, 0.0f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.ContentPadding(FMargin(4.0f, 2.0f))
					.ToolTipText(FText::FromString(TEXT("Previous Match (Shift+Enter)")))
					.OnClicked_Lambda([this]() -> FReply
					{
						FindPrevious();
						return FReply::Handled();
					})
					[
						SNew(SImage)
						.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.ArrowUp")))
						.DesiredSizeOverride(FVector2D(10.0f, 10.0f))
					]
				]

				// Next Match
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.0f, 0.0f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.ContentPadding(FMargin(4.0f, 2.0f))
					.ToolTipText(FText::FromString(TEXT("Next Match (Enter)")))
					.OnClicked_Lambda([this]() -> FReply
					{
						FindNext();
						return FReply::Handled();
					})
					[
						SNew(SImage)
						.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.ArrowDown")))
						.DesiredSizeOverride(FVector2D(10.0f, 10.0f))
					]
				]

				// Match Count Indicator
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(6.0f, 0.0f)
				[
					SAssignNew(MatchCountTextBlock, STextBlock)
					.Text(FText::FromString(TEXT("0 of 0")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f))
				]

				// Match Case
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(4.0f, 0.0f)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this]() { return bMatchCase ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
					{
						bMatchCase = (NewState == ECheckBoxState::Checked);
						PerformSearch();
					})
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Aa")))
						.ToolTipText(FText::FromString(TEXT("Match Case")))
					]
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SSpacer)
				]

				// Close Find Bar
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.0f, 0.0f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.ContentPadding(FMargin(2.0f))
					.ToolTipText(FText::FromString(TEXT("Close Find Bar (Esc)")))
					.OnClicked_Lambda([this]() -> FReply
					{
						ToggleFindBar(false);
						return FReply::Handled();
					})
					[
						SNew(SImage)
						.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.CloseTab")))
						.DesiredSizeOverride(FVector2D(10.0f, 10.0f))
					]
				]
			]
		]

		// ---------------------------------------------------------------------
		// 3. Breadcrumbs Bar (Project > Module > Folder > File)
		// ---------------------------------------------------------------------
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
			.BorderBackgroundColor(FLinearColor(0.08f, 0.08f, 0.10f, 0.95f))
			.Padding(FMargin(8.0f, 3.0f))
			[
				SAssignNew(BreadcrumbsBox, SHorizontalBox)
			]
		]

		// ---------------------------------------------------------------------
		// 4. Code Editor & IntelliSense Overlay
		// ---------------------------------------------------------------------
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(0.0f)
		[
			SNew(SBorder)
			.BorderBackgroundColor_Lambda([]() { return FCppEditorSettings::Get().GetEditorBackgroundColor(); })
			.Padding(2.0f)
			[
				SNew(SOverlay)

				// Base code editor with line number gutter and ghost overlay
				+ SOverlay::Slot()
				[
					SNew(SHorizontalBox)

					// Gutter with line numbers
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						GutterWidget.ToSharedRef()
					]

					// Code Editor with Ghost Text Overlay
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						[
							CodeTextBox.ToSharedRef()
						]
						+ SOverlay::Slot()
						[
							SAssignNew(GhostOverlayWidget, SCppEditorGhostOverlay, this)
							.TargetTextBox(CodeTextBox)
						]
					]
				]

				// Floating IntelliSense popup
				+ SOverlay::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Top)
				.Padding(TAttribute<FMargin>::CreateSP(this, &SCppEditorPane::GetIntelliSenseMargin))
				[
					SAssignNew(IntelliSenseBox, SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("Menu.Background"))
					.BorderBackgroundColor(FLinearColor(0.10f, 0.10f, 0.12f, 0.98f))
					.Padding(2.0f)
					.Visibility(EVisibility::Collapsed)
					[
						SNew(SHorizontalBox)

						// 1. Suggestions List
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SBox)
							.WidthOverride(260.0f)
							.MaxDesiredHeight(260.0f)
							[
								SAssignNew(IntelliSenseListView, SListView<TSharedPtr<FIntelliSenseItem>>)
								.ListItemsSource(&FilteredIntelliSenseItems)
								.OnGenerateRow(this, &SCppEditorPane::OnGenerateIntelliSenseRow)
								.OnSelectionChanged(this, &SCppEditorPane::OnIntelliSenseSelectionChanged)
								.OnMouseButtonDoubleClick(this, &SCppEditorPane::OnIntelliSenseItemDoubleClicked)
								.SelectionMode(ESelectionMode::Single)
							]
						]

						// 2. Documentation & Signature Flyout Panel
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SAssignNew(IntelliSenseDocBox, SBorder)
							.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
							.BorderBackgroundColor(FLinearColor(0.13f, 0.13f, 0.15f, 1.0f))
							.Padding(FMargin(8.0f, 6.0f))
							.Visibility(EVisibility::Collapsed)
							[
								SNew(SBox)
								.WidthOverride(320.0f)
								.MaxDesiredHeight(260.0f)
								[
									SNew(SVerticalBox)

									// Category Tag
									+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(0.0f, 0.0f, 0.0f, 4.0f)
									[
										SAssignNew(DocCategoryTextBlock, STextBlock)
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
										.ColorAndOpacity(FLinearColor(0.5f, 0.7f, 1.0f, 1.0f))
									]

									// Signature Block
									+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(0.0f, 0.0f, 0.0f, 6.0f)
									[
										SNew(SBorder)
										.BorderImage(FAppStyle::Get().GetBrush("Menu.Background"))
										.BorderBackgroundColor(FLinearColor(0.06f, 0.06f, 0.08f, 1.0f))
										.Padding(FMargin(6.0f, 4.0f))
										[
											SAssignNew(DocSignatureTextBlock, STextBlock)
											.Font(FCppSyntaxHighlighterMarshaller::GetEditorFont(9.5f))
											.ColorAndOpacity(FLinearColor(0.863f, 0.863f, 0.667f, 1.0f))
											.AutoWrapText(true)
										]
									]

									// Description Text
									+ SVerticalBox::Slot()
									.FillHeight(1.0f)
									.Padding(0.0f, 0.0f, 0.0f, 6.0f)
									[
										SNew(SScrollBox)
										+ SScrollBox::Slot()
										[
											SAssignNew(DocDescriptionTextBlock, STextBlock)
											.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
											.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f))
											.AutoWrapText(true)
										]
									]

									// Documentation Link Button
									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SAssignNew(DocUrlButton, SButton)
										.ButtonStyle(FAppStyle::Get(), "SimpleButton")
										.ToolTipText(FText::FromString(TEXT("Open official Unreal Engine documentation in browser")))
										.OnClicked_Lambda([this]() -> FReply
										{
											if (!ActiveDocUrl.IsEmpty())
											{
												FPlatformProcess::LaunchURL(*ActiveDocUrl, nullptr, nullptr);
											}
											return FReply::Handled();
										})
										[
											SNew(SHorizontalBox)
											+ SHorizontalBox::Slot()
											.AutoWidth()
											.VAlign(VAlign_Center)
											.Padding(0.0f, 0.0f, 4.0f, 0.0f)
											[
												SNew(SImage)
												.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.ShowInExplorer")))
												.DesiredSizeOverride(FVector2D(10.0f, 10.0f))
											]
											+ SHorizontalBox::Slot()
											.AutoWidth()
											.VAlign(VAlign_Center)
											[
												SNew(STextBlock)
												.Text(FText::FromString(TEXT("Epic Games Documentation")))
												.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
												.ColorAndOpacity(FLinearColor(0.33f, 0.65f, 1.0f, 1.0f))
											]
										]
									]
								]
							]
						]
					]
				]

				// Floating Hover Documentation (Quick Info) Card
				+ SOverlay::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Top)
				.Padding(TAttribute<FMargin>::CreateSP(this, &SCppEditorPane::GetHoverDocMargin))
				[
					SAssignNew(HoverDocCard, SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("Menu.Background"))
					.BorderBackgroundColor(FLinearColor(0.10f, 0.11f, 0.14f, 0.98f))
					.Padding(8.0f)
					.Visibility(EVisibility::Collapsed)
					[
						SNew(SBox)
						.WidthOverride(380.0f)
						[
							SNew(SVerticalBox)

							// Header: Category & Signature
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 0.0f, 0.0f, 4.0f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(0.0f, 0.0f, 6.0f, 0.0f)
								[
									SNew(SBorder)
									.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
									.BorderBackgroundColor(FLinearColor(0.2f, 0.4f, 0.7f, 0.9f))
									.Padding(FMargin(5.0f, 1.0f))
									[
										SAssignNew(HoverDocCategoryText, STextBlock)
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8.0f))
										.ColorAndOpacity(FLinearColor::White)
									]
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.VAlign(VAlign_Center)
								[
									SAssignNew(HoverDocSignatureText, STextBlock)
									.Font(FCppSyntaxHighlighterMarshaller::GetEditorFont(9.0f))
									.ColorAndOpacity(FLinearColor(0.35f, 0.75f, 1.0f, 1.0f))
									.AutoWrapText(true)
								]
							]

							// Description
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 2.0f, 0.0f, 4.0f)
							[
								SAssignNew(HoverDocDescriptionText, STextBlock)
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.5f))
								.ColorAndOpacity(FLinearColor(0.85f, 0.85f, 0.90f, 1.0f))
								.AutoWrapText(true)
							]

							// Epic Games Documentation Link
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 2.0f, 0.0f, 0.0f)
							[
								SAssignNew(HoverDocUrlButton, SButton)
								.ButtonStyle(FAppStyle::Get(), "SimpleButton")
								.ContentPadding(FMargin(2.0f))
								.OnClicked_Lambda([this]() -> FReply
								{
									if (!ActiveHoverDocUrl.IsEmpty())
									{
										FPlatformProcess::LaunchURL(*ActiveHoverDocUrl, nullptr, nullptr);
									}
									return FReply::Handled();
								})
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									.AutoWidth()
									.VAlign(VAlign_Center)
									.Padding(0.0f, 0.0f, 4.0f, 0.0f)
									[
										SNew(SImage)
										.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.ShowInExplorer")))
										.DesiredSizeOverride(FVector2D(10.0f, 10.0f))
									]
									+ SHorizontalBox::Slot()
									.AutoWidth()
									.VAlign(VAlign_Center)
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("Epic Games Documentation")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
										.ColorAndOpacity(FLinearColor(0.33f, 0.65f, 1.0f, 1.0f))
									]
								]
							]
						]
					]
				]
			]
		]
		// ---------------------------------------------------------------------
		// 4. Status Bar
		// ---------------------------------------------------------------------
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
			.BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.06f, 1.0f))
			.Padding(FMargin(8.0f, 2.0f))
			[
				SNew(SHorizontalBox)

				// Cursor Location
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(4.0f, 0.0f)
				[
					SAssignNew(StatusBarCursorText, STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.5f))
					.ColorAndOpacity(FLinearColor(0.7f, 0.8f, 0.9f, 1.0f))
					.Text_Lambda([this]()
					{
						return FText::FromString(FString::Printf(TEXT("Ln %d, Col %d"), GetCurrentLineIndex() + 1, GetCurrentColumnIndex() + 1));
					})
				]

				// Language Badge
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(8.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("C++20")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8.0f))
					.ColorAndOpacity(FLinearColor(0.35f, 0.7f, 1.0f, 1.0f))
				]

				// Indentation
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(8.0f, 0.0f)
				[
					SNew(STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.0f))
					.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f))
					.Text_Lambda([]()
					{
						switch (FCppEditorSettings::Get().TabSize)
						{
						case ECppTabSize::TwoSpaces: return FText::FromString(TEXT("Spaces: 2"));
						case ECppTabSize::TabCharacter: return FText::FromString(TEXT("Tab Size: 4"));
						default: return FText::FromString(TEXT("Spaces: 4"));
						}
					})
				]

				// Encoding
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(8.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("UTF-8")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.0f))
					.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f))
				]

				// Spacer
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SSpacer)
				]

				// Document Stats
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(8.0f, 0.0f)
				[
					SAssignNew(StatusBarDocStatsText, STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.5f))
					.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f))
					.Text_Lambda([this]()
					{
						return FText::FromString(FString::Printf(TEXT("%d lines"), GetTotalLineCount()));
					})
				]
			]
		]
		]
	];

	RebuildTabStrip();
}

SCppEditorPane::~SCppEditorPane()
{
	if (SettingsChangedHandle.IsValid())
	{
		FCppEditorSettings::Get().OnSettingsChanged.Remove(SettingsChangedHandle);
		SettingsChangedHandle.Reset();
	}
}

void SCppEditorPane::ApplySettings()
{
	const FCppEditorSettings& Settings = FCppEditorSettings::Get();

	// 1. Update Syntax Highlighter Marshaller with new theme colors
	if (SyntaxMarshaller.IsValid())
	{
		SyntaxMarshaller->SetSyntaxStyle(Settings.GetSyntaxStyle());
	}
	if (CodeTextBox.IsValid())
	{
		CodeTextBox->SetAutoWrapText(Settings.bWordWrap);

		// Force the internal text layout to flush and re-marshall runs with new style
		if (ActiveDocumentIndex >= 0 && ActiveDocumentIndex < OpenDocuments.Num())
		{
			TSharedPtr<FEditorDocument> Doc = OpenDocuments[ActiveDocumentIndex];
			FTextLocation CursorLoc = CodeTextBox->GetCursorLocation();

			Doc->bIsInternalTextChange = true;
			CodeTextBox->SetText(FText::GetEmpty());
			CodeTextBox->SetText(FText::FromString(Doc->CurrentContent));
			Doc->bIsInternalTextChange = false;

			CodeTextBox->GoTo(CursorLoc);
			CodeTextBox->ScrollTo(CursorLoc);
		}
		else
		{
			CodeTextBox->Refresh();
		}
	}

	// 2. Update Gutter visibility
	if (GutterWidget.IsValid())
	{
		GutterWidget->SetVisibility(Settings.bShowLineNumbers ? EVisibility::Visible : EVisibility::Collapsed);
	}

	// 3. Dismiss IntelliSense if disabled
	if (!Settings.bEnableIntelliSense && bIntelliSenseActive)
	{
		DismissIntelliSense();
	}
}

bool SCppEditorPane::OpenFile(const FString& InFilePath)
{
	if (!IFileManager::Get().FileExists(*InFilePath))
	{
		return false;
	}

	// 1. Check if already open
	for (int32 i = 0; i < OpenDocuments.Num(); ++i)
	{
		if (OpenDocuments[i]->FilePath.Equals(InFilePath, ESearchCase::IgnoreCase))
		{
			SetActiveDocumentIndex(i);
			return true;
		}
	}

	// 2. Load file from disk
	FString Content;
	if (!FFileHelper::LoadFileToString(Content, *InFilePath))
	{
		Log(FString::Printf(TEXT("[Error] Failed to load file: %s"), *InFilePath));
		return false;
	}

	TSharedPtr<FEditorDocument> Doc = MakeShared<FEditorDocument>();
	Doc->FilePath = InFilePath;
	Doc->Filename = FPaths::GetCleanFilename(InFilePath);
	Doc->SavedContent = Content;
	Doc->CurrentContent = Content;
	Doc->bIsDirty = false;
	Doc->UndoHistory.Empty();
	Doc->RedoHistory.Empty();

	int32 NewIdx = OpenDocuments.Add(Doc);
	SetActiveDocumentIndex(NewIdx);

	Log(FString::Printf(TEXT("[Pane] Opened: %s"), *Doc->Filename));
	return true;
}

bool SCppEditorPane::CloseFile(const FString& InFilePath)
{
	for (int32 i = 0; i < OpenDocuments.Num(); ++i)
	{
		if (OpenDocuments[i]->FilePath.Equals(InFilePath, ESearchCase::IgnoreCase))
		{
			OpenDocuments.RemoveAt(i);

			if (ActiveDocumentIndex >= OpenDocuments.Num())
			{
				ActiveDocumentIndex = OpenDocuments.Num() - 1;
			}

			if (ActiveDocumentIndex >= 0)
			{
				SetActiveDocumentIndex(ActiveDocumentIndex);
			}
			else
			{
				ActiveDocumentIndex = INDEX_NONE;
				if (CodeTextBox.IsValid())
				{
					CodeTextBox->SetText(FText::GetEmpty());
				}
				RebuildTabStrip();
				OnActiveDocumentChanged.ExecuteIfBound(FString());
			}
			return true;
		}
	}
	return false;
}

void SCppEditorPane::CloseActiveFile()
{
	if (ActiveDocumentIndex >= 0 && ActiveDocumentIndex < OpenDocuments.Num())
	{
		CloseFile(OpenDocuments[ActiveDocumentIndex]->FilePath);
	}
}

void SCppEditorPane::CloseOtherFiles(const FString& KeepFilePath)
{
	for (int32 i = OpenDocuments.Num() - 1; i >= 0; --i)
	{
		if (!OpenDocuments[i]->FilePath.Equals(KeepFilePath, ESearchCase::IgnoreCase))
		{
			OpenDocuments.RemoveAt(i);
		}
	}
	ActiveDocumentIndex = OpenDocuments.Num() > 0 ? 0 : INDEX_NONE;
	if (ActiveDocumentIndex >= 0)
	{
		SetActiveDocumentIndex(ActiveDocumentIndex);
	}
	else
	{
		if (CodeTextBox.IsValid())
		{
			CodeTextBox->SetText(FText::GetEmpty());
		}
		RebuildTabStrip();
		OnActiveDocumentChanged.ExecuteIfBound(FString());
	}
}

void SCppEditorPane::CloseAllFiles()
{
	OpenDocuments.Empty();
	ActiveDocumentIndex = INDEX_NONE;
	if (CodeTextBox.IsValid())
	{
		CodeTextBox->SetText(FText::GetEmpty());
	}
	RebuildTabStrip();
	OnActiveDocumentChanged.ExecuteIfBound(FString());
}

void SCppEditorPane::MoveDocumentTab(int32 OldIndex, int32 NewIndex)
{
	if (OldIndex < 0 || OldIndex >= OpenDocuments.Num() || NewIndex < 0 || NewIndex >= OpenDocuments.Num() || OldIndex == NewIndex)
	{
		return;
	}
	TSharedPtr<FEditorDocument> Doc = OpenDocuments[OldIndex];
	OpenDocuments.RemoveAt(OldIndex);
	OpenDocuments.Insert(Doc, NewIndex);
	ActiveDocumentIndex = NewIndex;
	RebuildTabStrip();
}

void SCppEditorPane::SetActiveDocumentIndex(int32 NewIndex)
{
	DismissGhostText();

	if (NewIndex < 0 || NewIndex >= OpenDocuments.Num())
	{
		ActiveDocumentIndex = INDEX_NONE;
		if (CodeTextBox.IsValid())
		{
			CodeTextBox->SetText(FText::GetEmpty());
		}
		RebuildTabStrip();
		UpdateBreadcrumbs();
		OnActiveDocumentChanged.ExecuteIfBound(FString());
		return;
	}

	ActiveDocumentIndex = NewIndex;
	TSharedPtr<FEditorDocument> Doc = OpenDocuments[ActiveDocumentIndex];

	if (CodeTextBox.IsValid())
	{
		Doc->bIsInternalTextChange = true;
		CodeTextBox->SetText(FText::FromString(Doc->CurrentContent));
		Doc->bIsInternalTextChange = false;
	}

	RebuildTabStrip();
	UpdateBreadcrumbs();
	FocusEditor();
	OnActiveDocumentChanged.ExecuteIfBound(Doc->FilePath);
}

void SCppEditorPane::RebuildTabStrip()
{
	if (!TabStripBox.IsValid())
	{
		return;
	}

	TabStripBox->ClearChildren();

	for (int32 i = 0; i < OpenDocuments.Num(); ++i)
	{
		TSharedPtr<FEditorDocument> Doc = OpenDocuments[i];
		const bool bIsActive = (i == ActiveDocumentIndex);

		FString Ext = FPaths::GetExtension(Doc->Filename).ToLower();
		FName TabIconName = TEXT("SlateLivePreview.Tree.SourceFile");
		if (Ext == TEXT("h") || Ext == TEXT("inl")) TabIconName = TEXT("SlateLivePreview.Tree.HeaderFile");
		else if (Ext == TEXT("cs")) TabIconName = TEXT("SlateLivePreview.Tree.BuildFile");

		FString DisplayTitle = FString::Printf(TEXT("%s%s"), *Doc->Filename, Doc->bIsDirty ? TEXT(" *") : TEXT(""));

		FLinearColor TabBg = bIsActive ? FLinearColor(0.18f, 0.18f, 0.18f, 1.0f) : FLinearColor(0.10f, 0.10f, 0.10f, 0.9f);
		FLinearColor TabBorder = bIsActive ? FLinearColor(0.0f, 0.48f, 0.8f, 1.0f) : FLinearColor(0.15f, 0.15f, 0.15f, 0.5f);

		TabStripBox->AddSlot()
			.AutoWidth()
			.Padding(1.0f, 0.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(TabBorder)
				.Padding(FMargin(1.0f, 1.0f, 1.0f, 0.0f))
				[
					SNew(SBorder)
					.BorderBackgroundColor(TabBg)
					.Padding(FMargin(6.0f, 3.0f))
					.OnMouseButtonDown_Lambda([this, Doc, i](const FGeometry&, const FPointerEvent& MouseEvent) -> FReply
					{
						if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
						{
							ShowTabContextMenu(MouseEvent, Doc, i);
							return FReply::Handled();
						}
						return FReply::Unhandled();
					})
					[
						SNew(SHorizontalBox)

						// Tab Clickable Label
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SButton)
							.ButtonStyle(FAppStyle::Get(), "SimpleButton")
							.ContentPadding(FMargin(0.0f))
							.OnClicked_Lambda([this, i]() -> FReply
							{
								SetActiveDocumentIndex(i);
								FocusEditor();
								return FReply::Handled();
							})
							[
								SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(0.0f, 0.0f, 4.0f, 0.0f)
								[
									SNew(SImage)
									.Image(FSlateLivePreviewStyle::GetBrush(TabIconName))
									.DesiredSizeOverride(FVector2D(13.0f, 13.0f))
								]

								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text(FText::FromString(DisplayTitle))
									.Font(FCoreStyle::GetDefaultFontStyle(bIsActive ? "Bold" : "Regular", 9))
									.ColorAndOpacity(bIsActive ? FLinearColor::White : FLinearColor(0.7f, 0.7f, 0.7f, 1.0f))
								]
							]
						]

						// Tab Close Button
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(4.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(SButton)
							.ButtonStyle(FAppStyle::Get(), "SimpleButton")
							.ToolTipText(FText::FromString(TEXT("Close Tab")))
							.ContentPadding(FMargin(2.0f, 1.0f))
							.OnClicked_Lambda([this, FilePath = Doc->FilePath]() -> FReply
							{
								CloseFile(FilePath);
								FocusEditor();
								return FReply::Handled();
							})
							[
								SNew(SImage)
								.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.CloseTab")))
								.DesiredSizeOverride(FVector2D(10.0f, 10.0f))
								.ColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.75f, 0.75f, 0.7f)))
							]
						]
					]
				]
			];
	}
}

void SCppEditorPane::OnCodeTextChanged(const FText& NewText)
{
	if (ActiveDocumentIndex < 0 || ActiveDocumentIndex >= OpenDocuments.Num())
	{
		return;
	}

	TSharedPtr<FEditorDocument> Doc = OpenDocuments[ActiveDocumentIndex];
	if (Doc->bIsInternalTextChange)
	{
		return;
	}

	FString CurrentString = NewText.ToString();
	if (CurrentString != Doc->CurrentContent)
	{
		const bool bWasDirty = Doc->bIsDirty;
		Doc->CurrentContent = MoveTemp(CurrentString);
		Doc->bIsDirty = (Doc->CurrentContent != Doc->SavedContent);

		// Record undo snapshot with debouncing (every 0.5s) to avoid allocating entire file string per keystroke
		double CurrentTime = FPlatformTime::Seconds();
		if (Doc->UndoHistory.Num() == 0 || (CurrentTime - Doc->LastUndoSnapshotTime) > 0.5)
		{
			Doc->UndoHistory.Push(Doc->CurrentContent);
			if (Doc->UndoHistory.Num() > 50)
			{
				Doc->UndoHistory.RemoveAt(0);
			}
			Doc->LastUndoSnapshotTime = CurrentTime;
		}
		Doc->RedoHistory.Empty();

		// Only rebuild tab strip if dirty state actually changed to avoid recreating Slate tabs on every keystroke
		if (bWasDirty != Doc->bIsDirty)
		{
			RebuildTabStrip();
		}

		OnDocumentContentChanged.ExecuteIfBound(Doc->FilePath, Doc->CurrentContent);

		if (bFindBarVisible && !CurrentSearchQuery.IsEmpty())
		{
			PerformSearch(false);
		}
	}

	UpdateIntelliSense();
}

bool SCppEditorPane::SaveCurrentFile()
{
	if (ActiveDocumentIndex < 0 || ActiveDocumentIndex >= OpenDocuments.Num())
	{
		return false;
	}

	TSharedPtr<FEditorDocument> Doc = OpenDocuments[ActiveDocumentIndex];
	if (FFileHelper::SaveStringToFile(Doc->CurrentContent, *Doc->FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		Doc->SavedContent = Doc->CurrentContent;
		Doc->bIsDirty = false;
		RebuildTabStrip();
		Log(FString::Printf(TEXT("[File] Saved: %s"), *Doc->Filename));
		return true;
	}

	Log(FString::Printf(TEXT("[Error] Failed to save file: %s"), *Doc->FilePath));
	return false;
}

bool SCppEditorPane::SaveAll()
{
	bool bAllSuccess = true;
	for (const auto& Doc : OpenDocuments)
	{
		if (Doc->bIsDirty)
		{
			if (FFileHelper::SaveStringToFile(Doc->CurrentContent, *Doc->FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
			{
				Doc->SavedContent = Doc->CurrentContent;
				Doc->bIsDirty = false;
			}
			else
			{
				bAllSuccess = false;
			}
		}
	}
	RebuildTabStrip();
	return bAllSuccess;
}

void SCppEditorPane::RevertCurrentFile()
{
	if (ActiveDocumentIndex < 0 || ActiveDocumentIndex >= OpenDocuments.Num())
	{
		return;
	}

	TSharedPtr<FEditorDocument> Doc = OpenDocuments[ActiveDocumentIndex];
	FString DiskContent;
	if (FFileHelper::LoadFileToString(DiskContent, *Doc->FilePath))
	{
		Doc->SavedContent = DiskContent;
		Doc->CurrentContent = DiskContent;
		Doc->bIsDirty = false;
		Doc->UndoHistory.Empty();
		Doc->RedoHistory.Empty();

		Doc->bIsInternalTextChange = true;
		if (CodeTextBox.IsValid())
		{
			CodeTextBox->SetText(FText::FromString(DiskContent));
		}
		Doc->bIsInternalTextChange = false;

		RebuildTabStrip();
		OnDocumentContentChanged.ExecuteIfBound(Doc->FilePath, Doc->CurrentContent);
		Log(FString::Printf(TEXT("[File] Reverted: %s"), *Doc->Filename));
	}
}

void SCppEditorPane::Undo()
{
	if (ActiveDocumentIndex < 0 || ActiveDocumentIndex >= OpenDocuments.Num())
	{
		return;
	}

	TSharedPtr<FEditorDocument> Doc = OpenDocuments[ActiveDocumentIndex];
	if (Doc->UndoHistory.Num() > 0)
	{
		Doc->RedoHistory.Push(Doc->CurrentContent);
		FString PrevContent = Doc->UndoHistory.Pop();

		Doc->bIsInternalTextChange = true;
		Doc->CurrentContent = PrevContent;
		Doc->bIsDirty = (Doc->CurrentContent != Doc->SavedContent);
		if (CodeTextBox.IsValid())
		{
			CodeTextBox->SetText(FText::FromString(PrevContent));
		}
		Doc->bIsInternalTextChange = false;

		RebuildTabStrip();
		OnDocumentContentChanged.ExecuteIfBound(Doc->FilePath, Doc->CurrentContent);
	}
}

void SCppEditorPane::Redo()
{
	if (ActiveDocumentIndex < 0 || ActiveDocumentIndex >= OpenDocuments.Num())
	{
		return;
	}

	TSharedPtr<FEditorDocument> Doc = OpenDocuments[ActiveDocumentIndex];
	if (Doc->RedoHistory.Num() > 0)
	{
		Doc->UndoHistory.Push(Doc->CurrentContent);
		FString NextContent = Doc->RedoHistory.Pop();

		Doc->bIsInternalTextChange = true;
		Doc->CurrentContent = NextContent;
		Doc->bIsDirty = (Doc->CurrentContent != Doc->SavedContent);
		if (CodeTextBox.IsValid())
		{
			CodeTextBox->SetText(FText::FromString(NextContent));
		}
		Doc->bIsInternalTextChange = false;

		RebuildTabStrip();
		OnDocumentContentChanged.ExecuteIfBound(Doc->FilePath, Doc->CurrentContent);
	}
}

void SCppEditorPane::ToggleFindBar(bool bShow)
{
	bFindBarVisible = bShow;
	if (FindBarBorder.IsValid())
	{
		FindBarBorder->SetVisibility(bShow ? EVisibility::Visible : EVisibility::Collapsed);
	}

	if (bShow && FindSearchBox.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(FindSearchBox, EFocusCause::SetDirectly);
		PerformSearch(false);
	}
	else if (!bShow)
	{
		FocusEditor();
	}
}

void SCppEditorPane::PerformSearch(bool bJumpToFirst)
{
	CurrentMatches.Empty();
	CurrentMatchIndex = INDEX_NONE;

	if (CurrentSearchQuery.IsEmpty() || ActiveDocumentIndex < 0 || ActiveDocumentIndex >= OpenDocuments.Num())
	{
		if (MatchCountTextBlock.IsValid())
		{
			MatchCountTextBlock->SetText(FText::FromString(TEXT("0 matches")));
		}
		return;
	}

	TSharedPtr<FEditorDocument> Doc = OpenDocuments[ActiveDocumentIndex];
	const FString& Content = Doc->CurrentContent;
	if (Content.IsEmpty())
	{
		if (MatchCountTextBlock.IsValid())
		{
			MatchCountTextBlock->SetText(FText::FromString(TEXT("0 matches")));
		}
		return;
	}

	// Precompute line start offsets for fast, zero-string-allocation line/column calculation
	TArray<int32> LineStartOffsets;
	LineStartOffsets.Add(0);
	for (int32 i = 0; i < Content.Len(); ++i)
	{
		if (Content[i] == TEXT('\n'))
		{
			LineStartOffsets.Add(i + 1);
		}
	}

	const ESearchCase::Type SearchCase = bMatchCase ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;
	const int32 QueryLen = CurrentSearchQuery.Len();
	if (QueryLen <= 0)
	{
		return;
	}

	int32 SearchOffset = 0;
	const int32 MaxMatches = 500; // Hard cap prevents RAM flooding on common queries like ' ' or 'e'

	while (SearchOffset < Content.Len())
	{
		int32 FoundPos = Content.Find(CurrentSearchQuery, SearchCase, ESearchDir::FromStart, SearchOffset);
		if (FoundPos == INDEX_NONE)
		{
			break;
		}

		// Find line and column using binary search on line offsets (zero string allocations)
		int32 LineIndex = Algo::UpperBound(LineStartOffsets, FoundPos) - 1;
		int32 ColumnIndex = FoundPos - LineStartOffsets[LineIndex];

		FTextMatch Match;
		Match.LineIndex = LineIndex;
		Match.ColumnIndex = ColumnIndex;
		CurrentMatches.Add(Match);

		if (CurrentMatches.Num() >= MaxMatches)
		{
			break;
		}

		SearchOffset = FoundPos + QueryLen;
	}

	if (CurrentMatches.Num() > 0)
	{
		CurrentMatchIndex = 0;
		if (bJumpToFirst)
		{
			JumpToMatch(CurrentMatchIndex);
		}
	}

	if (MatchCountTextBlock.IsValid())
	{
		if (CurrentMatches.Num() >= MaxMatches)
		{
			MatchCountTextBlock->SetText(FText::FromString(TEXT("500+ matches")));
		}
		else if (CurrentMatches.Num() > 0)
		{
			MatchCountTextBlock->SetText(FText::FromString(FString::Printf(TEXT("%d of %d"), CurrentMatchIndex + 1, CurrentMatches.Num())));
		}
		else
		{
			MatchCountTextBlock->SetText(FText::FromString(TEXT("No matches")));
		}
	}
}

void SCppEditorPane::FindNext()
{
	if (CurrentMatches.Num() == 0)
	{
		PerformSearch(true);
		return;
	}

	CurrentMatchIndex = (CurrentMatchIndex + 1) % CurrentMatches.Num();
	JumpToMatch(CurrentMatchIndex);

	if (MatchCountTextBlock.IsValid())
	{
		MatchCountTextBlock->SetText(FText::FromString(FString::Printf(TEXT("%d of %d"), CurrentMatchIndex + 1, CurrentMatches.Num())));
	}
}

void SCppEditorPane::FindPrevious()
{
	if (CurrentMatches.Num() == 0)
	{
		PerformSearch(true);
		return;
	}

	CurrentMatchIndex = (CurrentMatchIndex - 1 + CurrentMatches.Num()) % CurrentMatches.Num();
	JumpToMatch(CurrentMatchIndex);

	if (MatchCountTextBlock.IsValid())
	{
		MatchCountTextBlock->SetText(FText::FromString(FString::Printf(TEXT("%d of %d"), CurrentMatchIndex + 1, CurrentMatches.Num())));
	}
}

void SCppEditorPane::JumpToMatch(int32 MatchIndex)
{
	if (MatchIndex >= 0 && MatchIndex < CurrentMatches.Num() && CodeTextBox.IsValid())
	{
		const FTextMatch& Match = CurrentMatches[MatchIndex];
		FTextLocation Location(Match.LineIndex, Match.ColumnIndex);
		CodeTextBox->GoTo(Location);
		CodeTextBox->ScrollTo(Location);
	}
}

void SCppEditorPane::GoToLine(int32 LineNumber, int32 ColumnNumber)
{
	if (CodeTextBox.IsValid())
	{
		int32 LineIdx = FMath::Max(0, LineNumber - 1);
		int32 ColIdx = FMath::Max(0, ColumnNumber - 1);
		FTextLocation Location(LineIdx, ColIdx);
		CodeTextBox->GoTo(Location);
		CodeTextBox->ScrollTo(Location);
		FocusEditor();
	}
}

FString SCppEditorPane::GetActiveFilePath() const
{
	if (ActiveDocumentIndex >= 0 && ActiveDocumentIndex < OpenDocuments.Num())
	{
		return OpenDocuments[ActiveDocumentIndex]->FilePath;
	}
	return FString();
}

FString SCppEditorPane::GetActiveContent() const
{
	if (ActiveDocumentIndex >= 0 && ActiveDocumentIndex < OpenDocuments.Num())
	{
		return OpenDocuments[ActiveDocumentIndex]->CurrentContent;
	}
	return FString();
}

bool SCppEditorPane::IsCurrentDirty() const
{
	if (ActiveDocumentIndex >= 0 && ActiveDocumentIndex < OpenDocuments.Num())
	{
		return OpenDocuments[ActiveDocumentIndex]->bIsDirty;
	}
	return false;
}

void SCppEditorPane::FocusEditor()
{
	if (CodeTextBox.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(CodeTextBox, EFocusCause::SetDirectly);
	}
	if (OnPaneFocused.IsBound())
	{
		OnPaneFocused.Execute(SharedThis(this));
	}
}

void SCppEditorPane::SetIsActivePane(bool bInActive)
{
	bIsActivePane = bInActive;
}

TSharedPtr<SWidget> SCppEditorPane::OnEditorContextMenuOpening()
{
	FocusEditor();

	FMenuBuilder MenuBuilder(true, nullptr);

	const FString ActivePath = GetActiveFilePath();
	const bool bIsHeader = ActivePath.EndsWith(TEXT(".h")) || ActivePath.EndsWith(TEXT(".inl"));

	if (bIsHeader)
	{
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("Quick Fix: Generate Definition in .cpp")),
			FText::FromString(TEXT("Generate C++ member function definition in matching .cpp file")),
			FSlateIcon(FSlateLivePreviewStyle::GetStyleSetName(), TEXT("SlateLivePreview.QuickFix")),
			FUIAction(FExecuteAction::CreateSP(this, &SCppEditorPane::QuickActionGenerateDefinition))
		);
	}

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Switch Header / Source (Alt+O)")),
		FText::FromString(TEXT("Toggle between .h header and .cpp source file")),
		FSlateIcon(FSlateLivePreviewStyle::GetStyleSetName(), TEXT("SlateLivePreview.SwitchHeader")),
		FUIAction(FExecuteAction::CreateSP(this, &SCppEditorPane::ToggleHeaderSource))
	);

	MenuBuilder.AddSeparator();

	// Cut
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Cut (Ctrl+X)")),
		FText::FromString(TEXT("Cut selection to clipboard")),
		FSlateIcon(FSlateLivePreviewStyle::GetStyleSetName(), TEXT("SlateLivePreview.Cut")),
		FUIAction(FExecuteAction::CreateLambda([this]()
		{
			if (CodeTextBox.IsValid())
			{
				FText Selected = CodeTextBox->GetSelectedText();
				if (!Selected.IsEmpty())
				{
					FPlatformApplicationMisc::ClipboardCopy(*Selected.ToString());
					CodeTextBox->InsertTextAtCursor(TEXT(""));
				}
			}
		}))
	);

	// Copy
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Copy (Ctrl+C)")),
		FText::FromString(TEXT("Copy selection to clipboard")),
		FSlateIcon(FSlateLivePreviewStyle::GetStyleSetName(), TEXT("SlateLivePreview.CopyPath")),
		FUIAction(FExecuteAction::CreateLambda([this]()
		{
			if (CodeTextBox.IsValid())
			{
				FText Selected = CodeTextBox->GetSelectedText();
				if (!Selected.IsEmpty())
				{
					FPlatformApplicationMisc::ClipboardCopy(*Selected.ToString());
				}
			}
		}))
	);

	// Paste
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Paste (Ctrl+V)")),
		FText::FromString(TEXT("Paste text from clipboard")),
		FSlateIcon(FSlateLivePreviewStyle::GetStyleSetName(), TEXT("SlateLivePreview.Paste")),
		FUIAction(FExecuteAction::CreateLambda([this]()
		{
			FString PastedText;
			FPlatformApplicationMisc::ClipboardPaste(PastedText);
			if (!PastedText.IsEmpty() && CodeTextBox.IsValid())
			{
				CodeTextBox->InsertTextAtCursor(PastedText);
			}
		}))
	);

	MenuBuilder.AddSeparator();

	// Toggle Line Comment
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Toggle Line Comment (Ctrl+/)")),
		FText::FromString(TEXT("Comment or uncomment current line")),
		FSlateIcon(FSlateLivePreviewStyle::GetStyleSetName(), TEXT("SlateLivePreview.Comment")),
		FUIAction(FExecuteAction::CreateSP(this, &SCppEditorPane::ToggleLineComment))
	);

	// Duplicate Line
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Duplicate Line (Ctrl+D)")),
		FText::FromString(TEXT("Duplicate current line downwards")),
		FSlateIcon(FSlateLivePreviewStyle::GetStyleSetName(), TEXT("SlateLivePreview.Duplicate")),
		FUIAction(FExecuteAction::CreateSP(this, &SCppEditorPane::DuplicateLine))
	);

	// Delete Line
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Delete Line (Ctrl+Shift+K)")),
		FText::FromString(TEXT("Delete current line")),
		FSlateIcon(FSlateLivePreviewStyle::GetStyleSetName(), TEXT("SlateLivePreview.Delete")),
		FUIAction(FExecuteAction::CreateSP(this, &SCppEditorPane::DeleteLine))
	);

	MenuBuilder.AddSeparator();

	// Find
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Find in File (Ctrl+F)")),
		FText::FromString(TEXT("Open find bar")),
		FSlateIcon(FSlateLivePreviewStyle::GetStyleSetName(), TEXT("SlateLivePreview.Find")),
		FUIAction(FExecuteAction::CreateLambda([this]() { ToggleFindBar(true); }))
	);

	// Save
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Save File (Ctrl+S)")),
		FText::FromString(TEXT("Save current file")),
		FSlateIcon(FSlateLivePreviewStyle::GetStyleSetName(), TEXT("SlateLivePreview.Save")),
		FUIAction(FExecuteAction::CreateLambda([this]() { SaveCurrentFile(); }))
	);

	return MenuBuilder.MakeWidget();
}

void SCppEditorPane::ShowTabContextMenu(const FPointerEvent& MouseEvent, TSharedPtr<FEditorDocument> Doc, int32 DocIndex)
{
	if (!Doc.IsValid()) return;

	FocusEditor();

	FMenuBuilder MenuBuilder(true, nullptr);

	// Close Tab
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Close Tab")),
		FText::FromString(TEXT("Close this tab")),
		FSlateIcon(FSlateLivePreviewStyle::GetStyleSetName(), TEXT("SlateLivePreview.CloseTab")),
		FUIAction(FExecuteAction::CreateLambda([this, FilePath = Doc->FilePath]()
		{
			CloseFile(FilePath);
		}))
	);

	// Close Others
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Close Other Tabs")),
		FText::FromString(TEXT("Close all other tabs except this one")),
		FSlateIcon(FSlateLivePreviewStyle::GetStyleSetName(), TEXT("SlateLivePreview.CloseTab")),
		FUIAction(FExecuteAction::CreateLambda([this, FilePath = Doc->FilePath]()
		{
			CloseOtherFiles(FilePath);
		}))
	);

	// Close All
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Close All Tabs")),
		FText::FromString(TEXT("Close all tabs in this pane")),
		FSlateIcon(FSlateLivePreviewStyle::GetStyleSetName(), TEXT("SlateLivePreview.Delete")),
		FUIAction(FExecuteAction::CreateSP(this, &SCppEditorPane::CloseAllFiles))
	);

	MenuBuilder.AddSeparator();

	// Move Left
	if (DocIndex > 0)
	{
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("Move Tab Left")),
			FText::FromString(TEXT("Shift tab to the left")),
			FSlateIcon(FSlateLivePreviewStyle::GetStyleSetName(), TEXT("SlateLivePreview.ArrowLeft")),
			FUIAction(FExecuteAction::CreateLambda([this, DocIndex]()
			{
				MoveDocumentTab(DocIndex, DocIndex - 1);
			}))
		);
	}

	// Move Right
	if (DocIndex < OpenDocuments.Num() - 1)
	{
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("Move Tab Right")),
			FText::FromString(TEXT("Shift tab to the right")),
			FSlateIcon(FSlateLivePreviewStyle::GetStyleSetName(), TEXT("SlateLivePreview.ArrowRight")),
			FUIAction(FExecuteAction::CreateLambda([this, DocIndex]()
			{
				MoveDocumentTab(DocIndex, DocIndex + 1);
			}))
		);
	}

	// Move to Other Pane
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Move to Other Split Pane")),
		FText::FromString(TEXT("Move this tab into the opposite editor split pane")),
		FSlateIcon(FSlateLivePreviewStyle::GetStyleSetName(), TEXT("SlateLivePreview.SplitView")),
		FUIAction(FExecuteAction::CreateLambda([this, FilePath = Doc->FilePath]()
		{
			if (OnMoveDocumentRequested.IsBound())
			{
				OnMoveDocumentRequested.Execute(FilePath, SharedThis(this));
			}
		}))
	);

	MenuBuilder.AddSeparator();

	// Copy Path
	FString FilePath = Doc->FilePath;
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Copy File Path")),
		FText::FromString(TEXT("Copy absolute file path to clipboard")),
		FSlateIcon(FSlateLivePreviewStyle::GetStyleSetName(), TEXT("SlateLivePreview.CopyPath")),
		FUIAction(FExecuteAction::CreateLambda([FilePath]()
		{
			FPlatformApplicationMisc::ClipboardCopy(*FilePath);
		}))
	);

	// Reveal in Explorer
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Reveal in Explorer")),
		FText::FromString(TEXT("Open containing folder in Windows Explorer")),
		FSlateIcon(FSlateLivePreviewStyle::GetStyleSetName(), TEXT("SlateLivePreview.ShowInExplorer")),
		FUIAction(FExecuteAction::CreateLambda([FilePath]()
		{
			FPlatformProcess::ExploreFolder(*FPaths::GetPath(FilePath));
		}))
	);

	FSlateApplication::Get().PushMenu(
		SharedThis(this),
		FWidgetPath(),
		MenuBuilder.MakeWidget(),
		MouseEvent.GetScreenSpacePosition(),
		FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu)
	);
}

void SCppEditorPane::Log(const FString& Message)
{
	if (OnPaneLog.IsBound())
	{
		OnPaneLog.Execute(Message);
	}
}

FReply SCppEditorPane::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	DismissHoverDoc();
	FocusEditor();
	return FReply::Unhandled();
}

FReply SCppEditorPane::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.IsControlDown() && InKeyEvent.GetKey() == EKeys::F)
	{
		ToggleFindBar(!bFindBarVisible);
		return FReply::Handled();
	}
	if (InKeyEvent.IsControlDown() && InKeyEvent.GetKey() == EKeys::Z && !InKeyEvent.IsShiftDown())
	{
		Undo();
		return FReply::Handled();
	}
	if ((InKeyEvent.IsControlDown() && InKeyEvent.GetKey() == EKeys::Y) ||
		(InKeyEvent.IsControlDown() && InKeyEvent.IsShiftDown() && InKeyEvent.GetKey() == EKeys::Z))
	{
		Redo();
		return FReply::Handled();
	}
	if (InKeyEvent.IsControlDown() && InKeyEvent.GetKey() == EKeys::S)
	{
		SaveCurrentFile();
		return FReply::Handled();
	}
	if (bFindBarVisible && InKeyEvent.GetKey() == EKeys::Escape)
	{
		ToggleFindBar(false);
		return FReply::Handled();
	}

	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

// ---------------------------------------------------------------------
// IntelliSense & Tab Implementation
// ---------------------------------------------------------------------

static TArray<TSharedPtr<FIntelliSenseItem>> MasterIntelliSenseDatabase;

void SCppEditorPane::EnsureIntelliSenseDatabaseLoaded()
{
	if (MasterIntelliSenseDatabase.Num() > 0)
	{
		return;
	}

	auto AddItem = [](const FString& Display, const FString& Insert, EIntelliSenseCategory Cat, const FString& Sig = TEXT(""), const FString& Desc = TEXT(""), const FString& DocUrl = TEXT(""), const FString& Scope = TEXT("general"))
	{
		TSharedPtr<FIntelliSenseItem> Item = MakeShared<FIntelliSenseItem>();
		Item->DisplayText = Display;
		Item->InsertText = Insert;
		Item->Category = Cat;
		Item->Signature = Sig;
		Item->Description = Desc;
		Item->DocUrl = DocUrl;
		Item->Scope = Scope;
		MasterIntelliSenseDatabase.Add(Item);
	};

	// -------------------------------------------------------------------------
	// 1. C++ Keywords & Core Constructs
	// -------------------------------------------------------------------------
	AddItem(TEXT("enum"), TEXT("enum "), EIntelliSenseCategory::Keyword, TEXT("enum EnumName { ... };"), TEXT("Declares a classic C++ enumeration type."), TEXT("https://en.cppreference.com/w/cpp/language/enum"));
	AddItem(TEXT("enum class"), TEXT("enum class "), EIntelliSenseCategory::Keyword, TEXT("enum class EnumName : uint8 { ... };"), TEXT("Declares a strongly-scoped, strongly-typed modern C++ enumeration class."), TEXT("https://en.cppreference.com/w/cpp/language/enum"));
	AddItem(TEXT("class"), TEXT("class "), EIntelliSenseCategory::Keyword, TEXT("class ClassName { ... };"), TEXT("Declares a user-defined class with private members by default."), TEXT("https://en.cppreference.com/w/cpp/language/class"));
	AddItem(TEXT("struct"), TEXT("struct "), EIntelliSenseCategory::Keyword, TEXT("struct StructName { ... };"), TEXT("Declares a user-defined structure with public members by default."), TEXT("https://en.cppreference.com/w/cpp/language/class"));
	AddItem(TEXT("public:"), TEXT("public:\n\t"), EIntelliSenseCategory::Keyword, TEXT("public:"), TEXT("Specifies that following members are accessible from any context."));
	AddItem(TEXT("private:"), TEXT("private:\n\t"), EIntelliSenseCategory::Keyword, TEXT("private:"), TEXT("Specifies that following members are only accessible within the enclosing class."));
	AddItem(TEXT("protected:"), TEXT("protected:\n\t"), EIntelliSenseCategory::Keyword, TEXT("protected:"), TEXT("Specifies that following members are accessible in this class and derived classes."));
	AddItem(TEXT("virtual"), TEXT("virtual "), EIntelliSenseCategory::Keyword, TEXT("virtual ReturnType FuncName(...);"), TEXT("Declares a member function that can be overridden in derived classes."));
	AddItem(TEXT("override"), TEXT("override"), EIntelliSenseCategory::Keyword, TEXT("ReturnType FuncName(...) override;"), TEXT("Explicitly asserts that a virtual function overrides a base class function."));
	AddItem(TEXT("const"), TEXT("const "), EIntelliSenseCategory::Keyword, TEXT("const Type VarName;"), TEXT("Specifies that the value of an object or return cannot be modified."));
	AddItem(TEXT("constexpr"), TEXT("constexpr "), EIntelliSenseCategory::Keyword, TEXT("constexpr Type VarName = Value;"), TEXT("Specifies that value or function can be evaluated at compile time."));
	AddItem(TEXT("return"), TEXT("return "), EIntelliSenseCategory::Keyword, TEXT("return Expression;"), TEXT("Terminates execution of the current function and returns the result."));
	AddItem(TEXT("auto"), TEXT("auto "), EIntelliSenseCategory::Keyword, TEXT("auto VarName = Initializer;"), TEXT("Deduces variable type automatically from its initializer expression."));
	AddItem(TEXT("if"), TEXT("if ()"), EIntelliSenseCategory::Keyword, TEXT("if (Condition) { ... }"), TEXT("Executes statements conditionally if Condition evaluates to true."));
	AddItem(TEXT("else"), TEXT("else\n{\n\t\n}"), EIntelliSenseCategory::Keyword, TEXT("else { ... }"), TEXT("Executes statements when preceding if condition evaluates to false."));
	AddItem(TEXT("else if"), TEXT("else if ()"), EIntelliSenseCategory::Keyword, TEXT("else if (Condition) { ... }"), TEXT("Chained conditional branch executed if earlier conditions were false."));
	AddItem(TEXT("for"), TEXT("for (int32 i = 0; i < ; ++i)"), EIntelliSenseCategory::Keyword, TEXT("for (Init; Condition; Iter) { ... }"), TEXT("Standard counting loop executing body while condition holds true."));
	AddItem(TEXT("while"), TEXT("while ()"), EIntelliSenseCategory::Keyword, TEXT("while (Condition) { ... }"), TEXT("Loop executing statements repeatedly until condition becomes false."));
	AddItem(TEXT("switch"), TEXT("switch ()"), EIntelliSenseCategory::Keyword, TEXT("switch (Expression) { case ...: break; }"), TEXT("Multi-way branch based on an integer or enumeration value."));
	AddItem(TEXT("case"), TEXT("case "), EIntelliSenseCategory::Keyword, TEXT("case ConstantValue:"), TEXT("Branch label inside a switch statement."));
	AddItem(TEXT("default:"), TEXT("default:\n\tbreak;"), EIntelliSenseCategory::Keyword, TEXT("default:"), TEXT("Fallback branch in a switch statement when no case matches."));
	AddItem(TEXT("break"), TEXT("break;"), EIntelliSenseCategory::Keyword, TEXT("break;"), TEXT("Terminates innermost loop or switch statement immediately."));
	AddItem(TEXT("continue"), TEXT("continue;"), EIntelliSenseCategory::Keyword, TEXT("continue;"), TEXT("Skips the rest of the current loop iteration and advances to next."));
	AddItem(TEXT("nullptr"), TEXT("nullptr"), EIntelliSenseCategory::Keyword, TEXT("nullptr"), TEXT("Literal representing a null pointer constant of type std::nullptr_t."));
	AddItem(TEXT("true"), TEXT("true"), EIntelliSenseCategory::Keyword, TEXT("true"), TEXT("Boolean literal true (1)."));
	AddItem(TEXT("false"), TEXT("false"), EIntelliSenseCategory::Keyword, TEXT("false"), TEXT("Boolean literal false (0)."));
	AddItem(TEXT("static"), TEXT("static "), EIntelliSenseCategory::Keyword, TEXT("static Type Name;"), TEXT("Specifies static storage duration or internal linkage for symbols."));
	AddItem(TEXT("void"), TEXT("void "), EIntelliSenseCategory::Keyword, TEXT("void"), TEXT("Type with an empty set of values; indicates function returns nothing."));
	AddItem(TEXT("bool"), TEXT("bool "), EIntelliSenseCategory::Keyword, TEXT("bool"), TEXT("Fundamental boolean type capable of holding true or false."));
	AddItem(TEXT("int32"), TEXT("int32 "), EIntelliSenseCategory::Keyword, TEXT("int32"), TEXT("Unreal standard 32-bit signed integer."));
	AddItem(TEXT("int64"), TEXT("int64 "), EIntelliSenseCategory::Keyword, TEXT("int64"), TEXT("Unreal standard 64-bit signed integer."));
	AddItem(TEXT("uint32"), TEXT("uint32 "), EIntelliSenseCategory::Keyword, TEXT("uint32"), TEXT("Unreal standard 32-bit unsigned integer."));
	AddItem(TEXT("uint8"), TEXT("uint8 "), EIntelliSenseCategory::Keyword, TEXT("uint8"), TEXT("Unreal standard 8-bit unsigned integer (byte)."));
	AddItem(TEXT("float"), TEXT("float "), EIntelliSenseCategory::Keyword, TEXT("float"), TEXT("Single-precision 32-bit IEEE 754 floating point type."));
	AddItem(TEXT("double"), TEXT("double "), EIntelliSenseCategory::Keyword, TEXT("double"), TEXT("Double-precision 64-bit IEEE 754 floating point type."));
	AddItem(TEXT("static_cast"), TEXT("static_cast<>()"), EIntelliSenseCategory::Keyword, TEXT("static_cast<TargetType>(Expression)"), TEXT("Performs explicit compile-time type conversion."));
	AddItem(TEXT("reinterpret_cast"), TEXT("reinterpret_cast<>()"), EIntelliSenseCategory::Keyword, TEXT("reinterpret_cast<TargetType>(Expression)"), TEXT("Converts any pointer type to any other pointer type without type checks."));
	AddItem(TEXT("template"), TEXT("template<typename T>"), EIntelliSenseCategory::Keyword, TEXT("template<typename T>"), TEXT("Declares a generic class, struct, function, or type alias."));
	AddItem(TEXT("typename"), TEXT("typename "), EIntelliSenseCategory::Keyword, TEXT("typename T"), TEXT("Introduces a type parameter or clarifies dependent type names."));
	AddItem(TEXT("using"), TEXT("using "), EIntelliSenseCategory::Keyword, TEXT("using NewName = ExistingType;"), TEXT("Introduces a type alias or brings namespace symbols into scope."));
	AddItem(TEXT("namespace"), TEXT("namespace "), EIntelliSenseCategory::Keyword, TEXT("namespace ScopeName { ... }"), TEXT("Groups global declarations to avoid symbol name collisions."));

	// -------------------------------------------------------------------------
	// 2. Slate Widgets (Types)
	// -------------------------------------------------------------------------
	AddItem(TEXT("SCompoundWidget"), TEXT("SCompoundWidget"), EIntelliSenseCategory::Type,
		TEXT("class SCompoundWidget : public SWidget"),
		TEXT("Base class for composite Slate widgets that contain nested child widgets inside ChildSlot."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("SVerticalBox"), TEXT("SVerticalBox"), EIntelliSenseCategory::Type,
		TEXT("class SVerticalBox : public SPanel"),
		TEXT("Arranges child widgets vertically in a sequence of slots (+ SVerticalBox::Slot())."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("SHorizontalBox"), TEXT("SHorizontalBox"), EIntelliSenseCategory::Type,
		TEXT("class SHorizontalBox : public SPanel"),
		TEXT("Arranges child widgets horizontally in a sequence of slots (+ SHorizontalBox::Slot())."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("SOverlay"), TEXT("SOverlay"), EIntelliSenseCategory::Type,
		TEXT("class SOverlay : public SPanel"),
		TEXT("Layers multiple child widgets on top of each other in the same layout rectangle (+ SOverlay::Slot())."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("SBorder"), TEXT("SBorder"), EIntelliSenseCategory::Type,
		TEXT("class SBorder : public SCompoundWidget"),
		TEXT("Draws a border box image, background color, and padding around a single child widget."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("SButton"), TEXT("SButton"), EIntelliSenseCategory::Type,
		TEXT("class SButton : public SCompoundWidget"),
		TEXT("Clickable push button widget with hover, pressed, and disabled visual states and OnClicked event."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("STextBlock"), TEXT("STextBlock"), EIntelliSenseCategory::Type,
		TEXT("class STextBlock : public SLeafWidget"),
		TEXT("Displays read-only styled text with custom font, color, shadow, wrapping, and justification."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("SCheckBox"), TEXT("SCheckBox"), EIntelliSenseCategory::Type,
		TEXT("class SCheckBox : public SCompoundWidget"),
		TEXT("Check box toggle widget supporting checked, unchecked, and undetermined states with OnCheckStateChanged."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("SScrollBox"), TEXT("SScrollBox"), EIntelliSenseCategory::Type,
		TEXT("class SScrollBox : public SCompoundWidget"),
		TEXT("Provides a scrollable viewport for widgets that overflow their allotted container space."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("SListView"), TEXT("SListView"), EIntelliSenseCategory::Type,
		TEXT("template<typename ItemType> class SListView : public STableViewBase"),
		TEXT("Virtualizing list view that displays item collections dynamically using OnGenerateRow."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("STreeView"), TEXT("STreeView"), EIntelliSenseCategory::Type,
		TEXT("template<typename ItemType> class STreeView : public SListView<ItemType>"),
		TEXT("Hierarchical tree view widget supporting expandable nodes, selection, and OnGetChildren."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("SSplitter"), TEXT("SSplitter"), EIntelliSenseCategory::Type,
		TEXT("class SSplitter : public SPanel"),
		TEXT("Allows users to dynamically resize adjacent slots horizontally or vertically via a draggable handle."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("SImage"), TEXT("SImage"), EIntelliSenseCategory::Type,
		TEXT("class SImage : public SLeafWidget"),
		TEXT("Displays an image brush, slate texture, or dynamic material in the Slate UI layout."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("SSlider"), TEXT("SSlider"), EIntelliSenseCategory::Type,
		TEXT("class SSlider : public SLeafWidget"),
		TEXT("Allows users to choose a numerical value from a continuous range (0.0 to 1.0) by dragging a thumb."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("SProgressBar"), TEXT("SProgressBar"), EIntelliSenseCategory::Type,
		TEXT("class SProgressBar : public SLeafWidget"),
		TEXT("Visual progress bar widget showing percent completion or indeterminate marquee animation."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("SSearchBox"), TEXT("SSearchBox"), EIntelliSenseCategory::Type,
		TEXT("class SSearchBox : public SCompoundWidget"),
		TEXT("Text input box styled specifically for search filters, including search icon and clear button."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("SEditableTextBox"), TEXT("SEditableTextBox"), EIntelliSenseCategory::Type,
		TEXT("class SEditableTextBox : public SBorder"),
		TEXT("Single-line text entry field with border frame and text commit events."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("SMultiLineEditableTextBox"), TEXT("SMultiLineEditableTextBox"), EIntelliSenseCategory::Type,
		TEXT("class SMultiLineEditableTextBox : public SBorder"),
		TEXT("Multi-line text entry field supporting scrolling, cursor positioning, and custom marshallers."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("SBox"), TEXT("SBox"), EIntelliSenseCategory::Type,
		TEXT("class SBox : public SCompoundWidget"),
		TEXT("Layout container allowing explicit control over desired width, height, min/max dimensions."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("SSpacer"), TEXT("SSpacer"), EIntelliSenseCategory::Type,
		TEXT("class SSpacer : public SLeafWidget"),
		TEXT("Empty widget that consumes layout space according to specified size."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("SColorBlock"), TEXT("SColorBlock"), EIntelliSenseCategory::Type,
		TEXT("class SColorBlock : public SLeafWidget"),
		TEXT("Renders a solid color rectangle, optionally supporting alpha checkerboard rendering."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("SNullWidget"), TEXT("SNullWidget::NullWidget"), EIntelliSenseCategory::Type,
		TEXT("static TSharedRef<SWidget> SNullWidget::NullWidget"),
		TEXT("Singleton null widget used as a placeholder where a valid SWidget reference is required."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	// -------------------------------------------------------------------------
	// 3. Unreal Types
	// -------------------------------------------------------------------------
	AddItem(TEXT("FString"), TEXT("FString"), EIntelliSenseCategory::Type,
		TEXT("class FString"),
		TEXT("Dynamic array of TCHAR characters representing mutable strings in Unreal Engine."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/fstring-in-unreal-engine"));

	AddItem(TEXT("FText"), TEXT("FText"), EIntelliSenseCategory::Type,
		TEXT("class FText"),
		TEXT("User-facing display text with support for localization, format arguments, and culture invariance."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/ftext-in-unreal-engine"));

	AddItem(TEXT("FName"), TEXT("FName"), EIntelliSenseCategory::Type,
		TEXT("struct FName"),
		TEXT("Lightweight case-insensitive hashed string identifier for fast comparisons and asset lookups."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/fname-in-unreal-engine"));

	AddItem(TEXT("FLinearColor"), TEXT("FLinearColor"), EIntelliSenseCategory::Type,
		TEXT("struct FLinearColor"),
		TEXT("RGBA color with floating point components in linear color space (0.0 to 1.0)."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/flinearcolor-in-unreal-engine"));

	AddItem(TEXT("FColor"), TEXT("FColor"), EIntelliSenseCategory::Type,
		TEXT("struct FColor"),
		TEXT("RGBA 32-bit color with 8 bits per channel (0 to 255) in sRGB color space."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/fcolor-in-unreal-engine"));

	AddItem(TEXT("FMargin"), TEXT("FMargin("), EIntelliSenseCategory::Type,
		TEXT("struct FMargin(float UniformMargin) or FMargin(float Left, float Top, float Right, float Bottom)"),
		TEXT("Describes outer margin or inner padding for Slate widgets across four borders."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("FReply"), TEXT("FReply"), EIntelliSenseCategory::Type,
		TEXT("struct FReply"),
		TEXT("Return value for Slate input events indicating whether an event was Handled or Unhandled."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("FGeometry"), TEXT("FGeometry"), EIntelliSenseCategory::Type,
		TEXT("struct FGeometry"),
		TEXT("Represents position, size, absolute coordinates, and transformation of a widget in the Slate tree."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("FSlateColor"), TEXT("FSlateColor"), EIntelliSenseCategory::Type,
		TEXT("struct FSlateColor"),
		TEXT("Encapsulates a color that can be dynamically linked to a Slate theme style rule."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("FSlateBrush"), TEXT("FSlateBrush"), EIntelliSenseCategory::Type,
		TEXT("struct FSlateBrush"),
		TEXT("Defines visual styling (box, image, border, or rounded rectangle) for Slate widgets."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("FSlateFontInfo"), TEXT("FSlateFontInfo"), EIntelliSenseCategory::Type,
		TEXT("struct FSlateFontInfo"),
		TEXT("Specifies typeface font object, typeface name, size, outline, and letter spacing for text."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("FVector"), TEXT("FVector"), EIntelliSenseCategory::Type,
		TEXT("struct FVector"),
		TEXT("3D vector of floats/doubles representing spatial positions, directions, or scale."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/fvector-in-unreal-engine"));

	AddItem(TEXT("FVector2D"), TEXT("FVector2D"), EIntelliSenseCategory::Type,
		TEXT("struct FVector2D"),
		TEXT("2D vector of floats/doubles representing 2D screen coordinates or widget sizes."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/fvector2d-in-unreal-engine"));

	AddItem(TEXT("TArray"), TEXT("TArray<"), EIntelliSenseCategory::Type,
		TEXT("template<typename InElementType, typename InAllocatorType = FDefaultAllocator> class TArray"),
		TEXT("Dynamically sized contiguous memory array type in Unreal Engine."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/tarray-in-unreal-engine"));

	AddItem(TEXT("TMap"), TEXT("TMap<"), EIntelliSenseCategory::Type,
		TEXT("template<typename KeyType, typename ValueType> class TMap"),
		TEXT("Associative hash container mapping unique keys to corresponding values."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/tmap-in-unreal-engine"));

	AddItem(TEXT("TSet"), TEXT("TSet<"), EIntelliSenseCategory::Type,
		TEXT("template<typename ElementType> class TSet"),
		TEXT("Fast hash container storing unique elements with O(1) average lookup."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/tset-in-unreal-engine"));

	AddItem(TEXT("TSharedPtr"), TEXT("TSharedPtr<"), EIntelliSenseCategory::Type,
		TEXT("template<class ObjectType, ESPMode Mode = ESPMode::ThreadSafe> class TSharedPtr"),
		TEXT("Reference-counted non-intrusive smart pointer retaining shared ownership of a C++ object."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/asynchronous-smart-pointers-in-unreal-engine"));

	AddItem(TEXT("TSharedRef"), TEXT("TSharedRef<"), EIntelliSenseCategory::Type,
		TEXT("template<class ObjectType, ESPMode Mode = ESPMode::ThreadSafe> class TSharedRef"),
		TEXT("Non-nullable reference-counted smart pointer guaranteeing a valid referenced object."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/asynchronous-smart-pointers-in-unreal-engine"));

	AddItem(TEXT("TWeakPtr"), TEXT("TWeakPtr<"), EIntelliSenseCategory::Type,
		TEXT("template<class ObjectType, ESPMode Mode = ESPMode::ThreadSafe> class TWeakPtr"),
		TEXT("Weak reference to a TSharedPtr-managed object that does not prevent its destruction."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/asynchronous-smart-pointers-in-unreal-engine"));

	AddItem(TEXT("TUniquePtr"), TEXT("TUniquePtr<"), EIntelliSenseCategory::Type,
		TEXT("template<class ObjectType> class TUniquePtr"),
		TEXT("Sole ownership smart pointer that frees the allocated object when out of scope."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/asynchronous-smart-pointers-in-unreal-engine"));

	AddItem(TEXT("TOptional"), TEXT("TOptional<"), EIntelliSenseCategory::Type,
		TEXT("template<typename OptionalType> class TOptional"),
		TEXT("Wraps a value that may or may not be initialized without heap allocation."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/toptional-in-unreal-engine"));

	AddItem(TEXT("FAppStyle"), TEXT("FAppStyle::Get()"), EIntelliSenseCategory::Type,
		TEXT("static const ISlateStyle& FAppStyle::Get()"),
		TEXT("Accesses the primary Unreal Editor application Slate style set for brushes, icons, and fonts."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("FCoreStyle"), TEXT("FCoreStyle::Get()"), EIntelliSenseCategory::Type,
		TEXT("static const ISlateStyle& FCoreStyle::Get()"),
		TEXT("Accesses core Slate styling shared across standalone applications and the engine."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	// -------------------------------------------------------------------------
	// 4. Unreal & Slate Macros
	// -------------------------------------------------------------------------
	AddItem(TEXT("SLATE_BEGIN_ARGS"), TEXT("SLATE_BEGIN_ARGS("), EIntelliSenseCategory::Macro,
		TEXT("SLATE_BEGIN_ARGS(WidgetType)"),
		TEXT("Begins widget argument declaration struct (FArguments) for Slate declarative syntax."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("macro"));

	AddItem(TEXT("SLATE_END_ARGS"), TEXT("SLATE_END_ARGS()"), EIntelliSenseCategory::Macro,
		TEXT("SLATE_END_ARGS()"),
		TEXT("Ends widget argument declaration struct for Slate declarative syntax."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("macro"));

	AddItem(TEXT("SLATE_ARGUMENT"), TEXT("SLATE_ARGUMENT("), EIntelliSenseCategory::Macro,
		TEXT("SLATE_ARGUMENT(ArgType, ArgName)"),
		TEXT("Declares a value argument for widget construction in FArguments."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("macro"));

	AddItem(TEXT("SLATE_ATTRIBUTE"), TEXT("SLATE_ATTRIBUTE("), EIntelliSenseCategory::Macro,
		TEXT("SLATE_ATTRIBUTE(AttrType, AttrName)"),
		TEXT("Declares a dynamic TAttribute argument or delegate binding in FArguments."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("macro"));

	AddItem(TEXT("SLATE_EVENT"), TEXT("SLATE_EVENT("), EIntelliSenseCategory::Macro,
		TEXT("SLATE_EVENT(DelegateType, EventName)"),
		TEXT("Declares an event / callback delegate argument for the widget."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("macro"));

	AddItem(TEXT("SLATE_STYLE_ARGUMENT"), TEXT("SLATE_STYLE_ARGUMENT("), EIntelliSenseCategory::Macro,
		TEXT("SLATE_STYLE_ARGUMENT(StyleType, ArgName)"),
		TEXT("Declares a Slate style pointer argument for custom widget styling."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("macro"));

	AddItem(TEXT("SNew"), TEXT("SNew("), EIntelliSenseCategory::Macro,
		TEXT("TSharedRef<WidgetType> SNew(WidgetType, ...)"),
		TEXT("Constructs and initializes a new Slate widget shared reference."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("macro"));

	AddItem(TEXT("SAssignNew"), TEXT("SAssignNew("), EIntelliSenseCategory::Macro,
		TEXT("TSharedRef<WidgetType> SAssignNew(OutTargetPtr, WidgetType, ...)"),
		TEXT("Constructs a Slate widget and assigns it to a TSharedPtr/TSharedRef variable."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("macro"));

	AddItem(TEXT("UPROPERTY"), TEXT("UPROPERTY()"), EIntelliSenseCategory::Macro,
		TEXT("UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=\"...\")"),
		TEXT("Exposes a member variable to Unreal reflection system, GC, and Blueprint editor."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-uproperties"), TEXT("macro"));

	AddItem(TEXT("UFUNCTION"), TEXT("UFUNCTION()"), EIntelliSenseCategory::Macro,
		TEXT("UFUNCTION(BlueprintCallable, Category=\"...\")"),
		TEXT("Exposes a C++ member function to Unreal reflection, Blueprints, RPCs, and delegates."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-ufunctions"), TEXT("macro"));

	AddItem(TEXT("UCLASS"), TEXT("UCLASS()"), EIntelliSenseCategory::Macro,
		TEXT("UCLASS(BlueprintType, Blueprintable)"),
		TEXT("Declares an Unreal UObject-derived class for reflection, serialization, and asset management."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-uclasses"), TEXT("macro"));

	AddItem(TEXT("USTRUCT"), TEXT("USTRUCT()"), EIntelliSenseCategory::Macro,
		TEXT("USTRUCT(BlueprintType)"),
		TEXT("Declares an Unreal value type struct with reflection and Blueprint support."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-ustructs"), TEXT("macro"));

	AddItem(TEXT("UENUM"), TEXT("UENUM()"), EIntelliSenseCategory::Macro,
		TEXT("UENUM(BlueprintType)"),
		TEXT("Declares an enumerated type for reflection, Blueprint dropdowns, and replication."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-uenums"), TEXT("macro"));

	AddItem(TEXT("GENERATED_BODY"), TEXT("GENERATED_BODY()"), EIntelliSenseCategory::Macro,
		TEXT("GENERATED_BODY()"),
		TEXT("Macro required inside UCLASS, USTRUCT to inject Unreal header tool boilerplate."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-uclasses"), TEXT("macro"));

	AddItem(TEXT("TEXT"), TEXT("TEXT(\"\")"), EIntelliSenseCategory::Macro,
		TEXT("TEXT(StringLiteral)"),
		TEXT("Wraps string literal in platform-native wide character literal (TCHAR)."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/string-handling-in-unreal-engine"), TEXT("macro"));

	AddItem(TEXT("LOCTEXT"), TEXT("LOCTEXT(\"Key\", \"Value\")"), EIntelliSenseCategory::Macro,
		TEXT("LOCTEXT(Key, TextLiteral)"),
		TEXT("Creates a localized FText literal within a localization namespace."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/localization-in-unreal-engine"), TEXT("macro"));

	AddItem(TEXT("check"), TEXT("check("), EIntelliSenseCategory::Macro,
		TEXT("check(Expression)"),
		TEXT("Halts execution if Expression evaluates to false in debug and development builds."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/asserts-in-unreal-engine"), TEXT("macro"));

	AddItem(TEXT("ensure"), TEXT("ensure("), EIntelliSenseCategory::Macro,
		TEXT("ensure(Expression)"),
		TEXT("Logs a callstack on first failure and continues execution; returns true if valid."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/asserts-in-unreal-engine"), TEXT("macro"));

	AddItem(TEXT("ensureMsgf"), TEXT("ensureMsgf("), EIntelliSenseCategory::Macro,
		TEXT("ensureMsgf(Expression, TEXT(\"Format\"), ...)"),
		TEXT("Logs a formatted error message and callstack on failure and continues execution."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/asserts-in-unreal-engine"), TEXT("macro"));

	AddItem(TEXT("UE_LOG"), TEXT("UE_LOG(LogTemp, Log, TEXT(\"\"));"), EIntelliSenseCategory::Macro,
		TEXT("UE_LOG(CategoryName, Verbosity, TEXT(\"Format\"), ...)"),
		TEXT("Emits structured log messages to the Output Log and log files."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/logging-in-unreal-engine"), TEXT("macro"));

	AddItem(TEXT("DECLARE_LOG_CATEGORY_EXTERN"), TEXT("DECLARE_LOG_CATEGORY_EXTERN("), EIntelliSenseCategory::Macro,
		TEXT("DECLARE_LOG_CATEGORY_EXTERN(CategoryName, DefaultVerbosity, CompileTimeVerbosity)"),
		TEXT("Declares a custom log category in a header file."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/logging-in-unreal-engine"), TEXT("macro"));

	AddItem(TEXT("DEFINE_LOG_CATEGORY"), TEXT("DEFINE_LOG_CATEGORY("), EIntelliSenseCategory::Macro,
		TEXT("DEFINE_LOG_CATEGORY(CategoryName)"),
		TEXT("Defines a previously declared custom log category in a C++ source file."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/logging-in-unreal-engine"), TEXT("macro"));

	// -------------------------------------------------------------------------
	// 5. Delegates & Events
	// -------------------------------------------------------------------------
	AddItem(TEXT("DECLARE_DELEGATE"), TEXT("DECLARE_DELEGATE("), EIntelliSenseCategory::Delegate,
		TEXT("DECLARE_DELEGATE(DelegateName)"),
		TEXT("Declares a single-cast delegate with void return type and no parameters."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/delegates-in-unreal-engine"), TEXT("delegate"));

	AddItem(TEXT("DECLARE_DELEGATE_OneParam"), TEXT("DECLARE_DELEGATE_OneParam("), EIntelliSenseCategory::Delegate,
		TEXT("DECLARE_DELEGATE_OneParam(DelegateName, Param1Type)"),
		TEXT("Declares a single-cast delegate taking one parameter."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/delegates-in-unreal-engine"), TEXT("delegate"));

	AddItem(TEXT("DECLARE_DELEGATE_TwoParams"), TEXT("DECLARE_DELEGATE_TwoParams("), EIntelliSenseCategory::Delegate,
		TEXT("DECLARE_DELEGATE_TwoParams(DelegateName, Param1Type, Param2Type)"),
		TEXT("Declares a single-cast delegate taking two parameters."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/delegates-in-unreal-engine"), TEXT("delegate"));

	AddItem(TEXT("DECLARE_DELEGATE_ThreeParams"), TEXT("DECLARE_DELEGATE_ThreeParams("), EIntelliSenseCategory::Delegate,
		TEXT("DECLARE_DELEGATE_ThreeParams(DelegateName, Param1Type, Param2Type, Param3Type)"),
		TEXT("Declares a single-cast delegate taking three parameters."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/delegates-in-unreal-engine"), TEXT("delegate"));

	AddItem(TEXT("DECLARE_DELEGATE_RetVal"), TEXT("DECLARE_DELEGATE_RetVal("), EIntelliSenseCategory::Delegate,
		TEXT("DECLARE_DELEGATE_RetVal(ReturnType, DelegateName)"),
		TEXT("Declares a single-cast delegate returning ReturnType with no parameters."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/delegates-in-unreal-engine"), TEXT("delegate"));

	AddItem(TEXT("DECLARE_DELEGATE_RetVal_OneParam"), TEXT("DECLARE_DELEGATE_RetVal_OneParam("), EIntelliSenseCategory::Delegate,
		TEXT("DECLARE_DELEGATE_RetVal_OneParam(ReturnType, DelegateName, Param1Type)"),
		TEXT("Declares a single-cast delegate returning ReturnType and taking one parameter."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/delegates-in-unreal-engine"), TEXT("delegate"));

	AddItem(TEXT("DECLARE_MULTICAST_DELEGATE"), TEXT("DECLARE_MULTICAST_DELEGATE("), EIntelliSenseCategory::Delegate,
		TEXT("DECLARE_MULTICAST_DELEGATE(DelegateName)"),
		TEXT("Declares a multicast delegate that can broadcast events to multiple listeners."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/delegates-in-unreal-engine"), TEXT("delegate"));

	AddItem(TEXT("DECLARE_MULTICAST_DELEGATE_OneParam"), TEXT("DECLARE_MULTICAST_DELEGATE_OneParam("), EIntelliSenseCategory::Delegate,
		TEXT("DECLARE_MULTICAST_DELEGATE_OneParam(DelegateName, Param1Type)"),
		TEXT("Declares a multicast delegate taking one parameter."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/delegates-in-unreal-engine"), TEXT("delegate"));

	AddItem(TEXT("DECLARE_MULTICAST_DELEGATE_TwoParams"), TEXT("DECLARE_MULTICAST_DELEGATE_TwoParams("), EIntelliSenseCategory::Delegate,
		TEXT("DECLARE_MULTICAST_DELEGATE_TwoParams(DelegateName, Param1Type, Param2Type)"),
		TEXT("Declares a multicast delegate taking two parameters."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/delegates-in-unreal-engine"), TEXT("delegate"));

	AddItem(TEXT("DECLARE_DYNAMIC_MULTICAST_DELEGATE"), TEXT("DECLARE_DYNAMIC_MULTICAST_DELEGATE("), EIntelliSenseCategory::Delegate,
		TEXT("DECLARE_DYNAMIC_MULTICAST_DELEGATE(DelegateName)"),
		TEXT("Declares a serializable multicast delegate bindable in Blueprints."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/delegates-in-unreal-engine"), TEXT("delegate"));

	AddItem(TEXT("DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam"), TEXT("DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam("), EIntelliSenseCategory::Delegate,
		TEXT("DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(DelegateName, Param1Type, Param1Name)"),
		TEXT("Declares a serializable multicast delegate taking one parameter bindable in Blueprints."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/delegates-in-unreal-engine"), TEXT("delegate"));

	AddItem(TEXT("FOnClicked"), TEXT("FOnClicked"), EIntelliSenseCategory::Delegate,
		TEXT("DECLARE_DELEGATE_RetVal(FReply, FOnClicked)"),
		TEXT("Standard Slate button click callback delegate returning FReply."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("delegate"));

	AddItem(TEXT("FOnTextChanged"), TEXT("FOnTextChanged"), EIntelliSenseCategory::Delegate,
		TEXT("DECLARE_DELEGATE_OneParam(FOnTextChanged, const FText&)"),
		TEXT("Fired whenever text changes in an editable text box or search box."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("delegate"));

	AddItem(TEXT("FOnTextCommitted"), TEXT("FOnTextCommitted"), EIntelliSenseCategory::Delegate,
		TEXT("DECLARE_DELEGATE_TwoParams(FOnTextCommitted, const FText&, ETextCommit::Type)"),
		TEXT("Fired when user commits text in input widgets via Enter key or loss of keyboard focus."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("delegate"));

	AddItem(TEXT("FOnCheckStateChanged"), TEXT("FOnCheckStateChanged"), EIntelliSenseCategory::Delegate,
		TEXT("DECLARE_DELEGATE_OneParam(FOnCheckStateChanged, ECheckBoxState)"),
		TEXT("Fired when an SCheckBox state changes between Checked, Unchecked, and Undetermined."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("delegate"));

	AddItem(TEXT("FOnGenerateRow"), TEXT("FOnGenerateRow"), EIntelliSenseCategory::Delegate,
		TEXT("TSharedRef<ITableRow> OnGenerateRow(ItemType Item, const TSharedRef<STableViewBase>& OwnerTable)"),
		TEXT("Delegate to create a table row widget for each item in SListView or STreeView."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("delegate"));

	AddItem(TEXT("FSimpleDelegate"), TEXT("FSimpleDelegate"), EIntelliSenseCategory::Delegate,
		TEXT("DECLARE_DELEGATE(FSimpleDelegate)"),
		TEXT("Standard engine delegate taking zero arguments and returning void."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/delegates-in-unreal-engine"), TEXT("delegate"));

	// -------------------------------------------------------------------------
	// 6. Slot & Chained Layout Methods (Scope = "slot")
	// -------------------------------------------------------------------------
	AddItem(TEXT("Padding"), TEXT("Padding("), EIntelliSenseCategory::Method,
		TEXT("FSlot& Padding(const FMargin& InMargin) or Padding(float Left, float Top, float Right, float Bottom)"),
		TEXT("Sets outer padding around the widget inside this slot."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("slot"));

	AddItem(TEXT("AutoHeight"), TEXT("AutoHeight()"), EIntelliSenseCategory::Method,
		TEXT("FSlot& AutoHeight()"),
		TEXT("Sizes the vertical slot automatically to fit the desired height of its child widget."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("slot"));

	AddItem(TEXT("AutoWidth"), TEXT("AutoWidth()"), EIntelliSenseCategory::Method,
		TEXT("FSlot& AutoWidth()"),
		TEXT("Sizes the horizontal slot automatically to fit the desired width of its child widget."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("slot"));

	AddItem(TEXT("FillHeight"), TEXT("FillHeight("), EIntelliSenseCategory::Method,
		TEXT("FSlot& FillHeight(float InValue)"),
		TEXT("Expands the slot vertically to take proportional share of available container height."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("slot"));

	AddItem(TEXT("FillWidth"), TEXT("FillWidth("), EIntelliSenseCategory::Method,
		TEXT("FSlot& FillWidth(float InValue)"),
		TEXT("Expands the slot horizontally to take proportional share of available container width."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("slot"));

	AddItem(TEXT("HAlign"), TEXT("HAlign("), EIntelliSenseCategory::Method,
		TEXT("FSlot& HAlign(EHorizontalAlignment InHAlign)"),
		TEXT("Aligns the child widget horizontally within the slot (HAlign_Left, HAlign_Center, HAlign_Right, HAlign_Fill)."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("slot"));

	AddItem(TEXT("VAlign"), TEXT("VAlign("), EIntelliSenseCategory::Method,
		TEXT("FSlot& VAlign(EVerticalAlignment InVAlign)"),
		TEXT("Aligns the child widget vertically within the slot (VAlign_Top, VAlign_Center, VAlign_Bottom, VAlign_Fill)."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("slot"));

	AddItem(TEXT("AttachWidget"), TEXT("AttachWidget("), EIntelliSenseCategory::Method,
		TEXT("void AttachWidget(const TSharedRef<SWidget>& InWidget)"),
		TEXT("Attaches a child widget directly to this slot container."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("slot"));

	AddItem(TEXT("MaxDesiredHeight"), TEXT("MaxDesiredHeight("), EIntelliSenseCategory::Method,
		TEXT("FSlot& MaxDesiredHeight(float Height)"),
		TEXT("Sets upper boundary on the height this slot will occupy."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("slot"));

	AddItem(TEXT("MaxDesiredWidth"), TEXT("MaxDesiredWidth("), EIntelliSenseCategory::Method,
		TEXT("FSlot& MaxDesiredWidth(float Width)"),
		TEXT("Sets upper boundary on the width this slot will occupy."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("slot"));

	AddItem(TEXT("Slot"), TEXT("Slot()"), EIntelliSenseCategory::Method,
		TEXT("+ Container::Slot()"),
		TEXT("Declares a new child slot in a box panel (SVerticalBox, SHorizontalBox, SOverlay)."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("slot"));

	// -------------------------------------------------------------------------
	// 7. Smart Pointer Methods (Scope = "pointer", triggered after "->")
	// -------------------------------------------------------------------------
	AddItem(TEXT("IsValid"), TEXT("IsValid()"), EIntelliSenseCategory::Method,
		TEXT("bool IsValid() const"),
		TEXT("Returns true if the smart pointer is non-null and references a live C++ object."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/asynchronous-smart-pointers-in-unreal-engine"), TEXT("pointer"));

	AddItem(TEXT("Get"), TEXT("Get()"), EIntelliSenseCategory::Method,
		TEXT("ObjectType* Get() const"),
		TEXT("Returns raw C++ pointer without transferring ownership. Use with care."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/asynchronous-smart-pointers-in-unreal-engine"), TEXT("pointer"));

	AddItem(TEXT("ToSharedRef"), TEXT("ToSharedRef()"), EIntelliSenseCategory::Method,
		TEXT("TSharedRef<ObjectType> ToSharedRef() const"),
		TEXT("Converts a valid TSharedPtr into a non-nullable TSharedRef. Asserts if pointer is null."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/asynchronous-smart-pointers-in-unreal-engine"), TEXT("pointer"));

	AddItem(TEXT("Reset"), TEXT("Reset()"), EIntelliSenseCategory::Method,
		TEXT("void Reset()"),
		TEXT("Clears the pointer reference and decrements the target object reference count."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/asynchronous-smart-pointers-in-unreal-engine"), TEXT("pointer"));

	// -------------------------------------------------------------------------
	// 8. String Methods (Scope = "string", triggered after ".")
	// -------------------------------------------------------------------------
	AddItem(TEXT("Len"), TEXT("Len()"), EIntelliSenseCategory::Method,
		TEXT("int32 Len() const"),
		TEXT("Returns total character count of the string."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/fstring-in-unreal-engine"), TEXT("string"));

	AddItem(TEXT("IsEmpty"), TEXT("IsEmpty()"), EIntelliSenseCategory::Method,
		TEXT("bool IsEmpty() const"),
		TEXT("Returns true if string has zero characters."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/fstring-in-unreal-engine"), TEXT("string"));

	AddItem(TEXT("StartsWith"), TEXT("StartsWith("), EIntelliSenseCategory::Method,
		TEXT("bool StartsWith(const FString& InPrefix, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase) const"),
		TEXT("Returns true if string begins with specified prefix."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/fstring-in-unreal-engine"), TEXT("string"));

	AddItem(TEXT("EndsWith"), TEXT("EndsWith("), EIntelliSenseCategory::Method,
		TEXT("bool EndsWith(const FString& InSuffix, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase) const"),
		TEXT("Returns true if string ends with specified suffix."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/fstring-in-unreal-engine"), TEXT("string"));

	AddItem(TEXT("Contains"), TEXT("Contains("), EIntelliSenseCategory::Method,
		TEXT("bool Contains(const FString& SubStr, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase) const"),
		TEXT("Returns true if string contains substring anywhere."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/fstring-in-unreal-engine"), TEXT("string"));

	AddItem(TEXT("Mid"), TEXT("Mid("), EIntelliSenseCategory::Method,
		TEXT("FString Mid(int32 Start, int32 Count = MAX_int32) const"),
		TEXT("Extracts a substring starting at Start index for Count characters."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/fstring-in-unreal-engine"), TEXT("string"));

	AddItem(TEXT("Left"), TEXT("Left("), EIntelliSenseCategory::Method,
		TEXT("FString Left(int32 Count) const"),
		TEXT("Returns the leftmost Count characters of the string."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/fstring-in-unreal-engine"), TEXT("string"));

	AddItem(TEXT("Right"), TEXT("Right("), EIntelliSenseCategory::Method,
		TEXT("FString Right(int32 Count) const"),
		TEXT("Returns the rightmost Count characters of the string."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/fstring-in-unreal-engine"), TEXT("string"));

	AddItem(TEXT("TrimStartAndEnd"), TEXT("TrimStartAndEnd()"), EIntelliSenseCategory::Method,
		TEXT("FString TrimStartAndEnd() const"),
		TEXT("Returns copy of string stripped of leading and trailing whitespace."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/fstring-in-unreal-engine"), TEXT("string"));

	AddItem(TEXT("TrimStart"), TEXT("TrimStart()"), EIntelliSenseCategory::Method,
		TEXT("FString TrimStart() const"),
		TEXT("Returns copy of string stripped of leading whitespace."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/fstring-in-unreal-engine"), TEXT("string"));

	AddItem(TEXT("TrimEnd"), TEXT("TrimEnd()"), EIntelliSenseCategory::Method,
		TEXT("FString TrimEnd() const"),
		TEXT("Returns copy of string stripped of trailing whitespace."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/fstring-in-unreal-engine"), TEXT("string"));

	AddItem(TEXT("ToUpper"), TEXT("ToUpper()"), EIntelliSenseCategory::Method,
		TEXT("FString ToUpper() const"),
		TEXT("Returns uppercase copy of the string."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/fstring-in-unreal-engine"), TEXT("string"));

	AddItem(TEXT("ToLower"), TEXT("ToLower()"), EIntelliSenseCategory::Method,
		TEXT("FString ToLower() const"),
		TEXT("Returns lowercase copy of the string."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/fstring-in-unreal-engine"), TEXT("string"));

	AddItem(TEXT("Equals"), TEXT("Equals("), EIntelliSenseCategory::Method,
		TEXT("bool Equals(const FString& Other, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive) const"),
		TEXT("Compares this string to another string."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/fstring-in-unreal-engine"), TEXT("string"));

	// -------------------------------------------------------------------------
	// 9. Array Methods (Scope = "array", triggered after ".")
	// -------------------------------------------------------------------------
	AddItem(TEXT("Num"), TEXT("Num()"), EIntelliSenseCategory::Method,
		TEXT("int32 Num() const"),
		TEXT("Returns total number of elements currently stored in array."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/tarray-in-unreal-engine"), TEXT("array"));

	AddItem(TEXT("Add"), TEXT("Add("), EIntelliSenseCategory::Method,
		TEXT("int32 Add(const ElementType& Item)"),
		TEXT("Appends element to end of array and returns index of added item."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/tarray-in-unreal-engine"), TEXT("array"));

	AddItem(TEXT("Emplace"), TEXT("Emplace("), EIntelliSenseCategory::Method,
		TEXT("int32 Emplace(Args&&... InArgs)"),
		TEXT("Constructs element directly in place at end of array without copying."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/tarray-in-unreal-engine"), TEXT("array"));

	AddItem(TEXT("RemoveAt"), TEXT("RemoveAt("), EIntelliSenseCategory::Method,
		TEXT("void RemoveAt(int32 Index, int32 Count = 1, bool bAllowShrinking = true)"),
		TEXT("Removes elements at Index shifting subsequent elements left."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/tarray-in-unreal-engine"), TEXT("array"));

	AddItem(TEXT("Empty"), TEXT("Empty("), EIntelliSenseCategory::Method,
		TEXT("void Empty(int32 ExpectedNumElements = 0)"),
		TEXT("Clears all elements and deallocates memory buffer."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/tarray-in-unreal-engine"), TEXT("array"));

	AddItem(TEXT("IndexOfByKey"), TEXT("IndexOfByKey("), EIntelliSenseCategory::Method,
		TEXT("int32 IndexOfByKey(const KeyType& Key) const"),
		TEXT("Finds index of first matching element or returns INDEX_NONE."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/tarray-in-unreal-engine"), TEXT("array"));

	AddItem(TEXT("IsValidIndex"), TEXT("IsValidIndex("), EIntelliSenseCategory::Method,
		TEXT("bool IsValidIndex(int32 Index) const"),
		TEXT("Returns true if Index is >= 0 and < Num()."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/tarray-in-unreal-engine"), TEXT("array"));

	// -------------------------------------------------------------------------
	// 10. Color & Margin Fields & Methods (Scope = "color", "margin", triggered after ".")
	// -------------------------------------------------------------------------
	AddItem(TEXT("R"), TEXT("R"), EIntelliSenseCategory::Field,
		TEXT("float R"),
		TEXT("Red color channel component (0.0 to 1.0)."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/flinearcolor-in-unreal-engine"), TEXT("color"));

	AddItem(TEXT("G"), TEXT("G"), EIntelliSenseCategory::Field,
		TEXT("float G"),
		TEXT("Green color channel component (0.0 to 1.0)."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/flinearcolor-in-unreal-engine"), TEXT("color"));

	AddItem(TEXT("B"), TEXT("B"), EIntelliSenseCategory::Field,
		TEXT("float B"),
		TEXT("Blue color channel component (0.0 to 1.0)."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/flinearcolor-in-unreal-engine"), TEXT("color"));

	AddItem(TEXT("A"), TEXT("A"), EIntelliSenseCategory::Field,
		TEXT("float A"),
		TEXT("Alpha / opacity channel component (0.0 = fully transparent, 1.0 = fully opaque)."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/flinearcolor-in-unreal-engine"), TEXT("color"));

	AddItem(TEXT("ToFColor"), TEXT("ToFColor("), EIntelliSenseCategory::Method,
		TEXT("FColor ToFColor(bool bSRGB = true) const"),
		TEXT("Converts floating point linear color to 32-bit FColor representation."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/flinearcolor-in-unreal-engine"), TEXT("color"));

	AddItem(TEXT("Left"), TEXT("Left"), EIntelliSenseCategory::Field,
		TEXT("float Left"),
		TEXT("Left margin or padding in Slate units."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("margin"));

	AddItem(TEXT("Top"), TEXT("Top"), EIntelliSenseCategory::Field,
		TEXT("float Top"),
		TEXT("Top margin or padding in Slate units."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("margin"));

	AddItem(TEXT("Right"), TEXT("Right"), EIntelliSenseCategory::Field,
		TEXT("float Right"),
		TEXT("Right margin or padding in Slate units."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("margin"));

	AddItem(TEXT("Bottom"), TEXT("Bottom"), EIntelliSenseCategory::Field,
		TEXT("float Bottom"),
		TEXT("Bottom margin or padding in Slate units."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("margin"));

	AddItem(TEXT("GetTotalPadding"), TEXT("GetTotalPadding()"), EIntelliSenseCategory::Method,
		TEXT("FVector2D GetTotalPadding() const"),
		TEXT("Returns sum of horizontal (Left + Right) and vertical (Top + Bottom) padding."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"), TEXT("margin"));

	// -------------------------------------------------------------------------
	// 11. Enums & Values
	// -------------------------------------------------------------------------
	AddItem(TEXT("ECheckBoxState"), TEXT("ECheckBoxState"), EIntelliSenseCategory::Enum,
		TEXT("enum class ECheckBoxState : uint8 { Unchecked, Checked, Undetermined };"),
		TEXT("Tri-state check box state enumeration."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("ECheckBoxState::Checked"), TEXT("ECheckBoxState::Checked"), EIntelliSenseCategory::Enum,
		TEXT("ECheckBoxState::Checked"),
		TEXT("Check box is currently checked/active."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("ECheckBoxState::Unchecked"), TEXT("ECheckBoxState::Unchecked"), EIntelliSenseCategory::Enum,
		TEXT("ECheckBoxState::Unchecked"),
		TEXT("Check box is currently unchecked/inactive."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("ECheckBoxState::Undetermined"), TEXT("ECheckBoxState::Undetermined"), EIntelliSenseCategory::Enum,
		TEXT("ECheckBoxState::Undetermined"),
		TEXT("Check box state is neither checked nor unchecked (mixed state)."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("EVisibility"), TEXT("EVisibility"), EIntelliSenseCategory::Enum,
		TEXT("enum class EVisibility : uint8 { Visible, Collapsed, Hidden, HitTestInvisible, SelfHitTestInvisible };"),
		TEXT("Slate widget visibility and hit testing behavior."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("EVisibility::Visible"), TEXT("EVisibility::Visible"), EIntelliSenseCategory::Enum,
		TEXT("EVisibility::Visible"),
		TEXT("Visible and interactive to mouse/keyboard events."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("EVisibility::Collapsed"), TEXT("EVisibility::Collapsed"), EIntelliSenseCategory::Enum,
		TEXT("EVisibility::Collapsed"),
		TEXT("Invisible and takes zero layout space in container."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("EVisibility::Hidden"), TEXT("EVisibility::Hidden"), EIntelliSenseCategory::Enum,
		TEXT("EVisibility::Hidden"),
		TEXT("Invisible but reserves layout space in container."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("EVisibility::HitTestInvisible"), TEXT("EVisibility::HitTestInvisible"), EIntelliSenseCategory::Enum,
		TEXT("EVisibility::HitTestInvisible"),
		TEXT("Visible but completely non-interactive (passes clicks to widgets below)."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("EOrientation"), TEXT("EOrientation"), EIntelliSenseCategory::Enum,
		TEXT("enum EOrientation { Orient_Horizontal, Orient_Vertical };"),
		TEXT("Specifies horizontal or vertical orientation."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("Orient_Horizontal"), TEXT("Orient_Horizontal"), EIntelliSenseCategory::Enum,
		TEXT("Orient_Horizontal"),
		TEXT("Arranges elements horizontally."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("Orient_Vertical"), TEXT("Orient_Vertical"), EIntelliSenseCategory::Enum,
		TEXT("Orient_Vertical"),
		TEXT("Arranges elements vertically."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("HAlign_Fill"), TEXT("HAlign_Fill"), EIntelliSenseCategory::Enum,
		TEXT("HAlign_Fill"),
		TEXT("Stretches child widget to fill slot width."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("HAlign_Left"), TEXT("HAlign_Left"), EIntelliSenseCategory::Enum,
		TEXT("HAlign_Left"),
		TEXT("Aligns child widget to the left of slot."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("HAlign_Center"), TEXT("HAlign_Center"), EIntelliSenseCategory::Enum,
		TEXT("HAlign_Center"),
		TEXT("Aligns child widget to horizontal center of slot."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("HAlign_Right"), TEXT("HAlign_Right"), EIntelliSenseCategory::Enum,
		TEXT("HAlign_Right"),
		TEXT("Aligns child widget to the right of slot."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("VAlign_Fill"), TEXT("VAlign_Fill"), EIntelliSenseCategory::Enum,
		TEXT("VAlign_Fill"),
		TEXT("Stretches child widget to fill slot height."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("VAlign_Top"), TEXT("VAlign_Top"), EIntelliSenseCategory::Enum,
		TEXT("VAlign_Top"),
		TEXT("Aligns child widget to top of slot."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("VAlign_Center"), TEXT("VAlign_Center"), EIntelliSenseCategory::Enum,
		TEXT("VAlign_Center"),
		TEXT("Aligns child widget to vertical center of slot."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("VAlign_Bottom"), TEXT("VAlign_Bottom"), EIntelliSenseCategory::Enum,
		TEXT("VAlign_Bottom"),
		TEXT("Aligns child widget to bottom of slot."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("FLinearColor::White"), TEXT("FLinearColor::White"), EIntelliSenseCategory::Enum,
		TEXT("const FLinearColor FLinearColor::White = (1, 1, 1, 1)"),
		TEXT("Solid opaque white."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/flinearcolor-in-unreal-engine"));

	AddItem(TEXT("FLinearColor::Black"), TEXT("FLinearColor::Black"), EIntelliSenseCategory::Enum,
		TEXT("const FLinearColor FLinearColor::Black = (0, 0, 0, 1)"),
		TEXT("Solid opaque black."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/flinearcolor-in-unreal-engine"));

	AddItem(TEXT("FLinearColor::Red"), TEXT("FLinearColor::Red"), EIntelliSenseCategory::Enum,
		TEXT("const FLinearColor FLinearColor::Red = (1, 0, 0, 1)"),
		TEXT("Solid opaque red."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/flinearcolor-in-unreal-engine"));

	AddItem(TEXT("FLinearColor::Green"), TEXT("FLinearColor::Green"), EIntelliSenseCategory::Enum,
		TEXT("const FLinearColor FLinearColor::Green = (0, 1, 0, 1)"),
		TEXT("Solid opaque green."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/flinearcolor-in-unreal-engine"));

	AddItem(TEXT("FLinearColor::Blue"), TEXT("FLinearColor::Blue"), EIntelliSenseCategory::Enum,
		TEXT("const FLinearColor FLinearColor::Blue = (0, 0, 1, 1)"),
		TEXT("Solid opaque blue."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/flinearcolor-in-unreal-engine"));

	AddItem(TEXT("FLinearColor::Yellow"), TEXT("FLinearColor::Yellow"), EIntelliSenseCategory::Enum,
		TEXT("const FLinearColor FLinearColor::Yellow = (1, 1, 0, 1)"),
		TEXT("Solid opaque yellow."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/flinearcolor-in-unreal-engine"));

	AddItem(TEXT("FLinearColor::Transparent"), TEXT("FLinearColor::Transparent"), EIntelliSenseCategory::Enum,
		TEXT("const FLinearColor FLinearColor::Transparent = (0, 0, 0, 0)"),
		TEXT("Completely transparent color with 0 alpha."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/flinearcolor-in-unreal-engine"));

	AddItem(TEXT("FReply::Handled()"), TEXT("FReply::Handled()"), EIntelliSenseCategory::Enum,
		TEXT("static FReply FReply::Handled()"),
		TEXT("Signals to Slate that the input event was consumed and will not bubble up to parents."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("FReply::Unhandled()"), TEXT("FReply::Unhandled()"), EIntelliSenseCategory::Enum,
		TEXT("static FReply FReply::Unhandled()"),
		TEXT("Signals to Slate that the input event was ignored and should bubble up to parent widgets."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	// -------------------------------------------------------------------------
	// 12. Common Slate Variables
	// -------------------------------------------------------------------------
	AddItem(TEXT("ChildSlot"), TEXT("ChildSlot"), EIntelliSenseCategory::Variable,
		TEXT("FSimpleSlot& ChildSlot"),
		TEXT("The single child slot container belonging to SCompoundWidget subclasses."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("InArgs"), TEXT("InArgs"), EIntelliSenseCategory::Variable,
		TEXT("const FArguments& InArgs"),
		TEXT("Construct arguments struct passed into SWidget::Construct()."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	// -------------------------------------------------------------------------
	// 13. Slate Snippets (Fast UI authoring)
	// -------------------------------------------------------------------------
	AddItem(TEXT("sbtn"),
		TEXT("SNew(SButton)\n.OnClicked_Lambda([]() -> FReply\n{\n\treturn FReply::Handled();\n})\n[\n\tSNew(STextBlock).Text(FText::FromString(TEXT(\"Click Me\")))\n]"),
		EIntelliSenseCategory::Macro,
		TEXT("Snippet: SButton with OnClicked lambda"),
		TEXT("Generates a complete interactive SButton block with text child."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("svbox"),
		TEXT("SNew(SVerticalBox)\n+ SVerticalBox::Slot()\n.AutoHeight()\n[\n\t\n]"),
		EIntelliSenseCategory::Macro,
		TEXT("Snippet: SVerticalBox with Slot"),
		TEXT("Generates a vertical layout container with an auto-height slot."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("shbox"),
		TEXT("SNew(SHorizontalBox)\n+ SHorizontalBox::Slot()\n.AutoWidth()\n[\n\t\n]"),
		EIntelliSenseCategory::Macro,
		TEXT("Snippet: SHorizontalBox with Slot"),
		TEXT("Generates a horizontal layout container with an auto-width slot."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("sborder"),
		TEXT("SNew(SBorder)\n.BorderImage(FAppStyle::Get().GetBrush(\"ToolPanel.GroupBorder\"))\n.Padding(4.0f)\n[\n\t\n]"),
		EIntelliSenseCategory::Macro,
		TEXT("Snippet: SBorder styled panel"),
		TEXT("Generates a styled border container box with padding."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("stext"),
		TEXT("SNew(STextBlock)\n.Text(FText::FromString(TEXT(\"Sample Text\")))\n.Font(FCoreStyle::GetDefaultFontStyle(\"Regular\", 10))"),
		EIntelliSenseCategory::Macro,
		TEXT("Snippet: STextBlock with font"),
		TEXT("Generates a styled text display block."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("slist"),
		TEXT("SNew(SListView<TSharedPtr<FString>>)\n.ListItemsSource(&Items)\n.OnGenerateRow(this, &SMyWidget::OnGenerateRow)"),
		EIntelliSenseCategory::Macro,
		TEXT("Snippet: SListView setup"),
		TEXT("Generates a virtualized list view declaration."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	AddItem(TEXT("soverlay"),
		TEXT("SNew(SOverlay)\n+ SOverlay::Slot()\n[\n\t\n]"),
		EIntelliSenseCategory::Macro,
		TEXT("Snippet: SOverlay layered layout"),
		TEXT("Generates an overlay panel for stacking widgets."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-in-unreal-engine"));

	// -------------------------------------------------------------------------
	// 14. Core Unreal Engine Gameplay Classes & Framework
	// -------------------------------------------------------------------------
	AddItem(TEXT("AActor"), TEXT("AActor"), EIntelliSenseCategory::Type,
		TEXT("class AActor : public UObject"),
		TEXT("Base class for an Object that can be placed or spawned in a level. Actors support components, replication, transforms, and receive BeginPlay and Tick events."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/actors-in-unreal-engine"));

	AddItem(TEXT("APawn"), TEXT("APawn"), EIntelliSenseCategory::Type,
		TEXT("class APawn : public AActor"),
		TEXT("An Actor that can be possessed by Players or AI controllers. Serves as physical or virtual agent in the game world."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/pawns-in-unreal-engine"));

	AddItem(TEXT("ACharacter"), TEXT("ACharacter"), EIntelliSenseCategory::Type,
		TEXT("class ACharacter : public APawn"),
		TEXT("Specialized humanoid Pawn featuring CharacterMovementComponent, CapsuleComponent collision, and network-synchronized locomotion."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/characters-in-unreal-engine"));

	AddItem(TEXT("APlayerController"), TEXT("APlayerController"), EIntelliSenseCategory::Type,
		TEXT("class APlayerController : public AController"),
		TEXT("Base class for player controllers that consume input and translate player actions into Pawn movement or gameplay actions."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/player-controllers-in-unreal-engine"));

	AddItem(TEXT("AGameModeBase"), TEXT("AGameModeBase"), EIntelliSenseCategory::Type,
		TEXT("class AGameModeBase : public AInfo"),
		TEXT("Defines the rules of the game being played, default pawn and controller classes, spectator rules, and match lifecycle."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/game-mode-and-game-state-in-unreal-engine"));

	AddItem(TEXT("UObject"), TEXT("UObject"), EIntelliSenseCategory::Type,
		TEXT("class UObject"),
		TEXT("The base class of all Unreal Engine objects. Provides garbage collection, reflection metadata, serialization, and networking support."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/objects-in-unreal-engine"));

	AddItem(TEXT("UWorld"), TEXT("UWorld"), EIntelliSenseCategory::Type,
		TEXT("class UWorld : public UObject"),
		TEXT("Top-level simulation container representing a map or virtual environment, housing Levels, Actors, physics scenes, and timers."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-uclasses"));

	AddItem(TEXT("UActorComponent"), TEXT("UActorComponent"), EIntelliSenseCategory::Type,
		TEXT("class UActorComponent : public UObject"),
		TEXT("Base class for components providing reusable, modular behaviors and functionality that can be attached to Actors."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/components-in-unreal-engine"));

	AddItem(TEXT("USceneComponent"), TEXT("USceneComponent"), EIntelliSenseCategory::Type,
		TEXT("class USceneComponent : public UActorComponent"),
		TEXT("ActorComponent that possesses a transform (location, rotation, scale) and supports hierarchical attachment to other SceneComponents."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/components-in-unreal-engine"));

	AddItem(TEXT("UPrimitiveComponent"), TEXT("UPrimitiveComponent"), EIntelliSenseCategory::Type,
		TEXT("class UPrimitiveComponent : public USceneComponent"),
		TEXT("SceneComponent that contains or generates geometry for rendering and physics simulation (e.g. static meshes, collision shapes)."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/components-in-unreal-engine"));

	AddItem(TEXT("UStaticMeshComponent"), TEXT("UStaticMeshComponent"), EIntelliSenseCategory::Type,
		TEXT("class UStaticMeshComponent : public UMeshComponent"),
		TEXT("Renders an instance of a static mesh asset and handles collision, physics, and material assignments."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/components-in-unreal-engine"));

	AddItem(TEXT("USkeletalMeshComponent"), TEXT("USkeletalMeshComponent"), EIntelliSenseCategory::Type,
		TEXT("class USkeletalMeshComponent : public USkinnedMeshComponent"),
		TEXT("Renders an animated 3D character or skeletal asset with bone hierarchy, skinning, morph targets, and physics simulation."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/components-in-unreal-engine"));

	AddItem(TEXT("UCameraComponent"), TEXT("UCameraComponent"), EIntelliSenseCategory::Type,
		TEXT("class UCameraComponent : public USceneComponent"),
		TEXT("Represents a camera viewpoint and perspective projection settings inside an Actor."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/components-in-unreal-engine"));

	AddItem(TEXT("USpringArmComponent"), TEXT("USpringArmComponent"), EIntelliSenseCategory::Type,
		TEXT("class USpringArmComponent : public USceneComponent"),
		TEXT("Maintains a fixed distance from parent component (camera boom) and automatically retracts on geometry collision."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/components-in-unreal-engine"));

	AddItem(TEXT("UGameplayStatics"), TEXT("UGameplayStatics"), EIntelliSenseCategory::Type,
		TEXT("class UGameplayStatics : public UBlueprintFunctionLibrary"),
		TEXT("Static helper library for common gameplay actions: spawning actors, playing sounds, particle effects, damage, and level opening."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-statics-in-unreal-engine"));

	AddItem(TEXT("FRotator"), TEXT("FRotator"), EIntelliSenseCategory::Type,
		TEXT("struct FRotator(float InPitch, float InYaw, float InRoll)"),
		TEXT("Rotation angle container defining orientation using Pitch (Y-axis), Yaw (Z-axis), and Roll (X-axis) in degrees."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/frotator-in-unreal-engine"));

	AddItem(TEXT("FTransform"), TEXT("FTransform"), EIntelliSenseCategory::Type,
		TEXT("struct FTransform(const FQuat& InRot, const FVector& InTranslation, const FVector& InScale3D)"),
		TEXT("Combines 3D translation (FVector), rotation (FQuat), and 3D scale into a unified spatial transformation."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/ftransform-in-unreal-engine"));

	AddItem(TEXT("TSubclassOf"), TEXT("TSubclassOf<"), EIntelliSenseCategory::Type,
		TEXT("template<class TClass> class TSubclassOf"),
		TEXT("Type-safe template wrapper around UClass* that guarantees the assigned class derives from TClass."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/tsubclassof-in-unreal-engine"));

	AddItem(TEXT("TObjectPtr"), TEXT("TObjectPtr<"), EIntelliSenseCategory::Type,
		TEXT("template<class T> class TObjectPtr"),
		TEXT("Unreal Engine 5 smart pointer for UObject member properties. Supports lazy-loading, editor tracking, and 64-bit handle resolution."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-5-migration-guide"));

	AddItem(TEXT("TWeakObjectPtr"), TEXT("TWeakObjectPtr<"), EIntelliSenseCategory::Type,
		TEXT("template<class T> class TWeakObjectPtr"),
		TEXT("Weak reference to a UObject that automatically clears if the underlying object is garbage collected."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/objects-in-unreal-engine"));

	AddItem(TEXT("BeginPlay"), TEXT("BeginPlay()"), EIntelliSenseCategory::Method,
		TEXT("virtual void BeginPlay() override;"),
		TEXT("Called when the game starts or when this Actor is spawned into the world. Ideal for initialization logic requiring a valid world."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/actors-in-unreal-engine"));

	AddItem(TEXT("Tick"), TEXT("Tick(DeltaTime)"), EIntelliSenseCategory::Method,
		TEXT("virtual void Tick(float DeltaTime) override;"),
		TEXT("Called every frame to update this Actor. DeltaTime provides the elapsed time in seconds since the previous frame."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/actors-in-unreal-engine"));

	AddItem(TEXT("EndPlay"), TEXT("EndPlay("), EIntelliSenseCategory::Method,
		TEXT("virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;"),
		TEXT("Called when this Actor is being removed from the level, destroyed, or when the game ends."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/actors-in-unreal-engine"));

	AddItem(TEXT("GetWorld"), TEXT("GetWorld()"), EIntelliSenseCategory::Method,
		TEXT("UWorld* GetWorld() const;"),
		TEXT("Returns pointer to the UWorld simulation context that this Actor or Object resides in."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/actors-in-unreal-engine"));

	AddItem(TEXT("CreateDefaultSubobject"), TEXT("CreateDefaultSubobject<"), EIntelliSenseCategory::Method,
		TEXT("template<class TReturnType> TReturnType* CreateDefaultSubobject(FName SubobjectName);"),
		TEXT("Instantiates and registers a default subobject component inside an Actor constructor."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/actors-in-unreal-engine"));

	AddItem(TEXT("SetupPlayerInputComponent"), TEXT("SetupPlayerInputComponent("), EIntelliSenseCategory::Method,
		TEXT("virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;"),
		TEXT("Allows a Pawn or Character to bind player input actions and axes to member functions."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/pawns-in-unreal-engine"));

	AddItem(TEXT("SetActorLocation"), TEXT("SetActorLocation("), EIntelliSenseCategory::Method,
		TEXT("bool SetActorLocation(const FVector& NewLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr);"),
		TEXT("Moves the Actor root component to the specified world location, with optional collision sweep."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/actors-in-unreal-engine"));

	AddItem(TEXT("GetActorLocation"), TEXT("GetActorLocation()"), EIntelliSenseCategory::Method,
		TEXT("FVector GetActorLocation() const;"),
		TEXT("Returns current world coordinates location of the Actor root component."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/actors-in-unreal-engine"));

	AddItem(TEXT("SetActorRotation"), TEXT("SetActorRotation("), EIntelliSenseCategory::Method,
		TEXT("bool SetActorRotation(FRotator NewRotation);"),
		TEXT("Sets world space orientation of the Actor root component."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/actors-in-unreal-engine"));

	AddItem(TEXT("GetActorRotation"), TEXT("GetActorRotation()"), EIntelliSenseCategory::Method,
		TEXT("FRotator GetActorRotation() const;"),
		TEXT("Returns current world space orientation (Pitch, Yaw, Roll) of the Actor."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/actors-in-unreal-engine"));

	AddItem(TEXT("AttachToComponent"), TEXT("AttachToComponent("), EIntelliSenseCategory::Method,
		TEXT("bool AttachToComponent(USceneComponent* Parent, const FAttachmentTransformRules& AttachmentRules, FName SocketName = NAME_None);"),
		TEXT("Attaches this scene component to a parent component or skeletal socket."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/components-in-unreal-engine"));

	AddItem(TEXT("Destroy"), TEXT("Destroy()"), EIntelliSenseCategory::Method,
		TEXT("virtual bool Destroy(bool bNetForce = false, bool bShouldModifyLevel = true);"),
		TEXT("Destroys this Actor and schedules it for garbage collection removal from the level."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/actors-in-unreal-engine"));

	AddItem(TEXT("DeltaTime"), TEXT("DeltaTime"), EIntelliSenseCategory::Variable,
		TEXT("float DeltaTime"),
		TEXT("Frame time elapsed in seconds passed into Tick functions for frame-rate-independent physics and movement."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/actors-in-unreal-engine"));

	AddItem(TEXT("Super"), TEXT("Super::"), EIntelliSenseCategory::Keyword,
		TEXT("Super::MethodName(...)"),
		TEXT("Convenience typedef injected by GENERATED_BODY() to invoke the immediate parent class implementation."),
		TEXT("https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-uclasses"));
}

void SCppEditorPane::UpdateIntelliSense()
{
	if (!FCppEditorSettings::Get().bEnableIntelliSense)
	{
		DismissIntelliSense();
		return;
	}

	EnsureIntelliSenseDatabaseLoaded();

	if (!CodeTextBox.IsValid() || ActiveDocumentIndex == INDEX_NONE || ActiveDocumentIndex >= OpenDocuments.Num())
	{
		DismissIntelliSense();
		return;
	}

	FString CurrentLine;
	CodeTextBox->GetCurrentTextLine(CurrentLine);

	FTextLocation CursorLoc = CodeTextBox->GetCursorLocation();
	int32 Col = CursorLoc.GetOffset();
	if (Col <= 0 || Col > CurrentLine.Len())
	{
		DismissIntelliSense();
		return;
	}

	// 1. Identify alphanumeric identifier immediately before cursor (if any)
	int32 StartCol = Col - 1;
	while (StartCol >= 0)
	{
		TCHAR Ch = CurrentLine[StartCol];
		if (FChar::IsAlnum(Ch) || Ch == TEXT('_'))
		{
			StartCol--;
		}
		else
		{
			break;
		}
	}
	StartCol++; // First character of current identifier (if any)

	FString CleanPrefix;
	if (StartCol < Col)
	{
		CleanPrefix = CurrentLine.Mid(StartCol, Col - StartCol);
	}

	// 2. Check operator immediately before the identifier (or cursor)
	int32 OpCheckIndex = (CleanPrefix.IsEmpty()) ? (Col - 1) : (StartCol - 1);
	while (OpCheckIndex >= 0 && (CurrentLine[OpCheckIndex] == TEXT(' ') || CurrentLine[OpCheckIndex] == TEXT('\t')))
	{
		OpCheckIndex--;
	}

	enum class EContextOp { None, Dot, Arrow };
	EContextOp DetectedOp = EContextOp::None;
	int32 PrecedingSubjectEnd = INDEX_NONE;

	if (OpCheckIndex >= 1 && CurrentLine[OpCheckIndex - 1] == TEXT('-') && CurrentLine[OpCheckIndex] == TEXT('>'))
	{
		DetectedOp = EContextOp::Arrow;
		PrecedingSubjectEnd = OpCheckIndex - 2;
	}
	else if (OpCheckIndex >= 0 && CurrentLine[OpCheckIndex] == TEXT('.'))
	{
		// Make sure it's not a floating point literal (e.g. 0.5f)
		bool bIsFloatLiteral = false;
		if (OpCheckIndex > 0 && FChar::IsDigit(CurrentLine[OpCheckIndex - 1]))
		{
			bIsFloatLiteral = true;
		}

		if (!bIsFloatLiteral)
		{
			DetectedOp = EContextOp::Dot;
			PrecedingSubjectEnd = OpCheckIndex - 1;
		}
	}

	bool bShouldTrigger = false;
	FString TargetScope = TEXT("general");

	if (DetectedOp == EContextOp::Arrow)
	{
		// Always trigger for '->', even with empty prefix!
		bShouldTrigger = true;
		TargetScope = TEXT("pointer");
	}
	else if (DetectedOp == EContextOp::Dot)
	{
		// Always trigger for '.', even with empty prefix!
		bShouldTrigger = true;

		// Extract preceding subject token to detect scope
		while (PrecedingSubjectEnd >= 0 && (CurrentLine[PrecedingSubjectEnd] == TEXT(' ') || CurrentLine[PrecedingSubjectEnd] == TEXT('\t')))
		{
			PrecedingSubjectEnd--;
		}

		FString SubjectToken;
		if (PrecedingSubjectEnd >= 0)
		{
			int32 SubjStart = PrecedingSubjectEnd;
			if (CurrentLine[SubjStart] == TEXT(')'))
			{
				int32 ParenDepth = 1;
				SubjStart--;
				while (SubjStart >= 0 && ParenDepth > 0)
				{
					if (CurrentLine[SubjStart] == TEXT(')')) ParenDepth++;
					else if (CurrentLine[SubjStart] == TEXT('(')) ParenDepth--;
					SubjStart--;
				}
				while (SubjStart >= 0 && (FChar::IsAlnum(CurrentLine[SubjStart]) || CurrentLine[SubjStart] == TEXT('_') || CurrentLine[SubjStart] == TEXT(':')))
				{
					SubjStart--;
				}
				SubjStart++;
				SubjectToken = CurrentLine.Mid(SubjStart, PrecedingSubjectEnd - SubjStart + 1);
			}
			else
			{
				while (SubjStart >= 0 && (FChar::IsAlnum(CurrentLine[SubjStart]) || CurrentLine[SubjStart] == TEXT('_')))
				{
					SubjStart--;
				}
				SubjStart++;
				SubjectToken = CurrentLine.Mid(SubjStart, PrecedingSubjectEnd - SubjStart + 1);
			}
		}

		// Detect scope from SubjectToken
		if (SubjectToken.Contains(TEXT("Slot"), ESearchCase::IgnoreCase))
		{
			TargetScope = TEXT("slot");
		}
		else if (SubjectToken.Contains(TEXT("Color"), ESearchCase::IgnoreCase) || SubjectToken.Contains(TEXT("Tint"), ESearchCase::IgnoreCase))
		{
			TargetScope = TEXT("color");
		}
		else if (SubjectToken.Contains(TEXT("Margin"), ESearchCase::IgnoreCase) || SubjectToken.Contains(TEXT("Padding"), ESearchCase::IgnoreCase))
		{
			TargetScope = TEXT("margin");
		}
		else if (SubjectToken.Contains(TEXT("String"), ESearchCase::IgnoreCase) || SubjectToken.Contains(TEXT("Content"), ESearchCase::IgnoreCase) ||
		         SubjectToken.Contains(TEXT("Path"), ESearchCase::IgnoreCase) || SubjectToken.Contains(TEXT("Name"), ESearchCase::IgnoreCase) ||
		         SubjectToken.Contains(TEXT("Text"), ESearchCase::IgnoreCase) || SubjectToken.Contains(TEXT("Query"), ESearchCase::IgnoreCase))
		{
			TargetScope = TEXT("string");
		}
		else if (SubjectToken.Contains(TEXT("Array"), ESearchCase::IgnoreCase) || SubjectToken.Contains(TEXT("History"), ESearchCase::IgnoreCase) ||
		         SubjectToken.Contains(TEXT("Matches"), ESearchCase::IgnoreCase) || SubjectToken.Contains(TEXT("Documents"), ESearchCase::IgnoreCase) ||
		         SubjectToken.Contains(TEXT("List"), ESearchCase::IgnoreCase) || SubjectToken.Contains(TEXT("Items"), ESearchCase::IgnoreCase))
		{
			TargetScope = TEXT("array");
		}
		else
		{
			TargetScope = TEXT("dot_all");
		}
	}
	else
	{
		// Normal autocomplete: prefix >= 2 or uppercase 1-letter
		bShouldTrigger = (CleanPrefix.Len() >= 2) || (CleanPrefix.Len() >= 1 && FChar::IsUpper(CleanPrefix[0]));
		TargetScope = TEXT("general");
	}

	if (!bShouldTrigger)
	{
		DismissIntelliSense();
		return;
	}

	FilteredIntelliSenseItems.Reset();

	// 1. Gather all candidates: Static Database + Dynamic Document Symbols
	TArray<TSharedPtr<FIntelliSenseItem>> AllCandidates;
	AllCandidates.Reserve(MasterIntelliSenseDatabase.Num() + 64);
	AllCandidates.Append(MasterIntelliSenseDatabase);

	TArray<TSharedPtr<FIntelliSenseItem>> DocSymbols;
	HarvestDocumentSymbols(DocSymbols);
	AllCandidates.Append(DocSymbols);

	TArray<TSharedPtr<FIntelliSenseItem>> ExactPrefixMatches;
	TArray<TSharedPtr<FIntelliSenseItem>> CaseInsensitivePrefixMatches;
	TArray<TSharedPtr<FIntelliSenseItem>> SubstringMatches;
	TSet<FString> AddedNames;

	for (const TSharedPtr<FIntelliSenseItem>& Item : AllCandidates)
	{
		if (AddedNames.Contains(Item->DisplayText))
		{
			continue;
		}

		// Scope filtering
		if (TargetScope == TEXT("pointer"))
		{
			if (Item->Scope != TEXT("pointer") && Item->Category != EIntelliSenseCategory::Method)
			{
				continue;
			}
		}
		else if (TargetScope == TEXT("slot"))
		{
			if (Item->Scope != TEXT("slot") && Item->Category != EIntelliSenseCategory::Method)
			{
				continue;
			}
		}
		else if (TargetScope == TEXT("string"))
		{
			if (Item->Scope != TEXT("string"))
			{
				continue;
			}
		}
		else if (TargetScope == TEXT("array"))
		{
			if (Item->Scope != TEXT("array"))
			{
				continue;
			}
		}
		else if (TargetScope == TEXT("color"))
		{
			if (Item->Scope != TEXT("color"))
			{
				continue;
			}
		}
		else if (TargetScope == TEXT("margin"))
		{
			if (Item->Scope != TEXT("margin"))
			{
				continue;
			}
		}
		else if (TargetScope == TEXT("dot_all"))
		{
			if (Item->Scope != TEXT("slot") && Item->Scope != TEXT("string") && Item->Scope != TEXT("array") &&
			    Item->Scope != TEXT("color") && Item->Scope != TEXT("margin") &&
			    Item->Category != EIntelliSenseCategory::Method && Item->Category != EIntelliSenseCategory::Field)
			{
				continue;
			}
		}
		else // TargetScope == TEXT("general")
		{
			// In general scope, skip members that are exclusively accessed via dot or arrow
			if ((Item->Scope == TEXT("slot") || Item->Scope == TEXT("pointer") || Item->Category == EIntelliSenseCategory::Field) && CleanPrefix.Len() < 3)
			{
				continue;
			}
		}

		// If CleanPrefix is empty (just typed '.' or '->'), all scoped candidates match
		if (CleanPrefix.IsEmpty())
		{
			ExactPrefixMatches.Add(Item);
			AddedNames.Add(Item->DisplayText);
			continue;
		}

		if (Item->DisplayText.StartsWith(CleanPrefix, ESearchCase::CaseSensitive))
		{
			ExactPrefixMatches.Add(Item);
			AddedNames.Add(Item->DisplayText);
		}
		else if (Item->DisplayText.StartsWith(CleanPrefix, ESearchCase::IgnoreCase))
		{
			CaseInsensitivePrefixMatches.Add(Item);
			AddedNames.Add(Item->DisplayText);
		}
		else if (Item->DisplayText.Contains(CleanPrefix, ESearchCase::IgnoreCase))
		{
			SubstringMatches.Add(Item);
			AddedNames.Add(Item->DisplayText);
		}
	}

	FilteredIntelliSenseItems.Append(ExactPrefixMatches);
	FilteredIntelliSenseItems.Append(CaseInsensitivePrefixMatches);
	FilteredIntelliSenseItems.Append(SubstringMatches);

	if (FilteredIntelliSenseItems.Num() > 20)
	{
		FilteredIntelliSenseItems.SetNum(20);
	}

	if (FilteredIntelliSenseItems.Num() > 0)
	{
		bIntelliSenseActive = true;
		if (IntelliSenseBox.IsValid())
		{
			IntelliSenseBox->SetVisibility(EVisibility::Visible);
		}
		if (IntelliSenseListView.IsValid())
		{
			IntelliSenseListView->RequestListRefresh();
			IntelliSenseListView->SetSelection(FilteredIntelliSenseItems[0]);
		}
		UpdateDocPanel(FilteredIntelliSenseItems[0]);
	}
	else
	{
		DismissIntelliSense();
	}
}

void SCppEditorPane::DismissIntelliSense()
{
	bIntelliSenseActive = false;
	if (IntelliSenseBox.IsValid())
	{
		IntelliSenseBox->SetVisibility(EVisibility::Collapsed);
	}
	if (IntelliSenseDocBox.IsValid())
	{
		IntelliSenseDocBox->SetVisibility(EVisibility::Collapsed);
	}
}

void SCppEditorPane::CommitIntelliSense()
{
	if (!bIntelliSenseActive || FilteredIntelliSenseItems.Num() == 0 || !IntelliSenseListView.IsValid())
	{
		DismissIntelliSense();
		return;
	}

	TSharedPtr<FIntelliSenseItem> SelectedItem;
	TArray<TSharedPtr<FIntelliSenseItem>> SelectedList = IntelliSenseListView->GetSelectedItems();
	if (SelectedList.Num() > 0)
	{
		SelectedItem = SelectedList[0];
	}
	else
	{
		SelectedItem = FilteredIntelliSenseItems[0];
	}

	if (!SelectedItem.IsValid() || !CodeTextBox.IsValid() || ActiveDocumentIndex == INDEX_NONE || ActiveDocumentIndex >= OpenDocuments.Num())
	{
		DismissIntelliSense();
		return;
	}

	TSharedPtr<FEditorDocument> Doc = OpenDocuments[ActiveDocumentIndex];
	FTextLocation CursorLoc = CodeTextBox->GetCursorLocation();

	FString CurrentLine;
	CodeTextBox->GetCurrentTextLine(CurrentLine);

	int32 Col = CursorLoc.GetOffset();
	int32 LineIndex = CursorLoc.GetLineIndex();

	int32 StartCol = Col - 1;
	while (StartCol >= 0)
	{
		TCHAR Ch = CurrentLine[StartCol];
		if (FChar::IsAlnum(Ch) || Ch == TEXT('_'))
		{
			StartCol--;
		}
		else
		{
			break;
		}
	}
	StartCol++;

	int32 PrefixLen = FMath::Max(0, Col - StartCol);

	TArray<int32> LineStartOffsets;
	LineStartOffsets.Add(0);
	for (int32 i = 0; i < Doc->CurrentContent.Len(); ++i)
	{
		if (Doc->CurrentContent[i] == TEXT('\n'))
		{
			LineStartOffsets.Add(i + 1);
		}
	}

	if (LineIndex >= LineStartOffsets.Num())
	{
		DismissIntelliSense();
		return;
	}

	int32 CursorCharIndex = LineStartOffsets[LineIndex] + Col;
	int32 ReplaceStartIndex = CursorCharIndex - PrefixLen;

	if (ReplaceStartIndex < 0 || ReplaceStartIndex > Doc->CurrentContent.Len())
	{
		DismissIntelliSense();
		return;
	}

	Doc->UndoHistory.Add(Doc->CurrentContent);
	Doc->RedoHistory.Empty();

	const FString& Insert = SelectedItem->InsertText;
	FString NewContent = Doc->CurrentContent.Left(ReplaceStartIndex) + Insert + Doc->CurrentContent.Mid(CursorCharIndex);

	Doc->CurrentContent = NewContent;
	Doc->bIsDirty = (Doc->CurrentContent != Doc->SavedContent);
	Doc->bIsInternalTextChange = true;
	CodeTextBox->SetText(FText::FromString(NewContent));
	Doc->bIsInternalTextChange = false;

	int32 NewCol = StartCol + Insert.Len();
	FTextLocation NewLoc(LineIndex, NewCol);
	CodeTextBox->GoTo(NewLoc);
	CodeTextBox->ScrollTo(NewLoc);

	OnDocumentContentChanged.ExecuteIfBound(Doc->FilePath, Doc->CurrentContent);

	DismissIntelliSense();
}

void SCppEditorPane::IntelliSenseNavigate(int32 Direction)
{
	if (FilteredIntelliSenseItems.Num() == 0 || !IntelliSenseListView.IsValid())
	{
		return;
	}

	TArray<TSharedPtr<FIntelliSenseItem>> SelectedList = IntelliSenseListView->GetSelectedItems();
	int32 CurrentIndex = 0;
	if (SelectedList.Num() > 0)
	{
		CurrentIndex = FilteredIntelliSenseItems.IndexOfByKey(SelectedList[0]);
		if (CurrentIndex == INDEX_NONE)
		{
			CurrentIndex = 0;
		}
	}

	int32 NewIndex = FMath::Clamp(CurrentIndex + Direction, 0, FilteredIntelliSenseItems.Num() - 1);
	IntelliSenseListView->SetSelection(FilteredIntelliSenseItems[NewIndex]);
	IntelliSenseListView->RequestScrollIntoView(FilteredIntelliSenseItems[NewIndex]);
	UpdateDocPanel(FilteredIntelliSenseItems[NewIndex]);
}

void SCppEditorPane::OnIntelliSenseSelectionChanged(TSharedPtr<FIntelliSenseItem> SelectedItem, ESelectInfo::Type SelectInfo)
{
	UpdateDocPanel(SelectedItem);
}

void SCppEditorPane::UpdateDocPanel(TSharedPtr<FIntelliSenseItem> Item)
{
	if (!IntelliSenseDocBox.IsValid())
	{
		return;
	}

	if (!Item.IsValid())
	{
		IntelliSenseDocBox->SetVisibility(EVisibility::Collapsed);
		return;
	}

	// Always show documentation panel with structured information
	IntelliSenseDocBox->SetVisibility(EVisibility::Visible);

	// Category Tag
	if (DocCategoryTextBlock.IsValid())
	{
		FString CategoryStr;
		switch (Item->Category)
		{
		case EIntelliSenseCategory::Keyword:  CategoryStr = TEXT("C++ KEYWORD"); break;
		case EIntelliSenseCategory::Type:     CategoryStr = TEXT("UNREAL / SLATE TYPE"); break;
		case EIntelliSenseCategory::Method:   CategoryStr = (Item->Scope == TEXT("slot")) ? TEXT("SLATE SLOT METHOD") : TEXT("METHOD / FUNCTION"); break;
		case EIntelliSenseCategory::Field:    CategoryStr = TEXT("STRUCT / CLASS FIELD"); break;
		case EIntelliSenseCategory::Macro:    CategoryStr = TEXT("UNREAL / SLATE MACRO"); break;
		case EIntelliSenseCategory::Delegate: CategoryStr = TEXT("UNREAL DELEGATE / EVENT"); break;
		case EIntelliSenseCategory::Enum:     CategoryStr = TEXT("ENUM / CONSTANT"); break;
		case EIntelliSenseCategory::Variable: CategoryStr = TEXT("VARIABLE"); break;
		default:                              CategoryStr = TEXT("SYMBOL"); break;
		}
		DocCategoryTextBlock->SetText(FText::FromString(CategoryStr));
	}

	// Signature Block
	if (DocSignatureTextBlock.IsValid())
	{
		FString DisplaySig = Item->Signature.IsEmpty() ? Item->DisplayText : Item->Signature;
		DocSignatureTextBlock->SetText(FText::FromString(DisplaySig));
	}

	// Description Block
	if (DocDescriptionTextBlock.IsValid())
	{
		FString Desc = Item->Description.IsEmpty() ? FString::Printf(TEXT("Symbol declared or defined in code: %s"), *Item->DisplayText) : Item->Description;
		DocDescriptionTextBlock->SetText(FText::FromString(Desc));
	}

	// Documentation Link
	ActiveDocUrl = Item->DocUrl;
	if (DocUrlButton.IsValid())
	{
		DocUrlButton->SetVisibility(ActiveDocUrl.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible);
	}
}

TSharedRef<ITableRow> SCppEditorPane::OnGenerateIntelliSenseRow(TSharedPtr<FIntelliSenseItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	FLinearColor BadgeColor = FLinearColor(0.337f, 0.612f, 0.839f, 1.0f);
	FString BadgeText = TEXT("K");
	FString CategoryHint = TEXT("keyword");

	switch (Item->Category)
	{
	case EIntelliSenseCategory::Keyword:
		BadgeColor = FLinearColor(0.337f, 0.612f, 0.839f, 1.0f);
		BadgeText = TEXT("K");
		CategoryHint = TEXT("keyword");
		break;
	case EIntelliSenseCategory::Type:
		BadgeColor = FLinearColor(0.306f, 0.788f, 0.690f, 1.0f);
		BadgeText = TEXT("T");
		CategoryHint = TEXT("type");
		break;
	case EIntelliSenseCategory::Method:
		BadgeColor = FLinearColor(0.863f, 0.863f, 0.667f, 1.0f);
		BadgeText = TEXT("M");
		CategoryHint = TEXT("method");
		break;
	case EIntelliSenseCategory::Field:
		BadgeColor = FLinearColor(0.95f, 0.65f, 0.35f, 1.0f);
		BadgeText = TEXT("F");
		CategoryHint = TEXT("field");
		break;
	case EIntelliSenseCategory::Macro:
		BadgeColor = FLinearColor(0.773f, 0.525f, 0.753f, 1.0f);
		BadgeText = TEXT("#");
		CategoryHint = TEXT("macro");
		break;
	case EIntelliSenseCategory::Delegate:
		BadgeColor = FLinearColor(0.94f, 0.75f, 0.25f, 1.0f);
		BadgeText = TEXT("D");
		CategoryHint = TEXT("delegate");
		break;
	case EIntelliSenseCategory::Enum:
		BadgeColor = FLinearColor(0.45f, 0.85f, 0.55f, 1.0f);
		BadgeText = TEXT("E");
		CategoryHint = TEXT("enum");
		break;
	case EIntelliSenseCategory::Variable:
		BadgeColor = FLinearColor(0.612f, 0.804f, 0.996f, 1.0f);
		BadgeText = TEXT("V");
		CategoryHint = TEXT("variable");
		break;
	}

	return SNew(STableRow<TSharedPtr<FIntelliSenseItem>>, OwnerTable)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
				.BorderBackgroundColor(BadgeColor * 0.35f)
				.Padding(FMargin(4.0f, 1.0f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(BadgeText))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(BadgeColor)
				]
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item->DisplayText))
				.Font(FCppSyntaxHighlighterMarshaller::GetEditorFont(10.0f))
				.ColorAndOpacity(FLinearColor(0.9f, 0.9f, 0.9f, 1.0f))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(6.0f, 0.0f, 2.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(CategoryHint))
				.Font(FCoreStyle::GetDefaultFontStyle("Italic", 8))
				.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f, 0.8f))
			]
		];
}

void SCppEditorPane::OnIntelliSenseItemDoubleClicked(TSharedPtr<FIntelliSenseItem> Item)
{
	if (IntelliSenseListView.IsValid() && Item.IsValid())
	{
		IntelliSenseListView->SetSelection(Item);
		CommitIntelliSense();
		FocusEditor();
	}
}

FMargin SCppEditorPane::GetIntelliSenseMargin() const
{
	if (!CodeTextBox.IsValid())
	{
		return FMargin(40.0f, 40.0f, 0.0f, 0.0f);
	}
	FTextLocation Loc = CodeTextBox->GetCursorLocation();
	float X = FMath::Clamp(Loc.GetOffset() * 8.5f + 30.0f, 20.0f, 600.0f);
	float Y = FMath::Clamp((Loc.GetLineIndex() % 25 + 1) * 20.0f + 10.0f, 25.0f, 320.0f);
	return FMargin(X, Y, 0.0f, 0.0f);
}

FMargin SCppEditorPane::GetHoverDocMargin() const
{
	if (HoverDocCard.IsValid())
	{
		TSharedPtr<SWidget> ParentWidget = HoverDocCard->GetParentWidget();
		if (ParentWidget.IsValid())
		{
			FGeometry ParentGeo = ParentWidget->GetTickSpaceGeometry();
			FVector2D LocalPos = ParentGeo.AbsoluteToLocal(HoverDocScreenPosition);

			float X = (float)LocalPos.X + 8.0f;
			float Y = (float)LocalPos.Y + 22.0f;

			FVector2D LocalSize = (FVector2D)ParentGeo.GetLocalSize();
			const float CardWidth = 400.0f;
			const float CardHeight = 130.0f;

			if (X + CardWidth > LocalSize.X - 10.0f)
			{
				X = FMath::Max(10.0f, (float)(LocalSize.X - CardWidth - 10.0f));
			}
			if (Y + CardHeight > LocalSize.Y - 10.0f)
			{
				Y = FMath::Max(10.0f, (float)(LocalPos.Y - CardHeight - 12.0f));
			}

			return FMargin(FMath::Max(0.0f, X), FMath::Max(0.0f, Y), 0.0f, 0.0f);
		}
	}
	return FMargin(40.0f, 40.0f, 0.0f, 0.0f);
}

FString SCppEditorPane::GetWordAtScreenPosition(const FVector2D& ScreenPos)
{
	if (!CodeTextBox.IsValid() || ActiveDocumentIndex == INDEX_NONE || ActiveDocumentIndex >= OpenDocuments.Num())
	{
		return FString();
	}

	FGeometry TextGeo = CodeTextBox->GetTickSpaceGeometry();
	FVector2D LocalPos = TextGeo.AbsoluteToLocal(ScreenPos);
	FVector2D TextSize = (FVector2D)TextGeo.GetLocalSize();

	// Check if cursor is inside CodeTextBox
	if (LocalPos.X < 0.0f || LocalPos.X > TextSize.X || LocalPos.Y < 0.0f || LocalPos.Y > TextSize.Y)
	{
		return FString();
	}

	int32 TotalLines = GetTotalLineCount();
	if (TotalLines <= 0)
	{
		return FString();
	}

	FSlateFontInfo FontInfo = FCppEditorSettings::Get().GetFont();
	TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const float MeasuredLineHeight = (float)FontMeasure->GetMaxCharacterHeight(FontInfo);
	const float LineHeight = FMath::Max(18.0f, MeasuredLineHeight + 4.0f);

	int32 FirstLine = 0;
	if (CodeTextBox->GetVScrollBar().IsValid())
	{
		float ScrollFraction = CodeTextBox->GetVScrollBar()->DistanceFromTop();
		FirstLine = FMath::Clamp(FMath::FloorToInt(ScrollFraction * TotalLines), 0, FMath::Max(0, TotalLines - 1));
	}

	int32 LineIndex = FirstLine + FMath::FloorToInt(((float)LocalPos.Y - 4.0f) / LineHeight);
	if (LineIndex < 0 || LineIndex >= TotalLines)
	{
		return FString();
	}

	TSharedPtr<FEditorDocument> Doc = OpenDocuments[ActiveDocumentIndex];
	TArray<FString> Lines;
	Doc->CurrentContent.ParseIntoArrayLines(Lines, false);
	if (!Lines.IsValidIndex(LineIndex))
	{
		return FString();
	}

	const FString& Line = Lines[LineIndex];
	if (Line.IsEmpty())
	{
		return FString();
	}

	// Estimate character position by measuring substrings
	float TargetX = (float)LocalPos.X - 4.0f;
	if (TargetX < 0.0f)
	{
		return FString();
	}

	int32 FoundCharIdx = INDEX_NONE;
	float LastMeasuredX = 0.0f;
	for (int32 i = 1; i <= Line.Len(); ++i)
	{
		FString Sub = Line.Left(i);
		Sub.ReplaceInline(TEXT("\t"), TEXT("    "));
		float MeasuredX = (float)FontMeasure->Measure(Sub, FontInfo).X;
		if (TargetX >= LastMeasuredX && TargetX <= MeasuredX)
		{
			FoundCharIdx = i - 1;
			break;
		}
		LastMeasuredX = MeasuredX;
	}

	if (FoundCharIdx == INDEX_NONE || !Line.IsValidIndex(FoundCharIdx))
	{
		return FString();
	}

	// Check if this character is an identifier character
	TCHAR HitChar = Line[FoundCharIdx];
	if (!FChar::IsAlnum(HitChar) && HitChar != TEXT('_'))
	{
		return FString();
	}

	// Expand to entire identifier
	int32 Start = FoundCharIdx;
	while (Start > 0 && (FChar::IsAlnum(Line[Start - 1]) || Line[Start - 1] == TEXT('_')))
	{
		Start--;
	}

	int32 End = FoundCharIdx;
	while (End < Line.Len() && (FChar::IsAlnum(Line[End]) || Line[End] == TEXT('_')))
	{
		End++;
	}

	return Line.Mid(Start, End - Start);
}

void SCppEditorPane::ShowHoverDoc(const FString& Word, const FVector2D& ScreenPos)
{
	TSharedPtr<FIntelliSenseItem> Item = FindDocItemForWord(Word);
	if (!Item.IsValid())
	{
		DismissHoverDoc();
		return;
	}

	HoverDocScreenPosition = ScreenPos;
	LastHoveredWord = Word;
	bHoverDocVisible = true;

	if (HoverDocCategoryText.IsValid())
	{
		FString CategoryStr;
		switch (Item->Category)
		{
		case EIntelliSenseCategory::Keyword:  CategoryStr = TEXT("C++ KEYWORD"); break;
		case EIntelliSenseCategory::Type:     CategoryStr = TEXT("UNREAL / SLATE TYPE"); break;
		case EIntelliSenseCategory::Method:   CategoryStr = (Item->Scope == TEXT("slot")) ? TEXT("SLATE SLOT METHOD") : TEXT("METHOD / FUNCTION"); break;
		case EIntelliSenseCategory::Field:    CategoryStr = TEXT("STRUCT / CLASS FIELD"); break;
		case EIntelliSenseCategory::Macro:    CategoryStr = TEXT("UNREAL / SLATE MACRO"); break;
		case EIntelliSenseCategory::Delegate: CategoryStr = TEXT("UNREAL DELEGATE / EVENT"); break;
		case EIntelliSenseCategory::Enum:     CategoryStr = TEXT("ENUM / CONSTANT"); break;
		case EIntelliSenseCategory::Variable: CategoryStr = TEXT("VARIABLE"); break;
		default:                              CategoryStr = TEXT("SYMBOL"); break;
		}
		HoverDocCategoryText->SetText(FText::FromString(CategoryStr));
	}

	if (HoverDocSignatureText.IsValid())
	{
		FString DisplaySig = Item->Signature.IsEmpty() ? Item->DisplayText : Item->Signature;
		HoverDocSignatureText->SetText(FText::FromString(DisplaySig));
	}

	if (HoverDocDescriptionText.IsValid())
	{
		FString Desc = Item->Description.IsEmpty() ? FString::Printf(TEXT("Symbol declared in code: %s"), *Item->DisplayText) : Item->Description;
		HoverDocDescriptionText->SetText(FText::FromString(Desc));
	}

	ActiveHoverDocUrl = Item->DocUrl;
	if (HoverDocUrlButton.IsValid())
	{
		HoverDocUrlButton->SetVisibility(ActiveHoverDocUrl.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible);
	}

	if (HoverDocCard.IsValid())
	{
		HoverDocCard->SetVisibility(EVisibility::Visible);
	}
}

void SCppEditorPane::DismissHoverDoc()
{
	bHoverDocVisible = false;
	LastHoveredWord.Empty();
	if (HoverDocCard.IsValid())
	{
		HoverDocCard->SetVisibility(EVisibility::Collapsed);
	}
}

TSharedPtr<FIntelliSenseItem> SCppEditorPane::FindDocItemForWord(const FString& Word) const
{
	if (Word.IsEmpty())
	{
		return nullptr;
	}

	EnsureIntelliSenseDatabaseLoaded();

	// 1. Exact match in master database
	for (const auto& Item : MasterIntelliSenseDatabase)
	{
		if (Item->DisplayText.Equals(Word, ESearchCase::CaseSensitive))
		{
			return Item;
		}
	}

	// 2. Case-insensitive match in master database
	for (const auto& Item : MasterIntelliSenseDatabase)
	{
		if (Item->DisplayText.Equals(Word, ESearchCase::IgnoreCase))
		{
			return Item;
		}
	}

	// 3. Search document symbols
	TArray<TSharedPtr<FIntelliSenseItem>> DocSymbols;
	HarvestDocumentSymbols(DocSymbols);
	for (const auto& Sym : DocSymbols)
	{
		if (Sym->DisplayText.Equals(Word, ESearchCase::CaseSensitive))
		{
			return Sym;
		}
	}
	for (const auto& Sym : DocSymbols)
	{
		if (Sym->DisplayText.Equals(Word, ESearchCase::IgnoreCase))
		{
			return Sym;
		}
	}

	// 4. Dynamic inspection of active document lines to find definition/declaration of Word
	if (ActiveDocumentIndex >= 0 && ActiveDocumentIndex < OpenDocuments.Num())
	{
		const FString& Content = OpenDocuments[ActiveDocumentIndex]->CurrentContent;
		FString CurrentFileName = OpenDocuments[ActiveDocumentIndex]->Filename;
		TArray<FString> Lines;
		Content.ParseIntoArrayLines(Lines, false);

		for (const FString& RawLine : Lines)
		{
			FString Line = RawLine.TrimStartAndEnd();
			if (Line.IsEmpty() || Line.StartsWith(TEXT("//")) || Line.StartsWith(TEXT("/*")))
			{
				continue;
			}

			// Check if line contains Word as a distinct identifier
			int32 FoundIdx = Line.Find(Word);
			if (FoundIdx != INDEX_NONE)
			{
				// Verify word boundaries
				bool bStartOk = (FoundIdx == 0) || (!FChar::IsAlnum(Line[FoundIdx - 1]) && Line[FoundIdx - 1] != TEXT('_'));
				int32 EndIdx = FoundIdx + Word.Len();
				bool bEndOk = (EndIdx >= Line.Len()) || (!FChar::IsAlnum(Line[EndIdx]) && Line[EndIdx] != TEXT('_'));

				if (bStartOk && bEndOk)
				{
					// Class or struct definition
					if (Line.Contains(TEXT("class ")) || Line.Contains(TEXT("struct ")))
					{
						TSharedPtr<FIntelliSenseItem> Item = MakeShared<FIntelliSenseItem>();
						Item->DisplayText = Word;
						Item->Category = EIntelliSenseCategory::Type;
						Item->Signature = Line;
						Item->Description = FString::Printf(TEXT("User type '%s' declared in %s"), *Word, *CurrentFileName);
						return Item;
					}

					// Function declaration (has '(' and ')' or starts with virtual)
					if (Line.Contains(TEXT("(")) && Line.Contains(TEXT(")")))
					{
						TSharedPtr<FIntelliSenseItem> Item = MakeShared<FIntelliSenseItem>();
						Item->DisplayText = Word;
						Item->Category = EIntelliSenseCategory::Method;
						Item->Signature = Line;
						Item->Description = FString::Printf(TEXT("Member function '%s' declared in %s"), *Word, *CurrentFileName);
						return Item;
					}

					// Variable declaration (ends with ';' or has '=')
					if (Line.EndsWith(TEXT(";")) || Line.Contains(TEXT("=")))
					{
						TSharedPtr<FIntelliSenseItem> Item = MakeShared<FIntelliSenseItem>();
						Item->DisplayText = Word;
						Item->Category = EIntelliSenseCategory::Variable;
						Item->Signature = Line;
						Item->Description = FString::Printf(TEXT("Member or local variable '%s' declared in %s"), *Word, *CurrentFileName);
						return Item;
					}
				}
			}
		}
	}

	// 5. Intelligent Unreal Engine naming convention heuristics fallback
	if (Word.Len() >= 2 && FChar::IsUpper(Word[0]))
	{
		TCHAR Prefix = Word[0];
		if (FChar::IsUpper(Word[1]))
		{
			if (Prefix == TEXT('A'))
			{
				TSharedPtr<FIntelliSenseItem> Item = MakeShared<FIntelliSenseItem>();
				Item->DisplayText = Word;
				Item->Category = EIntelliSenseCategory::Type;
				Item->Signature = FString::Printf(TEXT("class %s : public AActor"), *Word);
				Item->Description = FString::Printf(TEXT("Unreal Engine Actor-derived class '%s'."), *Word);
				return Item;
			}
			else if (Prefix == TEXT('U'))
			{
				TSharedPtr<FIntelliSenseItem> Item = MakeShared<FIntelliSenseItem>();
				Item->DisplayText = Word;
				Item->Category = EIntelliSenseCategory::Type;
				Item->Signature = FString::Printf(TEXT("class %s : public UObject"), *Word);
				Item->Description = FString::Printf(TEXT("Unreal Engine UObject or Component class '%s'."), *Word);
				return Item;
			}
			else if (Prefix == TEXT('F'))
			{
				TSharedPtr<FIntelliSenseItem> Item = MakeShared<FIntelliSenseItem>();
				Item->DisplayText = Word;
				Item->Category = EIntelliSenseCategory::Type;
				Item->Signature = FString::Printf(TEXT("struct %s"), *Word);
				Item->Description = FString::Printf(TEXT("Unreal Engine value struct or utility type '%s'."), *Word);
				return Item;
			}
			else if (Prefix == TEXT('S'))
			{
				TSharedPtr<FIntelliSenseItem> Item = MakeShared<FIntelliSenseItem>();
				Item->DisplayText = Word;
				Item->Category = EIntelliSenseCategory::Type;
				Item->Signature = FString::Printf(TEXT("class %s : public SCompoundWidget"), *Word);
				Item->Description = FString::Printf(TEXT("Slate UI widget class '%s'."), *Word);
				return Item;
			}
			else if (Prefix == TEXT('E'))
			{
				TSharedPtr<FIntelliSenseItem> Item = MakeShared<FIntelliSenseItem>();
				Item->DisplayText = Word;
				Item->Category = EIntelliSenseCategory::Enum;
				Item->Signature = FString::Printf(TEXT("enum class %s : uint8"), *Word);
				Item->Description = FString::Printf(TEXT("Unreal Engine enumeration '%s'."), *Word);
				return Item;
			}
		}
	}

	return nullptr;
}

void SCppEditorPane::HarvestDocumentSymbols(TArray<TSharedPtr<FIntelliSenseItem>>& OutSymbols) const
{
	TSet<FString> CollectedNames;

	auto HarvestFromText = [&CollectedNames, &OutSymbols](const FString& Text)
	{
		const int32 Len = Text.Len();
		int32 Idx = 0;
		FString PreviousToken;

		while (Idx < Len)
		{
			TCHAR C = Text[Idx];

			// Skip single-line comments // ...
			if (C == TEXT('/') && Idx + 1 < Len && Text[Idx + 1] == TEXT('/'))
			{
				Idx += 2;
				while (Idx < Len && Text[Idx] != TEXT('\n'))
				{
					Idx++;
				}
				continue;
			}

			// Skip multi-line comments /* ... */
			if (C == TEXT('/') && Idx + 1 < Len && Text[Idx + 1] == TEXT('*'))
			{
				Idx += 2;
				while (Idx + 1 < Len && !(Text[Idx] == TEXT('*') && Text[Idx + 1] == TEXT('/')))
				{
					Idx++;
				}
				Idx += 2;
				continue;
			}

			// Skip string literals "..."
			if (C == TEXT('\"'))
			{
				Idx++;
				while (Idx < Len && Text[Idx] != TEXT('\"'))
				{
					if (Text[Idx] == TEXT('\\') && Idx + 1 < Len)
					{
						Idx += 2;
					}
					else
					{
						Idx++;
					}
				}
				Idx++;
				continue;
			}

			// Identifiers: [A-Za-z_][A-Za-z0-9_]*
			if (FChar::IsAlpha(C) || C == TEXT('_'))
			{
				int32 Start = Idx;
				while (Idx < Len && (FChar::IsAlnum(Text[Idx]) || Text[Idx] == TEXT('_')))
				{
					Idx++;
				}

				FString Token = Text.Mid(Start, Idx - Start);

				if (Token.Len() >= 3)
				{
					int32 Peek = Idx;
					while (Peek < Len && FChar::IsWhitespace(Text[Peek]))
					{
						Peek++;
					}
					bool bIsFunction = (Peek < Len && Text[Peek] == TEXT('('));

					if (!CollectedNames.Contains(Token))
					{
						CollectedNames.Add(Token);

						bool bAllUpper = true;
						for (TCHAR Ch : Token)
						{
							if (FChar::IsAlpha(Ch) && !FChar::IsUpper(Ch))
							{
								bAllUpper = false;
								break;
							}
						}

						EIntelliSenseCategory Cat = EIntelliSenseCategory::Variable;
						if (bIsFunction)
						{
							Cat = EIntelliSenseCategory::Method;
						}
						else if (PreviousToken == TEXT("class") || PreviousToken == TEXT("struct") || PreviousToken == TEXT("enum"))
						{
							Cat = EIntelliSenseCategory::Type;
						}
						else if (bAllUpper && Token.Len() >= 2)
						{
							Cat = EIntelliSenseCategory::Macro;
						}
						else if (Token.Len() >= 2 && (Token[0] == TEXT('F') || Token[0] == TEXT('U') || Token[0] == TEXT('A') || Token[0] == TEXT('S') || Token[0] == TEXT('T')) && FChar::IsUpper(Token[1]))
						{
							Cat = EIntelliSenseCategory::Type;
						}

						TSharedPtr<FIntelliSenseItem> Item = MakeShared<FIntelliSenseItem>();
						Item->DisplayText = Token;
						Item->InsertText = Token;
						Item->Category = Cat;
						Item->Scope = TEXT("general");
						Item->Signature = (Cat == EIntelliSenseCategory::Method) ? FString::Printf(TEXT("%s(...)"), *Token) : Token;
						Item->Description = FString::Printf(TEXT("Symbol harvested from open document: %s"), *Token);
						OutSymbols.Add(Item);
					}
				}

				PreviousToken = MoveTemp(Token);
				continue;
			}

			if (!FChar::IsWhitespace(C))
			{
				if (C != TEXT(':') && C != TEXT('*') && C != TEXT('&'))
				{
					PreviousToken.Reset();
				}
			}

			Idx++;
		}
	};

	// 1. Scan active document
	if (ActiveDocumentIndex >= 0 && ActiveDocumentIndex < OpenDocuments.Num())
	{
		HarvestFromText(OpenDocuments[ActiveDocumentIndex]->CurrentContent);
	}

	// 2. Scan other open documents
	for (int32 i = 0; i < OpenDocuments.Num(); ++i)
	{
		if (i != ActiveDocumentIndex)
		{
			HarvestFromText(OpenDocuments[i]->CurrentContent);
		}
	}
}

FReply SCppEditorPane::HandleCodeTextBoxKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent)
{
	DismissHoverDoc();
	const TCHAR Char = InCharacterEvent.GetCharacter();

	// Consume Tab character event so it NEVER double-inserts!
	if (Char == TEXT('\t'))
	{
		return FReply::Handled();
	}

	// Consume newline characters if Enter was already handled in KeyDown
	if (bEnterKeyHandledInKeyDown && (Char == TEXT('\r') || Char == TEXT('\n')))
	{
		bEnterKeyHandledInKeyDown = false;
		return FReply::Handled();
	}

	const FCppEditorSettings& Settings = FCppEditorSettings::Get();

	// Auto-close brackets & quotes if enabled
	if (Settings.bAutoCloseBrackets && CodeTextBox.IsValid())
	{
		FTextLocation CursorLoc = CodeTextBox->GetCursorLocation();
		FString CurrentLine;
		CodeTextBox->GetCurrentTextLine(CurrentLine);
		int32 Col = CursorLoc.GetOffset();

		// Skip-over closing punctuation if already present directly at cursor
		if ((Char == TEXT(')') || Char == TEXT(']') || Char == TEXT('}') || Char == TEXT('"') || Char == TEXT('\'')) &&
			Col < CurrentLine.Len() && CurrentLine[Col] == Char)
		{
			FTextLocation NewLoc(CursorLoc.GetLineIndex(), Col + 1);
			CodeTextBox->GoTo(NewLoc);
			CodeTextBox->ScrollTo(NewLoc);
			return FReply::Handled();
		}

		// Auto-insert matching pair
		FString Pair;
		if (Char == TEXT('(')) Pair = TEXT("()");
		else if (Char == TEXT('[')) Pair = TEXT("[]");
		else if (Char == TEXT('{')) Pair = TEXT("{}");
		else if (Char == TEXT('"'))
		{
			bool bPrecededByAlnum = (Col > 0 && FChar::IsAlnum(CurrentLine[Col - 1]));
			if (!bPrecededByAlnum)
			{
				Pair = TEXT("\"\"");
			}
		}
		else if (Char == TEXT('\''))
		{
			bool bPrecededByAlnum = (Col > 0 && FChar::IsAlnum(CurrentLine[Col - 1]));
			if (!bPrecededByAlnum)
			{
				Pair = TEXT("''");
			}
		}

		if (!Pair.IsEmpty())
		{
			CodeTextBox->InsertTextAtCursor(Pair);
			FTextLocation NewLoc(CursorLoc.GetLineIndex(), Col + 1);
			CodeTextBox->GoTo(NewLoc);
			CodeTextBox->ScrollTo(NewLoc);
			return FReply::Handled();
		}
	}

	if (HasGhostText())
	{
		DismissGhostText();
	}

	if (FCppEditorSettings::Get().bEnableAiInlineCompletion)
	{
		bPendingAiRequest = true;
		LastKeyStrokeTime = FPlatformTime::Seconds();
	}

	return FReply::Unhandled();
}

FReply SCppEditorPane::HandleCodeTextBoxKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	DismissHoverDoc();
	const FKey Key = InKeyEvent.GetKey();

	// 0. AI Copilot Ghost Text Key Handling
	if (HasGhostText())
	{
		if (Key == EKeys::Tab)
		{
			CommitGhostText();
			return FReply::Handled();
		}
		if (Key == EKeys::Escape)
		{
			DismissGhostText();
			return FReply::Handled();
		}
		if (Key == EKeys::Up || Key == EKeys::Down || Key == EKeys::Left || Key == EKeys::Right ||
			Key == EKeys::PageUp || Key == EKeys::PageDown || Key == EKeys::Home || Key == EKeys::End)
		{
			DismissGhostText();
		}
	}

	// Manual Trigger for AI Copilot Completion (Alt+/)
	if (InKeyEvent.IsAltDown() && Key == EKeys::Slash)
	{
		TriggerAiInlineCompletion();
		return FReply::Handled();
	}

	// 1. If IntelliSense popup is active, route navigation/commit keys
	if (bIntelliSenseActive && FilteredIntelliSenseItems.Num() > 0)
	{
		if (Key == EKeys::Up)
		{
			IntelliSenseNavigate(-1);
			return FReply::Handled();
		}
		if (Key == EKeys::Down)
		{
			IntelliSenseNavigate(1);
			return FReply::Handled();
		}
		if (Key == EKeys::Tab || Key == EKeys::Enter)
		{
			CommitIntelliSense();
			return FReply::Handled();
		}
		if (Key == EKeys::Escape)
		{
			DismissIntelliSense();
			return FReply::Handled();
		}
	}

	// 2. Smart Enter: preserve indentation level of current line and auto-indent on '{' or ':'
	if (Key == EKeys::Enter && !InKeyEvent.IsControlDown() && !InKeyEvent.IsAltDown())
	{
		const FCppEditorSettings& Settings = FCppEditorSettings::Get();
		if (Settings.bAutoIndent && CodeTextBox.IsValid())
		{
			FString CurrentLine;
			CodeTextBox->GetCurrentTextLine(CurrentLine);
			FTextLocation CursorLoc = CodeTextBox->GetCursorLocation();
			int32 Col = FMath::Clamp(CursorLoc.GetOffset(), 0, CurrentLine.Len());

			// Extract leading whitespace
			FString Indent;
			for (int32 i = 0; i < CurrentLine.Len(); ++i)
			{
				TCHAR Ch = CurrentLine[i];
				if (Ch == TEXT(' ') || Ch == TEXT('\t'))
				{
					Indent.AppendChar(Ch);
				}
				else
				{
					break;
				}
			}

			// Single indent step based on settings
			FString IndentStep;
			if (Settings.TabSize == ECppTabSize::TwoSpaces)
			{
				IndentStep = TEXT("  ");
			}
			else if (Settings.TabSize == ECppTabSize::TabCharacter)
			{
				IndentStep = TEXT("\t");
			}
			else
			{
				IndentStep = TEXT("    ");
			}

			// Check if line before cursor increases indent
			FString TextBeforeCursor = CurrentLine.Left(Col).TrimEnd();
			bool bIncreaseIndent = TextBeforeCursor.EndsWith(TEXT("{")) || TextBeforeCursor.EndsWith(TEXT(":"));
			FString NextIndent = Indent + (bIncreaseIndent ? IndentStep : TEXT(""));

			// Check if cursor is between '{' and '}'
			FString TextAfterCursor = CurrentLine.Mid(Col).TrimStart();
			if (TextBeforeCursor.EndsWith(TEXT("{")) && TextAfterCursor.StartsWith(TEXT("}")))
			{
				FString InsertStr = TEXT("\n") + NextIndent + TEXT("\n") + Indent;
				CodeTextBox->InsertTextAtCursor(InsertStr);

				// Move cursor to middle indented line
				FTextLocation NewLoc(CursorLoc.GetLineIndex() + 1, NextIndent.Len());
				CodeTextBox->GoTo(NewLoc);
				CodeTextBox->ScrollTo(NewLoc);
			}
			else
			{
				CodeTextBox->InsertTextAtCursor(TEXT("\n") + NextIndent);
			}

			bEnterKeyHandledInKeyDown = true;
			return FReply::Handled();
		}
	}

	// 3. Tab key handling when IntelliSense is not active: insert spaces to tab stop or tab character
	if (Key == EKeys::Tab && !InKeyEvent.IsControlDown() && !InKeyEvent.IsAltDown())
	{
		if (CodeTextBox.IsValid())
		{
			const FCppEditorSettings& Settings = FCppEditorSettings::Get();
			if (Settings.TabSize == ECppTabSize::TabCharacter)
			{
				CodeTextBox->InsertTextAtCursor(TEXT("\t"));
			}
			else
			{
				int32 TabWidth = (Settings.TabSize == ECppTabSize::TwoSpaces) ? 2 : 4;
				FTextLocation CursorLoc = CodeTextBox->GetCursorLocation();
				int32 SpacesToNextTabStop = TabWidth - (CursorLoc.GetOffset() % TabWidth);
				if (SpacesToNextTabStop <= 0)
				{
					SpacesToNextTabStop = TabWidth;
				}
				FString IndentSpaces = FString::ChrN(SpacesToNextTabStop, TEXT(' '));
				CodeTextBox->InsertTextAtCursor(IndentSpaces);
			}
		}
		return FReply::Handled();
	}

	// 4. Smart Backspace: delete matching pairs or indent spaces
	if (Key == EKeys::BackSpace && !InKeyEvent.IsControlDown() && !InKeyEvent.IsAltDown())
	{
		if (CodeTextBox.IsValid() && !CodeTextBox->AnyTextSelected() && ActiveDocumentIndex >= 0 && ActiveDocumentIndex < OpenDocuments.Num())
		{
			const FCppEditorSettings& Settings = FCppEditorSettings::Get();
			FTextLocation CursorLoc = CodeTextBox->GetCursorLocation();
			int32 Col = CursorLoc.GetOffset();
			int32 LineIndex = CursorLoc.GetLineIndex();
			FString CurrentLine;
			CodeTextBox->GetCurrentTextLine(CurrentLine);

			// A. Delete auto-closed pair if cursor is right between them
			if (Settings.bAutoCloseBrackets && Col > 0 && Col < CurrentLine.Len())
			{
				TCHAR Before = CurrentLine[Col - 1];
				TCHAR After = CurrentLine[Col];
				if ((Before == TEXT('(') && After == TEXT(')')) ||
					(Before == TEXT('[') && After == TEXT(']')) ||
					(Before == TEXT('{') && After == TEXT('}')) ||
					(Before == TEXT('"') && After == TEXT('"')) ||
					(Before == TEXT('\'') && After == TEXT('\'')))
				{
					TSharedPtr<FEditorDocument> Doc = OpenDocuments[ActiveDocumentIndex];
					TArray<int32> LineOffsets;
					LineOffsets.Add(0);
					for (int32 i = 0; i < Doc->CurrentContent.Len(); ++i)
					{
						if (Doc->CurrentContent[i] == TEXT('\n'))
						{
							LineOffsets.Add(i + 1);
						}
					}

					if (LineIndex < LineOffsets.Num())
					{
						int32 CharPos = LineOffsets[LineIndex] + Col;
						if (CharPos >= 1 && CharPos < Doc->CurrentContent.Len())
						{
							Doc->UndoHistory.Add(Doc->CurrentContent);
							Doc->RedoHistory.Empty();
							Doc->CurrentContent.RemoveAt(CharPos - 1, 2);
							Doc->bIsDirty = (Doc->CurrentContent != Doc->SavedContent);
							Doc->bIsInternalTextChange = true;
							CodeTextBox->SetText(FText::FromString(Doc->CurrentContent));
							Doc->bIsInternalTextChange = false;

							FTextLocation NewLoc(LineIndex, Col - 1);
							CodeTextBox->GoTo(NewLoc);
							CodeTextBox->ScrollTo(NewLoc);

							OnDocumentContentChanged.ExecuteIfBound(Doc->FilePath, Doc->CurrentContent);
							return FReply::Handled();
						}
					}
				}
			}

			// B. Delete indent width at once when at leading indent
			int32 IndentWidth = (Settings.TabSize == ECppTabSize::TwoSpaces) ? 2 : 4;
			if (Settings.TabSize != ECppTabSize::TabCharacter && Col >= IndentWidth && Col <= CurrentLine.Len())
			{
				bool bOnlyLeadingSpaces = true;
				for (int32 i = 0; i < Col; ++i)
				{
					if (CurrentLine[i] != TEXT(' '))
					{
						bOnlyLeadingSpaces = false;
						break;
					}
				}

				if (bOnlyLeadingSpaces && (Col % IndentWidth == 0))
				{
					TSharedPtr<FEditorDocument> Doc = OpenDocuments[ActiveDocumentIndex];
					TArray<int32> LineOffsets;
					LineOffsets.Add(0);
					for (int32 i = 0; i < Doc->CurrentContent.Len(); ++i)
					{
						if (Doc->CurrentContent[i] == TEXT('\n'))
						{
							LineOffsets.Add(i + 1);
						}
					}

					if (LineIndex < LineOffsets.Num())
					{
						int32 CharPos = LineOffsets[LineIndex] + Col;
						if (CharPos >= IndentWidth && CharPos <= Doc->CurrentContent.Len())
						{
							Doc->UndoHistory.Add(Doc->CurrentContent);
							Doc->RedoHistory.Empty();
							Doc->CurrentContent.RemoveAt(CharPos - IndentWidth, IndentWidth);
							Doc->bIsDirty = (Doc->CurrentContent != Doc->SavedContent);
							Doc->bIsInternalTextChange = true;
							CodeTextBox->SetText(FText::FromString(Doc->CurrentContent));
							Doc->bIsInternalTextChange = false;

							FTextLocation NewLoc(LineIndex, Col - IndentWidth);
							CodeTextBox->GoTo(NewLoc);
							CodeTextBox->ScrollTo(NewLoc);

							OnDocumentContentChanged.ExecuteIfBound(Doc->FilePath, Doc->CurrentContent);
							return FReply::Handled();
						}
					}
				}
			}
		}
	}

	// 5. Hotkeys from inside the CodeTextBox
	if (InKeyEvent.IsControlDown() && Key == EKeys::Slash)
	{
		ToggleLineComment();
		return FReply::Handled();
	}
	if (InKeyEvent.IsControlDown() && Key == EKeys::D && !InKeyEvent.IsShiftDown())
	{
		DuplicateLine();
		return FReply::Handled();
	}
	if (InKeyEvent.IsAltDown() && Key == EKeys::Up)
	{
		MoveLine(-1);
		return FReply::Handled();
	}
	if (InKeyEvent.IsAltDown() && Key == EKeys::Down)
	{
		MoveLine(1);
		return FReply::Handled();
	}
	if (InKeyEvent.IsControlDown() && InKeyEvent.IsShiftDown() && Key == EKeys::K)
	{
		DeleteLine();
		return FReply::Handled();
	}
	if (InKeyEvent.IsAltDown() && Key == EKeys::O)
	{
		ToggleHeaderSource();
		return FReply::Handled();
	}
	if (InKeyEvent.IsAltDown() && Key == EKeys::Enter)
	{
		QuickActionGenerateDefinition();
		return FReply::Handled();
	}

	if (InKeyEvent.IsControlDown() && Key == EKeys::Z && !InKeyEvent.IsShiftDown())
	{
		Undo();
		return FReply::Handled();
	}
	if ((InKeyEvent.IsControlDown() && Key == EKeys::Y) ||
		(InKeyEvent.IsControlDown() && InKeyEvent.IsShiftDown() && Key == EKeys::Z))
	{
		Redo();
		return FReply::Handled();
	}
	if (InKeyEvent.IsControlDown() && Key == EKeys::S)
	{
		SaveCurrentFile();
		return FReply::Handled();
	}
	if (InKeyEvent.IsControlDown() && Key == EKeys::F)
	{
		ToggleFindBar(!bFindBarVisible);
		return FReply::Handled();
	}
	if (InKeyEvent.IsControlDown() && Key == EKeys::P)
	{
		OnQuickOpenRequested.ExecuteIfBound();
		return FReply::Handled();
	}
	if (InKeyEvent.IsControlDown() && Key == EKeys::N)
	{
		OnNewClassRequested.ExecuteIfBound();
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

int32 SCppEditorPane::GetTotalLineCount() const
{
	if (ActiveDocumentIndex >= 0 && ActiveDocumentIndex < OpenDocuments.Num())
	{
		const FString& Content = OpenDocuments[ActiveDocumentIndex]->CurrentContent;
		int32 Lines = 1;
		for (int32 i = 0; i < Content.Len(); ++i)
		{
			if (Content[i] == TEXT('\n'))
			{
				Lines++;
			}
		}
		return Lines;
	}
	return 0;
}

int32 SCppEditorPane::GetCurrentLineIndex() const
{
	if (CodeTextBox.IsValid())
	{
		return CodeTextBox->GetCursorLocation().GetLineIndex();
	}
	return 0;
}

int32 SCppEditorPane::GetCurrentColumnIndex() const
{
	if (CodeTextBox.IsValid())
	{
		return CodeTextBox->GetCursorLocation().GetOffset();
	}
	return 0;
}

void SCppEditorPane::ToggleLineComment()
{
	if (!CodeTextBox.IsValid() || ActiveDocumentIndex == INDEX_NONE || ActiveDocumentIndex >= OpenDocuments.Num())
	{
		return;
	}

	TSharedPtr<FEditorDocument> Doc = OpenDocuments[ActiveDocumentIndex];
	FTextLocation CursorLoc = CodeTextBox->GetCursorLocation();
	int32 LineIndex = CursorLoc.GetLineIndex();
	int32 Col = CursorLoc.GetOffset();

	FString CurrentLine;
	CodeTextBox->GetCurrentTextLine(CurrentLine);

	Doc->UndoHistory.Add(Doc->CurrentContent);
	Doc->RedoHistory.Empty();

	TArray<int32> LineStartOffsets;
	LineStartOffsets.Add(0);
	for (int32 i = 0; i < Doc->CurrentContent.Len(); ++i)
	{
		if (Doc->CurrentContent[i] == TEXT('\n'))
		{
			LineStartOffsets.Add(i + 1);
		}
	}

	if (LineIndex >= LineStartOffsets.Num())
	{
		return;
	}

	int32 LineStart = LineStartOffsets[LineIndex];
	int32 LineEnd = (LineIndex + 1 < LineStartOffsets.Num()) ? (LineStartOffsets[LineIndex + 1] - 1) : Doc->CurrentContent.Len();

	FString ModifiedLine;
	FString Trimmed = CurrentLine.TrimStart();
	int32 LeadingSpaces = CurrentLine.Len() - Trimmed.Len();

	if (Trimmed.StartsWith(TEXT("//")))
	{
		int32 SlashPos = CurrentLine.Find(TEXT("//"));
		int32 RemoveLen = (SlashPos + 2 < CurrentLine.Len() && CurrentLine[SlashPos + 2] == TEXT(' ')) ? 3 : 2;
		ModifiedLine = CurrentLine.Left(SlashPos) + CurrentLine.Mid(SlashPos + RemoveLen);
	}
	else
	{
		ModifiedLine = CurrentLine.Left(LeadingSpaces) + TEXT("// ") + CurrentLine.Mid(LeadingSpaces);
	}

	Doc->CurrentContent = Doc->CurrentContent.Left(LineStart) + ModifiedLine + Doc->CurrentContent.Mid(LineEnd);
	Doc->bIsDirty = (Doc->CurrentContent != Doc->SavedContent);
	Doc->bIsInternalTextChange = true;
	CodeTextBox->SetText(FText::FromString(Doc->CurrentContent));
	Doc->bIsInternalTextChange = false;

	CodeTextBox->GoTo(FTextLocation(LineIndex, Col));
	CodeTextBox->ScrollTo(FTextLocation(LineIndex, Col));
	OnDocumentContentChanged.ExecuteIfBound(Doc->FilePath, Doc->CurrentContent);
}

void SCppEditorPane::DuplicateLine()
{
	if (!CodeTextBox.IsValid() || ActiveDocumentIndex == INDEX_NONE || ActiveDocumentIndex >= OpenDocuments.Num())
	{
		return;
	}

	TSharedPtr<FEditorDocument> Doc = OpenDocuments[ActiveDocumentIndex];
	FTextLocation CursorLoc = CodeTextBox->GetCursorLocation();
	int32 LineIndex = CursorLoc.GetLineIndex();
	int32 Col = CursorLoc.GetOffset();

	FString CurrentLine;
	CodeTextBox->GetCurrentTextLine(CurrentLine);

	Doc->UndoHistory.Add(Doc->CurrentContent);
	Doc->RedoHistory.Empty();

	TArray<int32> LineStartOffsets;
	LineStartOffsets.Add(0);
	for (int32 i = 0; i < Doc->CurrentContent.Len(); ++i)
	{
		if (Doc->CurrentContent[i] == TEXT('\n'))
		{
			LineStartOffsets.Add(i + 1);
		}
	}

	if (LineIndex >= LineStartOffsets.Num())
	{
		return;
	}

	int32 LineEnd = (LineIndex + 1 < LineStartOffsets.Num()) ? (LineStartOffsets[LineIndex + 1] - 1) : Doc->CurrentContent.Len();
	FString InsertText = TEXT("\n") + CurrentLine;

	Doc->CurrentContent = Doc->CurrentContent.Left(LineEnd) + InsertText + Doc->CurrentContent.Mid(LineEnd);
	Doc->bIsDirty = (Doc->CurrentContent != Doc->SavedContent);
	Doc->bIsInternalTextChange = true;
	CodeTextBox->SetText(FText::FromString(Doc->CurrentContent));
	Doc->bIsInternalTextChange = false;

	CodeTextBox->GoTo(FTextLocation(LineIndex + 1, Col));
	CodeTextBox->ScrollTo(FTextLocation(LineIndex + 1, Col));
	OnDocumentContentChanged.ExecuteIfBound(Doc->FilePath, Doc->CurrentContent);
}

void SCppEditorPane::MoveLine(int32 Direction)
{
	if (!CodeTextBox.IsValid() || ActiveDocumentIndex == INDEX_NONE || ActiveDocumentIndex >= OpenDocuments.Num())
	{
		return;
	}

	TSharedPtr<FEditorDocument> Doc = OpenDocuments[ActiveDocumentIndex];
	FTextLocation CursorLoc = CodeTextBox->GetCursorLocation();
	int32 LineIndex = CursorLoc.GetLineIndex();
	int32 Col = CursorLoc.GetOffset();

	TArray<FString> Lines;
	Doc->CurrentContent.ParseIntoArrayLines(Lines, false);

	int32 TargetIndex = LineIndex + Direction;
	if (TargetIndex < 0 || TargetIndex >= Lines.Num())
	{
		return;
	}

	Doc->UndoHistory.Add(Doc->CurrentContent);
	Doc->RedoHistory.Empty();

	Lines.Swap(LineIndex, TargetIndex);

	FString NewContent = FString::Join(Lines, TEXT("\n"));
	Doc->CurrentContent = NewContent;
	Doc->bIsDirty = (Doc->CurrentContent != Doc->SavedContent);
	Doc->bIsInternalTextChange = true;
	CodeTextBox->SetText(FText::FromString(Doc->CurrentContent));
	Doc->bIsInternalTextChange = false;

	CodeTextBox->GoTo(FTextLocation(TargetIndex, Col));
	CodeTextBox->ScrollTo(FTextLocation(TargetIndex, Col));
	OnDocumentContentChanged.ExecuteIfBound(Doc->FilePath, Doc->CurrentContent);
}

void SCppEditorPane::DeleteLine()
{
	if (!CodeTextBox.IsValid() || ActiveDocumentIndex == INDEX_NONE || ActiveDocumentIndex >= OpenDocuments.Num())
	{
		return;
	}

	TSharedPtr<FEditorDocument> Doc = OpenDocuments[ActiveDocumentIndex];
	FTextLocation CursorLoc = CodeTextBox->GetCursorLocation();
	int32 LineIndex = CursorLoc.GetLineIndex();

	TArray<FString> Lines;
	Doc->CurrentContent.ParseIntoArrayLines(Lines, false);

	if (LineIndex < 0 || LineIndex >= Lines.Num())
	{
		return;
	}

	Doc->UndoHistory.Add(Doc->CurrentContent);
	Doc->RedoHistory.Empty();

	Lines.RemoveAt(LineIndex);
	if (Lines.Num() == 0)
	{
		Lines.Add(TEXT(""));
	}

	FString NewContent = FString::Join(Lines, TEXT("\n"));
	Doc->CurrentContent = NewContent;
	Doc->bIsDirty = (Doc->CurrentContent != Doc->SavedContent);
	Doc->bIsInternalTextChange = true;
	CodeTextBox->SetText(FText::FromString(Doc->CurrentContent));
	Doc->bIsInternalTextChange = false;

	int32 NewLine = FMath::Clamp(LineIndex, 0, Lines.Num() - 1);
	CodeTextBox->GoTo(FTextLocation(NewLine, 0));
	CodeTextBox->ScrollTo(FTextLocation(NewLine, 0));
	OnDocumentContentChanged.ExecuteIfBound(Doc->FilePath, Doc->CurrentContent);
}

void SCppEditorPane::ToggleHeaderSource()
{
	FString ActivePath = GetActiveFilePath();
	if (ActivePath.IsEmpty())
	{
		return;
	}

	FString Ext = FPaths::GetExtension(ActivePath).ToLower();
	FString BaseName = FPaths::GetBaseFilename(ActivePath);
	FString Dir = FPaths::GetPath(ActivePath);

	TArray<FString> CandidatePaths;

	if (Ext == TEXT("h") || Ext == TEXT("hpp"))
	{
		CandidatePaths.Add(Dir / (BaseName + TEXT(".cpp")));
		if (Dir.Contains(TEXT("Public")))
		{
			CandidatePaths.Add(Dir.Replace(TEXT("Public"), TEXT("Private")) / (BaseName + TEXT(".cpp")));
		}
	}
	else if (Ext == TEXT("cpp"))
	{
		CandidatePaths.Add(Dir / (BaseName + TEXT(".h")));
		if (Dir.Contains(TEXT("Private")))
		{
			CandidatePaths.Add(Dir.Replace(TEXT("Private"), TEXT("Public")) / (BaseName + TEXT(".h")));
		}
	}

	for (const FString& Candidate : CandidatePaths)
	{
		if (IFileManager::Get().FileExists(*Candidate))
		{
			OpenFile(Candidate);
			return;
		}
	}

	Log(FString::Printf(TEXT("[Toggle] Corresponding file not found for: %s"), *FPaths::GetCleanFilename(ActivePath)));
}

void SCppEditorPane::QuickActionGenerateDefinition()
{
	FString ActivePath = GetActiveFilePath();
	if (!ActivePath.EndsWith(TEXT(".h")))
	{
		Log(TEXT("[QuickAction] Generate Definition is only available in header (.h) files."));
		return;
	}

	FString CurrentLine;
	CodeTextBox->GetCurrentTextLine(CurrentLine);
	FString Trimmed = CurrentLine.TrimStartAndEnd();

	if (!Trimmed.Contains(TEXT("(")) || !Trimmed.Contains(TEXT(")")) || !Trimmed.EndsWith(TEXT(";")) || Trimmed.Contains(TEXT("{")))
	{
		Log(TEXT("[QuickAction] Cursor is not on a valid function declaration in header."));
		return;
	}

	// 1. Find enclosing class name
	TSharedPtr<FEditorDocument> Doc = OpenDocuments[ActiveDocumentIndex];
	TArray<FString> Lines;
	Doc->CurrentContent.ParseIntoArrayLines(Lines, false);
	int32 CursorLine = CodeTextBox->GetCursorLocation().GetLineIndex();

	FString ClassName;
	for (int32 i = CursorLine; i >= 0; --i)
	{
		FString Line = Lines[i].TrimStartAndEnd();
		if (Line.StartsWith(TEXT("class ")) || Line.StartsWith(TEXT("struct ")))
		{
			TArray<FString> Tokens;
			Line.ParseIntoArrayWS(Tokens);
			for (int32 t = 1; t < Tokens.Num(); ++t)
			{
				FString Tok = Tokens[t];
				if (Tok.EndsWith(TEXT(":")) || Tok.EndsWith(TEXT(";")) || Tok == TEXT("final"))
				{
					continue;
				}
				if (!Tok.IsEmpty() && FChar::IsAlpha(Tok[0]))
				{
					ClassName = Tok;
					break;
				}
			}
			if (!ClassName.IsEmpty())
			{
				break;
			}
		}
	}

	// 2. Parse signature
	FString Sig = Trimmed.LeftChop(1).TrimEnd();
	if (Sig.EndsWith(TEXT("override")))
	{
		Sig = Sig.LeftChop(8).TrimEnd();
	}
	if (Sig.EndsWith(TEXT("final")))
	{
		Sig = Sig.LeftChop(5).TrimEnd();
	}

	Sig = Sig.Replace(TEXT("virtual "), TEXT(""));
	Sig = Sig.Replace(TEXT("static "), TEXT(""));
	Sig = Sig.Replace(TEXT("explicit "), TEXT(""));
	Sig = Sig.Replace(TEXT("inline "), TEXT(""));
	Sig = Sig.TrimStartAndEnd();

	int32 OpenParen = Sig.Find(TEXT("("));
	if (OpenParen == INDEX_NONE)
	{
		return;
	}

	FString ReturnAndName = Sig.Left(OpenParen).TrimEnd();
	FString ParamsAndConst = Sig.Mid(OpenParen);

	int32 LastSpace = ReturnAndName.Find(TEXT(" "), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	FString ReturnType;
	FString MethodName;
	if (LastSpace != INDEX_NONE)
	{
		ReturnType = ReturnAndName.Left(LastSpace).TrimEnd();
		MethodName = ReturnAndName.Mid(LastSpace + 1).TrimStart();
	}
	else
	{
		MethodName = ReturnAndName;
	}

	FString CleanParams;
	bool bInDefault = false;
	for (int32 c = 0; c < ParamsAndConst.Len(); ++c)
	{
		TCHAR Ch = ParamsAndConst[c];
		if (Ch == TEXT('='))
		{
			bInDefault = true;
			continue;
		}
		if (bInDefault && (Ch == TEXT(',') || Ch == TEXT(')')))
		{
			bInDefault = false;
		}
		if (!bInDefault)
		{
			CleanParams.AppendChar(Ch);
		}
	}

	FString QualifiedName = ClassName.IsEmpty() ? MethodName : (ClassName + TEXT("::") + MethodName);
	FString DefinitionCode = TEXT("\n\n");
	if (!ReturnType.IsEmpty())
	{
		DefinitionCode += ReturnType + TEXT(" ");
	}
	DefinitionCode += QualifiedName + CleanParams + TEXT("\n{\n\t\n}\n");

	// 3. Find target .cpp file
	FString BaseName = FPaths::GetBaseFilename(ActivePath);
	FString Dir = FPaths::GetPath(ActivePath);
	TArray<FString> CandidateCpp;
	CandidateCpp.Add(Dir / (BaseName + TEXT(".cpp")));
	if (Dir.Contains(TEXT("Public")))
	{
		CandidateCpp.Add(Dir.Replace(TEXT("Public"), TEXT("Private")) / (BaseName + TEXT(".cpp")));
	}

	FString TargetCppPath;
	for (const FString& Cand : CandidateCpp)
	{
		if (IFileManager::Get().FileExists(*Cand))
		{
			TargetCppPath = Cand;
			break;
		}
	}
	if (TargetCppPath.IsEmpty() && CandidateCpp.Num() > 0)
	{
		TargetCppPath = CandidateCpp.Last();
	}

	if (!TargetCppPath.IsEmpty())
	{
		OpenFile(TargetCppPath);
		if (ActiveDocumentIndex >= 0 && ActiveDocumentIndex < OpenDocuments.Num())
		{
			TSharedPtr<FEditorDocument> CppDoc = OpenDocuments[ActiveDocumentIndex];
			CppDoc->UndoHistory.Add(CppDoc->CurrentContent);
			CppDoc->RedoHistory.Empty();
			CppDoc->CurrentContent += DefinitionCode;
			CppDoc->bIsDirty = true;
			CppDoc->bIsInternalTextChange = true;
			CodeTextBox->SetText(FText::FromString(CppDoc->CurrentContent));
			CppDoc->bIsInternalTextChange = false;

			int32 TotalLines = GetTotalLineCount();
			FTextLocation NewLoc(FMath::Max(0, TotalLines - 2), 1);
			CodeTextBox->GoTo(NewLoc);
			CodeTextBox->ScrollTo(NewLoc);
			FocusEditor();

			Log(FString::Printf(TEXT("[QuickAction] Successfully generated definition for %s in %s"), *QualifiedName, *FPaths::GetCleanFilename(TargetCppPath)));
		}
	}
}

void SCppEditorPane::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	// Debounce AI Copilot Inline Completion
	if (bPendingAiRequest)
	{
		if (FCppEditorSettings::Get().bEnableAiInlineCompletion)
		{
			const double DelaySec = (double)FCppEditorSettings::Get().AiGhostTextDelayMs / 1000.0;
			if (FPlatformTime::Seconds() - LastKeyStrokeTime >= DelaySec)
			{
				bPendingAiRequest = false;
				TriggerAiInlineCompletion();
			}
		}
		else
		{
			bPendingAiRequest = false;
		}
	}

	// Hover documentation (Quick Info) detection
	if (FSlateApplication::IsInitialized())
	{
		FVector2D CurrentCursorPos = FSlateApplication::Get().GetCursorPos();
		const float MoveDist = FVector2D::Distance(CurrentCursorPos, LastMouseScreenPosition);
		if (MoveDist > 3.0f)
		{
			LastMouseScreenPosition = CurrentCursorPos;
			LastMouseMoveTime = FPlatformTime::Seconds();

			// If hover doc is visible, check if mouse moved over the hover card itself
			if (bHoverDocVisible)
			{
				bool bOverCard = false;
				if (HoverDocCard.IsValid() && HoverDocCard->GetVisibility() == EVisibility::Visible)
				{
					FGeometry CardGeo = HoverDocCard->GetTickSpaceGeometry();
					FVector2D CardLocal = CardGeo.AbsoluteToLocal(CurrentCursorPos);
					FVector2D CardSize = (FVector2D)CardGeo.GetLocalSize();
					if (CardLocal.X >= -10.0f && CardLocal.X <= CardSize.X + 10.0f &&
					    CardLocal.Y >= -10.0f && CardLocal.Y <= CardSize.Y + 10.0f)
					{
						bOverCard = true;
					}
				}

				// Stay open if cursor is over the card OR still within 45 pixels of the trigger location (the word)
				const float DistFromWord = FVector2D::Distance(CurrentCursorPos, HoverDocScreenPosition);
				if (!bOverCard && DistFromWord > 45.0f)
				{
					DismissHoverDoc();
				}
			}
		}
		else
		{
			// Mouse has been stationary
			const double StationaryDuration = FPlatformTime::Seconds() - LastMouseMoveTime;
			if (StationaryDuration >= 0.250 && !bIntelliSenseActive)
			{
				FString HoveredWord = GetWordAtScreenPosition(CurrentCursorPos);
				if (!HoveredWord.IsEmpty())
				{
					if (!bHoverDocVisible || HoveredWord != LastHoveredWord)
					{
						ShowHoverDoc(HoveredWord, CurrentCursorPos);
					}
				}
				else if (bHoverDocVisible)
				{
					bool bOverCard = false;
					if (HoverDocCard.IsValid() && HoverDocCard->GetVisibility() == EVisibility::Visible)
					{
						FGeometry CardGeo = HoverDocCard->GetTickSpaceGeometry();
						FVector2D CardLocal = CardGeo.AbsoluteToLocal(CurrentCursorPos);
						FVector2D CardSize = (FVector2D)CardGeo.GetLocalSize();
						if (CardLocal.X >= 0.0f && CardLocal.X <= CardSize.X && CardLocal.Y >= 0.0f && CardLocal.Y <= CardSize.Y)
						{
							bOverCard = true;
						}
					}
					if (!bOverCard)
					{
						DismissHoverDoc();
					}
				}
			}
		}
	}
}

void SCppEditorPane::TriggerAiInlineCompletion()
{
	if (!CodeTextBox.IsValid() || ActiveDocumentIndex == INDEX_NONE || ActiveDocumentIndex >= OpenDocuments.Num())
	{
		return;
	}

	TSharedPtr<FEditorDocument> Doc = OpenDocuments[ActiveDocumentIndex];
	FTextLocation CursorLoc = CodeTextBox->GetCursorLocation();

	// Calculate character index from line/offset
	const FString& Content = Doc->CurrentContent;
	int32 TargetLine = CursorLoc.GetLineIndex();
	int32 TargetOffset = CursorLoc.GetOffset();

	int32 CurrentLine = 0;
	int32 CharIndex = 0;
	for (int32 i = 0; i < Content.Len(); ++i)
	{
		if (CurrentLine == TargetLine)
		{
			CharIndex = i + FMath::Clamp(TargetOffset, 0, Content.Len() - i);
			break;
		}
		if (Content[i] == TEXT('\n'))
		{
			CurrentLine++;
		}
	}
	if (CurrentLine < TargetLine)
	{
		CharIndex = Content.Len();
	}

	// Extract prefix (up to 2000 chars before cursor) and suffix (up to 500 chars after cursor)
	int32 PrefixStart = FMath::Max(0, CharIndex - 2000);
	FString Prefix = Content.Mid(PrefixStart, CharIndex - PrefixStart);

	int32 SuffixLen = FMath::Min(500, Content.Len() - CharIndex);
	FString Suffix = Content.Mid(CharIndex, SuffixLen);

	TWeakPtr<SCppEditorPane> WeakThis = SharedThis(this);
	FCppAiAssistant::Get().RequestInlineCompletion(
		Prefix,
		Suffix,
		Doc->FilePath,
		FOnAiCompletionReceived::CreateLambda([WeakThis, CursorLoc](const FString& CompletionText, bool bSuccess)
		{
			if (TSharedPtr<SCppEditorPane> Pinned = WeakThis.Pin())
			{
				Pinned->OnAiCompletionReceived(CompletionText, bSuccess, CursorLoc);
			}
		})
	);
}

void SCppEditorPane::OnAiCompletionReceived(const FString& CompletionText, bool bSuccess, FTextLocation OriginalCursorLoc)
{
	if (!bSuccess || CompletionText.IsEmpty())
	{
		DismissGhostText();
		return;
	}

	if (!CodeTextBox.IsValid())
	{
		return;
	}

	FTextLocation CurrentLoc = CodeTextBox->GetCursorLocation();
	if (CurrentLoc != OriginalCursorLoc)
	{
		// Cursor moved in the meantime
		return;
	}

	ActiveGhostText = CompletionText;
	GhostTextLocation = OriginalCursorLoc;
	bGhostTextVisible = true;
}

void SCppEditorPane::CommitGhostText()
{
	if (HasGhostText() && CodeTextBox.IsValid())
	{
		FString TextToInsert = ActiveGhostText;
		DismissGhostText();
		CodeTextBox->InsertTextAtCursor(TextToInsert);
	}
}

void SCppEditorPane::DismissGhostText()
{
	bGhostTextVisible = false;
	ActiveGhostText.Empty();
	FCppAiAssistant::Get().CancelPendingCompletion();
}

void SCppEditorPane::InsertCodeAtCursor(const FString& InCode)
{
	if (CodeTextBox.IsValid() && !InCode.IsEmpty())
	{
		DismissGhostText();
		CodeTextBox->InsertTextAtCursor(InCode);
	}
}

void SCppEditorPane::UpdateBreadcrumbs()
{
	if (!BreadcrumbsBox.IsValid())
	{
		return;
	}

	BreadcrumbsBox->ClearChildren();

	if (ActiveDocumentIndex == INDEX_NONE || ActiveDocumentIndex >= OpenDocuments.Num())
	{
		return;
	}

	TSharedPtr<FEditorDocument> Doc = OpenDocuments[ActiveDocumentIndex];
	FString RelativePath = Doc->FilePath;
	FPaths::MakePathRelativeTo(RelativePath, *FPaths::ProjectDir());

	TArray<FString> Parts;
	RelativePath.ParseIntoArray(Parts, TEXT("/"), true);
	if (Parts.Num() == 0)
	{
		RelativePath.ParseIntoArray(Parts, TEXT("\\"), true);
	}

	FString ProjectName = FApp::GetProjectName();
	if (ProjectName.IsEmpty())
	{
		ProjectName = TEXT("Project");
	}

	Parts.Insert(ProjectName, 0);

	for (int32 i = 0; i < Parts.Num(); ++i)
	{
		const bool bIsLast = (i == Parts.Num() - 1);
		const FString& Part = Parts[i];

		BreadcrumbsBox->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Part))
				.Font(FCoreStyle::GetDefaultFontStyle(bIsLast ? "Bold" : "Regular", 9))
				.ColorAndOpacity(bIsLast ? FLinearColor(0.92f, 0.92f, 0.96f, 1.0f) : FLinearColor(0.55f, 0.58f, 0.65f, 1.0f))
				.ToolTipText(FText::FromString(Doc->FilePath))
			];

		if (!bIsLast)
		{
			BreadcrumbsBox->AddSlot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("›")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					.ColorAndOpacity(FLinearColor(0.35f, 0.38f, 0.45f, 0.8f))
				];
		}
	}
}

