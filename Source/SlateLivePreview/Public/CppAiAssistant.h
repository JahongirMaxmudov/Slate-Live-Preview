// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CppEditorSettings.h"
#include "Interfaces/IHttpRequest.h"
#include "Containers/Ticker.h"

DECLARE_DELEGATE_TwoParams(FOnAiCompletionReceived, const FString& /* CompletionText */, bool /* bSuccess */);
DECLARE_DELEGATE_TwoParams(FOnAiChatReceived, const FString& /* ResponseText */, bool /* bSuccess */);
DECLARE_DELEGATE_TwoParams(FOnAiTestResult, bool /* bSuccess */, const FString& /* Message */);
DECLARE_DELEGATE_TwoParams(FOnGitHubDeviceCodeReceived, const FString& /* UserCode */, const FString& /* VerificationUri */);
DECLARE_DELEGATE_TwoParams(FOnGitHubAuthComplete, bool /* bSuccess */, const FString& /* MessageOrUsername */);

/**
 * Universal AI Assistant & Copilot Client for C++ Studio.
 * Connects to Local Ollama, LM Studio, DeepSeek, OpenAI, GitHub Copilot, or any OpenAI-compatible API.
 */
class SLATELIVEPREVIEW_API FCppAiAssistant : public TSharedFromThis<FCppAiAssistant>
{
public:
	static FCppAiAssistant& Get();

	/**
	 * Request inline code completion (Ghost Text) for code continuation.
	 */
	void RequestInlineCompletion(
		const FString& InPrefix,
		const FString& InSuffix,
		const FString& InFilePath,
		FOnAiCompletionReceived InCallback
	);

	/**
	 * Send conversational message or code generation request.
	 */
	void SendChatMessage(
		const FString& InUserMessage,
		const FString& InCodeContext,
		const FString& InErrorContext,
		FOnAiChatReceived InCallback
	);

	/**
	 * Test connection to a specific or configured AI provider endpoint.
	 */
	void TestConnection(
		const FString& InEndpoint,
		const FString& InModel,
		const FString& InApiKey,
		FOnAiTestResult InCallback
	);
	void TestConnection(FOnAiTestResult InCallback);

	/** Cancel any ongoing completion request */
	void CancelPendingCompletion();

	bool IsRequestActive() const { return bIsRequestInFlight; }

	/**
	 * GitHub Copilot Device Authentication Flow (RFC 8628)
	 */
	void StartGitHubDeviceFlow(FOnGitHubDeviceCodeReceived InCodeReceived, FOnGitHubAuthComplete InComplete);
	void CancelGitHubAuth();
	bool IsGitHubAuthenticated() const;
	void SignOutOfGitHub();
	void FetchCopilotToken(const FString& InAccessToken, TFunction<void(bool, const FString&)> OnTokenFetched);

private:
	FCppAiAssistant() = default;
	~FCppAiAssistant() = default;

	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> ActiveCompletionRequest;
	bool bIsRequestInFlight = false;

	// GitHub Copilot Device Code Auth State
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> ActiveDeviceAuthRequest;
	FTSTicker::FDelegateHandle DevicePollTickerHandle;
	FString ActiveDeviceCode;
	int32 DevicePollInterval = 5;
	double DeviceAuthExpiresAt = 0.0;
	FOnGitHubAuthComplete ActiveAuthCompleteCallback;

	void PollGitHubDeviceToken();
	void FetchGitHubUsername(const FString& InAccessToken, TFunction<void(const FString&)> OnUsernameFetched);

	static FString CleanGeneratedCode(const FString& RawResponse);
};
