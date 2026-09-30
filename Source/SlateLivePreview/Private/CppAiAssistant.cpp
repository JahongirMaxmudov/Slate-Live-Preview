// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#include "CppAiAssistant.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonReader.h"
#include "Misc/Paths.h"

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

	if (!Settings.AiApiKey.IsEmpty())
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

	if (!Settings.AiApiKey.IsEmpty())
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
	Request->SetTimeout(10.0f);

	if (!InApiKey.IsEmpty())
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
