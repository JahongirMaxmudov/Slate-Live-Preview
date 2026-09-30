// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateColor.h"
#include "CppSyntaxHighlighter.h"

/**
 * Curated Color Themes for C++ Studio
 */
enum class ECppEditorTheme : uint8
{
	VSCodeDarkPlus = 0,
	VisualStudioDark,
	MonokaiPro,
	OneDarkPro,
	CyberpunkNeon,
	SolarizedDark,
	GitHubDark,
	Count
};

/**
 * Built-in Monospace Font Families
 */
enum class ECppEditorFont : uint8
{
	Consolas = 0,
	CascadiaCode,
	CourierNew,
	SlateDefaultMono,
	CustomFont,
	Count
};

/**
 * Tab width options
 */
enum class ECppTabSize : uint8
{
	TwoSpaces = 2,
	FourSpaces = 4,
	EightSpaces = 8,
	TabCharacter = 0
};

DECLARE_MULTICAST_DELEGATE(FOnCppEditorSettingsChanged);

/**
 * Supported AI Providers for Copilot and Code Assistant
 */
enum class EAiProvider : uint8
{
	LocalOllama = 0,
	LMStudio,
	DeepSeek,
	OpenAI,
	Custom,
	Count
};

/**
 * Central configuration manager for C++ Studio editor appearances and behaviors.
 * Persists user preferences to Unreal Engine config files (Saved/Config).
 */
class SLATELIVEPREVIEW_API FCppEditorSettings
{
public:
	static FCppEditorSettings& Get();

	void Load();
	void Save();
	void ResetToDefaults();

	// Active Settings
	ECppEditorTheme Theme = ECppEditorTheme::VSCodeDarkPlus;
	ECppEditorFont FontFamily = ECppEditorFont::Consolas;
	FString CustomFontPath;
	int32 FontSize = 11;
	ECppTabSize TabSize = ECppTabSize::FourSpaces;
	bool bAutoIndent = true;
	bool bAutoCloseBrackets = true;
	bool bShowLineNumbers = true;
	bool bEnableIntelliSense = true;
	bool bWordWrap = false;
	bool bHighlightActiveLine = true;
	bool bAutoSaveOnLiveCoding = true;
	bool bAutoReloadSlatePreview = true;

	// AI Copilot & Assistant Settings
	bool bEnableAiInlineCompletion = true;
	EAiProvider AiProvider = EAiProvider::LocalOllama;
	FString AiEndpoint = TEXT("http://localhost:11434/v1");
	FString AiModel = TEXT("deepseek-coder");
	FString AiApiKey;
	int32 AiGhostTextDelayMs = 400;
	int32 AiMaxTokens = 128;
	float AiTemperature = 0.2f;

	// Helpers for Slate widgets
	FSlateFontInfo GetFont(float SizeOverride = -1.0f) const;
	FSlateFontInfo GetFontBold(float SizeOverride = -1.0f) const;
	FCppSyntaxHighlighterMarshaller::FSyntaxTextStyle GetSyntaxStyle() const;
	FLinearColor GetEditorBackgroundColor() const;
	FLinearColor GetGutterBackgroundColor() const;

	static FString GetThemeDisplayName(ECppEditorTheme InTheme);
	static FString GetFontDisplayName(ECppEditorFont InFont);
	static const TArray<int32>& GetAvailableFontSizes();

	static FString GetAiProviderDisplayName(EAiProvider InProvider);
	static FString GetDefaultEndpointForProvider(EAiProvider InProvider);
	static FString GetDefaultModelForProvider(EAiProvider InProvider);

	// Broadcast when any settings are saved/applied
	FOnCppEditorSettingsChanged OnSettingsChanged;

	FCppEditorSettings();
	~FCppEditorSettings() = default;
};
