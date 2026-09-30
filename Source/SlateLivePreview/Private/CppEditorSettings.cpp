// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#include "CppEditorSettings.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Styling/CoreStyle.h"
#include "Fonts/CompositeFont.h"

static const TCHAR* ConfigSection = TEXT("CppStudioSettings");

FCppEditorSettings& FCppEditorSettings::Get()
{
	static FCppEditorSettings Instance;
	return Instance;
}

FCppEditorSettings::FCppEditorSettings()
{
	Load();
}

void FCppEditorSettings::ResetToDefaults()
{
	Theme = ECppEditorTheme::VSCodeDarkPlus;
	FontFamily = ECppEditorFont::Consolas;
	CustomFontPath.Empty();
	FontSize = 11;
	TabSize = ECppTabSize::FourSpaces;
	bAutoIndent = true;
	bAutoCloseBrackets = true;
	bShowLineNumbers = true;
	bEnableIntelliSense = true;
	bWordWrap = false;
	bHighlightActiveLine = true;
	bAutoSaveOnLiveCoding = true;
	bAutoReloadSlatePreview = true;

	bEnableAiInlineCompletion = true;
	AiProvider = EAiProvider::LocalOllama;
	AiEndpoint = TEXT("http://localhost:11434/v1");
	AiModel = TEXT("deepseek-coder");
	AiApiKey.Empty();
	AiGhostTextDelayMs = 400;
	AiMaxTokens = 128;
	AiTemperature = 0.2f;
}

void FCppEditorSettings::Load()
{
	if (!GConfig) return;

	int32 ThemeInt = (int32)Theme;
	GConfig->GetInt(ConfigSection, TEXT("Theme"), ThemeInt, GEditorPerProjectIni);
	if (ThemeInt >= 0 && ThemeInt < (int32)ECppEditorTheme::Count)
	{
		Theme = (ECppEditorTheme)ThemeInt;
	}

	int32 FontInt = (int32)FontFamily;
	GConfig->GetInt(ConfigSection, TEXT("FontFamily"), FontInt, GEditorPerProjectIni);
	if (FontInt >= 0 && FontInt < (int32)ECppEditorFont::Count)
	{
		FontFamily = (ECppEditorFont)FontInt;
	}

	GConfig->GetString(ConfigSection, TEXT("CustomFontPath"), CustomFontPath, GEditorPerProjectIni);
	GConfig->GetInt(ConfigSection, TEXT("FontSize"), FontSize, GEditorPerProjectIni);
	if (FontSize < 8 || FontSize > 32)
	{
		FontSize = 11;
	}

	int32 TabSizeInt = (int32)TabSize;
	GConfig->GetInt(ConfigSection, TEXT("TabSize"), TabSizeInt, GEditorPerProjectIni);
	TabSize = (ECppTabSize)TabSizeInt;

	GConfig->GetBool(ConfigSection, TEXT("AutoIndent"), bAutoIndent, GEditorPerProjectIni);
	GConfig->GetBool(ConfigSection, TEXT("AutoCloseBrackets"), bAutoCloseBrackets, GEditorPerProjectIni);
	GConfig->GetBool(ConfigSection, TEXT("ShowLineNumbers"), bShowLineNumbers, GEditorPerProjectIni);
	GConfig->GetBool(ConfigSection, TEXT("EnableIntelliSense"), bEnableIntelliSense, GEditorPerProjectIni);
	GConfig->GetBool(ConfigSection, TEXT("WordWrap"), bWordWrap, GEditorPerProjectIni);
	GConfig->GetBool(ConfigSection, TEXT("HighlightActiveLine"), bHighlightActiveLine, GEditorPerProjectIni);
	GConfig->GetBool(ConfigSection, TEXT("AutoSaveOnLiveCoding"), bAutoSaveOnLiveCoding, GEditorPerProjectIni);
	GConfig->GetBool(ConfigSection, TEXT("AutoReloadSlatePreview"), bAutoReloadSlatePreview, GEditorPerProjectIni);

	// AI Settings
	GConfig->GetBool(ConfigSection, TEXT("EnableAiInlineCompletion"), bEnableAiInlineCompletion, GEditorPerProjectIni);
	int32 AiProvInt = (int32)AiProvider;
	GConfig->GetInt(ConfigSection, TEXT("AiProvider"), AiProvInt, GEditorPerProjectIni);
	if (AiProvInt >= 0 && AiProvInt < (int32)EAiProvider::Count)
	{
		AiProvider = (EAiProvider)AiProvInt;
	}
	GConfig->GetString(ConfigSection, TEXT("AiEndpoint"), AiEndpoint, GEditorPerProjectIni);
	GConfig->GetString(ConfigSection, TEXT("AiModel"), AiModel, GEditorPerProjectIni);
	GConfig->GetString(ConfigSection, TEXT("AiApiKey"), AiApiKey, GEditorPerProjectIni);
	GConfig->GetInt(ConfigSection, TEXT("AiGhostTextDelayMs"), AiGhostTextDelayMs, GEditorPerProjectIni);
	GConfig->GetInt(ConfigSection, TEXT("AiMaxTokens"), AiMaxTokens, GEditorPerProjectIni);
	GConfig->GetFloat(ConfigSection, TEXT("AiTemperature"), AiTemperature, GEditorPerProjectIni);
}

void FCppEditorSettings::Save()
{
	if (!GConfig) return;

	GConfig->SetInt(ConfigSection, TEXT("Theme"), (int32)Theme, GEditorPerProjectIni);
	GConfig->SetInt(ConfigSection, TEXT("FontFamily"), (int32)FontFamily, GEditorPerProjectIni);
	GConfig->SetString(ConfigSection, TEXT("CustomFontPath"), *CustomFontPath, GEditorPerProjectIni);
	GConfig->SetInt(ConfigSection, TEXT("FontSize"), FontSize, GEditorPerProjectIni);
	GConfig->SetInt(ConfigSection, TEXT("TabSize"), (int32)TabSize, GEditorPerProjectIni);
	GConfig->SetBool(ConfigSection, TEXT("AutoIndent"), bAutoIndent, GEditorPerProjectIni);
	GConfig->SetBool(ConfigSection, TEXT("AutoCloseBrackets"), bAutoCloseBrackets, GEditorPerProjectIni);
	GConfig->SetBool(ConfigSection, TEXT("ShowLineNumbers"), bShowLineNumbers, GEditorPerProjectIni);
	GConfig->SetBool(ConfigSection, TEXT("EnableIntelliSense"), bEnableIntelliSense, GEditorPerProjectIni);
	GConfig->SetBool(ConfigSection, TEXT("WordWrap"), bWordWrap, GEditorPerProjectIni);
	GConfig->SetBool(ConfigSection, TEXT("HighlightActiveLine"), bHighlightActiveLine, GEditorPerProjectIni);
	GConfig->SetBool(ConfigSection, TEXT("AutoSaveOnLiveCoding"), bAutoSaveOnLiveCoding, GEditorPerProjectIni);
	GConfig->SetBool(ConfigSection, TEXT("AutoReloadSlatePreview"), bAutoReloadSlatePreview, GEditorPerProjectIni);

	// AI Settings
	GConfig->SetBool(ConfigSection, TEXT("EnableAiInlineCompletion"), bEnableAiInlineCompletion, GEditorPerProjectIni);
	GConfig->SetInt(ConfigSection, TEXT("AiProvider"), (int32)AiProvider, GEditorPerProjectIni);
	GConfig->SetString(ConfigSection, TEXT("AiEndpoint"), *AiEndpoint, GEditorPerProjectIni);
	GConfig->SetString(ConfigSection, TEXT("AiModel"), *AiModel, GEditorPerProjectIni);
	GConfig->SetString(ConfigSection, TEXT("AiApiKey"), *AiApiKey, GEditorPerProjectIni);
	GConfig->SetInt(ConfigSection, TEXT("AiGhostTextDelayMs"), AiGhostTextDelayMs, GEditorPerProjectIni);
	GConfig->SetInt(ConfigSection, TEXT("AiMaxTokens"), AiMaxTokens, GEditorPerProjectIni);
	GConfig->SetFloat(ConfigSection, TEXT("AiTemperature"), AiTemperature, GEditorPerProjectIni);

	GConfig->Flush(false, GEditorPerProjectIni);

	OnSettingsChanged.Broadcast();
}

FSlateFontInfo FCppEditorSettings::GetFont(float SizeOverride) const
{
	const float Size = (SizeOverride > 0.0f) ? SizeOverride : (float)FontSize;

	switch (FontFamily)
	{
	case ECppEditorFont::Consolas:
	{
		static const FString Path = TEXT("C:/Windows/Fonts/consola.ttf");
		static TSharedPtr<const FCompositeFont> CompFont;
		if (!CompFont.IsValid() && IFileManager::Get().FileExists(*Path))
		{
			CompFont = MakeShareable(new FStandaloneCompositeFont(TEXT("Consolas"), Path, EFontHinting::Default, EFontLoadingPolicy::LazyLoad));
		}
		if (CompFont.IsValid()) return FSlateFontInfo(CompFont, Size);
		break;
	}
	case ECppEditorFont::CascadiaCode:
	{
		static const FString Path = TEXT("C:/Windows/Fonts/CascadiaCode.ttf");
		static const FString FallbackPath = TEXT("C:/Windows/Fonts/CascadiaMono.ttf");
		static TSharedPtr<const FCompositeFont> CompFont;
		if (!CompFont.IsValid())
		{
			const FString& ChosenPath = IFileManager::Get().FileExists(*Path) ? Path : FallbackPath;
			if (IFileManager::Get().FileExists(*ChosenPath))
			{
				CompFont = MakeShareable(new FStandaloneCompositeFont(TEXT("CascadiaCode"), ChosenPath, EFontHinting::Default, EFontLoadingPolicy::LazyLoad));
			}
		}
		if (CompFont.IsValid()) return FSlateFontInfo(CompFont, Size);
		break;
	}
	case ECppEditorFont::CourierNew:
	{
		static const FString Path = TEXT("C:/Windows/Fonts/cour.ttf");
		static TSharedPtr<const FCompositeFont> CompFont;
		if (!CompFont.IsValid() && IFileManager::Get().FileExists(*Path))
		{
			CompFont = MakeShareable(new FStandaloneCompositeFont(TEXT("CourierNew"), Path, EFontHinting::Default, EFontLoadingPolicy::LazyLoad));
		}
		if (CompFont.IsValid()) return FSlateFontInfo(CompFont, Size);
		break;
	}
	case ECppEditorFont::CustomFont:
	{
		if (!CustomFontPath.IsEmpty() && IFileManager::Get().FileExists(*CustomFontPath))
		{
			TSharedPtr<const FCompositeFont> CompFont = MakeShareable(new FStandaloneCompositeFont(TEXT("CustomEditorFont"), CustomFontPath, EFontHinting::Default, EFontLoadingPolicy::LazyLoad));
			return FSlateFontInfo(CompFont, Size);
		}
		break;
	}
	default:
		break;
	}

	return FCoreStyle::GetDefaultFontStyle("Mono", Size);
}

FSlateFontInfo FCppEditorSettings::GetFontBold(float SizeOverride) const
{
	const float Size = (SizeOverride > 0.0f) ? SizeOverride : (float)FontSize;

	switch (FontFamily)
	{
	case ECppEditorFont::Consolas:
	{
		static const FString Path = TEXT("C:/Windows/Fonts/consolab.ttf");
		static TSharedPtr<const FCompositeFont> CompFont;
		if (!CompFont.IsValid() && IFileManager::Get().FileExists(*Path))
		{
			CompFont = MakeShareable(new FStandaloneCompositeFont(TEXT("ConsolasBold"), Path, EFontHinting::Default, EFontLoadingPolicy::LazyLoad));
		}
		if (CompFont.IsValid()) return FSlateFontInfo(CompFont, Size);
		break;
	}
	case ECppEditorFont::CascadiaCode:
	{
		return GetFont(Size);
	}
	case ECppEditorFont::CourierNew:
	{
		static const FString Path = TEXT("C:/Windows/Fonts/courbd.ttf");
		static TSharedPtr<const FCompositeFont> CompFont;
		if (!CompFont.IsValid() && IFileManager::Get().FileExists(*Path))
		{
			CompFont = MakeShareable(new FStandaloneCompositeFont(TEXT("CourierNewBold"), Path, EFontHinting::Default, EFontLoadingPolicy::LazyLoad));
		}
		if (CompFont.IsValid()) return FSlateFontInfo(CompFont, Size);
		break;
	}
	case ECppEditorFont::CustomFont:
	{
		return GetFont(Size);
	}
	default:
		break;
	}

	return FCoreStyle::GetDefaultFontStyle("Bold", Size);
}

FCppSyntaxHighlighterMarshaller::FSyntaxTextStyle FCppEditorSettings::GetSyntaxStyle() const
{
	FCppSyntaxHighlighterMarshaller::FSyntaxTextStyle Style;
	const FSlateFontInfo NormalFont = GetFont();
	const FSlateFontInfo BoldFont = GetFontBold();

	auto MakeStyle = [](const FSlateFontInfo& InFont, const FLinearColor& InColor) -> FTextBlockStyle
	{
		return FTextBlockStyle().SetFont(InFont).SetColorAndOpacity(InColor);
	};

	switch (Theme)
	{
	case ECppEditorTheme::VSCodeDarkPlus:
	default:
		Style.NormalTextStyle = MakeStyle(NormalFont, FLinearColor(0.831f, 0.831f, 0.831f, 1.0f));
		Style.KeywordTextStyle = MakeStyle(BoldFont, FLinearColor(0.337f, 0.612f, 0.839f, 1.0f));       // #569CD6
		Style.FlowControlTextStyle = MakeStyle(BoldFont, FLinearColor(0.773f, 0.525f, 0.753f, 1.0f));   // #C586C0
		Style.TypeTextStyle = MakeStyle(BoldFont, FLinearColor(0.306f, 0.788f, 0.690f, 1.0f));          // #4EC9B0
		Style.FunctionTextStyle = MakeStyle(BoldFont, FLinearColor(0.863f, 0.863f, 0.667f, 1.0f));      // #DCDCAA
		Style.VariableTextStyle = MakeStyle(NormalFont, FLinearColor(0.612f, 0.863f, 0.996f, 1.0f));    // #9CDCFE
		Style.PrimitiveTypeTextStyle = MakeStyle(BoldFont, FLinearColor(0.337f, 0.612f, 0.839f, 1.0f)); // #569CD6
		Style.MacroTextStyle = MakeStyle(BoldFont, FLinearColor(0.773f, 0.525f, 0.753f, 1.0f));          // #C586C0
		Style.PreProcessorTextStyle = MakeStyle(BoldFont, FLinearColor(0.773f, 0.525f, 0.753f, 1.0f));   // #C586C0
		Style.StringTextStyle = MakeStyle(NormalFont, FLinearColor(0.808f, 0.569f, 0.471f, 1.0f));        // #CE9178
		Style.CharacterTextStyle = MakeStyle(NormalFont, FLinearColor(0.843f, 0.729f, 0.490f, 1.0f));     // #D7BA7D
		Style.CommentTextStyle = MakeStyle(NormalFont, FLinearColor(0.416f, 0.600f, 0.333f, 1.0f));       // #6A9955
		Style.NumberTextStyle = MakeStyle(NormalFont, FLinearColor(0.710f, 0.808f, 0.659f, 1.0f));        // #B5CEA8
		Style.OperatorTextStyle = MakeStyle(NormalFont, FLinearColor(0.831f, 0.831f, 0.831f, 1.0f));
		break;

	case ECppEditorTheme::VisualStudioDark:
		Style.NormalTextStyle = MakeStyle(NormalFont, FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
		Style.KeywordTextStyle = MakeStyle(BoldFont, FLinearColor(0.337f, 0.612f, 0.839f, 1.0f));
		Style.FlowControlTextStyle = MakeStyle(BoldFont, FLinearColor(0.847f, 0.627f, 0.875f, 1.0f));
		Style.TypeTextStyle = MakeStyle(BoldFont, FLinearColor(0.306f, 0.788f, 0.690f, 1.0f));
		Style.FunctionTextStyle = MakeStyle(BoldFont, FLinearColor(0.863f, 0.863f, 0.667f, 1.0f));
		Style.VariableTextStyle = MakeStyle(NormalFont, FLinearColor(0.9f, 0.9f, 0.9f, 1.0f));
		Style.PrimitiveTypeTextStyle = MakeStyle(BoldFont, FLinearColor(0.337f, 0.612f, 0.839f, 1.0f));
		Style.MacroTextStyle = MakeStyle(BoldFont, FLinearColor(0.741f, 0.388f, 0.773f, 1.0f));
		Style.PreProcessorTextStyle = MakeStyle(BoldFont, FLinearColor(0.608f, 0.608f, 0.608f, 1.0f));
		Style.StringTextStyle = MakeStyle(NormalFont, FLinearColor(0.839f, 0.616f, 0.522f, 1.0f));
		Style.CharacterTextStyle = MakeStyle(NormalFont, FLinearColor(0.839f, 0.616f, 0.522f, 1.0f));
		Style.CommentTextStyle = MakeStyle(NormalFont, FLinearColor(0.341f, 0.651f, 0.290f, 1.0f));
		Style.NumberTextStyle = MakeStyle(NormalFont, FLinearColor(0.710f, 0.808f, 0.659f, 1.0f));
		Style.OperatorTextStyle = MakeStyle(NormalFont, FLinearColor(0.706f, 0.706f, 0.706f, 1.0f));
		break;

	case ECppEditorTheme::MonokaiPro:
		Style.NormalTextStyle = MakeStyle(NormalFont, FLinearColor(0.988f, 0.988f, 0.980f, 1.0f));       // #FCFCFA
		Style.KeywordTextStyle = MakeStyle(BoldFont, FLinearColor(1.0f, 0.380f, 0.533f, 1.0f));          // #FF6188 (Pink)
		Style.FlowControlTextStyle = MakeStyle(BoldFont, FLinearColor(1.0f, 0.380f, 0.533f, 1.0f));
		Style.TypeTextStyle = MakeStyle(BoldFont, FLinearColor(0.471f, 0.863f, 0.910f, 1.0f));           // #78DCE8 (Cyan)
		Style.FunctionTextStyle = MakeStyle(BoldFont, FLinearColor(0.663f, 0.863f, 0.463f, 1.0f));       // #A9DC76 (Lime)
		Style.VariableTextStyle = MakeStyle(NormalFont, FLinearColor(0.988f, 0.988f, 0.980f, 1.0f));
		Style.PrimitiveTypeTextStyle = MakeStyle(BoldFont, FLinearColor(0.471f, 0.863f, 0.910f, 1.0f));
		Style.MacroTextStyle = MakeStyle(BoldFont, FLinearColor(0.671f, 0.616f, 0.949f, 1.0f));          // #AB9DF2 (Violet)
		Style.PreProcessorTextStyle = MakeStyle(BoldFont, FLinearColor(1.0f, 0.380f, 0.533f, 1.0f));
		Style.StringTextStyle = MakeStyle(NormalFont, FLinearColor(1.0f, 0.847f, 0.400f, 1.0f));         // #FFD866 (Yellow)
		Style.CharacterTextStyle = MakeStyle(NormalFont, FLinearColor(1.0f, 0.847f, 0.400f, 1.0f));
		Style.CommentTextStyle = MakeStyle(NormalFont, FLinearColor(0.447f, 0.439f, 0.447f, 1.0f));      // #727072 (Muted)
		Style.NumberTextStyle = MakeStyle(NormalFont, FLinearColor(0.671f, 0.616f, 0.949f, 1.0f));
		Style.OperatorTextStyle = MakeStyle(NormalFont, FLinearColor(1.0f, 0.380f, 0.533f, 1.0f));
		break;

	case ECppEditorTheme::OneDarkPro:
		Style.NormalTextStyle = MakeStyle(NormalFont, FLinearColor(0.671f, 0.698f, 0.749f, 1.0f));       // #ABB2BF
		Style.KeywordTextStyle = MakeStyle(BoldFont, FLinearColor(0.776f, 0.471f, 0.867f, 1.0f));       // #C678DD (Purple)
		Style.FlowControlTextStyle = MakeStyle(BoldFont, FLinearColor(0.776f, 0.471f, 0.867f, 1.0f));
		Style.TypeTextStyle = MakeStyle(BoldFont, FLinearColor(0.898f, 0.753f, 0.482f, 1.0f));          // #E5C07B (Yellow)
		Style.FunctionTextStyle = MakeStyle(BoldFont, FLinearColor(0.380f, 0.686f, 0.937f, 1.0f));      // #61AFEF (Blue)
		Style.VariableTextStyle = MakeStyle(NormalFont, FLinearColor(0.878f, 0.424f, 0.459f, 1.0f));    // #E06C75 (Coral)
		Style.PrimitiveTypeTextStyle = MakeStyle(BoldFont, FLinearColor(0.337f, 0.714f, 0.761f, 1.0f)); // #56B6C2 (Cyan)
		Style.MacroTextStyle = MakeStyle(BoldFont, FLinearColor(0.878f, 0.424f, 0.459f, 1.0f));
		Style.PreProcessorTextStyle = MakeStyle(BoldFont, FLinearColor(0.776f, 0.471f, 0.867f, 1.0f));
		Style.StringTextStyle = MakeStyle(NormalFont, FLinearColor(0.596f, 0.765f, 0.475f, 1.0f));        // #98C379 (Green)
		Style.CharacterTextStyle = MakeStyle(NormalFont, FLinearColor(0.596f, 0.765f, 0.475f, 1.0f));
		Style.CommentTextStyle = MakeStyle(NormalFont, FLinearColor(0.361f, 0.388f, 0.439f, 1.0f));      // #5C6370
		Style.NumberTextStyle = MakeStyle(NormalFont, FLinearColor(0.820f, 0.604f, 0.400f, 1.0f));        // #D19A66
		Style.OperatorTextStyle = MakeStyle(NormalFont, FLinearColor(0.337f, 0.714f, 0.761f, 1.0f));
		break;

	case ECppEditorTheme::CyberpunkNeon:
		Style.NormalTextStyle = MakeStyle(NormalFont, FLinearColor(0.0f, 0.941f, 1.0f, 1.0f));           // Neon Cyan #00F0FF
		Style.KeywordTextStyle = MakeStyle(BoldFont, FLinearColor(1.0f, 0.0f, 0.498f, 1.0f));            // Neon Pink #FF007F
		Style.FlowControlTextStyle = MakeStyle(BoldFont, FLinearColor(1.0f, 0.0f, 0.498f, 1.0f));
		Style.TypeTextStyle = MakeStyle(BoldFont, FLinearColor(0.224f, 1.0f, 0.078f, 1.0f));            // Lime #39FF14
		Style.FunctionTextStyle = MakeStyle(BoldFont, FLinearColor(1.0f, 0.902f, 0.0f, 1.0f));          // Yellow #FFE600
		Style.VariableTextStyle = MakeStyle(NormalFont, FLinearColor(0.0f, 0.941f, 1.0f, 1.0f));
		Style.PrimitiveTypeTextStyle = MakeStyle(BoldFont, FLinearColor(1.0f, 0.404f, 0.0f, 1.0f));     // Orange #FF6700
		Style.MacroTextStyle = MakeStyle(BoldFont, FLinearColor(0.816f, 0.0f, 1.0f, 1.0f));             // Magenta #D000FF
		Style.PreProcessorTextStyle = MakeStyle(BoldFont, FLinearColor(0.816f, 0.0f, 1.0f, 1.0f));
		Style.StringTextStyle = MakeStyle(NormalFont, FLinearColor(1.0f, 0.522f, 0.0f, 1.0f));          // Sunset #FF8500
		Style.CharacterTextStyle = MakeStyle(NormalFont, FLinearColor(1.0f, 0.522f, 0.0f, 1.0f));
		Style.CommentTextStyle = MakeStyle(NormalFont, FLinearColor(0.541f, 0.475f, 0.659f, 1.0f));      // Violet Muted
		Style.NumberTextStyle = MakeStyle(NormalFont, FLinearColor(0.224f, 1.0f, 0.078f, 1.0f));
		Style.OperatorTextStyle = MakeStyle(NormalFont, FLinearColor(0.0f, 0.941f, 1.0f, 1.0f));
		break;

	case ECppEditorTheme::SolarizedDark:
		Style.NormalTextStyle = MakeStyle(NormalFont, FLinearColor(0.514f, 0.580f, 0.588f, 1.0f));       // #839496
		Style.KeywordTextStyle = MakeStyle(BoldFont, FLinearColor(0.522f, 0.600f, 0.0f, 1.0f));          // #859900 (Green)
		Style.FlowControlTextStyle = MakeStyle(BoldFont, FLinearColor(0.522f, 0.600f, 0.0f, 1.0f));
		Style.TypeTextStyle = MakeStyle(BoldFont, FLinearColor(0.710f, 0.537f, 0.0f, 1.0f));             // #B58900 (Yellow)
		Style.FunctionTextStyle = MakeStyle(BoldFont, FLinearColor(0.149f, 0.545f, 0.824f, 1.0f));      // #268BD2 (Blue)
		Style.VariableTextStyle = MakeStyle(NormalFont, FLinearColor(0.165f, 0.631f, 0.596f, 1.0f));    // #2AA198 (Cyan)
		Style.PrimitiveTypeTextStyle = MakeStyle(BoldFont, FLinearColor(0.796f, 0.294f, 0.086f, 1.0f)); // #CB4B16 (Orange)
		Style.MacroTextStyle = MakeStyle(BoldFont, FLinearColor(0.827f, 0.212f, 0.510f, 1.0f));          // #D33682 (Magenta)
		Style.PreProcessorTextStyle = MakeStyle(BoldFont, FLinearColor(0.796f, 0.294f, 0.086f, 1.0f));
		Style.StringTextStyle = MakeStyle(NormalFont, FLinearColor(0.165f, 0.631f, 0.596f, 1.0f));
		Style.CharacterTextStyle = MakeStyle(NormalFont, FLinearColor(0.165f, 0.631f, 0.596f, 1.0f));
		Style.CommentTextStyle = MakeStyle(NormalFont, FLinearColor(0.345f, 0.431f, 0.459f, 1.0f));      // #586E75
		Style.NumberTextStyle = MakeStyle(NormalFont, FLinearColor(0.424f, 0.443f, 0.769f, 1.0f));        // #6C71C4 (Violet)
		Style.OperatorTextStyle = MakeStyle(NormalFont, FLinearColor(0.514f, 0.580f, 0.588f, 1.0f));
		break;

	case ECppEditorTheme::GitHubDark:
		Style.NormalTextStyle = MakeStyle(NormalFont, FLinearColor(0.882f, 0.894f, 0.910f, 1.0f));       // #E1E4E8
		Style.KeywordTextStyle = MakeStyle(BoldFont, FLinearColor(0.976f, 0.459f, 0.514f, 1.0f));       // #F97583 (Coral)
		Style.FlowControlTextStyle = MakeStyle(BoldFont, FLinearColor(0.976f, 0.459f, 0.514f, 1.0f));
		Style.TypeTextStyle = MakeStyle(BoldFont, FLinearColor(0.702f, 0.573f, 0.941f, 1.0f));          // #B392F0 (Purple)
		Style.FunctionTextStyle = MakeStyle(BoldFont, FLinearColor(0.475f, 0.722f, 1.0f, 1.0f));        // #79B8FF (Blue)
		Style.VariableTextStyle = MakeStyle(NormalFont, FLinearColor(0.882f, 0.894f, 0.910f, 1.0f));
		Style.PrimitiveTypeTextStyle = MakeStyle(BoldFont, FLinearColor(0.976f, 0.459f, 0.514f, 1.0f));
		Style.MacroTextStyle = MakeStyle(BoldFont, FLinearColor(0.976f, 0.459f, 0.514f, 1.0f));
		Style.PreProcessorTextStyle = MakeStyle(BoldFont, FLinearColor(0.976f, 0.459f, 0.514f, 1.0f));
		Style.StringTextStyle = MakeStyle(NormalFont, FLinearColor(0.620f, 0.796f, 1.0f, 1.0f));         // #9ECBFF (Soft Blue)
		Style.CharacterTextStyle = MakeStyle(NormalFont, FLinearColor(0.620f, 0.796f, 1.0f, 1.0f));
		Style.CommentTextStyle = MakeStyle(NormalFont, FLinearColor(0.416f, 0.451f, 0.490f, 1.0f));      // #6A737D
		Style.NumberTextStyle = MakeStyle(NormalFont, FLinearColor(0.475f, 0.722f, 1.0f, 1.0f));
		Style.OperatorTextStyle = MakeStyle(NormalFont, FLinearColor(0.976f, 0.459f, 0.514f, 1.0f));
		break;
	}

	return Style;
}

FLinearColor FCppEditorSettings::GetEditorBackgroundColor() const
{
	switch (Theme)
	{
	case ECppEditorTheme::VSCodeDarkPlus:
	default:
		return FLinearColor(0.118f, 0.118f, 0.118f, 1.0f); // #1E1E1E
	case ECppEditorTheme::VisualStudioDark:
		return FLinearColor(0.118f, 0.118f, 0.118f, 1.0f);
	case ECppEditorTheme::MonokaiPro:
		return FLinearColor(0.176f, 0.165f, 0.180f, 1.0f); // #2D2A2E
	case ECppEditorTheme::OneDarkPro:
		return FLinearColor(0.157f, 0.173f, 0.204f, 1.0f); // #282C34
	case ECppEditorTheme::CyberpunkNeon:
		return FLinearColor(0.102f, 0.063f, 0.184f, 1.0f); // #1A102F
	case ECppEditorTheme::SolarizedDark:
		return FLinearColor(0.0f, 0.169f, 0.212f, 1.0f);   // #002B36
	case ECppEditorTheme::GitHubDark:
		return FLinearColor(0.141f, 0.161f, 0.180f, 1.0f); // #24292E
	}
}

FLinearColor FCppEditorSettings::GetGutterBackgroundColor() const
{
	FLinearColor Bg = GetEditorBackgroundColor();
	return FLinearColor(Bg.R * 0.85f, Bg.G * 0.85f, Bg.B * 0.85f, 1.0f);
}

FString FCppEditorSettings::GetThemeDisplayName(ECppEditorTheme InTheme)
{
	switch (InTheme)
	{
	case ECppEditorTheme::VSCodeDarkPlus:   return TEXT("VS Code Dark+ (Default)");
	case ECppEditorTheme::VisualStudioDark: return TEXT("Visual Studio Classic Dark");
	case ECppEditorTheme::MonokaiPro:       return TEXT("Monokai Pro (Vibrant)");
	case ECppEditorTheme::OneDarkPro:       return TEXT("One Dark Pro (Modern)");
	case ECppEditorTheme::CyberpunkNeon:    return TEXT("Cyberpunk Neon (High Contrast)");
	case ECppEditorTheme::SolarizedDark:    return TEXT("Solarized Dark (Teal)");
	case ECppEditorTheme::GitHubDark:       return TEXT("GitHub Dark");
	default:                                return TEXT("Default Theme");
	}
}

FString FCppEditorSettings::GetFontDisplayName(ECppEditorFont InFont)
{
	switch (InFont)
	{
	case ECppEditorFont::Consolas:         return TEXT("Consolas (VS Classic)");
	case ECppEditorFont::CascadiaCode:      return TEXT("Cascadia Code (Modern)");
	case ECppEditorFont::CourierNew:       return TEXT("Courier New");
	case ECppEditorFont::SlateDefaultMono: return TEXT("Engine Default Monospace");
	case ECppEditorFont::CustomFont:       return TEXT("Custom Local Font (.ttf/.otf)...");
	default:                               return TEXT("Default Font");
	}
}

const TArray<int32>& FCppEditorSettings::GetAvailableFontSizes()
{
	static const TArray<int32> Sizes = { 9, 10, 11, 12, 13, 14, 16, 18, 20 };
	return Sizes;
}

FString FCppEditorSettings::GetAiProviderDisplayName(EAiProvider InProvider)
{
	switch (InProvider)
	{
	case EAiProvider::LocalOllama: return TEXT("Local Ollama (Offline, 100% Free, Private)");
	case EAiProvider::LMStudio:    return TEXT("LM Studio (Local server)");
	case EAiProvider::DeepSeek:    return TEXT("DeepSeek API (deepseek-coder)");
	case EAiProvider::OpenAI:      return TEXT("OpenAI (gpt-4o-mini / gpt-4o)");
	case EAiProvider::Custom:      return TEXT("Custom OpenAI-compatible Endpoint");
	default: return TEXT("Unknown");
	}
}

FString FCppEditorSettings::GetDefaultEndpointForProvider(EAiProvider InProvider)
{
	switch (InProvider)
	{
	case EAiProvider::LocalOllama: return TEXT("http://localhost:11434/v1");
	case EAiProvider::LMStudio:    return TEXT("http://localhost:1234/v1");
	case EAiProvider::DeepSeek:    return TEXT("https://api.deepseek.com/v1");
	case EAiProvider::OpenAI:      return TEXT("https://api.openai.com/v1");
	case EAiProvider::Custom:      return TEXT("http://localhost:8000/v1");
	default: return TEXT("http://localhost:11434/v1");
	}
}

FString FCppEditorSettings::GetDefaultModelForProvider(EAiProvider InProvider)
{
	switch (InProvider)
	{
	case EAiProvider::LocalOllama: return TEXT("deepseek-coder");
	case EAiProvider::LMStudio:    return TEXT("qwen2.5-coder-7b-instruct");
	case EAiProvider::DeepSeek:    return TEXT("deepseek-coder");
	case EAiProvider::OpenAI:      return TEXT("gpt-4o-mini");
	case EAiProvider::Custom:      return TEXT("default");
	default: return TEXT("deepseek-coder");
	}
}
