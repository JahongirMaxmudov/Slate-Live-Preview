// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#include "SCreateClassDialog.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "CppSyntaxHighlighter.h"

FString SCreateClassDialog::GetDefaultNameForTemplate(EClassTemplateType InTemplate)
{
	switch (InTemplate)
	{
	case EClassTemplateType::Character:
		return TEXT("MyCharacter");
	case EClassTemplateType::Pawn:
		return TEXT("MyPawn");
	case EClassTemplateType::AActorClass:
		return TEXT("MyActor");
	case EClassTemplateType::UActorComponentClass:
		return TEXT("MyActorComponent");
	case EClassTemplateType::SceneComponent:
		return TEXT("MySceneComponent");
	case EClassTemplateType::SlateWidget:
		return TEXT("SMyCustomWidget");
	case EClassTemplateType::UObjectClass:
		return TEXT("MyObject");
	case EClassTemplateType::UStructType:
		return TEXT("MyCustomStruct");
	case EClassTemplateType::EmptyCppClass:
	default:
		return TEXT("MyClass");
	}
}

void SCreateClassDialog::ResolveClassAndFileNames(
	EClassTemplateType InTemplate,
	const FString& InInputName,
	FString& OutClassName,
	FString& OutBaseName,
	FString& OutHeaderFileName,
	FString& OutSourceFileName)
{
	FString Raw = InInputName.TrimStartAndEnd();
	if (Raw.IsEmpty())
	{
		Raw = GetDefaultNameForTemplate(InTemplate);
	}

	TCHAR Prefix = TEXT('\0');
	bool bForceFileHasPrefix = false;

	switch (InTemplate)
	{
	case EClassTemplateType::Character:
	case EClassTemplateType::Pawn:
	case EClassTemplateType::AActorClass:
		Prefix = TEXT('A');
		break;
	case EClassTemplateType::UActorComponentClass:
	case EClassTemplateType::SceneComponent:
	case EClassTemplateType::UObjectClass:
		Prefix = TEXT('U');
		break;
	case EClassTemplateType::UStructType:
		Prefix = TEXT('F');
		break;
	case EClassTemplateType::SlateWidget:
		Prefix = TEXT('S');
		bForceFileHasPrefix = true;
		break;
	case EClassTemplateType::EmptyCppClass:
	default:
		Prefix = TEXT('\0');
		break;
	}

	if (Prefix != TEXT('\0'))
	{
		// Avoid double prefix (e.g. AMyCharacter -> AMyCharacter, MyCharacter -> AMyCharacter)
		if (Raw.Len() >= 2 && Raw[0] == Prefix && FChar::IsUpper(Raw[1]))
		{
			OutClassName = Raw;
			OutBaseName = Raw.Mid(1);
		}
		else
		{
			OutClassName = FString::Printf(TEXT("%c%s"), Prefix, *Raw);
			OutBaseName = Raw;
		}

		if (bForceFileHasPrefix)
		{
			OutHeaderFileName = OutClassName + TEXT(".h");
			OutSourceFileName = OutClassName + TEXT(".cpp");
		}
		else
		{
			OutHeaderFileName = OutBaseName + TEXT(".h");
			OutSourceFileName = OutBaseName + TEXT(".cpp");
		}
	}
	else
	{
		OutClassName = Raw;
		OutBaseName = Raw;
		OutHeaderFileName = Raw + TEXT(".h");
		OutSourceFileName = Raw + TEXT(".cpp");
	}
}

void SCreateClassDialog::SetSelectedTemplate(EClassTemplateType InTemplate)
{
	SelectedTemplate = InTemplate;
	ClassNameInput = GetDefaultNameForTemplate(InTemplate);
	if (ClassNameTextBox.IsValid())
	{
		ClassNameTextBox->SetText(FText::FromString(ClassNameInput));
	}
	UpdateGeneratedCode();
}

void SCreateClassDialog::Construct(const FArguments& InArgs)
{
	TargetDirectory = InArgs._DefaultDirectory;
	if (TargetDirectory.IsEmpty())
	{
		TargetDirectory = FPaths::ProjectPluginsDir() / TEXT("SlateLivePreview/Source/SlateLivePreview");
	}
	TargetDirectory = FPaths::ConvertRelativePathToFull(TargetDirectory);

	OnClassCreated = InArgs._OnClassCreated;
	OnCanceled = InArgs._OnCanceled;

	ClassNameInput = GetDefaultNameForTemplate(SelectedTemplate);

	struct FTemplateCardInfo
	{
		EClassTemplateType Template;
		FString Icon;
		FString Title;
		FString Description;
	};

	const TArray<FTemplateCardInfo> Cards = {
		{ EClassTemplateType::Character,            TEXT("🏃"), TEXT("Character"),       TEXT("An Actor that includes the ability to walk around.") },
		{ EClassTemplateType::Pawn,                 TEXT("♟️"),  TEXT("Pawn"),            TEXT("An Actor that can be controlled by players or AI.") },
		{ EClassTemplateType::AActorClass,          TEXT("🎭"), TEXT("Actor"),           TEXT("An object that can be placed or spawned in the world.") },
		{ EClassTemplateType::UActorComponentClass, TEXT("🧩"), TEXT("Actor Component"), TEXT("A reusable component that can be added to any Actor.") },
		{ EClassTemplateType::SceneComponent,       TEXT("📐"), TEXT("Scene Component"), TEXT("A component with a transform and hierarchy attachment.") },
		{ EClassTemplateType::SlateWidget,          TEXT("🖥️"), TEXT("Slate Widget"),    TEXT("Custom SCompoundWidget UI element with declarative syntax.") },
		{ EClassTemplateType::UObjectClass,         TEXT("📦"), TEXT("UObject"),         TEXT("The base class of Unreal Engine objects with reflection.") },
		{ EClassTemplateType::UStructType,          TEXT("🏷️"),  TEXT("UStruct"),         TEXT("A reflected value type struct marked with BlueprintType.") },
		{ EClassTemplateType::EmptyCppClass,        TEXT("📄"), TEXT("Empty C++"),       TEXT("A standard C++ class without Unreal inheritance.") },
	};

	TSharedRef<SUniformGridPanel> CardsGrid = SNew(SUniformGridPanel).SlotPadding(FMargin(3.0f));

	for (int32 i = 0; i < Cards.Num(); ++i)
	{
		const FTemplateCardInfo& Card = Cards[i];
		const int32 Row = i / 3;
		const int32 Col = i % 3;
		const EClassTemplateType CardType = Card.Template;

		CardsGrid->AddSlot(Col, Row)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			.ContentPadding(FMargin(0.0f))
			.OnClicked_Lambda([this, CardType]() -> FReply
			{
				SetSelectedTemplate(CardType);
				return FReply::Handled();
			})
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				.BorderBackgroundColor_Lambda([this, CardType]()
				{
					return (SelectedTemplate == CardType)
						? FLinearColor(0.0f, 0.48f, 0.8f, 1.0f)
						: FLinearColor(0.20f, 0.20f, 0.22f, 1.0f);
				})
				.Padding(1.5f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
					.BorderBackgroundColor_Lambda([this, CardType]()
					{
						return (SelectedTemplate == CardType)
							? FLinearColor(0.10f, 0.20f, 0.32f, 1.0f)
							: FLinearColor(0.12f, 0.12f, 0.14f, 1.0f);
					})
					.Padding(FMargin(8.0f, 6.0f))
					[
						SNew(SVerticalBox)

						// Icon + Title
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 3.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(0.0f, 0.0f, 6.0f, 0.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(Card.Icon))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
							]
							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(Card.Title))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9.5f))
								.ColorAndOpacity_Lambda([this, CardType]()
								{
									return (SelectedTemplate == CardType)
										? FLinearColor::White
										: FLinearColor(0.85f, 0.85f, 0.85f, 1.0f);
								})
							]
						]

						// Description
						+ SVerticalBox::Slot()
						.FillHeight(1.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(Card.Description))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.0f))
							.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.65f, 1.0f))
							.AutoWrapText(true)
						]
					]
				]
			]
		];
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.BorderBackgroundColor(FLinearColor(0.08f, 0.08f, 0.10f, 1.0f))
		.Padding(12.0f)
		[
			SNew(SVerticalBox)

			// -------------------------------------------------------------
			// 1. Header Section: CHOOSE PARENT CLASS
			// -------------------------------------------------------------
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("CHOOSE PARENT CLASS")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10.5f))
					.ColorAndOpacity(FLinearColor(0.35f, 0.75f, 1.0f, 1.0f))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Select a parent class to inherit functionality and standard Unreal patterns.")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.5f))
					.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f))
				]
			]

			// -------------------------------------------------------------
			// 2. Class Cards Grid
			// -------------------------------------------------------------
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				CardsGrid
			]

			// -------------------------------------------------------------
			// 3. Class Name & Settings
			// -------------------------------------------------------------
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 4.0f, 0.0f, 6.0f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
				.BorderBackgroundColor(FLinearColor(0.12f, 0.12f, 0.14f, 1.0f))
				.Padding(FMargin(10.0f, 6.0f))
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(0.0f, 0.0f, 8.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Class Name:")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9.5f))
							.ColorAndOpacity(FLinearColor::White)
						]
						+ SHorizontalBox::Slot()
						.FillWidth(0.6f)
						.VAlign(VAlign_Center)
						[
							SAssignNew(ClassNameTextBox, SEditableTextBox)
							.Text(FText::FromString(ClassNameInput))
							.Font(FCppSyntaxHighlighterMarshaller::GetEditorFont(10.0f))
							.OnTextChanged_Lambda([this](const FText& NewText)
							{
								ClassNameInput = NewText.ToString().TrimStartAndEnd();
								UpdateGeneratedCode();
							})
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(16.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(SCheckBox)
							.IsChecked(bSeparatePublicPrivate ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
							.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
							{
								bSeparatePublicPrivate = (NewState == ECheckBoxState::Checked);
								UpdateGeneratedCode();
							})
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Separate Public / Private directories")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.5f))
							]
						]
					]

					// Realtime Resolution Banner
					+ SVerticalBox::Slot().AutoHeight()
					[
						SAssignNew(PathPreviewTextBlock, STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.5f))
						.ColorAndOpacity(FLinearColor(0.45f, 0.85f, 0.55f, 1.0f))
						.AutoWrapText(true)
					]
				]
			]

			// -------------------------------------------------------------
			// 4. Live Code Preview (Split Header & Source)
			// -------------------------------------------------------------
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(0.0f, 2.0f, 0.0f, 8.0f)
			[
				SNew(SSplitter)
				.Orientation(Orient_Horizontal)

				// Header Preview (.h)
				+ SSplitter::Slot()
				.Value(0.5f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(2.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("🔷 Header File (.h):")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8.5f))
						.ColorAndOpacity(FLinearColor(0.35f, 0.75f, 1.0f, 1.0f))
					]
					+ SVerticalBox::Slot().FillHeight(1.0f)
					[
						SAssignNew(HeaderPreviewTextBox, SMultiLineEditableTextBox)
						.Font(FCppSyntaxHighlighterMarshaller::GetEditorFont(8.5f))
						.IsReadOnly(true)
						.AutoWrapText(false)
					]
				]

				// Source Preview (.cpp)
				+ SSplitter::Slot()
				.Value(0.5f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(2.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("⚡ Source File (.cpp):")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8.5f))
						.ColorAndOpacity(FLinearColor(0.45f, 0.90f, 0.55f, 1.0f))
					]
					+ SVerticalBox::Slot().FillHeight(1.0f)
					[
						SAssignNew(SourcePreviewTextBox, SMultiLineEditableTextBox)
						.Font(FCppSyntaxHighlighterMarshaller::GetEditorFont(8.5f))
						.IsReadOnly(true)
						.AutoWrapText(false)
					]
				]
			]

			// -------------------------------------------------------------
			// 5. Bottom Action Buttons
			// -------------------------------------------------------------
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SSpacer)
				]

				// Cancel Button
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4.0f, 0.0f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Cancel")))
					.OnClicked(this, &SCreateClassDialog::OnCancelClicked)
				]

				// Create Button
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4.0f, 0.0f)
				[
					SNew(SButton)
					.ButtonColorAndOpacity(FLinearColor(0.12f, 0.52f, 0.95f, 1.0f))
					.Text(FText::FromString(TEXT("🚀 Create Class")))
					.OnClicked(this, &SCreateClassDialog::OnCreateClicked)
				]
			]
		]
	];

	UpdateGeneratedCode();
}

void SCreateClassDialog::GenerateCode(FString& OutHeaderCode, FString& OutSourceCode, FString& OutHeaderPath, FString& OutSourcePath) const
{
	FString ClassName, BaseName, HeaderFileName, SourceFileName;
	ResolveClassAndFileNames(SelectedTemplate, ClassNameInput, ClassName, BaseName, HeaderFileName, SourceFileName);

	// Module API macro detection
	FString BaseDir = TargetDirectory;
	FString ModuleApiMacro = TEXT("CIRCUITNODES_API");
	if (BaseDir.Contains(TEXT("Plugins/SlateLivePreview")) || BaseDir.Contains(TEXT("Plugins\\SlateLivePreview")) || BaseDir.Contains(TEXT("SlateLivePreview")))
	{
		ModuleApiMacro = TEXT("SLATELIVEPREVIEW_API");
	}

	FString HeaderFolder = BaseDir;
	FString SourceFolder = BaseDir;

	if (bSeparatePublicPrivate)
	{
		if (BaseDir.Contains(TEXT("Public")) || BaseDir.Contains(TEXT("Private")))
		{
			FString ParentDir = FPaths::GetPath(BaseDir);
			HeaderFolder = ParentDir / TEXT("Public");
			SourceFolder = ParentDir / TEXT("Private");
		}
		else
		{
			HeaderFolder = BaseDir / TEXT("Public");
			SourceFolder = BaseDir / TEXT("Private");
		}
	}

	OutHeaderPath = HeaderFolder / HeaderFileName;
	OutSourcePath = SourceFolder / SourceFileName;

	switch (SelectedTemplate)
	{
	case EClassTemplateType::Character:
		OutHeaderCode = FString::Printf(TEXT(
			"// Copyright (c) 2026. All Rights Reserved.\n\n"
			"#pragma once\n\n"
			"#include \"CoreMinimal.h\"\n"
			"#include \"GameFramework/Character.h\"\n"
			"#include \"%s.generated.h\"\n\n"
			"UCLASS()\n"
			"class %s %s : public ACharacter\n"
			"{\n"
			"\tGENERATED_BODY()\n\n"
			"public:\n"
			"\t%s();\n\n"
			"protected:\n"
			"\tvirtual void BeginPlay() override;\n\n"
			"public:\n"
			"\tvirtual void Tick(float DeltaTime) override;\n"
			"\tvirtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;\n"
			"};\n"
		), *BaseName, *ModuleApiMacro, *ClassName, *ClassName);

		OutSourceCode = FString::Printf(TEXT(
			"// Copyright (c) 2026. All Rights Reserved.\n\n"
			"#include \"%s.h\"\n\n"
			"%s::%s()\n"
			"{\n"
			"\tPrimaryActorTick.bCanEverTick = true;\n"
			"}\n\n"
			"void %s::BeginPlay()\n"
			"{\n"
			"\tSuper::BeginPlay();\n"
			"}\n\n"
			"void %s::Tick(float DeltaTime)\n"
			"{\n"
			"\tSuper::Tick(DeltaTime);\n"
			"}\n\n"
			"void %s::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)\n"
			"{\n"
			"\tSuper::SetupPlayerInputComponent(PlayerInputComponent);\n"
			"}\n"
		), *BaseName, *ClassName, *ClassName, *ClassName, *ClassName, *ClassName);
		break;

	case EClassTemplateType::Pawn:
		OutHeaderCode = FString::Printf(TEXT(
			"// Copyright (c) 2026. All Rights Reserved.\n\n"
			"#pragma once\n\n"
			"#include \"CoreMinimal.h\"\n"
			"#include \"GameFramework/Pawn.h\"\n"
			"#include \"%s.generated.h\"\n\n"
			"UCLASS()\n"
			"class %s %s : public APawn\n"
			"{\n"
			"\tGENERATED_BODY()\n\n"
			"public:\n"
			"\t%s();\n\n"
			"protected:\n"
			"\tvirtual void BeginPlay() override;\n\n"
			"public:\n"
			"\tvirtual void Tick(float DeltaTime) override;\n"
			"\tvirtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;\n"
			"};\n"
		), *BaseName, *ModuleApiMacro, *ClassName, *ClassName);

		OutSourceCode = FString::Printf(TEXT(
			"// Copyright (c) 2026. All Rights Reserved.\n\n"
			"#include \"%s.h\"\n\n"
			"%s::%s()\n"
			"{\n"
			"\tPrimaryActorTick.bCanEverTick = true;\n"
			"}\n\n"
			"void %s::BeginPlay()\n"
			"{\n"
			"\tSuper::BeginPlay();\n"
			"}\n\n"
			"void %s::Tick(float DeltaTime)\n"
			"{\n"
			"\tSuper::Tick(DeltaTime);\n"
			"}\n\n"
			"void %s::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)\n"
			"{\n"
			"\tSuper::SetupPlayerInputComponent(PlayerInputComponent);\n"
			"}\n"
		), *BaseName, *ClassName, *ClassName, *ClassName, *ClassName, *ClassName);
		break;

	case EClassTemplateType::AActorClass:
		OutHeaderCode = FString::Printf(TEXT(
			"// Copyright (c) 2026. All Rights Reserved.\n\n"
			"#pragma once\n\n"
			"#include \"CoreMinimal.h\"\n"
			"#include \"GameFramework/Actor.h\"\n"
			"#include \"%s.generated.h\"\n\n"
			"UCLASS()\n"
			"class %s %s : public AActor\n"
			"{\n"
			"\tGENERATED_BODY()\n\n"
			"public:\n"
			"\t%s();\n\n"
			"protected:\n"
			"\tvirtual void BeginPlay() override;\n\n"
			"public:\n"
			"\tvirtual void Tick(float DeltaTime) override;\n"
			"};\n"
		), *BaseName, *ModuleApiMacro, *ClassName, *ClassName);

		OutSourceCode = FString::Printf(TEXT(
			"// Copyright (c) 2026. All Rights Reserved.\n\n"
			"#include \"%s.h\"\n\n"
			"%s::%s()\n"
			"{\n"
			"\tPrimaryActorTick.bCanEverTick = true;\n"
			"}\n\n"
			"void %s::BeginPlay()\n"
			"{\n"
			"\tSuper::BeginPlay();\n"
			"}\n\n"
			"void %s::Tick(float DeltaTime)\n"
			"{\n"
			"\tSuper::Tick(DeltaTime);\n"
			"}\n"
		), *BaseName, *ClassName, *ClassName, *ClassName, *ClassName);
		break;

	case EClassTemplateType::UActorComponentClass:
		OutHeaderCode = FString::Printf(TEXT(
			"// Copyright (c) 2026. All Rights Reserved.\n\n"
			"#pragma once\n\n"
			"#include \"CoreMinimal.h\"\n"
			"#include \"Components/ActorComponent.h\"\n"
			"#include \"%s.generated.h\"\n\n"
			"UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))\n"
			"class %s %s : public UActorComponent\n"
			"{\n"
			"\tGENERATED_BODY()\n\n"
			"public:\n"
			"\t%s();\n\n"
			"protected:\n"
			"\tvirtual void BeginPlay() override;\n\n"
			"public:\n"
			"\tvirtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;\n"
			"};\n"
		), *BaseName, *ModuleApiMacro, *ClassName, *ClassName);

		OutSourceCode = FString::Printf(TEXT(
			"// Copyright (c) 2026. All Rights Reserved.\n\n"
			"#include \"%s.h\"\n\n"
			"%s::%s()\n"
			"{\n"
			"\tPrimaryComponentTick.bCanEverTick = true;\n"
			"}\n\n"
			"void %s::BeginPlay()\n"
			"{\n"
			"\tSuper::BeginPlay();\n"
			"}\n\n"
			"void %s::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)\n"
			"{\n"
			"\tSuper::TickComponent(DeltaTime, TickType, ThisTickFunction);\n"
			"}\n"
		), *BaseName, *ClassName, *ClassName, *ClassName, *ClassName);
		break;

	case EClassTemplateType::SceneComponent:
		OutHeaderCode = FString::Printf(TEXT(
			"// Copyright (c) 2026. All Rights Reserved.\n\n"
			"#pragma once\n\n"
			"#include \"CoreMinimal.h\"\n"
			"#include \"Components/SceneComponent.h\"\n"
			"#include \"%s.generated.h\"\n\n"
			"UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))\n"
			"class %s %s : public USceneComponent\n"
			"{\n"
			"\tGENERATED_BODY()\n\n"
			"public:\n"
			"\t%s();\n\n"
			"protected:\n"
			"\tvirtual void BeginPlay() override;\n\n"
			"public:\n"
			"\tvirtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;\n"
			"};\n"
		), *BaseName, *ModuleApiMacro, *ClassName, *ClassName);

		OutSourceCode = FString::Printf(TEXT(
			"// Copyright (c) 2026. All Rights Reserved.\n\n"
			"#include \"%s.h\"\n\n"
			"%s::%s()\n"
			"{\n"
			"\tPrimaryComponentTick.bCanEverTick = true;\n"
			"}\n\n"
			"void %s::BeginPlay()\n"
			"{\n"
			"\tSuper::BeginPlay();\n"
			"}\n\n"
			"void %s::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)\n"
			"{\n"
			"\tSuper::TickComponent(DeltaTime, TickType, ThisTickFunction);\n"
			"}\n"
		), *BaseName, *ClassName, *ClassName, *ClassName, *ClassName);
		break;

	case EClassTemplateType::SlateWidget:
		OutHeaderCode = FString::Printf(TEXT(
			"// Copyright (c) 2026. All Rights Reserved.\n\n"
			"#pragma once\n\n"
			"#include \"CoreMinimal.h\"\n"
			"#include \"Widgets/SCompoundWidget.h\"\n"
			"#include \"Widgets/DeclarativeSyntaxSupport.h\"\n\n"
			"class %s %s : public SCompoundWidget\n"
			"{\n"
			"public:\n"
			"\tSLATE_BEGIN_ARGS(%s) {}\n"
			"\tSLATE_END_ARGS()\n\n"
			"\tvoid Construct(const FArguments& InArgs);\n"
			"};\n"
		), *ModuleApiMacro, *ClassName, *ClassName);

		OutSourceCode = FString::Printf(TEXT(
			"// Copyright (c) 2026. All Rights Reserved.\n\n"
			"#include \"%s.h\"\n"
			"#include \"Widgets/Layout/SBorder.h\"\n"
			"#include \"Widgets/Layout/SBox.h\"\n"
			"#include \"Widgets/Text/STextBlock.h\"\n"
			"#include \"Styling/AppStyle.h\"\n\n"
			"void %s::Construct(const FArguments& InArgs)\n"
			"{\n"
			"\tChildSlot\n"
			"\t[\n"
			"\t\tSNew(SBorder)\n"
			"\t\t.BorderImage(FAppStyle::Get().GetBrush(\"ToolPanel.GroupBorder\"))\n"
			"\t\t.Padding(8.0f)\n"
			"\t\t[\n"
			"\t\t\tSNew(STextBlock)\n"
			"\t\t\t.Text(FText::FromString(TEXT(\"Hello from %s!\")))\n"
			"\t\t]\n"
			"\t];\n"
			"}\n"
		), *ClassName, *ClassName, *ClassName);
		break;

	case EClassTemplateType::UObjectClass:
		OutHeaderCode = FString::Printf(TEXT(
			"// Copyright (c) 2026. All Rights Reserved.\n\n"
			"#pragma once\n\n"
			"#include \"CoreMinimal.h\"\n"
			"#include \"UObject/NoExportTypes.h\"\n"
			"#include \"%s.generated.h\"\n\n"
			"UCLASS(BlueprintType, Blueprintable)\n"
			"class %s %s : public UObject\n"
			"{\n"
			"\tGENERATED_BODY()\n\n"
			"public:\n"
			"\t%s();\n"
			"};\n"
		), *BaseName, *ModuleApiMacro, *ClassName, *ClassName);

		OutSourceCode = FString::Printf(TEXT(
			"// Copyright (c) 2026. All Rights Reserved.\n\n"
			"#include \"%s.h\"\n\n"
			"%s::%s()\n"
			"{\n"
			"}\n"
		), *BaseName, *ClassName, *ClassName);
		break;

	case EClassTemplateType::UStructType:
		OutHeaderCode = FString::Printf(TEXT(
			"// Copyright (c) 2026. All Rights Reserved.\n\n"
			"#pragma once\n\n"
			"#include \"CoreMinimal.h\"\n"
			"#include \"%s.generated.h\"\n\n"
			"USTRUCT(BlueprintType)\n"
			"struct %s %s\n"
			"{\n"
			"\tGENERATED_BODY()\n\n"
			"\tUPROPERTY(EditAnywhere, BlueprintReadWrite, Category = \"Properties\")\n"
			"\tint32 ID = 0;\n\n"
			"\tUPROPERTY(EditAnywhere, BlueprintReadWrite, Category = \"Properties\")\n"
			"\tFString Name;\n"
			"};\n"
		), *BaseName, *ModuleApiMacro, *ClassName);

		OutSourceCode = FString::Printf(TEXT(
			"// Copyright (c) 2026. All Rights Reserved.\n\n"
			"#include \"%s.h\"\n\n"
			"// Struct %s definitions if needed\n"
		), *BaseName, *ClassName);
		break;

	case EClassTemplateType::EmptyCppClass:
	default:
		OutHeaderCode = FString::Printf(TEXT(
			"// Copyright (c) 2026. All Rights Reserved.\n\n"
			"#pragma once\n\n"
			"#include \"CoreMinimal.h\"\n\n"
			"class %s %s\n"
			"{\n"
			"public:\n"
			"\t%s();\n"
			"\t~%s();\n"
			"};\n"
		), *ModuleApiMacro, *ClassName, *ClassName, *ClassName);

		OutSourceCode = FString::Printf(TEXT(
			"// Copyright (c) 2026. All Rights Reserved.\n\n"
			"#include \"%s.h\"\n\n"
			"%s::%s()\n"
			"{\n"
			"}\n\n"
			"%s::~%s()\n"
			"{\n"
			"}\n"
		), *BaseName, *ClassName, *ClassName, *ClassName, *ClassName);
		break;
	}
}

void SCreateClassDialog::UpdateGeneratedCode()
{
	FString HeaderCode, SourceCode, HeaderPath, SourcePath;
	GenerateCode(HeaderCode, SourceCode, HeaderPath, SourcePath);

	if (HeaderPreviewTextBox.IsValid())
	{
		HeaderPreviewTextBox->SetText(FText::FromString(HeaderCode));
	}
	if (SourcePreviewTextBox.IsValid())
	{
		SourcePreviewTextBox->SetText(FText::FromString(SourceCode));
	}
	if (PathPreviewTextBlock.IsValid())
	{
		FString ClassName, BaseName, HeaderFileName, SourceFileName;
		ResolveClassAndFileNames(SelectedTemplate, ClassNameInput, ClassName, BaseName, HeaderFileName, SourceFileName);

		PathPreviewTextBlock->SetText(FText::FromString(
			FString::Printf(TEXT("✨ Class: %s  |  🔷 Header: %s  |  ⚡ Source: %s"),
				*ClassName, *FPaths::GetCleanFilename(HeaderPath), *FPaths::GetCleanFilename(SourcePath))
		));
	}
}

FReply SCreateClassDialog::OnCreateClicked()
{
	FString HeaderCode, SourceCode, HeaderPath, SourcePath;
	GenerateCode(HeaderCode, SourceCode, HeaderPath, SourcePath);

	// Ensure directories exist
	IFileManager& FileManager = IFileManager::Get();
	FileManager.MakeDirectory(*FPaths::GetPath(HeaderPath), true);
	FileManager.MakeDirectory(*FPaths::GetPath(SourcePath), true);

	// Write files
	if (!FFileHelper::SaveStringToFile(HeaderCode, *HeaderPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		return FReply::Handled();
	}

	if (!FFileHelper::SaveStringToFile(SourceCode, *SourcePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		return FReply::Handled();
	}

	if (OnClassCreated.IsBound())
	{
		OnClassCreated.Execute(HeaderPath, SourcePath);
	}

	if (TSharedPtr<SWindow> Window = ParentWindow.Pin())
	{
		Window->RequestDestroyWindow();
	}

	return FReply::Handled();
}

FReply SCreateClassDialog::OnCancelClicked()
{
	if (OnCanceled.IsBound())
	{
		OnCanceled.Execute();
	}

	if (TSharedPtr<SWindow> Window = ParentWindow.Pin())
	{
		Window->RequestDestroyWindow();
	}

	return FReply::Handled();
}

void SCreateClassDialog::OpenModal(const FString& InDefaultDir, FOnClassCreated InOnCreated)
{
	TSharedRef<SWindow> ModalWindow = SNew(SWindow)
		.Title(FText::FromString(TEXT("Add C++ Class")))
		.ClientSize(FVector2D(880.0f, 640.0f))
		.SupportsMinimize(false)
		.SupportsMaximize(false)
		.SizingRule(ESizingRule::UserSized);

	TSharedRef<SCreateClassDialog> Dialog = SNew(SCreateClassDialog)
		.DefaultDirectory(InDefaultDir)
		.OnClassCreated(InOnCreated)
		.OnCanceled_Lambda([ModalWindow]()
		{
			ModalWindow->RequestDestroyWindow();
		});

	Dialog->ParentWindow = ModalWindow;
	ModalWindow->SetContent(Dialog);

	FSlateApplication::Get().AddModalWindow(ModalWindow, FSlateApplication::Get().GetActiveTopLevelWindow());
}
