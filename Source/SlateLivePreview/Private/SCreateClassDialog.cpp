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
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "CppSyntaxHighlighter.h"
#include "SlateLivePreviewStyle.h"
#include "UObject/UObjectIterator.h"

FString SCreateClassDialog::GetDefaultNameForTemplate(EClassTemplateType InTemplate)
{
	switch (InTemplate)
	{
	case EClassTemplateType::Character:             return TEXT("MyCharacter");
	case EClassTemplateType::Pawn:                  return TEXT("MyPawn");
	case EClassTemplateType::AActorClass:           return TEXT("MyActor");
	case EClassTemplateType::UActorComponentClass:  return TEXT("MyActorComponent");
	case EClassTemplateType::SceneComponent:        return TEXT("MySceneComponent");
	case EClassTemplateType::UserWidgetClass:       return TEXT("MyUserWidget");
	case EClassTemplateType::SlateWidget:           return TEXT("SMyCustomWidget");
	case EClassTemplateType::GameplayAbilityClass:  return TEXT("MyGameplayAbility");
	case EClassTemplateType::DataAssetClass:        return TEXT("MyDataAsset");
	case EClassTemplateType::AnimInstanceClass:     return TEXT("MyAnimInstance");
	case EClassTemplateType::UObjectClass:          return TEXT("MyObject");
	case EClassTemplateType::UStructType:           return TEXT("MyCustomStruct");
	case EClassTemplateType::EmptyCppClass:
	default:                                        return TEXT("MyClass");
	}
}

void SCreateClassDialog::ResolveClassAndFileNames(
	EClassTemplateType InTemplate,
	const FString& InInputName,
	TSharedPtr<FInheritableClassItem> InCustomItem,
	FString& OutClassName,
	FString& OutBaseName,
	FString& OutHeaderFileName,
	FString& OutSourceFileName)
{
	FString Raw = InInputName.TrimStartAndEnd();
	if (Raw.IsEmpty())
	{
		Raw = InCustomItem.IsValid() ? FString::Printf(TEXT("My%s"), *InCustomItem->CleanName) : GetDefaultNameForTemplate(InTemplate);
	}

	TCHAR Prefix = TEXT('\0');
	bool bForceFileHasPrefix = false;

	if (InCustomItem.IsValid())
	{
		Prefix = InCustomItem->Prefix;
		if (InCustomItem->bIsSlate)
		{
			bForceFileHasPrefix = true;
		}
	}
	else
	{
		switch (InTemplate)
		{
		case EClassTemplateType::Character:
		case EClassTemplateType::Pawn:
		case EClassTemplateType::AActorClass:
			Prefix = TEXT('A');
			break;
		case EClassTemplateType::UActorComponentClass:
		case EClassTemplateType::SceneComponent:
		case EClassTemplateType::UserWidgetClass:
		case EClassTemplateType::GameplayAbilityClass:
		case EClassTemplateType::DataAssetClass:
		case EClassTemplateType::AnimInstanceClass:
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
		default:
			Prefix = TEXT('\0');
			break;
		}
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
		OutHeaderFileName = OutBaseName + TEXT(".h");
		OutSourceFileName = OutBaseName + TEXT(".cpp");
	}
}

void SCreateClassDialog::InitCommonClasses()
{
	CommonClasses.Empty();

	auto AddCommon = [this](EClassTemplateType Type, const FString& ClassName, const FString& CleanName, const FString& BaseName, const FString& Header, const FString& Desc, TCHAR Prefix)
	{
		TSharedPtr<FInheritableClassItem> Item = MakeShared<FInheritableClassItem>();
		Item->TemplateType = Type;
		Item->ClassName = ClassName;
		Item->CleanName = CleanName;
		Item->BaseClassName = BaseName;
		Item->HeaderInclude = Header;
		Item->Description = Desc;
		Item->Prefix = Prefix;
		Item->bIsActor = (Prefix == TEXT('A'));
		Item->bIsUObject = (Prefix == TEXT('A') || Prefix == TEXT('U'));
		Item->bIsSlate = (Prefix == TEXT('S'));
		Item->bIsStruct = (Prefix == TEXT('F'));
		CommonClasses.Add(Item);
	};

	AddCommon(EClassTemplateType::Character, TEXT("ACharacter"), TEXT("Character"), TEXT("APawn"), TEXT("GameFramework/Character.h"), TEXT("Includes walking, running, jumping, and networking movement component."), TEXT('A'));
	AddCommon(EClassTemplateType::Pawn, TEXT("APawn"), TEXT("Pawn"), TEXT("AActor"), TEXT("GameFramework/Pawn.h"), TEXT("Actor that can be possessed by a PlayerController or AIController."), TEXT('A'));
	AddCommon(EClassTemplateType::AActorClass, TEXT("AActor"), TEXT("Actor"), TEXT("UObject"), TEXT("GameFramework/Actor.h"), TEXT("Base placeable or spawnable object in an Unreal Engine world."), TEXT('A'));
	AddCommon(EClassTemplateType::UActorComponentClass, TEXT("UActorComponent"), TEXT("ActorComponent"), TEXT("UObject"), TEXT("Components/ActorComponent.h"), TEXT("Reusable modular component that attaches behavior to any Actor."), TEXT('U'));
	AddCommon(EClassTemplateType::SceneComponent, TEXT("USceneComponent"), TEXT("SceneComponent"), TEXT("UActorComponent"), TEXT("Components/SceneComponent.h"), TEXT("Component with a 3D Transform (Location, Rotation, Scale)."), TEXT('U'));
	AddCommon(EClassTemplateType::UserWidgetClass, TEXT("UUserWidget"), TEXT("UserWidget"), TEXT("UWidget"), TEXT("Blueprint/UserWidget.h"), TEXT("Base class for UMG and Slate graphical UI menus, HUDs, and buttons."), TEXT('U'));
	AddCommon(EClassTemplateType::SlateWidget, TEXT("SCompoundWidget"), TEXT("SlateWidget"), TEXT("SWidget"), TEXT("Widgets/SCompoundWidget.h"), TEXT("Pure C++ declarative Slate UI widget for high performance Editor & Game UI."), TEXT('S'));
	AddCommon(EClassTemplateType::GameplayAbilityClass, TEXT("UGameplayAbility"), TEXT("GameplayAbility"), TEXT("UObject"), TEXT("Abilities/GameplayAbility.h"), TEXT("GAS (Gameplay Ability System) spell, skill, or passive ability."), TEXT('U'));
	AddCommon(EClassTemplateType::DataAssetClass, TEXT("UDataAsset"), TEXT("DataAsset"), TEXT("UObject"), TEXT("Engine/DataAsset.h"), TEXT("Lightweight asset storing designer data, stats, and configurations."), TEXT('U'));
	AddCommon(EClassTemplateType::AnimInstanceClass, TEXT("UAnimInstance"), TEXT("AnimInstance"), TEXT("UObject"), TEXT("Animation/AnimInstance.h"), TEXT("Controls skeletal mesh animation state machines, montages, and blending."), TEXT('U'));
	AddCommon(EClassTemplateType::UObjectClass, TEXT("UObject"), TEXT("Object"), TEXT(""), TEXT("UObject/NoExportTypes.h"), TEXT("Fundamental base class with Garbage Collection, Reflection, and RPCs."), TEXT('U'));
	AddCommon(EClassTemplateType::UStructType, TEXT("FMyCustomStruct"), TEXT("CustomStruct"), TEXT(""), TEXT(""), TEXT("Lightweight C++ data struct with Unreal USTRUCT() reflection."), TEXT('F'));
	AddCommon(EClassTemplateType::EmptyCppClass, TEXT("FMyClass"), TEXT("EmptyCppClass"), TEXT(""), TEXT(""), TEXT("Standard C++ class without Unreal UObject overhead."), TEXT('\0'));
}

void SCreateClassDialog::DiscoverEngineClasses()
{
	AllEngineClasses.Empty();

	// Curated comprehensive collection of popular engine classes
	struct FEnginePreset { const TCHAR* Name; const TCHAR* Clean; const TCHAR* Base; const TCHAR* Header; const TCHAR* Desc; TCHAR Prefix; };
	static const FEnginePreset Presets[] =
	{
		{ TEXT("APlayerController"), TEXT("PlayerController"), TEXT("AController"), TEXT("GameFramework/PlayerController.h"), TEXT("Manages player input, camera management, and HUD display."), TEXT('A') },
		{ TEXT("AGameModeBase"), TEXT("GameModeBase"), TEXT("AInfo"), TEXT("GameFramework/GameModeBase.h"), TEXT("Defines the match rules, spawn logic, and default pawn/controller."), TEXT('A') },
		{ TEXT("AGameStateBase"), TEXT("GameStateBase"), TEXT("AInfo"), TEXT("GameFramework/GameStateBase.h"), TEXT("Replicated state of the game match accessible by all clients."), TEXT('A') },
		{ TEXT("APlayerState"), TEXT("PlayerState"), TEXT("AInfo"), TEXT("GameFramework/PlayerState.h"), TEXT("Replicated player state (score, ping, player name)."), TEXT('A') },
		{ TEXT("AHUD"), TEXT("HUD"), TEXT("AActor"), TEXT("GameFramework/HUD.h"), TEXT("Classic 2D rendering canvas on top of player viewport."), TEXT('A') },
		{ TEXT("AAIController"), TEXT("AIController"), TEXT("AController"), TEXT("AIController.h"), TEXT("Controls automated NPC behavior trees, perceptions, and blackboard."), TEXT('A') },
		{ TEXT("UGameInstanceSubsystem"), TEXT("GameInstanceSubsystem"), TEXT("USubsystem"), TEXT("Subsystems/GameInstanceSubsystem.h"), TEXT("Global singleton lifecycle subsystem surviving level changes."), TEXT('U') },
		{ TEXT("UWorldSubsystem"), TEXT("WorldSubsystem"), TEXT("USubsystem"), TEXT("Subsystems/WorldSubsystem.h"), TEXT("Level/World lifecycle subsystem for managers, spawners, weather."), TEXT('U') },
		{ TEXT("UCameraComponent"), TEXT("CameraComponent"), TEXT("USceneComponent"), TEXT("Camera/CameraComponent.h"), TEXT("Camera viewport viewpoint for characters and vehicles."), TEXT('U') },
		{ TEXT("USpringArmComponent"), TEXT("SpringArmComponent"), TEXT("USceneComponent"), TEXT("GameFramework/SpringArmComponent.h"), TEXT("Third-person camera boom arm with collision avoidance."), TEXT('U') },
		{ TEXT("UStaticMeshComponent"), TEXT("StaticMeshComponent"), TEXT("UMeshComponent"), TEXT("Components/StaticMeshComponent.h"), TEXT("Renders 3D static geometry with material and physics support."), TEXT('U') },
		{ TEXT("USkeletalMeshComponent"), TEXT("SkeletalMeshComponent"), TEXT("USkinnedMeshComponent"), TEXT("Components/SkeletalMeshComponent.h"), TEXT("Renders rigged skeletal meshes with anim blueprint support."), TEXT('U') },
		{ TEXT("UBoxComponent"), TEXT("BoxComponent"), TEXT("UShapeComponent"), TEXT("Components/BoxComponent.h"), TEXT("Collision trigger box for overlaps and physics hits."), TEXT('U') },
		{ TEXT("USphereComponent"), TEXT("SphereComponent"), TEXT("UShapeComponent"), TEXT("Components/SphereComponent.h"), TEXT("Collision trigger sphere."), TEXT('U') },
		{ TEXT("USoundCue"), TEXT("SoundCue"), TEXT("USoundBase"), TEXT("Sound/SoundCue.h"), TEXT("Node-based audio graph player."), TEXT('U') },
		{ TEXT("UAudioComponent"), TEXT("AudioComponent"), TEXT("USceneComponent"), TEXT("Components/AudioComponent.h"), TEXT("Plays spatialized 3D sounds attached to actors."), TEXT('U') },
		{ TEXT("UPrimaryDataAsset"), TEXT("PrimaryDataAsset"), TEXT("UDataAsset"), TEXT("Engine/DataAsset.h"), TEXT("Data asset discoverable by the Asset Manager at runtime."), TEXT('U') },
		{ TEXT("USaveGame"), TEXT("SaveGame"), TEXT("UObject"), TEXT("GameFramework/SaveGame.h"), TEXT("Serializes game progress and player stats to disk."), TEXT('U') },
		{ TEXT("UBTTaskNode"), TEXT("BTTaskNode"), TEXT("UBTNode"), TEXT("BehaviorTree/BTTaskNode.h"), TEXT("Custom behavior tree AI task action."), TEXT('U') },
		{ TEXT("UBTService"), TEXT("BTService"), TEXT("UBTNode"), TEXT("BehaviorTree/BTService.h"), TEXT("Behavior tree background tick service checking conditions."), TEXT('U') },
		{ TEXT("UBTDecorator"), TEXT("BTDecorator"), TEXT("UBTNode"), TEXT("BehaviorTree/BTDecorator.h"), TEXT("Conditional gatekeeper on behavior tree branches."), TEXT('U') },
		{ TEXT("UCheatManager"), TEXT("CheatManager"), TEXT("UObject"), TEXT("GameFramework/CheatManager.h"), TEXT("Debugging cheat commands for developers."), TEXT('U') },
		{ TEXT("UDamageType"), TEXT("DamageType"), TEXT("UObject"), TEXT("GameFramework/DamageType.h"), TEXT("Defines damage properties (fire, physical, electrical)."), TEXT('U') }
	};

	for (const FEnginePreset& P : Presets)
	{
		TSharedPtr<FInheritableClassItem> Item = MakeShared<FInheritableClassItem>();
		Item->ClassName = P.Name;
		Item->CleanName = P.Clean;
		Item->BaseClassName = P.Base;
		Item->HeaderInclude = P.Header;
		Item->Description = P.Desc;
		Item->Prefix = P.Prefix;
		Item->bIsActor = (P.Prefix == TEXT('A'));
		Item->bIsUObject = true;
		Item->bIsProjectCustom = false;
		Item->TemplateType = EClassTemplateType::CustomInheritedClass;
		AllEngineClasses.Add(Item);
	}
}

void SCreateClassDialog::ScanProjectCustomClasses()
{
	ProjectCustomClasses.Empty();

	TArray<FString> SourceDirectories;
	SourceDirectories.Add(FPaths::ProjectDir() / TEXT("Source"));
	SourceDirectories.Add(FPaths::ProjectDir() / TEXT("Plugins"));

	for (const FString& SearchDir : SourceDirectories)
	{
		if (!IFileManager::Get().DirectoryExists(*SearchDir))
		{
			continue;
		}

		TArray<FString> FoundHeaders;
		IFileManager::Get().FindFilesRecursive(FoundHeaders, *SearchDir, TEXT("*.h"), true, false);

		for (const FString& HeaderPath : FoundHeaders)
		{
			// Skip intermediate, third-party, and generated files
			if (HeaderPath.Contains(TEXT("Intermediate")) || HeaderPath.Contains(TEXT(".generated.")) || HeaderPath.Contains(TEXT("ThirdParty")))
			{
				continue;
			}

			FString Content;
			if (FFileHelper::LoadFileToString(Content, *HeaderPath))
			{
				TArray<FString> Lines;
				Content.ParseIntoArrayLines(Lines);

				for (const FString& Line : Lines)
				{
					FString Trimmed = Line.TrimStartAndEnd();
					if (Trimmed.StartsWith(TEXT("class ")) || Trimmed.StartsWith(TEXT("struct ")))
					{
						// Look for inheritance: class [API] ClassName : public BaseName
						int32 ColonIdx = Trimmed.Find(TEXT(":"));
						if (ColonIdx != INDEX_NONE && Trimmed.Contains(TEXT("public ")))
						{
							FString LeftSide = Trimmed.Left(ColonIdx).TrimEnd();
							FString RightSide = Trimmed.Mid(ColonIdx + 1).TrimStart();

							TArray<FString> LeftTokens;
							LeftSide.ParseIntoArrayWS(LeftTokens);
							if (LeftTokens.Num() >= 2)
							{
								FString FullClassName = LeftTokens.Last();

								// Extract base class name
								TArray<FString> RightTokens;
								RightSide.ParseIntoArrayWS(RightTokens);
								FString BaseClassName;
								for (int32 t = 0; t < RightTokens.Num() - 1; ++t)
								{
									if (RightTokens[t] == TEXT("public"))
									{
										BaseClassName = RightTokens[t + 1];
										break;
									}
								}

								TCHAR Pfx = FullClassName.Len() >= 2 ? FullClassName[0] : TEXT('U');
								FString Clean = (Pfx == TEXT('A') || Pfx == TEXT('U') || Pfx == TEXT('S') || Pfx == TEXT('F'))
									? FullClassName.Mid(1) : FullClassName;

								TSharedPtr<FInheritableClassItem> CustomItem = MakeShared<FInheritableClassItem>();
								CustomItem->ClassName = FullClassName;
								CustomItem->CleanName = Clean;
								CustomItem->BaseClassName = BaseClassName;
								CustomItem->HeaderInclude = FPaths::GetCleanFilename(HeaderPath);
								CustomItem->Description = FString::Printf(TEXT("Custom Project Class defined in %s"), *FPaths::GetCleanFilename(HeaderPath));
								CustomItem->ModuleOrPath = FPaths::GetPath(HeaderPath);
								CustomItem->Prefix = Pfx;
								CustomItem->bIsProjectCustom = true;
								CustomItem->bIsActor = (Pfx == TEXT('A'));
								CustomItem->bIsSlate = (Pfx == TEXT('S'));
								CustomItem->bIsStruct = (Pfx == TEXT('F'));
								CustomItem->bIsUObject = (Pfx == TEXT('A') || Pfx == TEXT('U'));
								CustomItem->TemplateType = EClassTemplateType::CustomInheritedClass;

								ProjectCustomClasses.Add(CustomItem);
							}
						}
					}
				}
			}
		}
	}
}

void SCreateClassDialog::UpdateFilteredClasses()
{
	FilteredListClasses.Empty();

	const TArray<TSharedPtr<FInheritableClassItem>>& SourceList =
		(ActiveTab == EClassBrowserTab::ProjectCustom) ? ProjectCustomClasses : AllEngineClasses;

	for (const TSharedPtr<FInheritableClassItem>& Item : SourceList)
	{
		if (SearchFilterText.IsEmpty() ||
			Item->ClassName.Contains(SearchFilterText, ESearchCase::IgnoreCase) ||
			Item->BaseClassName.Contains(SearchFilterText, ESearchCase::IgnoreCase) ||
			Item->Description.Contains(SearchFilterText, ESearchCase::IgnoreCase))
		{
			FilteredListClasses.Add(Item);
		}
	}

	if (ClassListView.IsValid())
	{
		ClassListView->RequestListRefresh();
	}
}

void SCreateClassDialog::SetActiveTab(EClassBrowserTab InTab)
{
	ActiveTab = InTab;

	if (CommonCardsWidget.IsValid())
	{
		CommonCardsWidget->SetVisibility(ActiveTab == EClassBrowserTab::Common ? EVisibility::Visible : EVisibility::Collapsed);
	}
	if (ListBrowserWidget.IsValid())
	{
		ListBrowserWidget->SetVisibility(ActiveTab != EClassBrowserTab::Common ? EVisibility::Visible : EVisibility::Collapsed);
	}

	UpdateFilteredClasses();
}

void SCreateClassDialog::Construct(const FArguments& InArgs)
{
	TargetDirectory = InArgs._DefaultDirectory;
	OnClassCreated = InArgs._OnClassCreated;
	OnCanceled = InArgs._OnCanceled;

	InitCommonClasses();
	DiscoverEngineClasses();
	ScanProjectCustomClasses();

	if (CommonClasses.Num() > 0)
	{
		SelectedClassItem = CommonClasses[0];
		SelectedTemplate = CommonClasses[0]->TemplateType;
		ClassNameInput = TEXT("My") + CommonClasses[0]->CleanName;
	}

	// 1. Build Grid of Common Cards
	TSharedRef<SUniformGridPanel> CardsGrid = SNew(SUniformGridPanel).SlotPadding(FMargin(4.0f));
	const int32 NumColumns = 3;

	for (int32 i = 0; i < CommonClasses.Num(); ++i)
	{
		TSharedPtr<FInheritableClassItem> Item = CommonClasses[i];
		const int32 Col = i % NumColumns;
		const int32 Row = i / NumColumns;

		CardsGrid->AddSlot(Col, Row)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			.ContentPadding(FMargin(8.0f, 6.0f))
			.OnClicked_Lambda([this, Item]() -> FReply
			{
				SelectClassItem(Item);
				return FReply::Handled();
			})
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				.BorderBackgroundColor_Lambda([this, Item]()
				{
					return (SelectedClassItem == Item) ? FLinearColor(0.0f, 0.45f, 0.85f, 0.95f) : FLinearColor(0.12f, 0.12f, 0.14f, 0.8f);
				})
				.Padding(6.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 3.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Item->ClassName))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9.5f))
						.ColorAndOpacity(FLinearColor::White)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(Item->Description))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.0f))
						.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.65f, 1.0f))
						.AutoWrapText(true)
					]
				]
			]
		];
	}

	CommonCardsWidget = SNew(SScrollBox)
		.Orientation(Orient_Vertical)
		+ SScrollBox::Slot()
		[
			CardsGrid
		];

	// 2. Build Virtualized Searchable List for Engine and Project Classes
	ListBrowserWidget = SNew(SBox)
		.HeightOverride(210.0f)
		[
			SAssignNew(ClassListView, SListView<TSharedPtr<FInheritableClassItem>>)
			.ListItemsSource(&FilteredListClasses)
			.OnGenerateRow(this, &SCreateClassDialog::OnGenerateClassRow)
			.OnSelectionChanged(this, &SCreateClassDialog::OnClassSelectionChanged)
			.SelectionMode(ESelectionMode::Single)
		];
	ListBrowserWidget->SetVisibility(EVisibility::Collapsed);

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.BorderBackgroundColor(FLinearColor(0.08f, 0.08f, 0.10f, 1.0f))
		.Padding(12.0f)
		[
			SNew(SVerticalBox)

			// -----------------------------------------------------------------
			// 1. Header & Navigation Tabs
			// -----------------------------------------------------------------
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(SHorizontalBox)

				// Title
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 12.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("NEW C++ CLASS")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12.0f))
					.ColorAndOpacity(FLinearColor(0.35f, 0.75f, 1.0f, 1.0f))
				]

				// Tabs: Common | All Engine | Project Classes
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.0f, 0.0f)
				[
					SNew(SButton)
					.ButtonColorAndOpacity_Lambda([this]() { return ActiveTab == EClassBrowserTab::Common ? FLinearColor(0.1f, 0.45f, 0.85f, 1.0f) : FLinearColor(0.2f, 0.2f, 0.22f, 1.0f); })
					.ContentPadding(FMargin(8.0f, 3.0f))
					.Text(FText::FromString(FString::Printf(TEXT("Common Classes (%d)"), CommonClasses.Num())))
					.OnClicked_Lambda([this]() -> FReply
					{
						SetActiveTab(EClassBrowserTab::Common);
						return FReply::Handled();
					})
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.0f, 0.0f)
				[
					SNew(SButton)
					.ButtonColorAndOpacity_Lambda([this]() { return ActiveTab == EClassBrowserTab::AllEngine ? FLinearColor(0.1f, 0.45f, 0.85f, 1.0f) : FLinearColor(0.2f, 0.2f, 0.22f, 1.0f); })
					.ContentPadding(FMargin(8.0f, 3.0f))
					.Text(FText::FromString(FString::Printf(TEXT("All Engine Classes (%d)"), AllEngineClasses.Num())))
					.OnClicked_Lambda([this]() -> FReply
					{
						SetActiveTab(EClassBrowserTab::AllEngine);
						return FReply::Handled();
					})
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.0f, 0.0f)
				[
					SNew(SButton)
					.ButtonColorAndOpacity_Lambda([this]() { return ActiveTab == EClassBrowserTab::ProjectCustom ? FLinearColor(0.1f, 0.45f, 0.85f, 1.0f) : FLinearColor(0.2f, 0.2f, 0.22f, 1.0f); })
					.ContentPadding(FMargin(8.0f, 3.0f))
					.Text(FText::FromString(FString::Printf(TEXT("Project Custom Classes (%d)"), ProjectCustomClasses.Num())))
					.OnClicked_Lambda([this]() -> FReply
					{
						SetActiveTab(EClassBrowserTab::ProjectCustom);
						return FReply::Handled();
					})
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SSpacer)
				]

				// Search Box
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SBox)
					.WidthOverride(220.0f)
					[
						SAssignNew(ClassSearchBox, SSearchBox)
						.HintText(FText::FromString(TEXT("Search parent classes...")))
						.OnTextChanged_Lambda([this](const FText& Text)
						{
							SearchFilterText = Text.ToString().TrimStartAndEnd();
							if (!SearchFilterText.IsEmpty() && ActiveTab == EClassBrowserTab::Common)
							{
								SetActiveTab(EClassBrowserTab::AllEngine);
							}
							UpdateFilteredClasses();
						})
					]
				]
			]

			// -----------------------------------------------------------------
			// 2. Class Selector Container (Common Grid or Virtualized List)
			// -----------------------------------------------------------------
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(SBox)
				.HeightOverride(210.0f)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						CommonCardsWidget.ToSharedRef()
					]
					+ SOverlay::Slot()
					[
						ListBrowserWidget.ToSharedRef()
					]
				]
			]

			// -----------------------------------------------------------------
			// 3. Class Name & Inherited Parent Header
			// -----------------------------------------------------------------
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 2.0f, 0.0f, 6.0f)
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
						.FillWidth(0.5f)
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
								.Text(FText::FromString(TEXT("Separate Public / Private folders")))
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

			// -----------------------------------------------------------------
			// 4. Live Code Preview (Split Header & Source)
			// -----------------------------------------------------------------
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
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 4.0f, 0.0f)
						[
							SNew(SImage)
							.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.Tree.HeaderFile")))
							.DesiredSizeOverride(FVector2D(14.0f, 14.0f))
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Header File (.h):")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8.5f))
							.ColorAndOpacity(FLinearColor(0.35f, 0.75f, 1.0f, 1.0f))
						]
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
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 4.0f, 0.0f)
						[
							SNew(SImage)
							.Image(FSlateLivePreviewStyle::GetBrush(TEXT("SlateLivePreview.Tree.SourceFile")))
							.DesiredSizeOverride(FVector2D(14.0f, 14.0f))
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Source File (.cpp):")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8.5f))
							.ColorAndOpacity(FLinearColor(0.45f, 0.90f, 0.55f, 1.0f))
						]
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

			// -----------------------------------------------------------------
			// 5. Bottom Action Buttons
			// -----------------------------------------------------------------
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot().FillWidth(1.0f) [ SNew(SSpacer) ]

				// Cancel
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(6.0f, 0.0f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Cancel")))
					.OnClicked(this, &SCreateClassDialog::OnCancelClicked)
				]

				// Create Class
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.ButtonColorAndOpacity(FLinearColor(0.12f, 0.52f, 0.95f, 1.0f))
					.ContentPadding(FMargin(16.0f, 4.0f))
					.Text(FText::FromString(TEXT("Create Class")))
					.OnClicked(this, &SCreateClassDialog::OnCreateClicked)
				]
			]
		]
	];

	UpdateGeneratedCode();
}

TSharedRef<ITableRow> SCreateClassDialog::OnGenerateClassRow(TSharedPtr<FInheritableClassItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	FLinearColor BadgeColor = Item->bIsProjectCustom ? FLinearColor(0.2f, 0.8f, 0.4f, 1.0f) : FLinearColor(0.35f, 0.75f, 1.0f, 1.0f);
	FString BadgeText = Item->bIsProjectCustom ? TEXT("PROJECT") : TEXT("ENGINE");

	return SNew(STableRow<TSharedPtr<FInheritableClassItem>>, OwnerTable)
		.Padding(FMargin(6.0f, 3.0f))
		[
			SNew(SHorizontalBox)

			// Badge
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
				.BorderBackgroundColor(BadgeColor * 0.35f)
				.Padding(FMargin(4.0f, 1.0f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(BadgeText))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7.5f))
					.ColorAndOpacity(BadgeColor)
				]
			]

			// Class Name
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item->ClassName))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9.0f))
				.ColorAndOpacity(FLinearColor::White)
			]

			// Inherits Base
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item->BaseClassName.IsEmpty() ? TEXT("") : FString::Printf(TEXT(": %s"), *Item->BaseClassName)))
				.Font(FCoreStyle::GetDefaultFontStyle("Italic", 8.0f))
				.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SSpacer)
			]

			// Description or Header Path
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item->HeaderInclude))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.0f))
				.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f))
			]
		];
}

void SCreateClassDialog::OnClassSelectionChanged(TSharedPtr<FInheritableClassItem> Item, ESelectInfo::Type)
{
	if (Item.IsValid())
	{
		SelectClassItem(Item);
	}
}

void SCreateClassDialog::SetSelectedTemplate(EClassTemplateType InTemplate)
{
	SelectedTemplate = InTemplate;
	for (const TSharedPtr<FInheritableClassItem>& Item : CommonClasses)
	{
		if (Item->TemplateType == InTemplate)
		{
			SelectClassItem(Item);
			break;
		}
	}
}

void SCreateClassDialog::SelectClassItem(TSharedPtr<FInheritableClassItem> Item)
{
	if (!Item.IsValid())
	{
		return;
	}

	SelectedClassItem = Item;
	SelectedTemplate = Item->TemplateType;

	// Automatically suggest clean new class name without double prefix
	if (Item->bIsProjectCustom)
	{
		ClassNameInput = FString::Printf(TEXT("My%s"), *Item->CleanName);
	}
	else
	{
		ClassNameInput = FString::Printf(TEXT("My%s"), *Item->CleanName);
	}

	if (ClassNameTextBox.IsValid())
	{
		ClassNameTextBox->SetText(FText::FromString(ClassNameInput));
	}

	UpdateGeneratedCode();
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
		ResolveClassAndFileNames(SelectedTemplate, ClassNameInput, SelectedClassItem, ClassName, BaseName, HeaderFileName, SourceFileName);
		PathPreviewTextBlock->SetText(FText::FromString(FString::Printf(
			TEXT("Creates class %s (Header: %s | Source: %s)"),
			*ClassName,
			*FPaths::GetCleanFilename(HeaderPath),
			*FPaths::GetCleanFilename(SourcePath)
		)));
	}
}

void SCreateClassDialog::GenerateCode(FString& OutHeaderCode, FString& OutSourceCode, FString& OutHeaderPath, FString& OutSourcePath) const
{
	FString ClassName, BaseName, HeaderFileName, SourceFileName;
	ResolveClassAndFileNames(SelectedTemplate, ClassNameInput, SelectedClassItem, ClassName, BaseName, HeaderFileName, SourceFileName);

	FString BaseDir = TargetDirectory;
	if (BaseDir.IsEmpty())
	{
		BaseDir = FPaths::ProjectDir() / TEXT("Source");
	}

	// Detect Module Name and API Macro
	FString ModuleName = TEXT("GAME");
	FString CurrentDir = BaseDir;
	while (!CurrentDir.IsEmpty() && CurrentDir != FPaths::GetPath(CurrentDir))
	{
		TArray<FString> BuildFiles;
		IFileManager::Get().FindFiles(BuildFiles, *(CurrentDir / TEXT("*.Build.cs")), true, false);
		if (BuildFiles.Num() > 0)
		{
			FString BuildFileName = FPaths::GetCleanFilename(BuildFiles[0]);
			if (BuildFileName.EndsWith(TEXT(".Build.cs"), ESearchCase::IgnoreCase))
			{
				ModuleName = BuildFileName.LeftChop(9);
			}
			else
			{
				ModuleName = FPaths::GetBaseFilename(BuildFileName);
			}
			break;
		}
		CurrentDir = FPaths::GetPath(CurrentDir);
	}
	FString ModuleApiMacro = ModuleName.ToUpper() + TEXT("_API");

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

	// Custom Inherited Class (Engine or Project Class)
	if (SelectedClassItem.IsValid() && SelectedClassItem->TemplateType == EClassTemplateType::CustomInheritedClass)
	{
		FString ParentInclude = SelectedClassItem->HeaderInclude;
		if (!ParentInclude.IsEmpty())
		{
			ParentInclude = FString::Printf(TEXT("#include \"%s\"\n"), *ParentInclude);
		}

		if (SelectedClassItem->bIsStruct)
		{
			OutHeaderCode = FString::Printf(TEXT(
				"// Copyright (c) 2026. All Rights Reserved.\n\n"
				"#pragma once\n\n"
				"#include \"CoreMinimal.h\"\n"
				"%s"
				"#include \"%s.generated.h\"\n\n"
				"USTRUCT(BlueprintType)\n"
				"struct %s %s %s\n"
				"{\n"
				"\tGENERATED_BODY()\n\n"
				"\t%s();\n"
				"};\n"
			), *ParentInclude, *BaseName, *ModuleApiMacro, *ClassName,
			   SelectedClassItem->ClassName.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(": public %s"), *SelectedClassItem->ClassName),
			   *ClassName);

			OutSourceCode = FString::Printf(TEXT(
				"// Copyright (c) 2026. All Rights Reserved.\n\n"
				"#include \"%s.h\"\n\n"
				"%s::%s()\n"
				"{\n"
				"}\n"
			), *BaseName, *ClassName, *ClassName);
		}
		else if (SelectedClassItem->bIsSlate)
		{
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
				"#include \"Widgets/Text/STextBlock.h\"\n\n"
				"void %s::Construct(const FArguments& InArgs)\n"
				"{\n"
				"\tChildSlot\n"
				"\t[\n"
				"\t\tSNew(SBorder)\n"
				"\t\t[\n"
				"\t\t\tSNew(STextBlock)\n"
				"\t\t\t.Text(FText::FromString(TEXT(\"%s\")))\n"
				"\t\t]\n"
				"\t];\n"
				"}\n"
			), *ClassName, *ClassName, *ClassName);
		}
		else if (SelectedClassItem->bIsActor)
		{
			OutHeaderCode = FString::Printf(TEXT(
				"// Copyright (c) 2026. All Rights Reserved.\n\n"
				"#pragma once\n\n"
				"#include \"CoreMinimal.h\"\n"
				"%s"
				"#include \"%s.generated.h\"\n\n"
				"UCLASS()\n"
				"class %s %s : public %s\n"
				"{\n"
				"\tGENERATED_BODY()\n\n"
				"public:\n"
				"\t%s();\n\n"
				"protected:\n"
				"\tvirtual void BeginPlay() override;\n\n"
				"public:\n"
				"\tvirtual void Tick(float DeltaTime) override;\n"
				"};\n"
			), *ParentInclude, *BaseName, *ModuleApiMacro, *ClassName, *SelectedClassItem->ClassName, *ClassName);

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
		}
		else
		{
			// Generic UObject
			OutHeaderCode = FString::Printf(TEXT(
				"// Copyright (c) 2026. All Rights Reserved.\n\n"
				"#pragma once\n\n"
				"#include \"CoreMinimal.h\"\n"
				"%s"
				"#include \"%s.generated.h\"\n\n"
				"UCLASS(Blueprintable, BlueprintType)\n"
				"class %s %s : public %s\n"
				"{\n"
				"\tGENERATED_BODY()\n\n"
				"public:\n"
				"\t%s();\n"
				"};\n"
			), *ParentInclude, *BaseName, *ModuleApiMacro, *ClassName, *SelectedClassItem->ClassName, *ClassName);

			OutSourceCode = FString::Printf(TEXT(
				"// Copyright (c) 2026. All Rights Reserved.\n\n"
				"#include \"%s.h\"\n\n"
				"%s::%s()\n"
				"{\n"
				"}\n"
			), *BaseName, *ClassName, *ClassName);
		}
		return;
	}

	// Preset Common Templates
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
			"\t%s();\n"
			"};\n"
		), *BaseName, *ModuleApiMacro, *ClassName, *ClassName);

		OutSourceCode = FString::Printf(TEXT(
			"// Copyright (c) 2026. All Rights Reserved.\n\n"
			"#include \"%s.h\"\n\n"
			"%s::%s()\n"
			"{\n"
			"\tPrimaryComponentTick.bCanEverTick = false;\n"
			"}\n"
		), *BaseName, *ClassName, *ClassName);
		break;

	case EClassTemplateType::UserWidgetClass:
		OutHeaderCode = FString::Printf(TEXT(
			"// Copyright (c) 2026. All Rights Reserved.\n\n"
			"#pragma once\n\n"
			"#include \"CoreMinimal.h\"\n"
			"#include \"Blueprint/UserWidget.h\"\n"
			"#include \"%s.generated.h\"\n\n"
			"UCLASS()\n"
			"class %s %s : public UUserWidget\n"
			"{\n"
			"\tGENERATED_BODY()\n\n"
			"protected:\n"
			"\tvirtual void NativeConstruct() override;\n"
			"\tvirtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;\n"
			"};\n"
		), *BaseName, *ModuleApiMacro, *ClassName);

		OutSourceCode = FString::Printf(TEXT(
			"// Copyright (c) 2026. All Rights Reserved.\n\n"
			"#include \"%s.h\"\n\n"
			"void %s::NativeConstruct()\n"
			"{\n"
			"\tSuper::NativeConstruct();\n"
			"}\n\n"
			"void %s::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)\n"
			"{\n"
			"\tSuper::NativeTick(MyGeometry, InDeltaTime);\n"
			"}\n"
		), *BaseName, *ClassName, *ClassName);
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
			"#include \"Widgets/Text/STextBlock.h\"\n\n"
			"void %s::Construct(const FArguments& InArgs)\n"
			"{\n"
			"\tChildSlot\n"
			"\t[\n"
			"\t\tSNew(SBorder)\n"
			"\t\t[\n"
			"\t\t\tSNew(STextBlock)\n"
			"\t\t\t.Text(FText::FromString(TEXT(\"%s\")))\n"
			"\t\t]\n"
			"\t];\n"
			"}\n"
		), *ClassName, *ClassName, *ClassName);
		break;

	default:
		// Generic UObject fallback
		OutHeaderCode = FString::Printf(TEXT(
			"// Copyright (c) 2026. All Rights Reserved.\n\n"
			"#pragma once\n\n"
			"#include \"CoreMinimal.h\"\n"
			"#include \"UObject/NoExportTypes.h\"\n"
			"#include \"%s.generated.h\"\n\n"
			"UCLASS(Blueprintable, BlueprintType)\n"
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
	}
}

FReply SCreateClassDialog::OnCreateClicked()
{
	FString HeaderCode, SourceCode, HeaderPath, SourcePath;
	GenerateCode(HeaderCode, SourceCode, HeaderPath, SourcePath);

	if (FFileHelper::SaveStringToFile(HeaderCode, *HeaderPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM) &&
		FFileHelper::SaveStringToFile(SourceCode, *SourcePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		OnClassCreated.ExecuteIfBound(HeaderPath, SourcePath);

		if (ParentWindow.IsValid())
		{
			ParentWindow.Pin()->RequestDestroyWindow();
		}
		return FReply::Handled();
	}

	return FReply::Handled();
}

FReply SCreateClassDialog::OnCancelClicked()
{
	OnCanceled.ExecuteIfBound();
	if (ParentWindow.IsValid())
	{
		ParentWindow.Pin()->RequestDestroyWindow();
	}
	return FReply::Handled();
}

void SCreateClassDialog::OpenModal(const FString& InDefaultDir, FOnClassCreated InOnCreated)
{
	TSharedRef<SWindow> ModalWindow = SNew(SWindow)
		.Title(FText::FromString(TEXT("Add C++ Class - C++ Studio")))
		.ClientSize(FVector2D(880.0f, 720.0f))
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.SizingRule(ESizingRule::FixedSize);

	TSharedRef<SCreateClassDialog> Dialog = SNew(SCreateClassDialog)
		.DefaultDirectory(InDefaultDir)
		.OnClassCreated(InOnCreated)
		.OnCanceled(FSimpleDelegate::CreateLambda([ModalWindow]()
		{
			ModalWindow->RequestDestroyWindow();
		}));

	Dialog->ParentWindow = ModalWindow;
	ModalWindow->SetContent(Dialog);

	FSlateApplication::Get().AddModalWindow(ModalWindow, nullptr);
}
