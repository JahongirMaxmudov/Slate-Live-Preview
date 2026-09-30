// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"

DECLARE_DELEGATE_TwoParams(FOnSlateWatchedFileChanged, const FString& /* FilePath */, const FString& /* Content */);

class SLATELIVEPREVIEW_API FSlateFileWatcher : public FTSTickerObjectBase
{
public:
	explicit FSlateFileWatcher(float InCheckInterval = 0.25f);
	virtual ~FSlateFileWatcher() override;

	void StartWatching(const FString& InFilePath, const FOnSlateWatchedFileChanged& InCallback);
	void StopWatching();

	bool IsWatching() const { return bIsWatching; }
	const FString& GetWatchedFilePath() const { return WatchedFilePath; }

	// FTSTickerObjectBase interface
	virtual bool Tick(float DeltaTime) override;

private:
	FString WatchedFilePath;
	FDateTime LastFileTimeStamp;
	FOnSlateWatchedFileChanged Callback;
	bool bIsWatching = false;

	void CheckFileModification();
};
