// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#include "SlateWidgetSnapshotter.h"
#include "Framework/Application/SlateApplication.h"
#include "Modules/ModuleManager.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

FString FSlateWidgetSnapshotter::GetSnapshotDirectory()
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SlateLivePreview"));
}

bool FSlateWidgetSnapshotter::CaptureWidgetToPng(const TSharedRef<SWidget>& InWidget, FString& OutSavedPath, const FString& CustomFileName)
{
	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}

	TArray<FColor> ColorData;
	FIntVector OutSize(0, 0, 0);

	bool bCaptured = FSlateApplication::Get().TakeScreenshot(InWidget, ColorData, OutSize);
	if (!bCaptured || OutSize.X <= 0 || OutSize.Y <= 0 || ColorData.Num() == 0)
	{
		return false;
	}

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

	if (!ImageWrapper.IsValid())
	{
		return false;
	}

	if (!ImageWrapper->SetRaw(ColorData.GetData(), ColorData.Num() * sizeof(FColor), OutSize.X, OutSize.Y, ERGBFormat::BGRA, 8))
	{
		return false;
	}

	TArray64<uint8> Compressed = ImageWrapper->GetCompressed();
	if (Compressed.Num() == 0)
	{
		return false;
	}

	FString Dir = GetSnapshotDirectory();
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*Dir))
	{
		PlatformFile.CreateDirectoryTree(*Dir);
	}

	OutSavedPath = FPaths::Combine(Dir, CustomFileName.IsEmpty() ? TEXT("preview.png") : CustomFileName);
	return FFileHelper::SaveArrayToFile(MakeArrayView(Compressed.GetData(), Compressed.Num()), *OutSavedPath);
}
