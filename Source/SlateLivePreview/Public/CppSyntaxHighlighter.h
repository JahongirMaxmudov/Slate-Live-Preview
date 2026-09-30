// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Text/SyntaxHighlighterTextLayoutMarshaller.h"
#include "Framework/Text/SyntaxTokenizer.h"
#include "Styling/SlateTypes.h"
#include "Fonts/SlateFontInfo.h"

/**
 * High-performance, zero-leak Tokenizer for C++ and Unreal Slate code
 */
class SLATELIVEPREVIEW_API FCppSyntaxTokenizer : public ISyntaxTokenizer
{
public:
	static TSharedRef<FCppSyntaxTokenizer> Create();
	virtual ~FCppSyntaxTokenizer() override = default;

	virtual void Process(TArray<FTokenizedLine>& OutTokenizedLines, const FString& Input) override;

	bool IsKeyword(const FStringView& InToken) const;
	bool IsFlowControl(const FStringView& InToken) const;
	bool IsType(const FStringView& InToken) const;
	bool IsUnrealType(const FStringView& InToken) const;
	bool IsPrimitiveType(const FStringView& InToken) const;
	bool IsMacro(const FStringView& InToken) const;
	bool IsFunctionCall(const FString& Input, int32 TokenEnd, int32 LineEnd) const;
	bool IsVariableOrParameter(const FStringView& InToken) const;

protected:
	FCppSyntaxTokenizer();

private:
	TSet<FName> Keywords;
	TSet<FName> FlowControl;
	TSet<FName> Types;
	TSet<FName> PrimitiveTypes;
	TSet<FName> Macros;
	TArray<FString> Operators;
};

/**
 * Syntax highlighter with authentic VS Code C++ Extension colors & MVS Consolas font
 */
class SLATELIVEPREVIEW_API FCppSyntaxHighlighterMarshaller : public FSyntaxHighlighterTextLayoutMarshaller
{
public:
	struct FSyntaxTextStyle
	{
		FTextBlockStyle NormalTextStyle;
		FTextBlockStyle KeywordTextStyle;
		FTextBlockStyle FlowControlTextStyle;
		FTextBlockStyle TypeTextStyle;
		FTextBlockStyle FunctionTextStyle;
		FTextBlockStyle VariableTextStyle;
		FTextBlockStyle PrimitiveTypeTextStyle;
		FTextBlockStyle MacroTextStyle;
		FTextBlockStyle PreProcessorTextStyle;
		FTextBlockStyle StringTextStyle;
		FTextBlockStyle CharacterTextStyle;
		FTextBlockStyle CommentTextStyle;
		FTextBlockStyle NumberTextStyle;
		FTextBlockStyle OperatorTextStyle;
	};

	static TSharedRef<FCppSyntaxHighlighterMarshaller> Create(const FSyntaxTextStyle& InSyntaxTextStyle);
	static TSharedRef<FCppSyntaxHighlighterMarshaller> CreateDefaultDark();
	static FSlateFontInfo GetEditorFont(float Size = 11.0f);
	static FSlateFontInfo GetEditorFontBold(float Size = 11.0f);

	virtual ~FCppSyntaxHighlighterMarshaller() override = default;

	void SetSyntaxStyle(const FSyntaxTextStyle& InSyntaxTextStyle) { SyntaxTextStyle = InSyntaxTextStyle; }

	// Return true so syntax highlighting updates live as the user edits code
	virtual bool RequiresLiveUpdate() const override { return true; }

protected:
	enum class EParseState : uint8
	{
		None,
		LookingForString,
		LookingForCharacter,
		LookingForSingleLineComment,
		LookingForMultiLineComment,
	};

	virtual void ParseTokens(const FString& SourceString, FTextLayout& TargetTextLayout, TArray<ISyntaxTokenizer::FTokenizedLine> TokenizedLines) override;
	FTextLayout::FNewLineData ProcessTokenizedLine(const ISyntaxTokenizer::FTokenizedLine& TokenizedLine, const int32& LineNumber, const FString& SourceString, EParseState& CurrentParseState);

	FCppSyntaxHighlighterMarshaller(TSharedPtr<FCppSyntaxTokenizer> InTokenizer, const FSyntaxTextStyle& InSyntaxTextStyle);

	TSharedPtr<FCppSyntaxTokenizer> CppTokenizer;
	FSyntaxTextStyle SyntaxTextStyle;
};
