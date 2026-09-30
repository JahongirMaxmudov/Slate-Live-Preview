// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#include "SCppSettingsDialog.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "DesktopPlatformModule.h"
#include "SlateLivePreviewStyle.h"
#include "CppAiAssistant.h"
#include "HAL/PlatformApplicationMisc.h"
#include "HAL/PlatformProcess.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

static const TCHAR* SettingsSampleCode = 
	TEXT("#include \"CoreMinimal.h\"\n")
	TEXT("#include \"GameFramework/Character.h\"\n\n")
	TEXT("// Live Theme & Font Preview\n")
	TEXT("class APlayerCharacter : public ACharacter\n")
	TEXT("{\n")
	TEXT("    GENERATED_BODY()\n")
	TEXT("public:\n")
	TEXT("    int32 Health = 100;\n")
	TEXT("    virtual void BeginPlay() override;\n")
	TEXT("    void TakeDamage(float DamageAmount);\n")
	TEXT("};\n");

void SCppSettingsDialog::Construct(const FArguments& InArgs)
{
	ParentWindow = InArgs._ParentWindow;

	// Copy current active settings
	FCppEditorSettings& Current = FCppEditorSettings::Get();
	TempTheme = Current.Theme;
	TempFontFamily = Current.FontFamily;
	TempCustomFontPath = Current.CustomFontPath;
	TempFontSize = Current.FontSize;
	TempTabSize = Current.TabSize;
	bTempAutoIndent = Current.bAutoIndent;
	bTempAutoCloseBrackets = Current.bAutoCloseBrackets;
	bTempShowLineNumbers = Current.bShowLineNumbers;
	bTempEnableIntelliSense = Current.bEnableIntelliSense;
	bTempWordWrap = Current.bWordWrap;
	bTempHighlightActiveLine = Current.bHighlightActiveLine;
	bTempAutoSaveOnLiveCoding = Current.bAutoSaveOnLiveCoding;
	bTempAutoReloadSlatePreview = Current.bAutoReloadSlatePreview;
	bTempEnableAiInlineCompletion = Current.bEnableAiInlineCompletion;
	TempAiProvider = Current.AiProvider;
	TempAiEndpoint = Current.AiEndpoint;
	TempAiModel = Current.AiModel;
	TempAiApiKey = Current.AiApiKey;
	TempAiGhostTextDelayMs = Current.AiGhostTextDelayMs;

	// Populate Theme Options
	for (int32 i = 0; i < (int32)ECppEditorTheme::Count; ++i)
	{
		TSharedPtr<FString> Option = MakeShared<FString>(FCppEditorSettings::GetThemeDisplayName((ECppEditorTheme)i));
		ThemeOptions.Add(Option);
		if ((ECppEditorTheme)i == TempTheme)
		{
			SelectedThemeOption = Option;
		}
	}

	// Populate Font Options
	for (int32 i = 0; i < (int32)ECppEditorFont::Count; ++i)
	{
		TSharedPtr<FString> Option = MakeShared<FString>(FCppEditorSettings::GetFontDisplayName((ECppEditorFont)i));
		FontOptions.Add(Option);
		if ((ECppEditorFont)i == TempFontFamily)
		{
			SelectedFontOption = Option;
		}
	}

	// Populate Font Sizes
	for (int32 Size : FCppEditorSettings::GetAvailableFontSizes())
	{
		TSharedPtr<FString> Option = MakeShared<FString>(FString::Printf(TEXT("%d pt"), Size));
		FontSizeOptions.Add(Option);
		if (Size == TempFontSize)
		{
			SelectedFontSizeOption = Option;
		}
	}

	// Populate AI Provider Options
	for (int32 i = 0; i < (int32)EAiProvider::Count; ++i)
	{
		TSharedPtr<FString> Option = MakeShared<FString>(FCppEditorSettings::GetAiProviderDisplayName((EAiProvider)i));
		AiProviderOptions.Add(Option);
		if ((EAiProvider)i == TempAiProvider)
		{
			SelectedAiProviderOption = Option;
		}
	}

	PreviewMarshaller = FCppSyntaxHighlighterMarshaller::Create(Current.GetSyntaxStyle());

	PreviewTextBox = SNew(SMultiLineEditableTextBox)
		.Marshaller(PreviewMarshaller)
		.Font_Lambda([this]()
		{
			FCppEditorSettings Temp;
			Temp.FontFamily = TempFontFamily;
			Temp.CustomFontPath = TempCustomFontPath;
			Temp.FontSize = TempFontSize;
			return Temp.GetFont();
		})
		.IsReadOnly(true)
		.AutoWrapText(false)
		.Text(FText::FromString(SettingsSampleCode));

	UpdatePreview();

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(12.0f)
		[
			SNew(SVerticalBox)

			// -----------------------------------------------------------------
			// 1. Header (Icon + Title + Subtitle)
			// -----------------------------------------------------------------
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 10.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(SImage)
					.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.Settings")))
					.DesiredSizeOverride(FVector2D(22.0f, 22.0f))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("C++ Studio Settings & Preferences")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
						.ColorAndOpacity(FLinearColor::White)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Customize syntax color themes, typography, tab indentations, and smart features")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.5f))
						.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.65f, 1.0f))
					]
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 10.0f)
			[
				SNew(SSeparator)
			]

			// -----------------------------------------------------------------
			// 2. Main Scrollable Body
			// -----------------------------------------------------------------
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SScrollBox)

				// Section A: Appearance & Typography
				+ SScrollBox::Slot()
				.Padding(0.0f, 0.0f, 0.0f, 12.0f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
					.Padding(10.0f)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 8.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Theme & Typography")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10.5f))
							.ColorAndOpacity(FLinearColor(0.2f, 0.7f, 1.0f, 1.0f))
						]

						// 1. Color Theme Dropdown
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 4.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(0.35f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Color Theme:")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
							]
							+ SHorizontalBox::Slot()
							.FillWidth(0.65f)
							[
								SNew(SComboBox<TSharedPtr<FString>>)
								.OptionsSource(&ThemeOptions)
								.InitiallySelectedItem(SelectedThemeOption)
								.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) -> TSharedRef<SWidget>
								{
									return SNew(STextBlock).Text(FText::FromString(*Item)).Font(FCoreStyle::GetDefaultFontStyle("Regular", 9));
								})
								.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Selected, ESelectInfo::Type)
								{
									if (Selected.IsValid())
									{
										SelectedThemeOption = Selected;
										int32 Index = ThemeOptions.IndexOfByKey(Selected);
										if (Index != INDEX_NONE)
										{
											TempTheme = (ECppEditorTheme)Index;
											UpdatePreview();
										}
									}
								})
								[
									SNew(STextBlock)
									.Text_Lambda([this]() -> FText
									{
										return SelectedThemeOption.IsValid() ? FText::FromString(*SelectedThemeOption) : FText::GetEmpty();
									})
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								]
							]
						]

						// 2. Font Family Dropdown
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 4.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(0.35f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Font Family:")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
							]
							+ SHorizontalBox::Slot()
							.FillWidth(0.65f)
							[
								SNew(SComboBox<TSharedPtr<FString>>)
								.OptionsSource(&FontOptions)
								.InitiallySelectedItem(SelectedFontOption)
								.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) -> TSharedRef<SWidget>
								{
									return SNew(STextBlock).Text(FText::FromString(*Item)).Font(FCoreStyle::GetDefaultFontStyle("Regular", 9));
								})
								.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Selected, ESelectInfo::Type)
								{
									if (Selected.IsValid())
									{
										SelectedFontOption = Selected;
										int32 Index = FontOptions.IndexOfByKey(Selected);
										if (Index != INDEX_NONE)
										{
											TempFontFamily = (ECppEditorFont)Index;
											UpdatePreview();
										}
									}
								})
								[
									SNew(STextBlock)
									.Text_Lambda([this]() -> FText
									{
										return SelectedFontOption.IsValid() ? FText::FromString(*SelectedFontOption) : FText::GetEmpty();
									})
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								]
							]
						]

						// 2b. Custom Font Path (Visible only if CustomFont selected)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 4.0f)
						[
							SNew(SHorizontalBox)
							.Visibility_Lambda([this]()
							{
								return (TempFontFamily == ECppEditorFont::CustomFont) ? EVisibility::Visible : EVisibility::Collapsed;
							})
							+ SHorizontalBox::Slot()
							.FillWidth(0.35f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Font File Path:")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.5f))
							]
							+ SHorizontalBox::Slot()
							.FillWidth(0.53f)
							.Padding(0.0f, 0.0f, 4.0f, 0.0f)
							[
								SNew(SEditableTextBox)
								.Text(FText::FromString(TempCustomFontPath))
								.HintText(FText::FromString(TEXT("Path to .ttf or .otf file...")))
								.OnTextChanged_Lambda([this](const FText& Text)
								{
									TempCustomFontPath = Text.ToString();
									UpdatePreview();
								})
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SButton)
								.Text(FText::FromString(TEXT("Browse...")))
								.OnClicked(this, &SCppSettingsDialog::OnBrowseFontClicked)
							]
						]

						// 3. Font Size Dropdown
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 4.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(0.35f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Font Size:")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
							]
							+ SHorizontalBox::Slot()
							.FillWidth(0.65f)
							[
								SNew(SComboBox<TSharedPtr<FString>>)
								.OptionsSource(&FontSizeOptions)
								.InitiallySelectedItem(SelectedFontSizeOption)
								.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) -> TSharedRef<SWidget>
								{
									return SNew(STextBlock).Text(FText::FromString(*Item)).Font(FCoreStyle::GetDefaultFontStyle("Regular", 9));
								})
								.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Selected, ESelectInfo::Type)
								{
									if (Selected.IsValid())
									{
										SelectedFontSizeOption = Selected;
										int32 Index = FontSizeOptions.IndexOfByKey(Selected);
										if (Index != INDEX_NONE && Index < FCppEditorSettings::GetAvailableFontSizes().Num())
										{
											TempFontSize = FCppEditorSettings::GetAvailableFontSizes()[Index];
											UpdatePreview();
										}
									}
								})
								[
									SNew(STextBlock)
									.Text_Lambda([this]() -> FText
									{
										return SelectedFontSizeOption.IsValid() ? FText::FromString(*SelectedFontSizeOption) : FText::GetEmpty();
									})
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								]
							]
						]

						// 4. Live Syntax Preview Box
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 8.0f, 0.0f, 0.0f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 0.0f, 0.0f, 3.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Live Preview:")))
								.Font(FCoreStyle::GetDefaultFontStyle("Italic", 8.5f))
								.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f))
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SBox)
								.HeightOverride(130.0f)
								[
									PreviewTextBox.ToSharedRef()
								]
							]
						]
					]
				]

				// Section B: Editor Formatting & Smart Features
				+ SScrollBox::Slot()
				.Padding(0.0f, 0.0f, 0.0f, 12.0f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
					.Padding(10.0f)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 8.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Editor Behaviors & Formatting")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10.5f))
							.ColorAndOpacity(FLinearColor(0.2f, 0.7f, 1.0f, 1.0f))
						]

						// Tab Size (Segmented Radio)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 3.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(0.35f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Tab Width:")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
							]
							+ SHorizontalBox::Slot()
							.FillWidth(0.65f)
							[
								SNew(SHorizontalBox)

								// 4 Spaces
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(0.0f, 0.0f, 12.0f, 0.0f)
								[
									SNew(SCheckBox)
									.IsChecked_Lambda([this]() { return (TempTabSize == ECppTabSize::FourSpaces) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
									.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { if (State == ECheckBoxState::Checked) TempTabSize = ECppTabSize::FourSpaces; })
									[
										SNew(STextBlock).Text(FText::FromString(TEXT("4 Spaces"))).Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.5f))
									]
								]

								// 2 Spaces
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(0.0f, 0.0f, 12.0f, 0.0f)
								[
									SNew(SCheckBox)
									.IsChecked_Lambda([this]() { return (TempTabSize == ECppTabSize::TwoSpaces) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
									.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { if (State == ECheckBoxState::Checked) TempTabSize = ECppTabSize::TwoSpaces; })
									[
										SNew(STextBlock).Text(FText::FromString(TEXT("2 Spaces"))).Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.5f))
									]
								]

								// Tab Character
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SCheckBox)
									.IsChecked_Lambda([this]() { return (TempTabSize == ECppTabSize::TabCharacter) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
									.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { if (State == ECheckBoxState::Checked) TempTabSize = ECppTabSize::TabCharacter; })
									[
										SNew(STextBlock).Text(FText::FromString(TEXT("Tab (\\t)"))).Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.5f))
									]
								]
							]
						]

						// Checkboxes for smart editor features
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 5.0f)
						[
							SNew(SCheckBox)
							.IsChecked(bTempAutoIndent ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
							.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bTempAutoIndent = (State == ECheckBoxState::Checked); })
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("Auto-Indent on Enter (Preserves indentation depth and indents inside braces)")))
							]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 5.0f)
						[
							SNew(SCheckBox)
							.IsChecked(bTempAutoCloseBrackets ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
							.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bTempAutoCloseBrackets = (State == ECheckBoxState::Checked); })
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("Auto-Close Brackets & Quotes (Automatically pairs (), {}, [], \"\", '')")))
							]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 5.0f)
						[
							SNew(SCheckBox)
							.IsChecked(bTempShowLineNumbers ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
							.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bTempShowLineNumbers = (State == ECheckBoxState::Checked); })
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("Show Line Numbers Gutter in Editor")))
							]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 5.0f)
						[
							SNew(SCheckBox)
							.IsChecked(bTempEnableIntelliSense ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
							.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bTempEnableIntelliSense = (State == ECheckBoxState::Checked); })
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("Enable IntelliSense Auto-Completion Suggestions Popup")))
							]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 5.0f)
						[
							SNew(SCheckBox)
							.IsChecked(bTempWordWrap ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
							.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bTempWordWrap = (State == ECheckBoxState::Checked); })
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("Word Wrap (Soft wrap long code lines)")))
							]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 5.0f)
						[
							SNew(SCheckBox)
							.IsChecked(bTempHighlightActiveLine ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
							.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bTempHighlightActiveLine = (State == ECheckBoxState::Checked); })
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("Highlight Active Editor Line")))
							]
						]
					]
				]

				// Section C: Build & Live Preview Integration
				+ SScrollBox::Slot()
				.Padding(0.0f, 0.0f, 0.0f, 12.0f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
					.Padding(10.0f)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 8.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Build & Live Preview")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10.5f))
							.ColorAndOpacity(FLinearColor(0.2f, 0.7f, 1.0f, 1.0f))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 5.0f)
						[
							SNew(SCheckBox)
							.IsChecked(bTempAutoSaveOnLiveCoding ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
							.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bTempAutoSaveOnLiveCoding = (State == ECheckBoxState::Checked); })
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("Auto-Save All Open Files Before Triggering Live Coding")))
							]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 5.0f)
						[
							SNew(SCheckBox)
							.IsChecked(bTempAutoReloadSlatePreview ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
							.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bTempAutoReloadSlatePreview = (State == ECheckBoxState::Checked); })
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("Real-time Slate Live Preview Auto-Refresh on Code Edit")))
							]
						]
					]
				]

				// Section D: AI Copilot & Assistant (Ollama, LM Studio, DeepSeek, OpenAI)
				+ SScrollBox::Slot()
				.Padding(0.0f, 0.0f, 0.0f, 12.0f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
					.Padding(10.0f)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 8.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("AI Copilot & Assistant (Ollama, LM Studio, DeepSeek, OpenAI)")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10.5f))
							.ColorAndOpacity(FLinearColor(0.6f, 0.4f, 1.0f, 1.0f))
						]

						// Enable AI Inline Completion Checkbox
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 5.0f)
						[
							SNew(SCheckBox)
							.IsChecked(bTempEnableAiInlineCompletion ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
							.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bTempEnableAiInlineCompletion = (State == ECheckBoxState::Checked); })
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("Enable Inline AI Ghost Text (Tab to accept, Esc to dismiss, Alt+/ to trigger)")))
							]
						]

						// AI Provider Dropdown
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 4.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(0.35f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Provider:")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
							]
							+ SHorizontalBox::Slot()
							.FillWidth(0.65f)
							[
								SNew(SComboBox<TSharedPtr<FString>>)
								.OptionsSource(&AiProviderOptions)
								.InitiallySelectedItem(SelectedAiProviderOption)
								.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) -> TSharedRef<SWidget>
								{
									return SNew(STextBlock).Text(FText::FromString(*Item)).Font(FCoreStyle::GetDefaultFontStyle("Regular", 9));
								})
								.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Selected, ESelectInfo::Type)
								{
									if (Selected.IsValid())
									{
										SelectedAiProviderOption = Selected;
										int32 Index = AiProviderOptions.IndexOfByKey(Selected);
										if (Index != INDEX_NONE)
										{
											TempAiProvider = (EAiProvider)Index;
											UpdateAiProviderDefaults();
										}
									}
								})
								[
									SNew(STextBlock)
									.Text_Lambda([this]() -> FText
									{
										return SelectedAiProviderOption.IsValid() ? FText::FromString(*SelectedAiProviderOption) : FText::GetEmpty();
									})
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								]
							]
						]

						// GitHub Copilot Device Authorization Card (visible when GitHubCopilot is selected)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 6.0f)
						[
							SAssignNew(GitHubCopilotCard, SBorder)
							.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
							.BorderBackgroundColor(FLinearColor(0.10f, 0.14f, 0.20f, 1.0f))
							.Padding(8.0f)
							.Visibility_Lambda([this]()
							{
								return (TempAiProvider == EAiProvider::GitHubCopilot) ? EVisibility::Visible : EVisibility::Collapsed;
							})
							[
								SNew(SVerticalBox)

								// Status Header
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
										SNew(SImage)
										.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.AIAssistant")))
										.DesiredSizeOverride(FVector2D(14.0f, 14.0f))
									]
									+ SHorizontalBox::Slot()
									.FillWidth(1.0f)
									.VAlign(VAlign_Center)
									[
										SAssignNew(GitHubAuthStatusText, STextBlock)
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9.0f))
										.ColorAndOpacity_Lambda([this]()
										{
											if (!GitHubAuthStatusMessage.IsEmpty() && (GitHubAuthStatusMessage.Contains(TEXT("failed")) || GitHubAuthStatusMessage.Contains(TEXT("Failed")) || GitHubAuthStatusMessage.Contains(TEXT("timed out"))))
											{
												return FLinearColor(1.0f, 0.35f, 0.35f, 1.0f);
											}
											if (!ActiveGitHubUserCode.IsEmpty())
											{
												return FLinearColor(1.0f, 0.85f, 0.2f, 1.0f);
											}
											return FCppAiAssistant::Get().IsGitHubAuthenticated() ? FLinearColor(0.35f, 0.88f, 0.50f, 1.0f) : FLinearColor(0.9f, 0.9f, 0.95f, 1.0f);
										})
										.Text_Lambda([this]() -> FText
										{
											if (!GitHubAuthStatusMessage.IsEmpty())
											{
												return FText::FromString(GitHubAuthStatusMessage);
											}
											const FCppEditorSettings& Settings = FCppEditorSettings::Get();
											if (FCppAiAssistant::Get().IsGitHubAuthenticated())
											{
												FString User = Settings.GitHubUsername.IsEmpty() ? TEXT("Authorized Account") : Settings.GitHubUsername;
												return FText::FromString(FString::Printf(TEXT("Signed in to GitHub as %s (Copilot Active)"), *User));
											}
											if (!ActiveGitHubUserCode.IsEmpty())
											{
												return FText::FromString(TEXT("Waiting for approval at github.com/login/device..."));
											}
											return FText::FromString(TEXT("Authenticate via GitHub OAuth (VS Code Device Flow):"));
										})
									]
								]

								// Dedicated Device Code Display Banner (appears during device auth flow)
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(0.0f, 6.0f, 0.0f, 6.0f)
								[
									SNew(SBorder)
									.Visibility_Lambda([this]()
									{
										return ActiveGitHubUserCode.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
									})
									.BorderBackgroundColor(FLinearColor(0.08f, 0.22f, 0.42f, 0.95f))
									.Padding(FMargin(12.0f, 10.0f))
									[
										SNew(SVerticalBox)
										// Header label
										+ SVerticalBox::Slot()
										.AutoHeight()
										.Padding(0.0f, 0.0f, 0.0f, 4.0f)
										[
											SNew(STextBlock)
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9.5f))
											.ColorAndOpacity(FLinearColor(1.0f, 0.85f, 0.25f, 1.0f))
											.Text(FText::FromString(TEXT("1. Copy this GitHub Device Code:")))
										]
										// Big Code Display & Action Buttons
										+ SVerticalBox::Slot()
										.AutoHeight()
										.Padding(0.0f, 2.0f, 0.0f, 6.0f)
										[
											SNew(SHorizontalBox)
											+ SHorizontalBox::Slot()
											.AutoWidth()
											.VAlign(VAlign_Center)
											[
												SNew(SBorder)
												.BorderBackgroundColor(FLinearColor(0.03f, 0.08f, 0.16f, 1.0f))
												.Padding(FMargin(14.0f, 6.0f))
												[
													SNew(STextBlock)
													.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18.0f))
													.ColorAndOpacity(FLinearColor(0.25f, 1.0f, 0.6f, 1.0f))
													.Text_Lambda([this]()
													{
														return FText::FromString(ActiveGitHubUserCode);
													})
												]
											]
											+ SHorizontalBox::Slot()
											.AutoWidth()
											.VAlign(VAlign_Center)
											.Padding(10.0f, 0.0f, 6.0f, 0.0f)
											[
												SNew(SButton)
												.ButtonStyle(FAppStyle::Get(), "PrimaryButton")
												.ContentPadding(FMargin(10.0f, 6.0f))
												.Text(FText::FromString(TEXT("Copy Code")))
												.ToolTipText(FText::FromString(TEXT("Copy this 8-digit code to clipboard")))
												.OnClicked(this, &SCppSettingsDialog::OnCopyUserCodeClicked)
											]
											+ SHorizontalBox::Slot()
											.AutoWidth()
											.VAlign(VAlign_Center)
											[
												SNew(SButton)
												.ContentPadding(FMargin(10.0f, 6.0f))
												.Text(FText::FromString(TEXT("Open Browser")))
												.ToolTipText(FText::FromString(TEXT("Open github.com/login/device in your default browser")))
												.OnClicked_Lambda([this]()
												{
													const FString TargetUrl = ActiveGitHubVerificationUri.IsEmpty() ? FString(TEXT("https://github.com/login/device")) : ActiveGitHubVerificationUri;
													FPlatformProcess::LaunchURL(*TargetUrl, nullptr, nullptr);
													return FReply::Handled();
												})
											]
										]
										// Helpful instruction note
										+ SVerticalBox::Slot()
										.AutoHeight()
										[
											SNew(STextBlock)
											.Font(FCoreStyle::GetDefaultFontStyle("Italic", 8.5f))
											.ColorAndOpacity(FLinearColor(0.8f, 0.85f, 0.95f, 0.9f))
											.Text(FText::FromString(TEXT("2. The code is already copied to clipboard! Switch to your browser window (may be minimized or behind Unreal), paste code & authorize.")))
										]
									]
								]

								// Actions row
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(0.0f, 4.0f, 0.0f, 0.0f)
								[
									SNew(SHorizontalBox)

									// Sign In Button
									+ SHorizontalBox::Slot()
									.AutoWidth()
									.Padding(0.0f, 0.0f, 6.0f, 0.0f)
									[
										SAssignNew(GitHubSignInButton, SButton)
										.ButtonStyle(FAppStyle::Get(), "PrimaryButton")
										.Text_Lambda([this]()
										{
											if (FCppAiAssistant::Get().IsGitHubAuthenticated())
											{
												return FText::FromString(TEXT("Re-authenticate"));
											}
											if (!ActiveGitHubUserCode.IsEmpty())
											{
												return FText::FromString(TEXT("Restart Device Flow"));
											}
											if (bIsDeviceAuthInProgress)
											{
												return FText::FromString(TEXT("Cancel Authorization"));
											}
											return FText::FromString(TEXT("Sign in with GitHub (Device Flow)"));
										})
										.ToolTipText(FText::FromString(TEXT("Opens github.com/login/device with a verification code, exactly like in VS Code Copilot")))
										.OnClicked(this, &SCppSettingsDialog::OnSignInWithGitHubClicked)
									]

									// Sign Out Button
									+ SHorizontalBox::Slot()
									.AutoWidth()
									[
										SNew(SButton)
										.Visibility_Lambda([this]()
										{
											return FCppAiAssistant::Get().IsGitHubAuthenticated() ? EVisibility::Visible : EVisibility::Collapsed;
										})
										.Text(FText::FromString(TEXT("Sign Out")))
										.ToolTipText(FText::FromString(TEXT("Disconnect stored GitHub Copilot credentials")))
										.OnClicked(this, &SCppSettingsDialog::OnSignOutOfGitHubClicked)
									]
								]
							]
						]

						// Endpoint URL
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 4.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(0.35f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("API Endpoint:")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
							]
							+ SHorizontalBox::Slot()
							.FillWidth(0.65f)
							[
								SNew(SEditableTextBox)
								.Text_Lambda([this]() { return FText::FromString(TempAiEndpoint); })
								.OnTextChanged_Lambda([this](const FText& Text) { TempAiEndpoint = Text.ToString(); })
							]
						]

						// Model Name
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 4.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(0.35f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Model Name:")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
							]
							+ SHorizontalBox::Slot()
							.FillWidth(0.65f)
							[
								SNew(SEditableTextBox)
								.Text_Lambda([this]() { return FText::FromString(TempAiModel); })
								.OnTextChanged_Lambda([this](const FText& Text) { TempAiModel = Text.ToString(); })
							]
						]

						// API Key
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 4.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(0.35f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("API Key:")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
							]
							+ SHorizontalBox::Slot()
							.FillWidth(0.65f)
							[
								SNew(SEditableTextBox)
								.Text_Lambda([this]() { return FText::FromString(TempAiApiKey); })
								.HintText(FText::FromString(TEXT("Not required for local Ollama / LM Studio")))
								.IsPassword(true)
								.OnTextChanged_Lambda([this](const FText& Text) { TempAiApiKey = Text.ToString(); })
							]
						]

						// Test Connection Button & Status
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 6.0f, 0.0f, 0.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SButton)
								.Text(FText::FromString(TEXT("Test Connection")))
								.OnClicked(this, &SCppSettingsDialog::OnTestAiConnectionClicked)
							]
							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.VAlign(VAlign_Center)
							.Padding(10.0f, 0.0f)
							[
								SAssignNew(AiTestStatusTextBlock, STextBlock)
								.Text(FText::GetEmpty())
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.5f))
							]
						]
					]
				]
			]

			// -----------------------------------------------------------------
			// 3. Bottom Action Buttons
			// -----------------------------------------------------------------
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)

				// Reset to Defaults
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Reset to Defaults")))
					.ToolTipText(FText::FromString(TEXT("Restore default VS Code Dark+ theme and standard settings")))
					.OnClicked(this, &SCppSettingsDialog::OnResetClicked)
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SSpacer)
				]

				// Cancel
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(6.0f, 0.0f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Cancel")))
					.OnClicked(this, &SCppSettingsDialog::OnCancelClicked)
				]

				// Apply & Save
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.ButtonColorAndOpacity(FLinearColor(0.12f, 0.52f, 0.95f, 1.0f))
					.ContentPadding(FMargin(12.0f, 4.0f))
					.Text(FText::FromString(TEXT("Apply & Save")))
					.OnClicked(this, &SCppSettingsDialog::OnApplyClicked)
				]
			]
		]
	];
}

void SCppSettingsDialog::UpdatePreview()
{
	FCppEditorSettings TempSettings;
	TempSettings.Theme = TempTheme;
	TempSettings.FontFamily = TempFontFamily;
	TempSettings.CustomFontPath = TempCustomFontPath;
	TempSettings.FontSize = TempFontSize;

	if (PreviewMarshaller.IsValid())
	{
		PreviewMarshaller->SetSyntaxStyle(TempSettings.GetSyntaxStyle());
	}
	if (PreviewTextBox.IsValid())
	{
		PreviewTextBox->SetText(FText::GetEmpty());
		PreviewTextBox->SetText(FText::FromString(SettingsSampleCode));
	}
}

FReply SCppSettingsDialog::OnBrowseFontClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		TArray<FString> OutFiles;
		const FString Title = TEXT("Select Monospace Font (*.ttf; *.otf)");
		const FString FileTypes = TEXT("Font Files (*.ttf;*.otf)|*.ttf;*.otf|All Files (*.*)|*.*");

		if (DesktopPlatform->OpenFileDialog(
			nullptr,
			Title,
			TEXT("C:/Windows/Fonts"),
			TEXT(""),
			FileTypes,
			EFileDialogFlags::None,
			OutFiles) && OutFiles.Num() > 0)
		{
			TempCustomFontPath = OutFiles[0];
			UpdatePreview();
		}
	}
	return FReply::Handled();
}

FReply SCppSettingsDialog::OnResetClicked()
{
	FCppEditorSettings::Get().ResetToDefaults();
	FCppEditorSettings::Get().Save();

	if (TSharedPtr<SWindow> Window = ParentWindow.Pin())
	{
		Window->RequestDestroyWindow();
	}
	return FReply::Handled();
}

FReply SCppSettingsDialog::OnCancelClicked()
{
	if (TSharedPtr<SWindow> Window = ParentWindow.Pin())
	{
		Window->RequestDestroyWindow();
	}
	return FReply::Handled();
}

void SCppSettingsDialog::UpdateAiProviderDefaults()
{
	TempAiEndpoint = FCppEditorSettings::GetDefaultEndpointForProvider(TempAiProvider);
	TempAiModel = FCppEditorSettings::GetDefaultModelForProvider(TempAiProvider);
	if (TempAiProvider == EAiProvider::LocalOllama || TempAiProvider == EAiProvider::LMStudio)
	{
		TempAiApiKey.Empty();
	}
	else if (TempAiProvider == EAiProvider::GitHubCopilot)
	{
		const FCppEditorSettings& Settings = FCppEditorSettings::Get();
		TempAiApiKey = Settings.GitHubAccessToken;
	}
}

void SCppSettingsDialog::UpdateGitHubAuthCard()
{
	// Status text and button labels update via Lambdas
}

FReply SCppSettingsDialog::OnSignInWithGitHubClicked()
{
	if (bIsDeviceAuthInProgress && ActiveGitHubUserCode.IsEmpty())
	{
		FCppAiAssistant::Get().CancelGitHubAuth();
		bIsDeviceAuthInProgress = false;
		ActiveGitHubUserCode.Empty();
		ActiveGitHubVerificationUri.Empty();
		GitHubAuthStatusMessage = TEXT("Authorization cancelled.");
		return FReply::Handled();
	}

	bIsDeviceAuthInProgress = true;
	ActiveGitHubUserCode.Empty();
	ActiveGitHubVerificationUri.Empty();
	GitHubAuthStatusMessage = TEXT("Contacting GitHub for device authorization code...");

	TWeakPtr<SCppSettingsDialog> WeakThis = SharedThis(this);
	FCppAiAssistant::Get().StartGitHubDeviceFlow(
		FOnGitHubDeviceCodeReceived::CreateLambda([WeakThis](const FString& UserCode, const FString& Uri)
		{
			if (TSharedPtr<SCppSettingsDialog> Pinned = WeakThis.Pin())
			{
				Pinned->bIsDeviceAuthInProgress = true;
				Pinned->ActiveGitHubUserCode = UserCode;
				Pinned->ActiveGitHubVerificationUri = Uri;
				Pinned->GitHubAuthStatusMessage = FString::Printf(TEXT("Code received: %s — waiting for browser confirmation..."), *UserCode);
			}

			// Show Notification Toast with link and code
			FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("GitHub Copilot Code: %s (Copied to Clipboard)"), *UserCode)));
			Info.ExpireDuration = 15.0f;
			Info.bFireAndForget = true;
			const FString LaunchUri = Uri;
			Info.Hyperlink = FSimpleDelegate::CreateLambda([LaunchUri]()
			{
				FPlatformProcess::LaunchURL(*LaunchUri, nullptr, nullptr);
			});
			Info.HyperlinkText = FText::FromString(TEXT("Open Browser (github.com/login/device)"));
			FSlateNotificationManager::Get().AddNotification(Info);
		}),
		FOnGitHubAuthComplete::CreateLambda([WeakThis](bool bSuccess, const FString& MessageOrUser)
		{
			if (TSharedPtr<SCppSettingsDialog> Pinned = WeakThis.Pin())
			{
				Pinned->bIsDeviceAuthInProgress = false;
				Pinned->ActiveGitHubUserCode.Empty();
				if (bSuccess)
				{
					const FCppEditorSettings& Settings = FCppEditorSettings::Get();
					Pinned->TempAiApiKey = Settings.GitHubAccessToken;
					Pinned->TempAiEndpoint = Settings.AiEndpoint;
					Pinned->TempAiModel = Settings.AiModel;
					Pinned->GitHubAuthStatusMessage = FString::Printf(TEXT("Signed in as %s! Copilot is active."), *MessageOrUser);
				}
				else
				{
					Pinned->GitHubAuthStatusMessage = FString::Printf(TEXT("GitHub sign in failed: %s"), *MessageOrUser);
				}
			}

			FNotificationInfo Info(FText::FromString(bSuccess 
				? FString::Printf(TEXT("GitHub Copilot Connected: %s"), *MessageOrUser)
				: FString::Printf(TEXT("GitHub Copilot Failed: %s"), *MessageOrUser)));
			Info.ExpireDuration = 6.0f;
			Info.bFireAndForget = true;
			FSlateNotificationManager::Get().AddNotification(Info);
		})
	);

	return FReply::Handled();
}

FReply SCppSettingsDialog::OnCopyUserCodeClicked()
{
	if (!ActiveGitHubUserCode.IsEmpty())
	{
		FPlatformApplicationMisc::ClipboardCopy(*ActiveGitHubUserCode);

		FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("Copied '%s' to clipboard!"), *ActiveGitHubUserCode)));
		Info.ExpireDuration = 3.0f;
		Info.bFireAndForget = true;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
	if (!ActiveGitHubVerificationUri.IsEmpty())
	{
		FPlatformProcess::LaunchURL(*ActiveGitHubVerificationUri, nullptr, nullptr);
	}
	return FReply::Handled();
}

FReply SCppSettingsDialog::OnSignOutOfGitHubClicked()
{
	FCppAiAssistant::Get().SignOutOfGitHub();
	bIsDeviceAuthInProgress = false;
	ActiveGitHubUserCode.Empty();
	ActiveGitHubVerificationUri.Empty();
	GitHubAuthStatusMessage = TEXT("Signed out of GitHub Copilot.");
	TempAiApiKey.Empty();

	FNotificationInfo Info(FText::FromString(TEXT("Signed out of GitHub Copilot.")));
	Info.ExpireDuration = 4.0f;
	Info.bFireAndForget = true;
	FSlateNotificationManager::Get().AddNotification(Info);

	return FReply::Handled();
}

FReply SCppSettingsDialog::OnTestAiConnectionClicked()
{
	if (AiTestStatusTextBlock.IsValid())
	{
		AiTestStatusTextBlock->SetText(FText::FromString(TEXT("Testing connection...")));
		AiTestStatusTextBlock->SetColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f));
	}

	TWeakPtr<SCppSettingsDialog> WeakThis = SharedThis(this);
	FCppAiAssistant::Get().TestConnection(
		TempAiEndpoint,
		TempAiModel,
		TempAiApiKey,
		FOnAiTestResult::CreateLambda([WeakThis](bool bSuccess, const FString& Message)
		{
			if (TSharedPtr<SCppSettingsDialog> Pinned = WeakThis.Pin())
			{
				if (Pinned->AiTestStatusTextBlock.IsValid())
				{
					Pinned->AiTestStatusTextBlock->SetText(FText::FromString(Message));
					Pinned->AiTestStatusTextBlock->SetColorAndOpacity(bSuccess ? FLinearColor(0.2f, 0.9f, 0.4f, 1.0f) : FLinearColor(1.0f, 0.35f, 0.35f, 1.0f));
				}
			}
		})
	);

	return FReply::Handled();
}

FReply SCppSettingsDialog::OnApplyClicked()
{
	FCppEditorSettings& Settings = FCppEditorSettings::Get();
	Settings.Theme = TempTheme;
	Settings.FontFamily = TempFontFamily;
	Settings.CustomFontPath = TempCustomFontPath;
	Settings.FontSize = TempFontSize;
	Settings.TabSize = TempTabSize;
	Settings.bAutoIndent = bTempAutoIndent;
	Settings.bAutoCloseBrackets = bTempAutoCloseBrackets;
	Settings.bShowLineNumbers = bTempShowLineNumbers;
	Settings.bEnableIntelliSense = bTempEnableIntelliSense;
	Settings.bWordWrap = bTempWordWrap;
	Settings.bHighlightActiveLine = bTempHighlightActiveLine;
	Settings.bAutoSaveOnLiveCoding = bTempAutoSaveOnLiveCoding;
	Settings.bAutoReloadSlatePreview = bTempAutoReloadSlatePreview;
	Settings.bEnableAiInlineCompletion = bTempEnableAiInlineCompletion;
	Settings.AiProvider = TempAiProvider;
	Settings.AiEndpoint = TempAiEndpoint;
	Settings.AiModel = TempAiModel;
	Settings.AiApiKey = TempAiApiKey;
	if (TempAiProvider == EAiProvider::GitHubCopilot && !TempAiApiKey.IsEmpty())
	{
		Settings.GitHubAccessToken = TempAiApiKey;
	}
	Settings.AiGhostTextDelayMs = TempAiGhostTextDelayMs;

	Settings.Save();

	if (TSharedPtr<SWindow> Window = ParentWindow.Pin())
	{
		Window->RequestDestroyWindow();
	}
	return FReply::Handled();
}

void SCppSettingsDialog::OpenModal(TSharedPtr<SWidget> ParentWidget)
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(FText::FromString(TEXT("C++ Studio Settings & Preferences")))
		.ClientSize(FVector2D(680.0f, 720.0f))
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.SizingRule(ESizingRule::FixedSize);

	Window->SetContent(
		SNew(SCppSettingsDialog)
		.ParentWindow(Window)
	);

	TSharedPtr<SWindow> ParentTopLevelWindow;
	if (ParentWidget.IsValid())
	{
		ParentTopLevelWindow = FSlateApplication::Get().FindWidgetWindow(ParentWidget.ToSharedRef());
	}
	if (!ParentTopLevelWindow.IsValid())
	{
		ParentTopLevelWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
	}

	if (ParentTopLevelWindow.IsValid())
	{
		FSlateApplication::Get().AddWindowAsNativeChild(Window, ParentTopLevelWindow.ToSharedRef());
	}
	else
	{
		FSlateApplication::Get().AddWindow(Window);
	}

	Window->BringToFront();
}
