// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#include "SlateParser.h"
#include "Misc/Char.h"

// ----------------------------------------------------------------------------
// FSlateCodeLexer
// ----------------------------------------------------------------------------

FSlateCodeLexer::FSlateCodeLexer(const FString& InSource)
	: Source(InSource)
{
}

bool FSlateCodeLexer::IsAtEnd() const
{
	return Cursor >= Source.Len();
}

TCHAR FSlateCodeLexer::Peek() const
{
	if (IsAtEnd()) return TEXT('\0');
	return Source[Cursor];
}

TCHAR FSlateCodeLexer::PeekNext() const
{
	if (Cursor + 1 >= Source.Len()) return TEXT('\0');
	return Source[Cursor + 1];
}

TCHAR FSlateCodeLexer::Advance()
{
	if (IsAtEnd()) return TEXT('\0');
	TCHAR C = Source[Cursor++];
	if (C == TEXT('\n'))
	{
		CurrentLine++;
		CurrentColumn = 1;
	}
	else
	{
		CurrentColumn++;
	}
	return C;
}

void FSlateCodeLexer::SkipWhitespaceAndComments()
{
	while (!IsAtEnd())
	{
		TCHAR C = Peek();
		if (FChar::IsWhitespace(C))
		{
			Advance();
		}
		else if (C == TEXT('/') && PeekNext() == TEXT('/'))
		{
			// Single-line comment
			while (!IsAtEnd() && Peek() != TEXT('\n'))
			{
				Advance();
			}
		}
		else if (C == TEXT('/') && PeekNext() == TEXT('*'))
		{
			// Multi-line comment
			Advance(); // /
			Advance(); // *
			while (!IsAtEnd())
			{
				if (Peek() == TEXT('*') && PeekNext() == TEXT('/'))
				{
					Advance();
					Advance();
					break;
				}
				Advance();
			}
		}
		else
		{
			break;
		}
	}
}

TArray<FSlateToken> FSlateCodeLexer::Tokenize()
{
	TArray<FSlateToken> Tokens;
	while (!IsAtEnd())
	{
		SkipWhitespaceAndComments();
		if (IsAtEnd()) break;

		FSlateToken Token = ScanToken();
		if (Token.Type != ESlateTokenType::Unknown)
		{
			Tokens.Add(Token);
		}
	}

	FSlateToken EofToken;
	EofToken.Type = ESlateTokenType::EndOfFile;
	EofToken.Line = CurrentLine;
	EofToken.Column = CurrentColumn;
	Tokens.Add(EofToken);

	return Tokens;
}

FSlateToken FSlateCodeLexer::ScanToken()
{
	int32 StartLine = CurrentLine;
	int32 StartCol = CurrentColumn;
	TCHAR C = Advance();

	auto MakeToken = [&](ESlateTokenType Type, const FString& Text) -> FSlateToken
	{
		FSlateToken T;
		T.Type = Type;
		T.Text = Text;
		T.Line = StartLine;
		T.Column = StartCol;
		return T;
	};

	switch (C)
	{
	case TEXT('+'): return MakeToken(ESlateTokenType::Plus, TEXT("+"));
	case TEXT('.'):
		if (FChar::IsDigit(Peek()))
		{
			Cursor--; // rewind
			CurrentColumn--;
			return ScanNumber();
		}
		return MakeToken(ESlateTokenType::Dot, TEXT("."));
	case TEXT(','): return MakeToken(ESlateTokenType::Comma, TEXT(","));
	case TEXT('('): return MakeToken(ESlateTokenType::OpenParen, TEXT("("));
	case TEXT(')'): return MakeToken(ESlateTokenType::CloseParen, TEXT(")"));
	case TEXT('['): return MakeToken(ESlateTokenType::OpenBracket, TEXT("["));
	case TEXT(']'): return MakeToken(ESlateTokenType::CloseBracket, TEXT("]"));
	case TEXT('{'): return MakeToken(ESlateTokenType::OpenBrace, TEXT("{"));
	case TEXT('}'): return MakeToken(ESlateTokenType::CloseBrace, TEXT("}"));
	case TEXT(';'): return MakeToken(ESlateTokenType::Semicolon, TEXT(";"));
	case TEXT(':'):
		if (Peek() == TEXT(':'))
		{
			Advance();
			return MakeToken(ESlateTokenType::DoubleColon, TEXT("::"));
		}
		return MakeToken(ESlateTokenType::Colon, TEXT(":"));

	case TEXT('"'):
		return ScanString(TEXT('"'));

	case TEXT('\''):
		return ScanString(TEXT('\''));

	default:
		if (C == TEXT('-') && (FChar::IsDigit(Peek()) || Peek() == TEXT('.')))
		{
			Cursor--;
			CurrentColumn--;
			return ScanNumber();
		}
		if (FChar::IsDigit(C))
		{
			Cursor--;
			CurrentColumn--;
			return ScanNumber();
		}
		if (FChar::IsAlpha(C) || C == TEXT('_'))
		{
			Cursor--;
			CurrentColumn--;
			return ScanIdentifierOrKeyword();
		}
		break;
	}

	return MakeToken(ESlateTokenType::Unknown, FString::Printf(TEXT("%c"), C));
}

FSlateToken FSlateCodeLexer::ScanIdentifierOrKeyword()
{
	int32 StartLine = CurrentLine;
	int32 StartCol = CurrentColumn;
	int32 StartIdx = Cursor;

	while (!IsAtEnd())
	{
		TCHAR C = Peek();
		if (FChar::IsAlnum(C) || C == TEXT('_'))
		{
			Advance();
		}
		else if (C == TEXT(':') && PeekNext() == TEXT(':'))
		{
			// Allow Scope::Identifier
			Advance();
			Advance();
		}
		else
		{
			break;
		}
	}

	FString Ident = Source.Mid(StartIdx, Cursor - StartIdx);

	// Check if this was TEXT("...") macro
	if (Ident.Equals(TEXT("TEXT")) && Peek() == TEXT('('))
	{
		Advance(); // (
		SkipWhitespaceAndComments();
		if (Peek() == TEXT('"'))
		{
			Advance(); // Consume opening quote
			FSlateToken StrTok = ScanString(TEXT('"'));
			SkipWhitespaceAndComments();
			if (Peek() == TEXT(')'))
			{
				Advance(); // )
			}
			StrTok.Line = StartLine;
			StrTok.Column = StartCol;
			return StrTok;
		}
	}

	FSlateToken Tok;
	Tok.Type = ESlateTokenType::Identifier;
	Tok.Text = Ident;
	Tok.Line = StartLine;
	Tok.Column = StartCol;
	return Tok;
}

FSlateToken FSlateCodeLexer::ScanNumber()
{
	int32 StartLine = CurrentLine;
	int32 StartCol = CurrentColumn;
	int32 StartIdx = Cursor;

	if (Peek() == TEXT('-'))
	{
		Advance();
	}

	bool bHasDot = false;
	while (!IsAtEnd())
	{
		TCHAR C = Peek();
		if (FChar::IsDigit(C))
		{
			Advance();
		}
		else if (C == TEXT('.') && !bHasDot && FChar::IsDigit(PeekNext()))
		{
			bHasDot = true;
			Advance();
		}
		else if (C == TEXT('f') || C == TEXT('F'))
		{
			Advance();
			break;
		}
		else
		{
			break;
		}
	}

	FString NumStr = Source.Mid(StartIdx, Cursor - StartIdx);
	if (NumStr.EndsWith(TEXT("f"), ESearchCase::IgnoreCase))
	{
		NumStr.LeftChopInline(1);
	}

	FSlateToken Tok;
	Tok.Type = ESlateTokenType::Number;
	Tok.Text = NumStr;
	Tok.Line = StartLine;
	Tok.Column = StartCol;
	return Tok;
}

FSlateToken FSlateCodeLexer::ScanString(TCHAR QuoteChar)
{
	int32 StartLine = CurrentLine;
	int32 StartCol = CurrentColumn;
	FString Result;

	while (!IsAtEnd())
	{
		TCHAR C = Advance();
		if (C == QuoteChar)
		{
			break;
		}
		if (C == TEXT('\\') && !IsAtEnd())
		{
			TCHAR Escaped = Advance();
			switch (Escaped)
			{
			case TEXT('n'): Result.AppendChar(TEXT('\n')); break;
			case TEXT('t'): Result.AppendChar(TEXT('\t')); break;
			case TEXT('r'): Result.AppendChar(TEXT('\r')); break;
			case TEXT('\\'): Result.AppendChar(TEXT('\\')); break;
			case TEXT('"'): Result.AppendChar(TEXT('"')); break;
			case TEXT('\''): Result.AppendChar(TEXT('\'')); break;
			default: Result.AppendChar(Escaped); break;
			}
		}
		else
		{
			Result.AppendChar(C);
		}
	}

	FSlateToken Tok;
	Tok.Type = ESlateTokenType::String;
	Tok.Text = Result;
	Tok.Line = StartLine;
	Tok.Column = StartCol;
	return Tok;
}

// ----------------------------------------------------------------------------
// FSlateCodeParser
// ----------------------------------------------------------------------------

FSlateCodeParser::FSlateCodeParser(const TArray<FSlateToken>& InTokens)
	: Tokens(InTokens)
{
}

bool FSlateCodeParser::IsAtEnd() const
{
	return Current >= Tokens.Num() || Tokens[Current].Type == ESlateTokenType::EndOfFile;
}

const FSlateToken& FSlateCodeParser::Peek() const
{
	if (IsAtEnd()) return Tokens.Last();
	return Tokens[Current];
}

const FSlateToken& FSlateCodeParser::Previous() const
{
	if (Current == 0) return Tokens[0];
	return Tokens[Current - 1];
}

FSlateToken FSlateCodeParser::Advance()
{
	if (!IsAtEnd())
	{
		Current++;
	}
	return Previous();
}

bool FSlateCodeParser::Check(ESlateTokenType Type) const
{
	if (IsAtEnd()) return false;
	return Peek().Type == Type;
}

bool FSlateCodeParser::Match(ESlateTokenType Type)
{
	if (Check(Type))
	{
		Advance();
		return true;
	}
	return false;
}

bool FSlateCodeParser::Expect(ESlateTokenType Type, const FString& ErrorMessage)
{
	if (Check(Type))
	{
		Advance();
		return true;
	}
	Errors.Add(FString::Printf(TEXT("[Line %d] %s (found '%s')"),
		Peek().Line, *ErrorMessage, *Peek().Text));
	return false;
}

void FSlateCodeParser::SynchronizeAfterError()
{
	Advance();
	while (!IsAtEnd())
	{
		if (Previous().Type == ESlateTokenType::Semicolon) return;
		if (Check(ESlateTokenType::Plus)) return;
		if (Check(ESlateTokenType::CloseBracket)) return;
		Advance();
	}
}

FString FSlateCodeParser::ExtractSlateBlockFromSource(const FString& InCppContent)
{
	// Look for SNew( or SAssignNew(
	int32 IdxSNew = InCppContent.Find(TEXT("SNew("));
	int32 IdxSAssign = InCppContent.Find(TEXT("SAssignNew("));

	int32 StartIdx = INDEX_NONE;
	if (IdxSNew != INDEX_NONE && IdxSAssign != INDEX_NONE)
	{
		StartIdx = FMath::Min(IdxSNew, IdxSAssign);
	}
	else if (IdxSNew != INDEX_NONE)
	{
		StartIdx = IdxSNew;
	}
	else
	{
		StartIdx = IdxSAssign;
	}

	if (StartIdx == INDEX_NONE)
	{
		// Not found, return as is (could be already just a snippet)
		return InCppContent;
	}

	// Find the end by balancing parentheses and brackets
	int32 ParenDepth = 0;
	int32 BracketDepth = 0;
	int32 Len = InCppContent.Len();
	int32 EndIdx = Len;

	for (int32 i = StartIdx; i < Len; ++i)
	{
		TCHAR C = InCppContent[i];
		if (C == TEXT('(')) ParenDepth++;
		else if (C == TEXT(')')) ParenDepth--;
		else if (C == TEXT('[')) BracketDepth++;
		else if (C == TEXT(']')) BracketDepth--;
		else if (C == TEXT(';') && ParenDepth <= 0 && BracketDepth <= 0)
		{
			EndIdx = i;
			break;
		}
	}

	return InCppContent.Mid(StartIdx, EndIdx - StartIdx);
}

bool FSlateCodeParser::ParseSource(const FString& InSourceCode,
	TSharedPtr<FSlateWidgetNode>& OutRoot,
	TArray<FString>& OutErrors,
	TArray<FString>& OutWarnings)
{
	FString CleanCode = ExtractSlateBlockFromSource(InSourceCode);
	FSlateCodeLexer Lexer(CleanCode);
	TArray<FSlateToken> Tokens = Lexer.Tokenize();

	if (Tokens.Num() <= 1)
	{
		OutErrors.Add(TEXT("Source code is empty or has no recognizable tokens."));
		return false;
	}

	FSlateCodeParser Parser(Tokens);
	bool bSuccess = Parser.Parse(OutRoot);
	OutErrors.Append(Parser.GetErrors());
	OutWarnings.Append(Parser.GetWarnings());
	return bSuccess;
}

bool FSlateCodeParser::Parse(TSharedPtr<FSlateWidgetNode>& OutRootNode)
{
	// Find the first SNew or SAssignNew
	while (!IsAtEnd())
	{
		if (Check(ESlateTokenType::Identifier))
		{
			if (Peek().Text.Equals(TEXT("SNew")) || Peek().Text.Equals(TEXT("SAssignNew")))
			{
				OutRootNode = ParseWidgetInstantiation();
				return OutRootNode.IsValid();
			}
		}
		Advance();
	}

	Errors.Add(TEXT("No SNew(...) or SAssignNew(...) expression found."));
	return false;
}

TSharedPtr<FSlateWidgetNode> FSlateCodeParser::ParseWidgetInstantiation()
{
	if (!Check(ESlateTokenType::Identifier))
	{
		return nullptr;
	}

	FSlateToken MacroToken = Advance();
	bool bIsAssign = MacroToken.Text.Equals(TEXT("SAssignNew"));

	if (!Expect(ESlateTokenType::OpenParen, TEXT("Expected '(' after SNew / SAssignNew")))
	{
		return nullptr;
	}

	TSharedPtr<FSlateWidgetNode> Node = MakeShared<FSlateWidgetNode>();
	Node->Line = MacroToken.Line;

	if (bIsAssign)
	{
		// SAssignNew(Variable, WidgetType)
		if (Expect(ESlateTokenType::Identifier, TEXT("Expected variable name in SAssignNew")))
		{
			Node->VariableName = Previous().Text;
		}
		Expect(ESlateTokenType::Comma, TEXT("Expected ',' between variable and widget type"));
	}

	if (Expect(ESlateTokenType::Identifier, TEXT("Expected widget type (e.g. SBorder, SButton)")))
	{
		Node->WidgetType = Previous().Text;
	}

	Expect(ESlateTokenType::CloseParen, TEXT("Expected ')' after widget type"));

	// Parse chained calls .Property(...), slots + Slot(), and content [ ... ]
	while (!IsAtEnd())
	{
		if (Match(ESlateTokenType::Dot))
		{
			FSlatePropertyNode Prop;
			if (ParsePropertyCall(Prop))
			{
				Node->Properties.Add(Prop);
			}
		}
		else if (Match(ESlateTokenType::Plus))
		{
			ParseSlot(*Node);
		}
		else if (Check(ESlateTokenType::OpenBracket))
		{
			ParseChildContent(*Node);
			break;
		}
		else
		{
			break;
		}
	}

	return Node;
}

bool FSlateCodeParser::ParsePropertyCall(FSlatePropertyNode& OutProperty)
{
	if (!Expect(ESlateTokenType::Identifier, TEXT("Expected property or method name after '.'")))
	{
		return false;
	}

	OutProperty.PropertyName = Previous().Text;
	OutProperty.Line = Previous().Line;

	if (!Expect(ESlateTokenType::OpenParen, TEXT("Expected '(' after property name")))
	{
		return false;
	}

	// Parse arguments inside parentheses
	if (!Check(ESlateTokenType::CloseParen))
	{
		do
		{
			FSlateAstValue Val = ParseValue();
			OutProperty.Arguments.Add(Val);
		} while (Match(ESlateTokenType::Comma));
	}

	Expect(ESlateTokenType::CloseParen, TEXT("Expected ')' to close property arguments"));
	return true;
}

FSlateAstValue FSlateCodeParser::ParseValue()
{
	if (Check(ESlateTokenType::Number))
	{
		Advance();
		return FSlateAstValue::FromNumber(FCString::Atod(*Previous().Text));
	}

	if (Check(ESlateTokenType::String))
	{
		Advance();
		return FSlateAstValue::FromString(Previous().Text);
	}

	if (Check(ESlateTokenType::Identifier))
	{
		FString Ident = Peek().Text;

		// Boolean literals
		if (Ident.Equals(TEXT("true"), ESearchCase::IgnoreCase))
		{
			Advance();
			return FSlateAstValue::FromBool(true);
		}
		if (Ident.Equals(TEXT("false"), ESearchCase::IgnoreCase))
		{
			Advance();
			return FSlateAstValue::FromBool(false);
		}

		// Nested SNew widget
		if (Ident.Equals(TEXT("SNew")) || Ident.Equals(TEXT("SAssignNew")))
		{
			TSharedPtr<FSlateWidgetNode> SubWidget = ParseWidgetInstantiation();
			return FSlateAstValue::FromWidget(SubWidget);
		}

		// Constructors
		if (Ident.Equals(TEXT("FLinearColor")) || Ident.Equals(TEXT("FColor")))
		{
			return ParseColorConstructor();
		}
		if (Ident.Equals(TEXT("FMargin")))
		{
			return ParseMarginConstructor();
		}
		if (Ident.Equals(TEXT("FText::FromString")) || Ident.Equals(TEXT("LOCTEXT")) || Ident.Equals(TEXT("INVTEXT")))
		{
			return ParseTextConstructor();
		}

		// Check for static color: FLinearColor::Red, FLinearColor::White, etc.
		if (Ident.StartsWith(TEXT("FLinearColor::")))
		{
			Advance();
			FString ColorName = Ident.Mid(14);
			if (ColorName.Equals(TEXT("White"), ESearchCase::IgnoreCase)) return FSlateAstValue::FromColor(FLinearColor::White);
			if (ColorName.Equals(TEXT("Black"), ESearchCase::IgnoreCase)) return FSlateAstValue::FromColor(FLinearColor::Black);
			if (ColorName.Equals(TEXT("Red"), ESearchCase::IgnoreCase)) return FSlateAstValue::FromColor(FLinearColor::Red);
			if (ColorName.Equals(TEXT("Green"), ESearchCase::IgnoreCase)) return FSlateAstValue::FromColor(FLinearColor::Green);
			if (ColorName.Equals(TEXT("Blue"), ESearchCase::IgnoreCase)) return FSlateAstValue::FromColor(FLinearColor::Blue);
			if (ColorName.Equals(TEXT("Yellow"), ESearchCase::IgnoreCase)) return FSlateAstValue::FromColor(FLinearColor::Yellow);
			if (ColorName.Equals(TEXT("Transparent"), ESearchCase::IgnoreCase)) return FSlateAstValue::FromColor(FLinearColor::Transparent);
			if (ColorName.Equals(TEXT("Gray"), ESearchCase::IgnoreCase)) return FSlateAstValue::FromColor(FLinearColor::Gray);
			return FSlateAstValue::FromColor(FLinearColor::White);
		}

		// General Enum / Identifier
		Advance();
		return FSlateAstValue::FromEnum(Ident);
	}

	Advance();
	return FSlateAstValue();
}

FSlateAstValue FSlateCodeParser::ParseColorConstructor()
{
	Advance(); // Consume FLinearColor or FColor

	if (!Match(ESlateTokenType::OpenParen))
	{
		return FSlateAstValue::FromColor(FLinearColor::White);
	}

	TArray<float> Components;
	while (!Check(ESlateTokenType::CloseParen) && !IsAtEnd())
	{
		if (Check(ESlateTokenType::Number))
		{
			Components.Add(static_cast<float>(FCString::Atod(*Peek().Text)));
			Advance();
		}
		else if (Check(ESlateTokenType::String))
		{
			// Hex color: FColor::FromHex("#RRGGBB")
			FString Hex = Peek().Text;
			Advance();
			Match(ESlateTokenType::CloseParen);
			return FSlateAstValue::FromColor(FColor::FromHex(Hex));
		}
		else
		{
			Advance();
		}
		Match(ESlateTokenType::Comma);
	}
	Match(ESlateTokenType::CloseParen);

	if (Components.Num() >= 4)
	{
		return FSlateAstValue::FromColor(FLinearColor(Components[0], Components[1], Components[2], Components[3]));
	}
	if (Components.Num() == 3)
	{
		return FSlateAstValue::FromColor(FLinearColor(Components[0], Components[1], Components[2], 1.0f));
	}
	if (Components.Num() == 1)
	{
		return FSlateAstValue::FromColor(FLinearColor(Components[0], Components[0], Components[0], 1.0f));
	}

	return FSlateAstValue::FromColor(FLinearColor::White);
}

FSlateAstValue FSlateCodeParser::ParseMarginConstructor()
{
	Advance(); // Consume FMargin

	if (!Match(ESlateTokenType::OpenParen))
	{
		return FSlateAstValue::FromMargin(FMargin(0.0f));
	}

	TArray<float> Margins;
	while (!Check(ESlateTokenType::CloseParen) && !IsAtEnd())
	{
		if (Check(ESlateTokenType::Number))
		{
			Margins.Add(static_cast<float>(FCString::Atod(*Peek().Text)));
			Advance();
		}
		else
		{
			Advance();
		}
		Match(ESlateTokenType::Comma);
	}
	Match(ESlateTokenType::CloseParen);

	if (Margins.Num() == 1)
	{
		return FSlateAstValue::FromMargin(FMargin(Margins[0]));
	}
	if (Margins.Num() == 2)
	{
		return FSlateAstValue::FromMargin(FMargin(Margins[0], Margins[1]));
	}
	if (Margins.Num() >= 4)
	{
		return FSlateAstValue::FromMargin(FMargin(Margins[0], Margins[1], Margins[2], Margins[3]));
	}

	return FSlateAstValue::FromMargin(FMargin(0.0f));
}

FSlateAstValue FSlateCodeParser::ParseTextConstructor()
{
	Advance(); // Consume FText::FromString or LOCTEXT
	Match(ESlateTokenType::OpenParen);

	FString ExtractedText;
	while (!Check(ESlateTokenType::CloseParen) && !IsAtEnd())
	{
		if (Check(ESlateTokenType::String))
		{
			ExtractedText = Peek().Text;
			Advance();
		}
		else
		{
			Advance();
		}
		Match(ESlateTokenType::Comma);
	}
	Match(ESlateTokenType::CloseParen);

	return FSlateAstValue::FromString(ExtractedText);
}

bool FSlateCodeParser::ParseChildContent(FSlateWidgetNode& TargetWidget)
{
	if (!Expect(ESlateTokenType::OpenBracket, TEXT("Expected '[' for child content")))
	{
		return false;
	}

	// Content inside [ ... ] can either be slots: + Container::Slot()[ ... ]
	// or a direct single child: SNew(SChild)
	while (!Check(ESlateTokenType::CloseBracket) && !IsAtEnd())
	{
		if (Match(ESlateTokenType::Plus))
		{
			ParseSlot(TargetWidget);
		}
		else if (Check(ESlateTokenType::Identifier))
		{
			if (Peek().Text.Equals(TEXT("SNew")) || Peek().Text.Equals(TEXT("SAssignNew")))
			{
				TargetWidget.DirectChild = ParseWidgetInstantiation();
			}
			else
			{
				Advance();
			}
		}
		else
		{
			Advance();
		}
	}

	Expect(ESlateTokenType::CloseBracket, TEXT("Expected ']' closing child content"));
	return true;
}

bool FSlateCodeParser::ParseSlot(FSlateWidgetNode& TargetWidget)
{
	FSlateSlotNode Slot;
	Slot.Line = Previous().Line;

	if (Expect(ESlateTokenType::Identifier, TEXT("Expected slot specifier (e.g. SVerticalBox::Slot or Slot)")))
	{
		Slot.SlotType = Previous().Text;
	}

	if (Match(ESlateTokenType::OpenParen))
	{
		// Arguments to Slot(Col, Row) for GridPanel or Slot()
		while (!Check(ESlateTokenType::CloseParen) && !IsAtEnd())
		{
			Advance();
		}
		Match(ESlateTokenType::CloseParen);
	}

	// Slot properties: .AutoHeight(), .Padding(...), .HAlign(...), etc.
	while (!IsAtEnd())
	{
		if (Match(ESlateTokenType::Dot))
		{
			FSlatePropertyNode Prop;
			if (ParsePropertyCall(Prop))
			{
				Slot.SlotProperties.Add(Prop);
			}
		}
		else if (Check(ESlateTokenType::OpenBracket))
		{
			// Slot child: [ SNew(...) ]
			Advance(); // [
			while (!Check(ESlateTokenType::CloseBracket) && !IsAtEnd())
			{
				if (Check(ESlateTokenType::Identifier) &&
					(Peek().Text.Equals(TEXT("SNew")) || Peek().Text.Equals(TEXT("SAssignNew"))))
				{
					Slot.ChildWidget = ParseWidgetInstantiation();
				}
				else
				{
					Advance();
				}
			}
			Expect(ESlateTokenType::CloseBracket, TEXT("Expected ']' closing slot content"));
			break;
		}
		else
		{
			break;
		}
	}

	TargetWidget.Slots.Add(Slot);
	return true;
}
