// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

enum class EClassTemplateType : uint8
{
	Character,
	Pawn,
	AActorClass,
	UActorComponentClass,
	SceneComponent,
	SlateWidget,
	UObjectClass,
	UStructType,
	EmptyCppClass
};

DECLARE_DELEGATE_TwoParams(FOnClassCreated, const FString& /* HeaderPath */, const FString& /* SourcePath */);

class SEditableTextBox;
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
	void SetClassName(const FString& InName) { ClassNameInput = InName; UpdateGeneratedCode(); }
	void GenerateCode(FString& OutHeaderCode, FString& OutSourceCode, FString& OutHeaderPath, FString& OutSourcePath) const;

	static void ResolveClassAndFileNames(
		EClassTemplateType InTemplate,
		const FString& InInputName,
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

	EClassTemplateType SelectedTemplate = EClassTemplateType::Character;
	FString ClassNameInput = TEXT("MyCharacter");
	bool bSeparatePublicPrivate = true;

	TSharedPtr<SEditableTextBox> ClassNameTextBox;
	TSharedPtr<SMultiLineEditableTextBox> HeaderPreviewTextBox;
	TSharedPtr<SMultiLineEditableTextBox> SourcePreviewTextBox;
	TSharedPtr<STextBlock> PathPreviewTextBlock;

	void UpdateGeneratedCode();
	FReply OnCreateClicked();
	FReply OnCancelClicked();
};
