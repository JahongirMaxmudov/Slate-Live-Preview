// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

enum class EClassTemplateType : uint8
{
	Character,
	Pawn,
	AActorClass,
	UActorComponentClass,
	SceneComponent,
	UserWidgetClass,
	SlateWidget,
	GameplayAbilityClass,
	DataAssetClass,
	AnimInstanceClass,
	UObjectClass,
	UStructType,
	EmptyCppClass,
	CustomInheritedClass
};

enum class EClassBrowserTab : uint8
{
	Common = 0,
	AllEngine,
	ProjectCustom
};

struct FInheritableClassItem
{
	FString ClassName;       // e.g. "ACharacter", "UUserWidget", "AMyCustomCharacter"
	FString CleanName;       // e.g. "Character", "UserWidget", "MyCustomCharacter"
	FString BaseClassName;   // e.g. "APawn", "UWidget", "ACharacter"
	FString HeaderInclude;   // e.g. "GameFramework/Character.h", "Blueprint/UserWidget.h", "MyCustomCharacter.h"
	FString Description;     // Description
	FString ModuleOrPath;    // Where it is located
	TCHAR Prefix = TEXT('U'); // 'A', 'U', 'S', 'F'
	bool bIsProjectCustom = false;
	bool bIsActor = false;
	bool bIsUObject = true;
	bool bIsSlate = false;
	bool bIsStruct = false;
	EClassTemplateType TemplateType = EClassTemplateType::CustomInheritedClass;
};

DECLARE_DELEGATE_TwoParams(FOnClassCreated, const FString& /* HeaderPath */, const FString& /* SourcePath */);

class SEditableTextBox;
class SSearchBox;
class SMultiLineEditableTextBox;
class STextBlock;
class SWindow;

class SLATELIVEPREVIEW_API SCreateClassDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCreateClassDialog)
		: _DefaultDirectory(TEXT(""))
	{}
		SLATE_ARGUMENT(FString, DefaultDirectory)
		SLATE_EVENT(FOnClassCreated, OnClassCreated)
		SLATE_EVENT(FSimpleDelegate, OnCanceled)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	static void OpenModal(const FString& InDefaultDir, FOnClassCreated InOnCreated);

	void SetSelectedTemplate(EClassTemplateType InTemplate);
	void SelectClassItem(TSharedPtr<FInheritableClassItem> Item);
	void SetClassName(const FString& InName) { ClassNameInput = InName; UpdateGeneratedCode(); }
	void GenerateCode(FString& OutHeaderCode, FString& OutSourceCode, FString& OutHeaderPath, FString& OutSourcePath) const;

	static void ResolveClassAndFileNames(
		EClassTemplateType InTemplate,
		const FString& InInputName,
		TSharedPtr<FInheritableClassItem> InCustomItem,
		FString& OutClassName,
		FString& OutBaseName,
		FString& OutHeaderFileName,
		FString& OutSourceFileName
	);

	static FString GetDefaultNameForTemplate(EClassTemplateType InTemplate);

private:
	FString TargetDirectory;
	FOnClassCreated OnClassCreated;
	FSimpleDelegate OnCanceled;
	TWeakPtr<SWindow> ParentWindow;

	EClassBrowserTab ActiveTab = EClassBrowserTab::Common;
	EClassTemplateType SelectedTemplate = EClassTemplateType::Character;
	TSharedPtr<FInheritableClassItem> SelectedClassItem;

	FString ClassNameInput = TEXT("MyCharacter");
	FString SearchFilterText;
	bool bSeparatePublicPrivate = true;

	// Class Catalogs
	TArray<TSharedPtr<FInheritableClassItem>> CommonClasses;
	TArray<TSharedPtr<FInheritableClassItem>> AllEngineClasses;
	TArray<TSharedPtr<FInheritableClassItem>> ProjectCustomClasses;
	TArray<TSharedPtr<FInheritableClassItem>> FilteredListClasses;

	// Widgets
	TSharedPtr<SEditableTextBox> ClassNameTextBox;
	TSharedPtr<SSearchBox> ClassSearchBox;
	TSharedPtr<SListView<TSharedPtr<FInheritableClassItem>>> ClassListView;
	TSharedPtr<SWidget> CommonCardsWidget;
	TSharedPtr<SWidget> ListBrowserWidget;
	TSharedPtr<SMultiLineEditableTextBox> HeaderPreviewTextBox;
	TSharedPtr<SMultiLineEditableTextBox> SourcePreviewTextBox;
	TSharedPtr<STextBlock> PathPreviewTextBlock;
	TSharedPtr<STextBlock> SelectedParentHeaderTextBlock;

	void InitCommonClasses();
	void DiscoverEngineClasses();
	void ScanProjectCustomClasses();
	void UpdateFilteredClasses();
	void SetActiveTab(EClassBrowserTab InTab);

	TSharedRef<ITableRow> OnGenerateClassRow(TSharedPtr<FInheritableClassItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void OnClassSelectionChanged(TSharedPtr<FInheritableClassItem> Item, ESelectInfo::Type SelectInfo);

	void UpdateGeneratedCode();
	FReply OnCreateClicked();
	FReply OnCancelClicked();
};
