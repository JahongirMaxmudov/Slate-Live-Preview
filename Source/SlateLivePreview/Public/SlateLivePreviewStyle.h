// Copyright (c) 2026 JMPingvin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"

/**
 * Custom Slate Style Set managing high-quality custom vector/PNG icons
 * for C++ Studio & Slate Live Preview without Unicode emojis.
 */
class SLATELIVEPREVIEW_API FSlateLivePreviewStyle
{
public:
	static void Initialize();
	static void Shutdown();
	static const ISlateStyle& Get();
	static FName GetStyleSetName();
	static const FSlateBrush* GetBrush(FName PropertyName, const ANSICHAR* Specifier = nullptr);

private:
	static TSharedRef<class FSlateStyleSet> Create();
	static TSharedPtr<class FSlateStyleSet> StyleInstance;
};
