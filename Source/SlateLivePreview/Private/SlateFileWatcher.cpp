// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#include "SlateFileWatcher.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"

FSlateFileWatcher::FSlateFileWatcher(float InCheckInterval)
	: FTSTickerObjectBase(InCheckInterval)
{
}

FSlateFileWatcher::~FSlateFileWatcher()
{
	StopWatching();
}

void FSlateFileWatcher::StartWatching(const FString& InFilePath, const FOnSlateWatchedFileChanged& InCallback)
{
	WatchedFilePath = InFilePath;
	Callback = InCallback;
	bIsWatching = true;

	if (IFileManager::Get().FileExists(*WatchedFilePath))
	{
		LastFileTimeStamp = IFileManager::Get().GetTimeStamp(*WatchedFilePath);
	}
	else
	{
		LastFileTimeStamp = FDateTime::MinValue();
	}
}

void FSlateFileWatcher::StopWatching()
{
	bIsWatching = false;
	WatchedFilePath.Empty();
	Callback.Unbind();
}

bool FSlateFileWatcher::Tick(float DeltaTime)
{
	if (bIsWatching)
	{
		CheckFileModification();
	}
	return true; // Keep ticker active
}

void FSlateFileWatcher::CheckFileModification()
{
	if (WatchedFilePath.IsEmpty() || !IFileManager::Get().FileExists(*WatchedFilePath))
	{
		return;
	}

	FDateTime CurrentTimeStamp = IFileManager::Get().GetTimeStamp(*WatchedFilePath);
	if (CurrentTimeStamp > LastFileTimeStamp)
	{
		LastFileTimeStamp = CurrentTimeStamp;

		FString Content;
		if (FFileHelper::LoadFileToString(Content, *WatchedFilePath))
		{
			if (Callback.IsBound())
			{
				Callback.Execute(WatchedFilePath, Content);
			}
		}
	}
}
