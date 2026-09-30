// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#include "SSlateLivePreviewViewport.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "Styling/AppStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

TWeakPtr<SSlateLivePreviewViewport> SSlateLivePreviewViewport::ActiveViewport = nullptr;

TWeakPtr<SSlateLivePreviewViewport> SSlateLivePreviewViewport::GetActiveViewport()
{
	return ActiveViewport;
}

void SSlateLivePreviewViewport::Construct(const FArguments& InArgs)
{
	ActiveViewport = SharedThis(this);

	ConstraintBox = SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center);

	ViewportBackgroundBorder = SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.06f, 0.06f, 0.06f, 1.0f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				ConstraintBox.ToSharedRef()
			]
		];

	ChildSlot
	[
		ViewportBackgroundBorder.ToSharedRef()
	];

	UpdateBackgroundStyle();
	UpdateSizeConstraint();
}

void SSlateLivePreviewViewport::SetPreviewWidget(const TSharedRef<SWidget>& InWidget)
{
	CurrentWidget = InWidget;
	if (ConstraintBox.IsValid())
	{
		ConstraintBox->SetContent(InWidget);
	}
}

void SSlateLivePreviewViewport::SetBackgroundMode(ESlatePreviewBackground InMode)
{
	CurrentBgMode = InMode;
	UpdateBackgroundStyle();
}

void SSlateLivePreviewViewport::SetSizePreset(ESlatePreviewSizePreset InPreset)
{
	CurrentSizePreset = InPreset;
	UpdateSizeConstraint();
}

void SSlateLivePreviewViewport::UpdateBackgroundStyle()
{
	if (!ViewportBackgroundBorder.IsValid()) return;

	switch (CurrentBgMode)
	{
	case ESlatePreviewBackground::SlateDark:
		ViewportBackgroundBorder->SetBorderBackgroundColor(FLinearColor(0.06f, 0.06f, 0.06f, 1.0f));
		break;
	case ESlatePreviewBackground::Checkerboard:
		ViewportBackgroundBorder->SetBorderBackgroundColor(FLinearColor(0.12f, 0.12f, 0.12f, 1.0f));
		break;
	case ESlatePreviewBackground::Light:
		ViewportBackgroundBorder->SetBorderBackgroundColor(FLinearColor(0.85f, 0.85f, 0.85f, 1.0f));
		break;
	case ESlatePreviewBackground::Black:
		ViewportBackgroundBorder->SetBorderBackgroundColor(FLinearColor::Black);
		break;
	}
}

void SSlateLivePreviewViewport::UpdateSizeConstraint()
{
	if (!ConstraintBox.IsValid()) return;

	switch (CurrentSizePreset)
	{
	case ESlatePreviewSizePreset::Auto:
		ConstraintBox->SetWidthOverride(FOptionalSize());
		ConstraintBox->SetHeightOverride(FOptionalSize());
		break;
	case ESlatePreviewSizePreset::Fixed400x300:
		ConstraintBox->SetWidthOverride(FOptionalSize(400.0f));
		ConstraintBox->SetHeightOverride(FOptionalSize(300.0f));
		break;
	case ESlatePreviewSizePreset::Fixed800x600:
		ConstraintBox->SetWidthOverride(FOptionalSize(800.0f));
		ConstraintBox->SetHeightOverride(FOptionalSize(600.0f));
		break;
	case ESlatePreviewSizePreset::Mobile390x844:
		ConstraintBox->SetWidthOverride(FOptionalSize(390.0f));
		ConstraintBox->SetHeightOverride(FOptionalSize(844.0f));
		break;
	}
}

bool SSlateLivePreviewViewport::SaveSnapshotToFile(const FString& InFilePath)
{
	FString TargetPath = InFilePath;
	if (TargetPath.IsEmpty())
	{
		TargetPath = FPaths::ProjectSavedDir() / TEXT("SlateLivePreview/preview.png");
	}

	TSharedPtr<SWidget> WidgetToCapture = CurrentWidget.IsValid() ? CurrentWidget : AsShared();

	TArray<FColor> ColorData;
	FIntVector Size;
	if (!FSlateApplication::Get().TakeScreenshot(WidgetToCapture.ToSharedRef(), ColorData, Size))
	{
		UE_LOG(LogTemp, Warning, TEXT("[SlateLivePreview] Failed to take screenshot of viewport widget."));
		return false;
	}

	if (ColorData.Num() == 0 || Size.X <= 0 || Size.Y <= 0)
	{
		return false;
	}

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	if (ImageWrapper.IsValid() && ImageWrapper->SetRaw(ColorData.GetData(), ColorData.Num() * sizeof(FColor), Size.X, Size.Y, ERGBFormat::BGRA, 8))
	{
		const TArray64<uint8>& CompressedData = ImageWrapper->GetCompressed();
		if (FFileHelper::SaveArrayToFile(CompressedData, *TargetPath))
		{
			UE_LOG(LogTemp, Display, TEXT("[SlateLivePreview] Snapshot successfully saved to: %s (%dx%d)"), *TargetPath, Size.X, Size.Y);
			return true;
		}
	}

	return false;
}

