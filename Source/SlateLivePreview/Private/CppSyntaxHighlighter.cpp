// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#include "CppSyntaxHighlighter.h"
#include "Framework/Text/IRun.h"
#include "Framework/Text/TextLayout.h"
#include "Framework/Text/ISlateRun.h"
#include "Framework/Text/SlateTextRun.h"
#include "HAL/FileManager.h"
#include "Styling/CoreStyle.h"

#include "Fonts/CompositeFont.h"

// ----------------------------------------------------------------------------
// FCppSyntaxTokenizer
// ----------------------------------------------------------------------------
TSharedRef<FCppSyntaxTokenizer> FCppSyntaxTokenizer::Create()
{
	return MakeShareable(new FCppSyntaxTokenizer());
}

FCppSyntaxTokenizer::FCppSyntaxTokenizer()
{
	const TCHAR* RawOperators[] =
	{
		TEXT("/*"), TEXT("*/"), TEXT("//"),
		TEXT("\""), TEXT("\'"),
		TEXT("::"), TEXT("->*"), TEXT("->"),
		TEXT("++"), TEXT("--"),
		TEXT("<<="), TEXT(">>="),
		TEXT("<="), TEXT(">="), TEXT("=="), TEXT("!="),
		TEXT("&&"), TEXT("||"),
		TEXT("+="), TEXT("-="), TEXT("*="), TEXT("/="), TEXT("%="),
		TEXT("&="), TEXT("|="), TEXT("^="),
		TEXT("<<"), TEXT(">>"),
		TEXT("+"), TEXT("-"), TEXT("*"), TEXT("/"), TEXT("%"),
		TEXT("<"), TEXT(">"), TEXT("="), TEXT("!"),
		TEXT("&"), TEXT("|"), TEXT("^"), TEXT("~"),
		TEXT("?"), TEXT(":"), TEXT(";"), TEXT(","),
		TEXT("{"), TEXT("}"), TEXT("("), TEXT(")"), TEXT("["), TEXT("]"),
	};

	for (const TCHAR* Op : RawOperators)
	{
		Operators.Add(Op);
	}

	const TCHAR* RawFlowControl[] =
	{
		TEXT("if"), TEXT("else"), TEXT("return"), TEXT("for"), TEXT("while"), TEXT("do"),
		TEXT("switch"), TEXT("case"), TEXT("default"), TEXT("break"), TEXT("continue"), TEXT("goto"),
		TEXT("co_await"), TEXT("co_return"), TEXT("co_yield"), TEXT("try"), TEXT("catch"), TEXT("throw"),
	};
	for (const TCHAR* Fc : RawFlowControl)
	{
		FlowControl.Add(FName(Fc));
	}

	const TCHAR* RawKeywords[] =
	{
		TEXT("alignas"), TEXT("alignof"), TEXT("class"), TEXT("concept"),
		TEXT("const"), TEXT("consteval"), TEXT("constexpr"), TEXT("constinit"), TEXT("const_cast"),
		TEXT("decltype"), TEXT("delete"), TEXT("dynamic_cast"), TEXT("enum"), TEXT("explicit"),
		TEXT("export"), TEXT("extern"), TEXT("false"), TEXT("final"), TEXT("friend"),
		TEXT("inline"), TEXT("mutable"), TEXT("namespace"), TEXT("new"), TEXT("noexcept"),
		TEXT("nullptr"), TEXT("operator"), TEXT("override"), TEXT("private"), TEXT("protected"),
		TEXT("public"), TEXT("register"), TEXT("reinterpret_cast"), TEXT("requires"),
		TEXT("signed"), TEXT("sizeof"), TEXT("static"), TEXT("static_assert"),
		TEXT("static_cast"), TEXT("struct"), TEXT("template"), TEXT("this"),
		TEXT("thread_local"), TEXT("true"), TEXT("typedef"), TEXT("typeid"), TEXT("typename"),
		TEXT("union"), TEXT("unsigned"), TEXT("using"), TEXT("virtual"), TEXT("volatile"),
	};
	for (const TCHAR* Kw : RawKeywords)
	{
		Keywords.Add(FName(Kw));
	}

	const TCHAR* RawPrimitiveTypes[] =
	{
		TEXT("auto"), TEXT("bool"), TEXT("char"), TEXT("char16_t"), TEXT("char32_t"),
		TEXT("double"), TEXT("float"), TEXT("int"), TEXT("int8"), TEXT("int16"), TEXT("int32"), TEXT("int64"),
		TEXT("long"), TEXT("short"), TEXT("size_t"), TEXT("uint8"), TEXT("uint16"), TEXT("uint32"), TEXT("uint64"),
		TEXT("void"), TEXT("wchar_t"), TEXT("TCHAR"), TEXT("ANSICHAR"), TEXT("WIDECHAR"),
		TEXT("intptr_t"), TEXT("uintptr_t"),
	};
	for (const TCHAR* Pt : RawPrimitiveTypes)
	{
		PrimitiveTypes.Add(FName(Pt));
	}

	const TCHAR* RawTypes[] =
	{
		TEXT("FString"), TEXT("FName"), TEXT("FText"),
		TEXT("FVector"), TEXT("FVector2D"), TEXT("FVector4"), TEXT("FRotator"), TEXT("FQuat"),
		TEXT("FTransform"), TEXT("FMatrix"), TEXT("FLinearColor"), TEXT("FColor"), TEXT("FMargin"),
		TEXT("FReply"), TEXT("FGeometry"), TEXT("FKeyEvent"), TEXT("FPointerEvent"), TEXT("FTextLocation"),
		TEXT("FSlateColor"), TEXT("FSlateFontInfo"),
		TEXT("TArray"), TEXT("TMap"), TEXT("TSet"), TEXT("TSharedPtr"), TEXT("TSharedRef"),
		TEXT("TWeakPtr"), TEXT("TUniquePtr"), TEXT("TObjectPtr"), TEXT("TOptional"),
		TEXT("UObject"), TEXT("AActor"), TEXT("APawn"), TEXT("ACharacter"),
		TEXT("UActorComponent"), TEXT("USceneComponent"), TEXT("UClass"), TEXT("UWorld"),
		TEXT("SWidget"), TEXT("SCompoundWidget"), TEXT("SBorder"), TEXT("SBox"), TEXT("SButton"),
		TEXT("STextBlock"), TEXT("SEditableTextBox"), TEXT("SMultiLineEditableTextBox"), TEXT("SSearchBox"),
		TEXT("SSlider"), TEXT("SCheckBox"), TEXT("SProgressBar"), TEXT("SImage"),
		TEXT("SScrollBox"), TEXT("SSeparator"), TEXT("SSpacer"), TEXT("SVerticalBox"),
		TEXT("SHorizontalBox"), TEXT("SOverlay"), TEXT("SSplitter"), TEXT("STreeView"),
		TEXT("SListView"), TEXT("STableRow"), TEXT("FAppStyle"), TEXT("FCoreStyle"),
	};
	for (const TCHAR* Ty : RawTypes)
	{
		Types.Add(FName(Ty));
	}

	const TCHAR* RawMacros[] =
	{
		TEXT("UPROPERTY"), TEXT("UFUNCTION"), TEXT("UCLASS"), TEXT("USTRUCT"), TEXT("UENUM"), TEXT("UMETA"),
		TEXT("GENERATED_BODY"), TEXT("GENERATED_UCLASS_BODY"), TEXT("GENERATED_USTRUCT_BODY"),
		TEXT("SLATE_BEGIN_ARGS"), TEXT("SLATE_END_ARGS"), TEXT("SLATE_ARGUMENT"), TEXT("SLATE_ATTRIBUTE"),
		TEXT("SLATE_EVENT"), TEXT("SLATE_NAMED_SLOT"), TEXT("SLATE_DEFAULT_SLOT"),
		TEXT("SNew"), TEXT("SAssignNew"), TEXT("TEXT"),
		TEXT("check"), TEXT("checkf"), TEXT("ensure"), TEXT("ensureMsgf"), TEXT("UE_LOG"),
	};
	for (const TCHAR* Mac : RawMacros)
	{
		Macros.Add(FName(Mac));
	}
}

bool FCppSyntaxTokenizer::IsKeyword(const FStringView& InToken) const
{
	return Keywords.Contains(FName(InToken));
}

bool FCppSyntaxTokenizer::IsFlowControl(const FStringView& InToken) const
{
	return FlowControl.Contains(FName(InToken));
}

bool FCppSyntaxTokenizer::IsType(const FStringView& InToken) const
{
	return Types.Contains(FName(InToken));
}

bool FCppSyntaxTokenizer::IsPrimitiveType(const FStringView& InToken) const
{
	return PrimitiveTypes.Contains(FName(InToken));
}

bool FCppSyntaxTokenizer::IsMacro(const FStringView& InToken) const
{
	return Macros.Contains(FName(InToken));
}

bool FCppSyntaxTokenizer::IsUnrealType(const FStringView& InToken) const
{
	if (InToken.Len() >= 2)
	{
		const TCHAR First = InToken[0];
		const TCHAR Second = InToken[1];
		// Unreal Engine naming conventions: S (Slate), F (Struct/Class), U (UObject), A (Actor), T (Template), E (Enum), I (Interface)
		if ((First == TEXT('S') || First == TEXT('F') || First == TEXT('U') || First == TEXT('A') || First == TEXT('T') || First == TEXT('E') || First == TEXT('I')) &&
			FChar::IsUpper(Second))
		{
			return true;
		}
	}
	return Types.Contains(FName(InToken));
}

bool FCppSyntaxTokenizer::IsFunctionCall(const FString& Input, int32 TokenEnd, int32 LineEnd) const
{
	int32 Peek = TokenEnd;
	while (Peek < LineEnd && FChar::IsWhitespace(Input[Peek]))
	{
		Peek++;
	}
	return (Peek < LineEnd && Input[Peek] == TEXT('('));
}

bool FCppSyntaxTokenizer::IsVariableOrParameter(const FStringView& InToken) const
{
	if (InToken.IsEmpty())
	{
		return false;
	}
	if (InToken == TEXT("ChildSlot"))
	{
		return true;
	}
	if (InToken.Len() >= 3 && InToken.StartsWith(TEXT("In")) && FChar::IsUpper(InToken[2]))
	{
		return true;
	}
	if (InToken.Len() >= 2 && InToken.StartsWith(TEXT("b")) && FChar::IsUpper(InToken[1]))
	{
		return true;
	}
	// Variables or members starting with lowercase letter (e.g. health, width, myVar)
	if (FChar::IsLower(InToken[0]))
	{
		return true;
	}
	return false;
}

static bool IsAllUpperStringView(const FStringView& InView)
{
	for (TCHAR C : InView)
	{
		if (FChar::IsAlpha(C) && !FChar::IsUpper(C))
		{
			return false;
		}
	}
	return true;
}

void FCppSyntaxTokenizer::Process(TArray<FTokenizedLine>& OutTokenizedLines, const FString& Input)
{
	TArray<FTextRange> LineRanges;
	FTextRange::CalculateLineRangesFromString(Input, LineRanges);

	for (const FTextRange& LineRange : LineRanges)
	{
		FTokenizedLine TokenizedLine;
		TokenizedLine.Range = LineRange;

		if (LineRange.IsEmpty())
		{
			TokenizedLine.Tokens.Emplace(ETokenType::Literal, LineRange);
			OutTokenizedLines.Add(TokenizedLine);
			continue;
		}

		int32 CurrentOffset = LineRange.BeginIndex;
		while (CurrentOffset < LineRange.EndIndex)
		{
			const TCHAR CurrentChar = Input[CurrentOffset];

			// 1. Whitespace
			if (FChar::IsWhitespace(CurrentChar))
			{
				int32 WhitespaceEnd = CurrentOffset + 1;
				while (WhitespaceEnd < LineRange.EndIndex && FChar::IsWhitespace(Input[WhitespaceEnd]))
				{
					WhitespaceEnd++;
				}
				TokenizedLine.Tokens.Emplace(ETokenType::Literal, FTextRange(CurrentOffset, WhitespaceEnd));
				CurrentOffset = WhitespaceEnd;
				continue;
			}

			// 2. Operators & Comments (matching longest operator first)
			bool bMatchedOp = false;
			for (const FString& Op : Operators)
			{
				if (CurrentOffset + Op.Len() <= LineRange.EndIndex)
				{
					if (FCString::Strncmp(&Input[CurrentOffset], *Op, Op.Len()) == 0)
					{
						int32 OpEnd = CurrentOffset + Op.Len();
						TokenizedLine.Tokens.Emplace(ETokenType::Syntax, FTextRange(CurrentOffset, OpEnd));
						CurrentOffset = OpEnd;
						bMatchedOp = true;
						break;
					}
				}
			}
			if (bMatchedOp)
			{
				continue;
			}

			// 3. Preprocessor directives (#...)
			if (CurrentChar == TEXT('#'))
			{
				int32 PreEnd = CurrentOffset + 1;
				while (PreEnd < LineRange.EndIndex && FChar::IsAlpha(Input[PreEnd]))
				{
					PreEnd++;
				}
				TokenizedLine.Tokens.Emplace(ETokenType::Syntax, FTextRange(CurrentOffset, PreEnd));
				CurrentOffset = PreEnd;
				continue;
			}

			// 4. Identifiers (Keywords, Types, Functions, Variables, Macros)
			if (FChar::IsAlpha(CurrentChar) || CurrentChar == TEXT('_'))
			{
				int32 IdEnd = CurrentOffset + 1;
				while (IdEnd < LineRange.EndIndex && FChar::IsIdentifier(Input[IdEnd]))
				{
					IdEnd++;
				}

				TokenizedLine.Tokens.Emplace(ETokenType::Syntax, FTextRange(CurrentOffset, IdEnd));
				CurrentOffset = IdEnd;
				continue;
			}

			// 5. Numbers
			if (FChar::IsDigit(CurrentChar))
			{
				int32 NumEnd = CurrentOffset + 1;
				while (NumEnd < LineRange.EndIndex)
				{
					TCHAR C = Input[NumEnd];
					if (FChar::IsDigit(C) || FChar::IsHexDigit(C) || C == TEXT('.') || C == TEXT('x') || C == TEXT('X') || C == TEXT('f') || C == TEXT('F') || C == TEXT('u') || C == TEXT('U') || C == TEXT('l') || C == TEXT('L'))
					{
						NumEnd++;
					}
					else
					{
						break;
					}
				}
				TokenizedLine.Tokens.Emplace(ETokenType::Syntax, FTextRange(CurrentOffset, NumEnd));
				CurrentOffset = NumEnd;
				continue;
			}

			// 6. Fallback: Group consecutive unknown literal characters
			int32 LitEnd = CurrentOffset + 1;
			while (LitEnd < LineRange.EndIndex)
			{
				TCHAR C = Input[LitEnd];
				if (FChar::IsWhitespace(C) || FChar::IsAlpha(C) || FChar::IsDigit(C) || C == TEXT('_') || C == TEXT('#') || C == TEXT('\"') || C == TEXT('\''))
				{
					break;
				}
				LitEnd++;
			}
			TokenizedLine.Tokens.Emplace(ETokenType::Literal, FTextRange(CurrentOffset, LitEnd));
			CurrentOffset = LitEnd;
		}

		OutTokenizedLines.Add(TokenizedLine);
	}
}

// ----------------------------------------------------------------------------
// FCppSyntaxHighlighterMarshaller
// ----------------------------------------------------------------------------
FSlateFontInfo FCppSyntaxHighlighterMarshaller::GetEditorFont(float Size)
{
	static const FString ConsolasPath = TEXT("C:/Windows/Fonts/consola.ttf");
	static TSharedPtr<const FCompositeFont> ConsolasCompositeFont;
	if (!ConsolasCompositeFont.IsValid() && IFileManager::Get().FileExists(*ConsolasPath))
	{
		ConsolasCompositeFont = MakeShareable(new FStandaloneCompositeFont(TEXT("Consolas"), ConsolasPath, EFontHinting::Default, EFontLoadingPolicy::LazyLoad));
	}

	if (ConsolasCompositeFont.IsValid())
	{
		return FSlateFontInfo(ConsolasCompositeFont, Size);
	}
	return FCoreStyle::GetDefaultFontStyle("Mono", Size);
}

FSlateFontInfo FCppSyntaxHighlighterMarshaller::GetEditorFontBold(float Size)
{
	static const FString ConsolasBoldPath = TEXT("C:/Windows/Fonts/consolab.ttf");
	static TSharedPtr<const FCompositeFont> ConsolasBoldCompositeFont;
	if (!ConsolasBoldCompositeFont.IsValid() && IFileManager::Get().FileExists(*ConsolasBoldPath))
	{
		ConsolasBoldCompositeFont = MakeShareable(new FStandaloneCompositeFont(TEXT("ConsolasBold"), ConsolasBoldPath, EFontHinting::Default, EFontLoadingPolicy::LazyLoad));
	}

	if (ConsolasBoldCompositeFont.IsValid())
	{
		return FSlateFontInfo(ConsolasBoldCompositeFont, Size);
	}
	return FCoreStyle::GetDefaultFontStyle("Bold", Size);
}

TSharedRef<FCppSyntaxHighlighterMarshaller> FCppSyntaxHighlighterMarshaller::Create(const FSyntaxTextStyle& InSyntaxTextStyle)
{
	TSharedRef<FCppSyntaxTokenizer> Tokenizer = FCppSyntaxTokenizer::Create();
	return MakeShareable(new FCppSyntaxHighlighterMarshaller(Tokenizer, InSyntaxTextStyle));
}

TSharedRef<FCppSyntaxHighlighterMarshaller> FCppSyntaxHighlighterMarshaller::CreateDefaultDark()
{
	FSyntaxTextStyle Style;

	// Authentic MVS Consolas font - 11pt crisp, readable size
	const FSlateFontInfo EditorFont = GetEditorFont(11.0f);
	const FSlateFontInfo EditorFontBold = GetEditorFontBold(11.0f);

	// VS Code C++ Extension Dark+ Authentic Theme:
	// Normal text / identifiers: #D4D4D4
	Style.NormalTextStyle = FTextBlockStyle()
		.SetFont(EditorFont)
		.SetColorAndOpacity(FLinearColor(0.831f, 0.831f, 0.831f, 1.0f));

	// Keywords (class, virtual, override, const, auto): VS Code Blue #569CD6
	Style.KeywordTextStyle = FTextBlockStyle()
		.SetFont(EditorFontBold)
		.SetColorAndOpacity(FLinearColor(0.337f, 0.612f, 0.839f, 1.0f));

	// Flow Control (if, else, return, for, while, switch): VS Code Control Flow Purple #C586C0
	Style.FlowControlTextStyle = FTextBlockStyle()
		.SetFont(EditorFontBold)
		.SetColorAndOpacity(FLinearColor(0.773f, 0.525f, 0.753f, 1.0f));

	// User / Engine Types & Structs (SSampleSlateWidget, SBorder, FArguments, FMargin, FLinearColor): VS Code Mint Teal #4EC9B0
	Style.TypeTextStyle = FTextBlockStyle()
		.SetFont(EditorFontBold)
		.SetColorAndOpacity(FLinearColor(0.306f, 0.788f, 0.690f, 1.0f));

	// Functions / Methods (Construct, BorderBackgroundColor, Padding, AutoHeight, Slot): VS Code Yellow #DCDCAA
	Style.FunctionTextStyle = FTextBlockStyle()
		.SetFont(EditorFontBold)
		.SetColorAndOpacity(FLinearColor(0.863f, 0.863f, 0.667f, 1.0f));

	// Variables / Parameters / Members (InArgs, ChildSlot): VS Code Light Sky Blue #9CDCFE
	Style.VariableTextStyle = FTextBlockStyle()
		.SetFont(EditorFont)
		.SetColorAndOpacity(FLinearColor(0.612f, 0.863f, 0.996f, 1.0f));

	// Primitive Types (int, float, bool, void, int32): VS Code Blue #569CD6
	Style.PrimitiveTypeTextStyle = FTextBlockStyle()
		.SetFont(EditorFontBold)
		.SetColorAndOpacity(FLinearColor(0.337f, 0.612f, 0.839f, 1.0f));

	// Macros / Unreal Macros (UPROPERTY, UFUNCTION, SNew, SAssignNew, TEXT): VS Code Orchid #C586C0
	Style.MacroTextStyle = FTextBlockStyle()
		.SetFont(EditorFontBold)
		.SetColorAndOpacity(FLinearColor(0.773f, 0.525f, 0.753f, 1.0f));

	// Preprocessor (#include, #pragma, #define): VS Code Purple #C586C0
	Style.PreProcessorTextStyle = FTextBlockStyle()
		.SetFont(EditorFontBold)
		.SetColorAndOpacity(FLinearColor(0.773f, 0.525f, 0.753f, 1.0f));

	// Strings ("..."): VS Code Warm Orange #CE9178
	Style.StringTextStyle = FTextBlockStyle()
		.SetFont(EditorFont)
		.SetColorAndOpacity(FLinearColor(0.808f, 0.569f, 0.471f, 1.0f));

	// Chars ('...'): VS Code Gold #D7BA7D
	Style.CharacterTextStyle = FTextBlockStyle()
		.SetFont(EditorFont)
		.SetColorAndOpacity(FLinearColor(0.843f, 0.729f, 0.490f, 1.0f));

	// Comments (//, /* */): VS Code Green #6A9955
	Style.CommentTextStyle = FTextBlockStyle()
		.SetFont(EditorFont)
		.SetColorAndOpacity(FLinearColor(0.416f, 0.600f, 0.333f, 1.0f));

	// Numbers (0-9, 1.0f, 0x10): VS Code Light Sage Green #B5CEA8
	Style.NumberTextStyle = FTextBlockStyle()
		.SetFont(EditorFont)
		.SetColorAndOpacity(FLinearColor(0.710f, 0.808f, 0.659f, 1.0f));

	// Operators (+, -, *, ->, ::, ;): Light Grey #D4D4D4
	Style.OperatorTextStyle = FTextBlockStyle()
		.SetFont(EditorFont)
		.SetColorAndOpacity(FLinearColor(0.831f, 0.831f, 0.831f, 1.0f));

	return Create(Style);
}

FCppSyntaxHighlighterMarshaller::FCppSyntaxHighlighterMarshaller(
	TSharedPtr<FCppSyntaxTokenizer> InTokenizer,
	const FSyntaxTextStyle& InSyntaxTextStyle)
	: FSyntaxHighlighterTextLayoutMarshaller(InTokenizer)
	, CppTokenizer(InTokenizer)
	, SyntaxTextStyle(InSyntaxTextStyle)
{
}

void FCppSyntaxHighlighterMarshaller::ParseTokens(
	const FString& SourceString,
	FTextLayout& TargetTextLayout,
	TArray<ISyntaxTokenizer::FTokenizedLine> TokenizedLines)
{
	TArray<FTextLayout::FNewLineData> LinesToAdd;
	LinesToAdd.Reserve(TokenizedLines.Num());

	int32 LineNo = 0;
	EParseState ParseState = EParseState::None;
	for (const ISyntaxTokenizer::FTokenizedLine& TokenizedLine : TokenizedLines)
	{
		LinesToAdd.Add(ProcessTokenizedLine(TokenizedLine, LineNo, SourceString, ParseState));
		LineNo++;
	}

	TargetTextLayout.AddLines(LinesToAdd);
}

FTextLayout::FNewLineData FCppSyntaxHighlighterMarshaller::ProcessTokenizedLine(
	const ISyntaxTokenizer::FTokenizedLine& TokenizedLine,
	const int32& LineNumber,
	const FString& SourceString,
	EParseState& ParseState)
{
	TSharedRef<FString> ModelString = MakeShareable(new FString());
	TArray<TSharedRef<IRun>> Runs;

	if (TokenizedLine.Tokens.Num() == 0)
	{
		// Ensure empty lines still have at least 1 run for Slate text layout stability
		FTextRange EmptyRange(0, 0);
		FRunInfo RunInfo(TEXT("SyntaxHighlight.CPP.Normal"));
		Runs.Add(FSlateTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.NormalTextStyle, EmptyRange));
		return FTextLayout::FNewLineData(MoveTemp(ModelString), MoveTemp(Runs));
	}

	for (const ISyntaxTokenizer::FToken& Token : TokenizedLine.Tokens)
	{
		FStringView TokenView = FStringView(SourceString).Mid(Token.Range.BeginIndex, Token.Range.Len());
		const FTextRange ModelRange(ModelString->Len(), ModelString->Len() + TokenView.Len());
		ModelString->Append(TokenView.GetData(), TokenView.Len());

		FRunInfo RunInfo(TEXT("SyntaxHighlight.CPP.Normal"));
		FTextBlockStyle TextBlockStyle = SyntaxTextStyle.NormalTextStyle;

		const bool bIsWhitespace = (TokenView.TrimEnd().Len() == 0);
		if (!bIsWhitespace)
		{
			bool bHasMatchedSyntax = false;
			if (Token.Type == ISyntaxTokenizer::ETokenType::Syntax)
			{
				if (ParseState == EParseState::None && TokenView == TEXT("\""))
				{
					RunInfo.Name = TEXT("SyntaxHighlight.CPP.String");
					TextBlockStyle = SyntaxTextStyle.StringTextStyle;
					ParseState = EParseState::LookingForString;
					bHasMatchedSyntax = true;
				}
				else if (ParseState == EParseState::LookingForString && TokenView == TEXT("\""))
				{
					RunInfo.Name = TEXT("SyntaxHighlight.CPP.String");
					TextBlockStyle = SyntaxTextStyle.StringTextStyle;
					ParseState = EParseState::None;
					bHasMatchedSyntax = true;
				}
				else if (ParseState == EParseState::None && TokenView == TEXT("\'"))
				{
					RunInfo.Name = TEXT("SyntaxHighlight.CPP.Character");
					TextBlockStyle = SyntaxTextStyle.CharacterTextStyle;
					ParseState = EParseState::LookingForCharacter;
					bHasMatchedSyntax = true;
				}
				else if (ParseState == EParseState::LookingForCharacter && TokenView == TEXT("\'"))
				{
					RunInfo.Name = TEXT("SyntaxHighlight.CPP.Character");
					TextBlockStyle = SyntaxTextStyle.CharacterTextStyle;
					ParseState = EParseState::None;
					bHasMatchedSyntax = true;
				}
				else if (ParseState == EParseState::None && TokenView.StartsWith(TEXT("#")))
				{
					RunInfo.Name = TEXT("SyntaxHighlight.CPP.PreProcessor");
					TextBlockStyle = SyntaxTextStyle.PreProcessorTextStyle;
					ParseState = EParseState::None;
					bHasMatchedSyntax = true;
				}
				else if (ParseState == EParseState::None && TokenView == TEXT("//"))
				{
					RunInfo.Name = TEXT("SyntaxHighlight.CPP.Comment");
					TextBlockStyle = SyntaxTextStyle.CommentTextStyle;
					ParseState = EParseState::LookingForSingleLineComment;
					bHasMatchedSyntax = true;
				}
				else if (ParseState == EParseState::None && TokenView == TEXT("/*"))
				{
					RunInfo.Name = TEXT("SyntaxHighlight.CPP.Comment");
					TextBlockStyle = SyntaxTextStyle.CommentTextStyle;
					ParseState = EParseState::LookingForMultiLineComment;
					bHasMatchedSyntax = true;
				}
				else if (ParseState == EParseState::LookingForMultiLineComment && TokenView == TEXT("*/"))
				{
					RunInfo.Name = TEXT("SyntaxHighlight.CPP.Comment");
					TextBlockStyle = SyntaxTextStyle.CommentTextStyle;
					ParseState = EParseState::None;
					bHasMatchedSyntax = true;
				}
				else if (ParseState == EParseState::None && CppTokenizer.IsValid())
				{
					if (CppTokenizer->IsFlowControl(TokenView))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.CPP.FlowControl");
						TextBlockStyle = SyntaxTextStyle.FlowControlTextStyle;
						bHasMatchedSyntax = true;
					}
					else if (CppTokenizer->IsMacro(TokenView) || (TokenView.Len() >= 2 && IsAllUpperStringView(TokenView)))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.CPP.Macro");
						TextBlockStyle = SyntaxTextStyle.MacroTextStyle;
						bHasMatchedSyntax = true;
					}
					else if (CppTokenizer->IsKeyword(TokenView))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.CPP.Keyword");
						TextBlockStyle = SyntaxTextStyle.KeywordTextStyle;
						bHasMatchedSyntax = true;
					}
					else if (CppTokenizer->IsPrimitiveType(TokenView))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.CPP.PrimitiveType");
						TextBlockStyle = SyntaxTextStyle.PrimitiveTypeTextStyle;
						bHasMatchedSyntax = true;
					}
					else if (CppTokenizer->IsType(TokenView) || CppTokenizer->IsUnrealType(TokenView))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.CPP.Type");
						TextBlockStyle = SyntaxTextStyle.TypeTextStyle;
						bHasMatchedSyntax = true;
					}
					else if (CppTokenizer->IsFunctionCall(SourceString, Token.Range.EndIndex, TokenizedLine.Range.EndIndex))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.CPP.Function");
						TextBlockStyle = SyntaxTextStyle.FunctionTextStyle;
						bHasMatchedSyntax = true;
					}
					else if (FChar::IsAlpha(TokenView[0]) || TokenView[0] == TEXT('_'))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.CPP.Variable");
						TextBlockStyle = SyntaxTextStyle.VariableTextStyle;
						bHasMatchedSyntax = true;
					}
					else if (FChar::IsDigit(TokenView[0]))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.CPP.Number");
						TextBlockStyle = SyntaxTextStyle.NumberTextStyle;
						bHasMatchedSyntax = true;
					}
					else if (!FChar::IsIdentifier(TokenView[0]))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.CPP.Operator");
						TextBlockStyle = SyntaxTextStyle.OperatorTextStyle;
						bHasMatchedSyntax = true;
					}
				}
			}

			if (Token.Type == ISyntaxTokenizer::ETokenType::Literal || !bHasMatchedSyntax)
			{
				if (ParseState == EParseState::LookingForString)
				{
					RunInfo.Name = TEXT("SyntaxHighlight.CPP.String");
					TextBlockStyle = SyntaxTextStyle.StringTextStyle;
				}
				else if (ParseState == EParseState::LookingForCharacter)
				{
					RunInfo.Name = TEXT("SyntaxHighlight.CPP.Character");
					TextBlockStyle = SyntaxTextStyle.CharacterTextStyle;
				}
				else if (ParseState == EParseState::LookingForSingleLineComment || ParseState == EParseState::LookingForMultiLineComment)
				{
					RunInfo.Name = TEXT("SyntaxHighlight.CPP.Comment");
					TextBlockStyle = SyntaxTextStyle.CommentTextStyle;
				}
			}
		}

		TSharedRef<ISlateRun> Run = FSlateTextRun::Create(RunInfo, ModelString, TextBlockStyle, ModelRange);
		Runs.Add(Run);
	}

	if (ParseState != EParseState::LookingForMultiLineComment)
	{
		ParseState = EParseState::None;
	}

	return FTextLayout::FNewLineData(MoveTemp(ModelString), MoveTemp(Runs));
}
