// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#include "CppAiAssistant.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonReader.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "HAL/PlatformApplicationMisc.h"
#include "HAL/PlatformProcess.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Containers/Ticker.h"

FCppAiAssistant& FCppAiAssistant::Get()
{
	static FCppAiAssistant Instance;
	return Instance;
}

void FCppAiAssistant::CancelPendingCompletion()
{
	if (ActiveCompletionRequest.IsValid() && ActiveCompletionRequest->GetStatus() == EHttpRequestStatus::Processing)
	{
		ActiveCompletionRequest->CancelRequest();
		ActiveCompletionRequest.Reset();
	}
	bIsRequestInFlight = false;
}

FString FCppAiAssistant::CleanGeneratedCode(const FString& RawResponse)
{
	FString Code = RawResponse.TrimStartAndEnd();

	// Strip markdown code block fences if present
	if (Code.StartsWith(TEXT("```cpp")))
	{
		Code = Code.Mid(6).TrimStart();
	}
	else if (Code.StartsWith(TEXT("```")))
	{
		Code = Code.Mid(3).TrimStart();
	}

	if (Code.EndsWith(TEXT("```")))
	{
		Code = Code.LeftChop(3).TrimEnd();
	}

	return Code;
}

void FCppAiAssistant::RequestInlineCompletion(
	const FString& InPrefix,
	const FString& InSuffix,
	const FString& InFilePath,
	FOnAiCompletionReceived InCallback)
{
	const FCppEditorSettings& Settings = FCppEditorSettings::Get();
	if (!Settings.bEnableAiInlineCompletion)
	{
		InCallback.ExecuteIfBound(FString(), false);
		return;
	}

	CancelPendingCompletion();

	FString Endpoint = Settings.AiEndpoint.TrimEnd();
	if (!Endpoint.EndsWith(TEXT("/chat/completions")))
	{
		if (Endpoint.EndsWith(TEXT("/")))
		{
			Endpoint += TEXT("chat/completions");
		}
		else
		{
			Endpoint += TEXT("/chat/completions");
		}
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Endpoint);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetTimeout(5.0f); // Fast 5-second timeout for snappy inline code completion

	if (Settings.AiProvider == EAiProvider::GitHubCopilot || Endpoint.Contains(TEXT("githubcopilot.com")))
	{
		Request->SetHeader(TEXT("Editor-Version"), TEXT("vscode/1.85.0"));
		Request->SetHeader(TEXT("Editor-Plugin-Version"), TEXT("copilot-chat/0.11.1"));
		Request->SetHeader(TEXT("Copilot-Integration-Id"), TEXT("vscode-chat"));
		Request->SetHeader(TEXT("User-Agent"), TEXT("GitHubCopilot/1.138.0"));

		FString AuthToken = Settings.CopilotSessionToken;
		if (AuthToken.IsEmpty())
		{
			AuthToken = Settings.GitHubAccessToken;
		}
		if (AuthToken.IsEmpty())
		{
			AuthToken = Settings.AiApiKey;
		}
		if (!AuthToken.IsEmpty())
		{
			Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *AuthToken));
		}
	}
	else if (!Settings.AiApiKey.IsEmpty())
	{
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *Settings.AiApiKey));
	}

	// Tailor prefix and suffix to avoid overloading tokens
	FString CleanPrefix = InPrefix.Len() > 2000 ? InPrefix.Right(2000) : InPrefix;
	FString CleanSuffix = InSuffix.Len() > 1000 ? InSuffix.Left(1000) : InSuffix;
	FString FileName = FPaths::GetCleanFilename(InFilePath);

	// Construct system and user prompt for high-precision inline ghost text completion
	FString SystemPrompt = TEXT(
		"You are an expert C++20 and Unreal Engine 5.8 code completion engine.\n"
		"Provide ONLY the continuation code that should be inserted directly at the cursor.\n"
		"Do NOT repeat any code from before the cursor. Do NOT provide explanations or markdown fences. Output plain raw code only."
	);

	FString UserPrompt = FString::Printf(
		TEXT("// File: %s\n"
		     "// Code before cursor:\n%s"
		     "<CURSOR>"
		     "\n// Code after cursor:\n%s"),
		*FileName, *CleanPrefix, *CleanSuffix
	);

	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	RootObject->SetStringField(TEXT("model"), Settings.AiModel.IsEmpty() ? TEXT("deepseek-coder") : Settings.AiModel);
	RootObject->SetNumberField(TEXT("temperature"), Settings.AiTemperature);
	RootObject->SetNumberField(TEXT("max_tokens"), FMath::Clamp(Settings.AiMaxTokens, 16, 256));

	TArray<TSharedPtr<FJsonValue>> MessagesArray;

	TSharedPtr<FJsonObject> SystemMsg = MakeShared<FJsonObject>();
	SystemMsg->SetStringField(TEXT("role"), TEXT("system"));
	SystemMsg->SetStringField(TEXT("content"), SystemPrompt);
	MessagesArray.Add(MakeShared<FJsonValueObject>(SystemMsg));

	TSharedPtr<FJsonObject> UserMsg = MakeShared<FJsonObject>();
	UserMsg->SetStringField(TEXT("role"), TEXT("user"));
	UserMsg->SetStringField(TEXT("content"), UserPrompt);
	MessagesArray.Add(MakeShared<FJsonValueObject>(UserMsg));

	RootObject->SetArrayField(TEXT("messages"), MessagesArray);

	FString RequestPayload;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestPayload);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

	Request->SetContentAsString(RequestPayload);

	bIsRequestInFlight = true;
	ActiveCompletionRequest = Request;

	Request->OnProcessRequestComplete().BindLambda(
		[this, InCallback](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bConnectedSuccessfully)
		{
			bIsRequestInFlight = false;
			ActiveCompletionRequest.Reset();

			if (!bConnectedSuccessfully || !HttpResponse.IsValid() || HttpResponse->GetResponseCode() < 200 || HttpResponse->GetResponseCode() >= 300)
			{
				InCallback.ExecuteIfBound(FString(), false);
				return;
			}

			FString ResponseBody = HttpResponse->GetContentAsString();
			TSharedPtr<FJsonObject> JsonResponse;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);

			if (FJsonSerializer::Deserialize(Reader, JsonResponse) && JsonResponse.IsValid())
			{
				const TArray<TSharedPtr<FJsonValue>>* Choices = nullptr;
				if (JsonResponse->TryGetArrayField(TEXT("choices"), Choices) && Choices && Choices->Num() > 0)
				{
					TSharedPtr<FJsonObject> FirstChoice = (*Choices)[0]->AsObject();
					if (FirstChoice.IsValid())
					{
						TSharedPtr<FJsonObject> MessageObj = FirstChoice->GetObjectField(TEXT("message"));
						if (MessageObj.IsValid() && MessageObj->HasTypedField<EJson::String>(TEXT("content")))
						{
							FString RawContent = MessageObj->GetStringField(TEXT("content"));
							FString CleanCode = CleanGeneratedCode(RawContent);
							InCallback.ExecuteIfBound(CleanCode, !CleanCode.IsEmpty());
							return;
						}
					}
				}
			}

			InCallback.ExecuteIfBound(FString(), false);
		}
	);

	Request->ProcessRequest();
}

void FCppAiAssistant::SendChatMessage(
	const FString& InUserMessage,
	const FString& InCodeContext,
	const FString& InErrorContext,
	FOnAiChatReceived InCallback)
{
	const FCppEditorSettings& Settings = FCppEditorSettings::Get();
	FString Endpoint = Settings.AiEndpoint.TrimEnd();
	if (!Endpoint.EndsWith(TEXT("/chat/completions")))
	{
		if (Endpoint.EndsWith(TEXT("/")))
		{
			Endpoint += TEXT("chat/completions");
		}
		else
		{
			Endpoint += TEXT("/chat/completions");
		}
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Endpoint);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetTimeout(45.0f);

	if (Settings.AiProvider == EAiProvider::GitHubCopilot || Endpoint.Contains(TEXT("githubcopilot.com")))
	{
		Request->SetHeader(TEXT("Editor-Version"), TEXT("vscode/1.85.0"));
		Request->SetHeader(TEXT("Editor-Plugin-Version"), TEXT("copilot-chat/0.11.1"));
		Request->SetHeader(TEXT("Copilot-Integration-Id"), TEXT("vscode-chat"));
		Request->SetHeader(TEXT("User-Agent"), TEXT("GitHubCopilot/1.138.0"));

		FString AuthToken = Settings.CopilotSessionToken;
		if (AuthToken.IsEmpty())
		{
			AuthToken = Settings.GitHubAccessToken;
		}
		if (AuthToken.IsEmpty())
		{
			AuthToken = Settings.AiApiKey;
		}
		if (!AuthToken.IsEmpty())
		{
			Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *AuthToken));
		}
	}
	else if (!Settings.AiApiKey.IsEmpty())
	{
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *Settings.AiApiKey));
	}

	FString SystemPrompt = TEXT(
		"You are C++ Studio AI Assistant, an elite Unreal Engine 5.8 and Slate UI framework expert.\n"
		"You write production-grade, performant, modern C++20 and Slate code.\n"
		"Always format code cleanly inside ```cpp ... ``` blocks.\n"
		"Be concise, direct, helpful, and provide complete implementations without placeholders."
	);

	FString PromptContent;
	if (!InErrorContext.IsEmpty())
	{
		PromptContent += FString::Printf(TEXT("Compiler Error:\n%s\n\n"), *InErrorContext);
	}
	if (!InCodeContext.IsEmpty())
	{
		PromptContent += FString::Printf(TEXT("Source Code Context:\n```cpp\n%s\n```\n\n"), *InCodeContext);
	}
	PromptContent += InUserMessage;

	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	RootObject->SetStringField(TEXT("model"), Settings.AiModel.IsEmpty() ? TEXT("deepseek-coder") : Settings.AiModel);
	RootObject->SetNumberField(TEXT("temperature"), 0.3);
	RootObject->SetNumberField(TEXT("max_tokens"), 2048);

	TArray<TSharedPtr<FJsonValue>> MessagesArray;

	TSharedPtr<FJsonObject> SystemMsg = MakeShared<FJsonObject>();
	SystemMsg->SetStringField(TEXT("role"), TEXT("system"));
	SystemMsg->SetStringField(TEXT("content"), SystemPrompt);
	MessagesArray.Add(MakeShared<FJsonValueObject>(SystemMsg));

	TSharedPtr<FJsonObject> UserMsg = MakeShared<FJsonObject>();
	UserMsg->SetStringField(TEXT("role"), TEXT("user"));
	UserMsg->SetStringField(TEXT("content"), PromptContent);
	MessagesArray.Add(MakeShared<FJsonValueObject>(UserMsg));

	RootObject->SetArrayField(TEXT("messages"), MessagesArray);

	FString RequestPayload;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestPayload);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

	Request->SetContentAsString(RequestPayload);

	Request->OnProcessRequestComplete().BindLambda(
		[InCallback](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bConnectedSuccessfully)
		{
			if (!bConnectedSuccessfully || !HttpResponse.IsValid())
			{
				InCallback.ExecuteIfBound(TEXT("Network error: Failed to reach AI provider endpoint. Ensure server or local Ollama is running."), false);
				return;
			}

			if (HttpResponse->GetResponseCode() < 200 || HttpResponse->GetResponseCode() >= 300)
			{
				InCallback.ExecuteIfBound(FString::Printf(TEXT("HTTP %d: %s"), HttpResponse->GetResponseCode(), *HttpResponse->GetContentAsString()), false);
				return;
			}

			FString ResponseBody = HttpResponse->GetContentAsString();
			TSharedPtr<FJsonObject> JsonResponse;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);

			if (FJsonSerializer::Deserialize(Reader, JsonResponse) && JsonResponse.IsValid())
			{
				const TArray<TSharedPtr<FJsonValue>>* Choices = nullptr;
				if (JsonResponse->TryGetArrayField(TEXT("choices"), Choices) && Choices && Choices->Num() > 0)
				{
					TSharedPtr<FJsonObject> FirstChoice = (*Choices)[0]->AsObject();
					if (FirstChoice.IsValid())
					{
						TSharedPtr<FJsonObject> MessageObj = FirstChoice->GetObjectField(TEXT("message"));
						if (MessageObj.IsValid() && MessageObj->HasTypedField<EJson::String>(TEXT("content")))
						{
							FString Content = MessageObj->GetStringField(TEXT("content"));
							InCallback.ExecuteIfBound(Content, true);
							return;
						}
					}
				}
			}

			InCallback.ExecuteIfBound(TEXT("Failed to parse response JSON from AI provider."), false);
		}
	);

	Request->ProcessRequest();
}

void FCppAiAssistant::TestConnection(FOnAiTestResult InCallback)
{
	const FCppEditorSettings& Settings = FCppEditorSettings::Get();
	TestConnection(Settings.AiEndpoint, Settings.AiModel, Settings.AiApiKey, InCallback);
}

void FCppAiAssistant::TestConnection(
	const FString& InEndpoint,
	const FString& InModel,
	const FString& InApiKey,
	FOnAiTestResult InCallback)
{
	FString Endpoint = InEndpoint.TrimEnd();
	if (!Endpoint.EndsWith(TEXT("/chat/completions")))
	{
		if (Endpoint.EndsWith(TEXT("/")))
		{
			Endpoint += TEXT("chat/completions");
		}
		else
		{
			Endpoint += TEXT("/chat/completions");
		}
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Endpoint);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetTimeout(5.0f);

	const FCppEditorSettings& Settings = FCppEditorSettings::Get();
	if (InEndpoint.Contains(TEXT("githubcopilot.com")) || Settings.AiProvider == EAiProvider::GitHubCopilot)
	{
		Request->SetHeader(TEXT("Editor-Version"), TEXT("vscode/1.85.0"));
		Request->SetHeader(TEXT("Editor-Plugin-Version"), TEXT("copilot-chat/0.11.1"));
		Request->SetHeader(TEXT("Copilot-Integration-Id"), TEXT("vscode-chat"));
		Request->SetHeader(TEXT("User-Agent"), TEXT("GitHubCopilot/1.138.0"));

		FString AuthToken = InApiKey;
		if (AuthToken.IsEmpty())
		{
			AuthToken = Settings.CopilotSessionToken.IsEmpty() ? Settings.GitHubAccessToken : Settings.CopilotSessionToken;
		}
		if (!AuthToken.IsEmpty())
		{
			Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *AuthToken));
		}
		else
		{
			InCallback.ExecuteIfBound(false, TEXT("Please sign in with GitHub first to test Copilot connection."));
			return;
		}
	}
	else if (!InApiKey.IsEmpty())
	{
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *InApiKey));
	}

	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	RootObject->SetStringField(TEXT("model"), InModel.IsEmpty() ? TEXT("deepseek-coder") : InModel);
	RootObject->SetNumberField(TEXT("max_tokens"), 2);

	TArray<TSharedPtr<FJsonValue>> MessagesArray;
	TSharedPtr<FJsonObject> PingMsg = MakeShared<FJsonObject>();
	PingMsg->SetStringField(TEXT("role"), TEXT("user"));
	PingMsg->SetStringField(TEXT("content"), TEXT("ping"));
	MessagesArray.Add(MakeShared<FJsonValueObject>(PingMsg));
	RootObject->SetArrayField(TEXT("messages"), MessagesArray);

	FString RequestPayload;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestPayload);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
	Request->SetContentAsString(RequestPayload);

	Request->OnProcessRequestComplete().BindLambda(
		[InCallback, ModelName = InModel](FHttpRequestPtr, FHttpResponsePtr HttpResponse, bool bConnectedSuccessfully)
		{
			if (!bConnectedSuccessfully || !HttpResponse.IsValid())
			{
				InCallback.ExecuteIfBound(false, TEXT("Connection failed. Check if local Ollama/LM Studio or cloud endpoint is running."));
				return;
			}

			if (HttpResponse->GetResponseCode() == 200)
			{
				InCallback.ExecuteIfBound(true, FString::Printf(TEXT("Success! Connected to model '%s' (HTTP 200 OK)."), *ModelName));
			}
			else
			{
				InCallback.ExecuteIfBound(false, FString::Printf(TEXT("HTTP %d: %s"), HttpResponse->GetResponseCode(), *HttpResponse->GetContentAsString()));
			}
		}
	);

	Request->ProcessRequest();
}

bool FCppAiAssistant::IsGitHubAuthenticated() const
{
	const FCppEditorSettings& Settings = FCppEditorSettings::Get();
	return !Settings.GitHubAccessToken.IsEmpty() || (!Settings.CopilotSessionToken.IsEmpty() && Settings.AiProvider == EAiProvider::GitHubCopilot);
}

void FCppAiAssistant::SignOutOfGitHub()
{
	CancelGitHubAuth();
	FCppEditorSettings& Settings = FCppEditorSettings::Get();
	Settings.GitHubAccessToken.Empty();
	Settings.GitHubUsername.Empty();
	Settings.CopilotSessionToken.Empty();
	Settings.CopilotTokenExpiresAt = 0.0;
	Settings.Save();
}

void FCppAiAssistant::CancelGitHubAuth()
{
	if (DevicePollTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(DevicePollTickerHandle);
		DevicePollTickerHandle.Reset();
	}
	if (ActiveDeviceAuthRequest.IsValid() && ActiveDeviceAuthRequest->GetStatus() == EHttpRequestStatus::Processing)
	{
		ActiveDeviceAuthRequest->CancelRequest();
		ActiveDeviceAuthRequest.Reset();
	}
	ActiveDeviceCode.Empty();
}

void FCppAiAssistant::StartGitHubDeviceFlow(FOnGitHubDeviceCodeReceived InCodeReceived, FOnGitHubAuthComplete InComplete)
{
	CancelGitHubAuth();
	ActiveAuthCompleteCallback = InComplete;

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(TEXT("https://github.com/login/device/code"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/x-www-form-urlencoded"));
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
	Request->SetTimeout(10.0f);

	// Official GitHub Copilot Device Flow Client ID
	FString Payload = TEXT("client_id=Iv1.b507a08c87ecfe98&scope=read:user");
	Request->SetContentAsString(Payload);

	Request->OnProcessRequestComplete().BindLambda(
		[this, InCodeReceived](FHttpRequestPtr, FHttpResponsePtr Response, bool bSuccess)
		{
			ActiveDeviceAuthRequest.Reset();
			if (!bSuccess || !Response.IsValid() || Response->GetResponseCode() != 200)
			{
				FString Err = Response.IsValid() ? Response->GetContentAsString() : TEXT("Network failure");
				ActiveAuthCompleteCallback.ExecuteIfBound(false, FString::Printf(TEXT("Failed to start device auth: %s"), *Err));
				return;
			}

			TSharedPtr<FJsonObject> RootObj;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
			if (!FJsonSerializer::Deserialize(Reader, RootObj) || !RootObj.IsValid())
			{
				ActiveAuthCompleteCallback.ExecuteIfBound(false, TEXT("Invalid response from GitHub."));
				return;
			}

			ActiveDeviceCode = RootObj->GetStringField(TEXT("device_code"));
			FString UserCode = RootObj->GetStringField(TEXT("user_code"));
			FString VerificationUri = RootObj->GetStringField(TEXT("verification_uri"));
			int32 Interval = RootObj->GetIntegerField(TEXT("interval"));
			int32 ExpiresIn = RootObj->GetIntegerField(TEXT("expires_in"));

			if (Interval <= 0) Interval = 5;
			if (ExpiresIn <= 0) ExpiresIn = 900;

			DevicePollInterval = Interval;
			DeviceAuthExpiresAt = FPlatformTime::Seconds() + (double)ExpiresIn;

			// Copy user code to clipboard for convenience
			FPlatformApplicationMisc::ClipboardCopy(*UserCode);

			// Open browser to verification URI
			FPlatformProcess::LaunchURL(*VerificationUri, nullptr, nullptr);

			// Notify UI
			InCodeReceived.ExecuteIfBound(UserCode, VerificationUri);

			// Start polling loop
			DevicePollTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
				TEXT("GitHubCopilotDevicePoll"),
				(float)DevicePollInterval,
				[this](float) -> bool
				{
					PollGitHubDeviceToken();
					return false; // single execution; will reschedule on each tick
				}
			);
		}
	);

	ActiveDeviceAuthRequest = Request;
	Request->ProcessRequest();
}

void FCppAiAssistant::PollGitHubDeviceToken()
{
	if (ActiveDeviceCode.IsEmpty())
	{
		return;
	}

	if (FPlatformTime::Seconds() >= DeviceAuthExpiresAt)
	{
		CancelGitHubAuth();
		ActiveAuthCompleteCallback.ExecuteIfBound(false, TEXT("Authorization session timed out. Please try again."));
		return;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(TEXT("https://github.com/login/oauth/access_token"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/x-www-form-urlencoded"));
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
	Request->SetTimeout(10.0f);

	FString Payload = FString::Printf(
		TEXT("client_id=Iv1.b507a08c87ecfe98&device_code=%s&grant_type=urn:ietf:params:oauth:grant-type:device_code"),
		*FGenericPlatformHttp::UrlEncode(ActiveDeviceCode)
	);
	Request->SetContentAsString(Payload);

	Request->OnProcessRequestComplete().BindLambda(
		[this](FHttpRequestPtr, FHttpResponsePtr Response, bool bSuccess)
		{
			ActiveDeviceAuthRequest.Reset();
			if (!bSuccess || !Response.IsValid())
			{
				// Retry on next interval
				DevicePollTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
					TEXT("GitHubCopilotDevicePoll"),
					(float)DevicePollInterval,
					[this](float) -> bool { PollGitHubDeviceToken(); return false; }
				);
				return;
			}

			TSharedPtr<FJsonObject> RootObj;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
			if (!FJsonSerializer::Deserialize(Reader, RootObj) || !RootObj.IsValid())
			{
				DevicePollTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
					TEXT("GitHubCopilotDevicePoll"),
					(float)DevicePollInterval,
					[this](float) -> bool { PollGitHubDeviceToken(); return false; }
				);
				return;
			}

			if (RootObj->HasField(TEXT("error")))
			{
				FString Error = RootObj->GetStringField(TEXT("error"));
				if (Error == TEXT("authorization_pending"))
				{
					// Keep waiting
					DevicePollTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
						TEXT("GitHubCopilotDevicePoll"),
						(float)DevicePollInterval,
						[this](float) -> bool { PollGitHubDeviceToken(); return false; }
					);
					return;
				}
				else if (Error == TEXT("slow_down"))
				{
					DevicePollInterval += 5;
					DevicePollTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
						TEXT("GitHubCopilotDevicePoll"),
						(float)DevicePollInterval,
						[this](float) -> bool { PollGitHubDeviceToken(); return false; }
					);
					return;
				}
				else
				{
					FString Desc = RootObj->HasField(TEXT("error_description")) ? RootObj->GetStringField(TEXT("error_description")) : Error;
					CancelGitHubAuth();
					ActiveAuthCompleteCallback.ExecuteIfBound(false, Desc);
					return;
				}
			}

			if (RootObj->HasField(TEXT("access_token")))
			{
				FString AccessToken = RootObj->GetStringField(TEXT("access_token"));
				CancelGitHubAuth();

				FCppEditorSettings& Settings = FCppEditorSettings::Get();
				Settings.GitHubAccessToken = AccessToken;
				Settings.AiProvider = EAiProvider::GitHubCopilot;
				Settings.AiEndpoint = TEXT("https://api.githubcopilot.com");
				Settings.AiModel = TEXT("gpt-4o");
				Settings.Save();

				// Fetch Username & Copilot Session Token
				FetchGitHubUsername(AccessToken, [this, AccessToken](const FString& Username)
				{
					FCppEditorSettings& Settings = FCppEditorSettings::Get();
					Settings.GitHubUsername = Username;
					Settings.Save();

					FetchCopilotToken(AccessToken, [this, Username](bool bCopilotSuccess, const FString& CopilotMsg)
					{
						if (bCopilotSuccess)
						{
							ActiveAuthCompleteCallback.ExecuteIfBound(true, Username);
						}
						else
						{
							ActiveAuthCompleteCallback.ExecuteIfBound(true, FString::Printf(TEXT("%s (Note: %s)"), *Username, *CopilotMsg));
						}
					});
				});
			}
		}
	);

	ActiveDeviceAuthRequest = Request;
	Request->ProcessRequest();
}

void FCppAiAssistant::FetchGitHubUsername(const FString& InAccessToken, TFunction<void(const FString&)> OnUsernameFetched)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(TEXT("https://api.github.com/user"));
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("token %s"), *InAccessToken));
	Request->SetHeader(TEXT("User-Agent"), TEXT("GitHubCopilot/1.138.0"));
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));

	Request->OnProcessRequestComplete().BindLambda(
		[OnUsernameFetched](FHttpRequestPtr, FHttpResponsePtr Response, bool bSuccess)
		{
			if (bSuccess && Response.IsValid() && Response->GetResponseCode() == 200)
			{
				TSharedPtr<FJsonObject> RootObj;
				TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
				if (FJsonSerializer::Deserialize(Reader, RootObj) && RootObj.IsValid() && RootObj->HasField(TEXT("login")))
				{
					OnUsernameFetched(RootObj->GetStringField(TEXT("login")));
					return;
				}
			}
			OnUsernameFetched(TEXT("GitHub User"));
		}
	);
	Request->ProcessRequest();
}

void FCppAiAssistant::FetchCopilotToken(const FString& InAccessToken, TFunction<void(bool, const FString&)> OnTokenFetched)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(TEXT("https://api.github.com/copilot_internal/v2/token"));
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("token %s"), *InAccessToken));
	Request->SetHeader(TEXT("Editor-Version"), TEXT("vscode/1.85.0"));
	Request->SetHeader(TEXT("Editor-Plugin-Version"), TEXT("copilot-chat/0.11.1"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("GitHubCopilot/1.138.0"));
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));

	Request->OnProcessRequestComplete().BindLambda(
		[OnTokenFetched](FHttpRequestPtr, FHttpResponsePtr Response, bool bSuccess)
		{
			if (bSuccess && Response.IsValid() && Response->GetResponseCode() == 200)
			{
				TSharedPtr<FJsonObject> RootObj;
				TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
				if (FJsonSerializer::Deserialize(Reader, RootObj) && RootObj.IsValid() && RootObj->HasField(TEXT("token")))
				{
					FString Token = RootObj->GetStringField(TEXT("token"));
					double ExpiresAt = (double)RootObj->GetIntegerField(TEXT("expires_at"));

					FCppEditorSettings& Settings = FCppEditorSettings::Get();
					Settings.CopilotSessionToken = Token;
					Settings.CopilotTokenExpiresAt = ExpiresAt;
					Settings.Save();

					OnTokenFetched(true, TEXT("Copilot token acquired successfully."));
					return;
				}
			}

			FString Err = Response.IsValid() ? FString::Printf(TEXT("HTTP %d: %s"), Response->GetResponseCode(), *Response->GetContentAsString()) : TEXT("Request failed");
			OnTokenFetched(false, Err);
		}
	);
	Request->ProcessRequest();
}
