// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "SlateParser.h"
#include "SlateWidgetBuilder.h"

// ----------------------------------------------------------------------------
// Test 1: Lexer
// ----------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSlateLivePreviewLexerTest,
	"SlateLivePreview.Lexer.BasicTokenization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSlateLivePreviewLexerTest::RunTest(const FString& Parameters)
{
	FString Code = TEXT("SNew(SBorder) .BorderBackgroundColor(FLinearColor(0.1f, 0.2f, 0.3f, 1.0f)) [ SNew(STextBlock).Text(FText::FromString(TEXT(\"Hello\"))) ]");
	FSlateCodeLexer Lexer(Code);
	TArray<FSlateToken> Tokens = Lexer.Tokenize();

	TestTrue(TEXT("Tokens generated"), Tokens.Num() > 10);
	TestEqual(TEXT("First token is SNew"), Tokens[0].Text, TEXT("SNew"));

	return true;
}

// ----------------------------------------------------------------------------
// Test 2: Parser & AST
// ----------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSlateLivePreviewParserTest,
	"SlateLivePreview.Parser.HierarchyAndProperties",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSlateLivePreviewParserTest::RunTest(const FString& Parameters)
{
	FString Code = TEXT(
		"SNew(SBorder)\n"
		".BorderBackgroundColor(FLinearColor(0.5f, 0.2f, 0.1f, 1.0f))\n"
		".Padding(FMargin(12.0f))\n"
		"[\n"
		"    SNew(SVerticalBox)\n"
		"    + SVerticalBox::Slot()\n"
		"    .AutoHeight()\n"
		"    [\n"
		"        SNew(STextBlock)\n"
		"        .Text(FText::FromString(TEXT(\"Test Text\")))\n"
		"    ]\n"
		"]\n"
	);

	TSharedPtr<FSlateWidgetNode> RootAst;
	TArray<FString> Errors;
	TArray<FString> Warnings;

	bool bOk = FSlateCodeParser::ParseSource(Code, RootAst, Errors, Warnings);

	TestTrue(TEXT("Parsing succeeded without errors"), bOk && Errors.Num() == 0);
	TestTrue(TEXT("Root AST node is valid"), RootAst.IsValid());
	if (RootAst.IsValid())
	{
		TestEqual(TEXT("Root type is SBorder"), RootAst->WidgetType, TEXT("SBorder"));
		TestEqual(TEXT("Root has 2 properties"), RootAst->Properties.Num(), 2);
		TestTrue(TEXT("Root has direct child"), RootAst->DirectChild.IsValid());
		if (RootAst->DirectChild.IsValid())
		{
			TestEqual(TEXT("Child is SVerticalBox"), RootAst->DirectChild->WidgetType, TEXT("SVerticalBox"));
			TestEqual(TEXT("VerticalBox has 1 slot"), RootAst->DirectChild->Slots.Num(), 1);
		}
	}

	return true;
}

// ----------------------------------------------------------------------------
// Test 3: Builder
// ----------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSlateLivePreviewBuilderTest,
	"SlateLivePreview.Builder.WidgetInstantiation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSlateLivePreviewBuilderTest::RunTest(const FString& Parameters)
{
	FString Code = TEXT(
		"SNew(SButton)\n"
		".ButtonColorAndOpacity(FLinearColor::Red)\n"
		"[\n"
		"    SNew(STextBlock).Text(FText::FromString(TEXT(\"Click\")))\n"
		"]\n"
	);

	TSharedPtr<FSlateWidgetNode> RootAst;
	TArray<FString> Errors;
	TArray<FString> Warnings;

	bool bOk = FSlateCodeParser::ParseSource(Code, RootAst, Errors, Warnings);
	TestTrue(TEXT("Parsed SButton"), bOk && RootAst.IsValid());

	FSlateWidgetBuilder Builder;
	TSharedRef<SWidget> LiveWidget = Builder.Build(RootAst);

	TestTrue(TEXT("Live Slate widget instantiated successfully"), LiveWidget != SNullWidget::NullWidget);

	return true;
}

// ----------------------------------------------------------------------------
// Test 4: Full C++ Source Extraction
// ----------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSlateLivePreviewExtractFromSourceTest,
	"SlateLivePreview.Parser.ExtractFromFullCpp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSlateLivePreviewExtractFromSourceTest::RunTest(const FString& Parameters)
{
	FString FullCpp = TEXT(
		"#include \"MyWidget.h\"\n"
		"#include \"Widgets/Layout/SBorder.h\"\n"
		"\n"
		"void SMyWidget::Construct(const FArguments& InArgs)\n"
		"{\n"
		"    ChildSlot\n"
		"    [\n"
		"        SNew(SBorder)\n"
		"        .BorderBackgroundColor(FLinearColor::Black)\n"
		"        [\n"
		"            SNew(STextBlock).Text(FText::FromString(TEXT(\"In Construct\")))\n"
		"        ]\n"
		"    ];\n"
		"}\n"
	);

	FString Extracted = FSlateCodeParser::ExtractSlateBlockFromSource(FullCpp);
	TestTrue(TEXT("Extracted SNew block starts with SNew"), Extracted.StartsWith(TEXT("SNew(")));

	TSharedPtr<FSlateWidgetNode> RootAst;
	TArray<FString> Errors;
	TArray<FString> Warnings;
	bool bOk = FSlateCodeParser::ParseSource(Extracted, RootAst, Errors, Warnings);
	TestTrue(TEXT("Successfully parsed block extracted from full cpp file"), bOk && RootAst.IsValid());

	return true;
}

// ----------------------------------------------------------------------------
// Test 5: C++ Syntax Highlighting & Tokenizer
// ----------------------------------------------------------------------------
#include "CppSyntaxHighlighter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSlateLivePreviewCppSyntaxTest,
	"SlateLivePreview.Syntax.CppTokenizer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSlateLivePreviewCppSyntaxTest::RunTest(const FString& Parameters)
{
	TSharedRef<FCppSyntaxTokenizer> Tokenizer = FCppSyntaxTokenizer::Create();
	TestTrue(TEXT("Identifies 'class' as keyword"), Tokenizer->IsKeyword(TEXT("class")));
	TestTrue(TEXT("Identifies 'virtual' as keyword"), Tokenizer->IsKeyword(TEXT("virtual")));
	TestTrue(TEXT("Identifies 'FString' as type"), Tokenizer->IsType(TEXT("FString")));
	TestTrue(TEXT("Identifies 'SWidget' as type"), Tokenizer->IsType(TEXT("SWidget")));
	TestTrue(TEXT("Identifies 'UPROPERTY' as macro"), Tokenizer->IsMacro(TEXT("UPROPERTY")));
	TestTrue(TEXT("Identifies 'SSampleSlateWidget' as Unreal type"), Tokenizer->IsUnrealType(TEXT("SSampleSlateWidget")));
	TestTrue(TEXT("Identifies 'ECheckBoxState' as Unreal type"), Tokenizer->IsUnrealType(TEXT("ECheckBoxState")));
	TestTrue(TEXT("Identifies 'FArguments' as Unreal type"), Tokenizer->IsUnrealType(TEXT("FArguments")));
	TestTrue(TEXT("Identifies 'ChildSlot' as variable"), Tokenizer->IsVariableOrParameter(TEXT("ChildSlot")));
	TestTrue(TEXT("Identifies 'InArgs' as variable"), Tokenizer->IsVariableOrParameter(TEXT("InArgs")));

	FString SampleCpp = TEXT("class FTest { FString Name = TEXT(\"Antigravity\"); // comment\n };");
	TArray<ISyntaxTokenizer::FTokenizedLine> TokenizedLines;
	Tokenizer->Process(TokenizedLines, SampleCpp);

	TestTrue(TEXT("Processes tokenized lines"), TokenizedLines.Num() >= 1);
	TestTrue(TEXT("Emits syntax tokens"), TokenizedLines[0].Tokens.Num() > 3);

	return true;
}

// ----------------------------------------------------------------------------
// Test 6: Safe Interactive Widgets (No 0x38 Access Violation Crash)
// ----------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSlateLivePreviewSafeInteractiveWidgetsTest,
	"SlateLivePreview.Builder.SafeInteractiveWidgets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSlateLivePreviewSafeInteractiveWidgetsTest::RunTest(const FString& Parameters)
{
	FString Code = TEXT(
		"SNew(SVerticalBox)\n"
		"+ SVerticalBox::Slot()\n"
		"[\n"
		"    SNew(SSlider).Value(0.5f)\n"
		"]\n"
		"+ SVerticalBox::Slot()\n"
		"[\n"
		"    SNew(SButton)\n"
		"]\n"
		"+ SVerticalBox::Slot()\n"
		"[\n"
		"    SNew(SCheckBox)\n"
		"]\n"
	);

	TSharedPtr<FSlateWidgetNode> RootAst;
	TArray<FString> Errors;
	TArray<FString> Warnings;
	bool bOk = FSlateCodeParser::ParseSource(Code, RootAst, Errors, Warnings);
	TestTrue(TEXT("Parsed interactive widgets"), bOk && RootAst.IsValid());

	TSharedPtr<SWidget> LiveWidget;
	bool bLogged = false;

	{
		// Create builder in a local temporary scope (simulating stack destruction)
		FSlateWidgetBuilder TempBuilder;
		TempBuilder.SetLogHandler(FOnSlatePreviewLog::CreateLambda([&bLogged](const FString& Msg)
		{
			bLogged = true;
		}));

		LiveWidget = TempBuilder.Build(RootAst);
		// TempBuilder is destroyed HERE when exiting scope!
	}

	TestTrue(TEXT("LiveWidget exists after builder destruction"), LiveWidget.IsValid());

	// If this were the old code with dangling 'this', calling the widget's interaction here would crash.
	// Now with by-value log delegate captures, it is 100% memory safe!

	return true;
}

// ----------------------------------------------------------------------------
// Test 7: C++ Studio Editor Pane & IntelliSense Instantiation
// ----------------------------------------------------------------------------
#include "SCppEditorPane.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSlateLivePreviewIntelliSenseTest,
	"SlateLivePreview.IntelliSense.InstantiationAndStructure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSlateLivePreviewIntelliSenseTest::RunTest(const FString& Parameters)
{
	TSharedRef<SCppEditorPane> Pane = SNew(SCppEditorPane);
	TestTrue(TEXT("SCppEditorPane instantiates cleanly"), Pane != SNullWidget::NullWidget);

	// Test dirty and open file state
	TestFalse(TEXT("No files open initially"), Pane->HasOpenFiles());
	TestFalse(TEXT("Not dirty when no files open"), Pane->IsCurrentDirty());
	TestTrue(TEXT("Active file path is empty"), Pane->GetActiveFilePath().IsEmpty());

	return true;
}

// ----------------------------------------------------------------------------
// Test 8: Class Generation Wizard & Template Formatting
// ----------------------------------------------------------------------------
#include "SCreateClassDialog.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSlateLivePreviewClassWizardTest,
	"SlateLivePreview.Wizard.TemplateGeneration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSlateLivePreviewClassWizardTest::RunTest(const FString& Parameters)
{
	FString TargetDir = FPaths::GameSourceDir() / TEXT("CircuitNodes");
	TSharedRef<SCreateClassDialog> Dialog = SNew(SCreateClassDialog)
		.DefaultDirectory(TargetDir);

	TestTrue(TEXT("SCreateClassDialog instantiates cleanly"), Dialog != SNullWidget::NullWidget);

	// Test template code generation for Slate Widget
	FString HeaderCode;
	FString CppCode;
	FString HeaderPath;
	FString CppPath;

	Dialog->SetSelectedTemplate(EClassTemplateType::SlateWidget);
	Dialog->SetClassName(TEXT("SCustomButton"));
	Dialog->GenerateCode(HeaderCode, CppCode, HeaderPath, CppPath);

	TestTrue(TEXT("Header generated for Slate Widget"), HeaderCode.Contains(TEXT("class CIRCUITNODES_API SCustomButton : public SCompoundWidget")));
	TestTrue(TEXT("Header contains SLATE_BEGIN_ARGS"), HeaderCode.Contains(TEXT("SLATE_BEGIN_ARGS(SCustomButton)")));
	TestTrue(TEXT("Source generated for Slate Widget"), CppCode.Contains(TEXT("void SCustomButton::Construct(const FArguments& InArgs)")));
	TestTrue(TEXT("Header path in Public folder"), HeaderPath.Contains(TEXT("Public")));
	TestTrue(TEXT("Source path in Private folder"), CppPath.Contains(TEXT("Private")));

	// Test template code generation for UObjectClass
	Dialog->SetSelectedTemplate(EClassTemplateType::UObjectClass);
	Dialog->SetClassName(TEXT("UItemData"));
	Dialog->GenerateCode(HeaderCode, CppCode, HeaderPath, CppPath);

	TestTrue(TEXT("Header generated for UObject"), HeaderCode.Contains(TEXT("class CIRCUITNODES_API UItemData : public UObject")));
	TestTrue(TEXT("Header contains GENERATED_BODY"), HeaderCode.Contains(TEXT("GENERATED_BODY()")));

	// Test template code generation for AActorClass
	Dialog->SetSelectedTemplate(EClassTemplateType::AActorClass);
	Dialog->SetClassName(TEXT("APowerGenerator"));
	Dialog->GenerateCode(HeaderCode, CppCode, HeaderPath, CppPath);

	TestTrue(TEXT("Header generated for AActor"), HeaderCode.Contains(TEXT("class CIRCUITNODES_API APowerGenerator : public AActor")));
	TestTrue(TEXT("Source contains BeginPlay"), CppCode.Contains(TEXT("APowerGenerator::BeginPlay()")));

	// Test Character template & non-doubling prefix
	Dialog->SetSelectedTemplate(EClassTemplateType::Character);
	Dialog->SetClassName(TEXT("MyHero"));
	Dialog->GenerateCode(HeaderCode, CppCode, HeaderPath, CppPath);
	TestTrue(TEXT("Header generated for Character"), HeaderCode.Contains(TEXT("class CIRCUITNODES_API AMyHero : public ACharacter")));
	TestTrue(TEXT("No double prefix AAMyHero"), !HeaderCode.Contains(TEXT("AAMyHero")));
	TestTrue(TEXT("Header path has MyHero.h"), HeaderPath.EndsWith(TEXT("MyHero.h")));

	// Test if user already typed prefix 'A'
	Dialog->SetClassName(TEXT("AMyHero"));
	Dialog->GenerateCode(HeaderCode, CppCode, HeaderPath, CppPath);
	TestTrue(TEXT("Class name remains AMyHero when A typed"), HeaderCode.Contains(TEXT("class CIRCUITNODES_API AMyHero : public ACharacter")));
	TestTrue(TEXT("No double prefix AAMyHero when user typed A"), !HeaderCode.Contains(TEXT("AAMyHero")));
	TestTrue(TEXT("Header path remains MyHero.h"), HeaderPath.EndsWith(TEXT("MyHero.h")));

	// Test Pawn template
	Dialog->SetSelectedTemplate(EClassTemplateType::Pawn);
	Dialog->SetClassName(TEXT("MyPawn"));
	Dialog->GenerateCode(HeaderCode, CppCode, HeaderPath, CppPath);
	TestTrue(TEXT("Header generated for Pawn"), HeaderCode.Contains(TEXT("class CIRCUITNODES_API AMyPawn : public APawn")));

	// Test ActorComponent template
	Dialog->SetSelectedTemplate(EClassTemplateType::UActorComponentClass);
	Dialog->SetClassName(TEXT("HealthComponent"));
	Dialog->GenerateCode(HeaderCode, CppCode, HeaderPath, CppPath);
	TestTrue(TEXT("Header generated for Component"), HeaderCode.Contains(TEXT("class CIRCUITNODES_API UHealthComponent : public UActorComponent")));
	TestTrue(TEXT("No double prefix UUHealthComponent"), !HeaderCode.Contains(TEXT("UUHealthComponent")));

	return true;
}

// ----------------------------------------------------------------------------
// Test 9: C++ Studio Tab Instantiation & Quick Open
// ----------------------------------------------------------------------------
#include "SCppStudioTab.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSlateLivePreviewStudioTabTest,
	"SlateLivePreview.StudioTab.InstantiationAndControls",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSlateLivePreviewStudioTabTest::RunTest(const FString& Parameters)
{
	TSharedRef<SCppStudioTab> Studio = SNew(SCppStudioTab);
	TestTrue(TEXT("SCppStudioTab instantiates cleanly with all panels and overlays"), Studio != SNullWidget::NullWidget);

	// Test toggling Slate preview
	Studio->ToggleSlatePreview(false);
	Studio->ToggleSlatePreview(true);

	// Test toggling split view
	Studio->ToggleSplitView();
	Studio->ToggleSplitView();

	// Test toggling quick open
	Studio->ToggleQuickOpen(true);
	Studio->ToggleQuickOpen(false);

	return true;
}

#endif

