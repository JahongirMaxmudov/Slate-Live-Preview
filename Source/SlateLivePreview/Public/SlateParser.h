// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SlateAst.h"

enum class ESlateTokenType : uint8
{
	EndOfFile,
	Identifier,
	Number,
	String,
	Plus,
	Dot,
	Comma,
	OpenParen,
	CloseParen,
	OpenBracket,
	CloseBracket,
	OpenBrace,
	CloseBrace,
	Colon,
	DoubleColon,
	Semicolon,
	Unknown
};

struct FSlateToken
{
	ESlateTokenType Type = ESlateTokenType::EndOfFile;
	FString Text;
	int32 Line = 1;
	int32 Column = 1;
};

class SLATELIVEPREVIEW_API FSlateCodeLexer
{
public:
	explicit FSlateCodeLexer(const FString& InSource);

	TArray<FSlateToken> Tokenize();
	const TArray<FString>& GetErrors() const { return Errors; }

private:
	FString Source;
	int32 Cursor = 0;
	int32 CurrentLine = 1;
	int32 CurrentColumn = 1;
	TArray<FString> Errors;

	bool IsAtEnd() const;
	TCHAR Peek() const;
	TCHAR PeekNext() const;
	TCHAR Advance();
	void SkipWhitespaceAndComments();
	FSlateToken ScanToken();
	FSlateToken ScanIdentifierOrKeyword();
	FSlateToken ScanNumber();
	FSlateToken ScanString(TCHAR QuoteChar);
};

class SLATELIVEPREVIEW_API FSlateCodeParser
{
public:
	explicit FSlateCodeParser(const TArray<FSlateToken>& InTokens);

	bool Parse(TSharedPtr<FSlateWidgetNode>& OutRootNode);
	const TArray<FString>& GetErrors() const { return Errors; }
	const TArray<FString>& GetWarnings() const { return Warnings; }

	/** Utility to extract SNew(...) or SAssignNew(...) from a full .cpp file */
	static FString ExtractSlateBlockFromSource(const FString& InCppContent);

	/** Parse directly from string */
	static bool ParseSource(const FString& InSourceCode,
		TSharedPtr<FSlateWidgetNode>& OutRoot,
		TArray<FString>& OutErrors,
		TArray<FString>& OutWarnings);

private:
	TArray<FSlateToken> Tokens;
	int32 Current = 0;
	TArray<FString> Errors;
	TArray<FString> Warnings;

	bool IsAtEnd() const;
	const FSlateToken& Peek() const;
	const FSlateToken& Previous() const;
	FSlateToken Advance();
	bool Check(ESlateTokenType Type) const;
	bool Match(ESlateTokenType Type);
	bool Expect(ESlateTokenType Type, const FString& ErrorMessage);

	TSharedPtr<FSlateWidgetNode> ParseWidgetInstantiation();
	bool ParsePropertyCall(FSlatePropertyNode& OutProperty);
	FSlateAstValue ParseValue();
	FSlateAstValue ParseColorConstructor();
	FSlateAstValue ParseMarginConstructor();
	FSlateAstValue ParseTextConstructor();
	bool ParseChildContent(FSlateWidgetNode& TargetWidget);
	bool ParseSlot(FSlateWidgetNode& TargetWidget);

	void SynchronizeAfterError();
};
