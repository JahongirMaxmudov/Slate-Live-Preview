// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"

class SLATELIVEPREVIEW_API FSlateWidgetSnapshotter
{
public:
	/**
	 * Takes a screenshot of the specified widget and saves it to a PNG file.
	 * Default path is [ProjectDir]/Saved/SlateLivePreview/preview.png
	 */
	static bool CaptureWidgetToPng(const TSharedRef<SWidget>& InWidget, FString& OutSavedPath, const FString& CustomFileName = TEXT("preview.png"));

	/** Gets the default snapshot directory */
	static FString GetSnapshotDirectory();
};
