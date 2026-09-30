// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "Layout/Margin.h"
#include "Types/SlateEnums.h"

enum class ESlateAstValueType : uint8
{
	None,
	Number,
	Boolean,
	String,
	Color,
	Margin,
	EnumOrIdentifier,
	Widget
};

struct SLATELIVEPREVIEW_API FSlateAstValue
{
	ESlateAstValueType Type = ESlateAstValueType::None;
	double NumberValue = 0.0;
	bool BoolValue = false;
	FString StringValue;
	FLinearColor ColorValue = FLinearColor::White;
	FMargin MarginValue = FMargin(0.0f);
	TSharedPtr<struct FSlateWidgetNode> WidgetValue;

	static FSlateAstValue FromNumber(double InVal)
	{
		FSlateAstValue V;
		V.Type = ESlateAstValueType::Number;
		V.NumberValue = InVal;
		return V;
	}

	static FSlateAstValue FromBool(bool InVal)
	{
		FSlateAstValue V;
		V.Type = ESlateAstValueType::Boolean;
		V.BoolValue = InVal;
		return V;
	}

	static FSlateAstValue FromString(const FString& InVal)
	{
		FSlateAstValue V;
		V.Type = ESlateAstValueType::String;
		V.StringValue = InVal;
		return V;
	}

	static FSlateAstValue FromEnum(const FString& InVal)
	{
		FSlateAstValue V;
		V.Type = ESlateAstValueType::EnumOrIdentifier;
		V.StringValue = InVal;
		return V;
	}

	static FSlateAstValue FromColor(const FLinearColor& InVal)
	{
		FSlateAstValue V;
		V.Type = ESlateAstValueType::Color;
		V.ColorValue = InVal;
		return V;
	}

	static FSlateAstValue FromMargin(const FMargin& InVal)
	{
		FSlateAstValue V;
		V.Type = ESlateAstValueType::Margin;
		V.MarginValue = InVal;
		return V;
	}

	static FSlateAstValue FromWidget(const TSharedPtr<struct FSlateWidgetNode>& InVal)
	{
		FSlateAstValue V;
		V.Type = ESlateAstValueType::Widget;
		V.WidgetValue = InVal;
		return V;
	}

	float AsFloat(float DefaultVal = 0.0f) const
	{
		return (Type == ESlateAstValueType::Number) ? static_cast<float>(NumberValue) : DefaultVal;
	}

	int32 AsInt(int32 DefaultVal = 0) const
	{
		return (Type == ESlateAstValueType::Number) ? static_cast<int32>(NumberValue) : DefaultVal;
	}

	bool AsBool(bool DefaultVal = false) const
	{
		return (Type == ESlateAstValueType::Boolean) ? BoolValue : DefaultVal;
	}

	FText AsText(const FText& DefaultVal = FText::GetEmpty()) const
	{
		if (Type == ESlateAstValueType::String)
		{
			return FText::FromString(StringValue);
		}
		if (Type == ESlateAstValueType::Number)
		{
			return FText::AsNumber(NumberValue);
		}
		return DefaultVal;
	}

	FLinearColor AsColor(const FLinearColor& DefaultVal = FLinearColor::White) const
	{
		return (Type == ESlateAstValueType::Color) ? ColorValue : DefaultVal;
	}

	FMargin AsMargin(const FMargin& DefaultVal = FMargin(0.0f)) const
	{
		if (Type == ESlateAstValueType::Margin)
		{
			return MarginValue;
		}
		if (Type == ESlateAstValueType::Number)
		{
			return FMargin(static_cast<float>(NumberValue));
		}
		return DefaultVal;
	}

	EHorizontalAlignment AsHAlign(EHorizontalAlignment DefaultVal = HAlign_Fill) const
	{
		if (Type == ESlateAstValueType::EnumOrIdentifier)
		{
			if (StringValue.Equals(TEXT("HAlign_Fill"), ESearchCase::IgnoreCase)) return HAlign_Fill;
			if (StringValue.Equals(TEXT("HAlign_Left"), ESearchCase::IgnoreCase)) return HAlign_Left;
			if (StringValue.Equals(TEXT("HAlign_Center"), ESearchCase::IgnoreCase)) return HAlign_Center;
			if (StringValue.Equals(TEXT("HAlign_Right"), ESearchCase::IgnoreCase)) return HAlign_Right;
		}
		return DefaultVal;
	}

	EVerticalAlignment AsVAlign(EVerticalAlignment DefaultVal = VAlign_Fill) const
	{
		if (Type == ESlateAstValueType::EnumOrIdentifier)
		{
			if (StringValue.Equals(TEXT("VAlign_Fill"), ESearchCase::IgnoreCase)) return VAlign_Fill;
			if (StringValue.Equals(TEXT("VAlign_Top"), ESearchCase::IgnoreCase)) return VAlign_Top;
			if (StringValue.Equals(TEXT("VAlign_Center"), ESearchCase::IgnoreCase)) return VAlign_Center;
			if (StringValue.Equals(TEXT("VAlign_Bottom"), ESearchCase::IgnoreCase)) return VAlign_Bottom;
		}
		return DefaultVal;
	}

	EVisibility AsVisibility(EVisibility DefaultVal = EVisibility::Visible) const
	{
		if (Type == ESlateAstValueType::EnumOrIdentifier)
		{
			if (StringValue.Contains(TEXT("Visible"))) return EVisibility::Visible;
			if (StringValue.Contains(TEXT("Collapsed"))) return EVisibility::Collapsed;
			if (StringValue.Contains(TEXT("Hidden"))) return EVisibility::Hidden;
			if (StringValue.Contains(TEXT("HitTestInvisible"))) return EVisibility::HitTestInvisible;
			if (StringValue.Contains(TEXT("SelfHitTestInvisible"))) return EVisibility::SelfHitTestInvisible;
		}
		return DefaultVal;
	}
};

struct SLATELIVEPREVIEW_API FSlatePropertyNode
{
	FString PropertyName;
	TArray<FSlateAstValue> Arguments;
	int32 Line = 1;

	bool HasArgs() const { return Arguments.Num() > 0; }
	const FSlateAstValue& GetFirstArg() const { return Arguments[0]; }
};

struct SLATELIVEPREVIEW_API FSlateSlotNode
{
	FString SlotType; // e.g. "Slot", "SVerticalBox::Slot"
	TArray<FSlatePropertyNode> SlotProperties;
	TSharedPtr<struct FSlateWidgetNode> ChildWidget;
	int32 Line = 1;

	const FSlatePropertyNode* FindProperty(const FString& InName) const
	{
		for (const FSlatePropertyNode& Prop : SlotProperties)
		{
			if (Prop.PropertyName.Equals(InName, ESearchCase::IgnoreCase))
			{
				return &Prop;
			}
		}
		return nullptr;
	}
};

struct SLATELIVEPREVIEW_API FSlateWidgetNode : public TSharedFromThis<FSlateWidgetNode>
{
	FString WidgetType; // e.g. "SBorder", "SVerticalBox", "STextBlock"
	FString VariableName; // For SAssignNew(Var, Type)
	TArray<FSlatePropertyNode> Properties;
	TArray<FSlateSlotNode> Slots;
	TSharedPtr<FSlateWidgetNode> DirectChild; // For single-child widgets: SBorder()[ SNew(...) ]
	int32 Line = 1;

	const FSlatePropertyNode* FindProperty(const FString& InName) const
	{
		for (const FSlatePropertyNode& Prop : Properties)
		{
			if (Prop.PropertyName.Equals(InName, ESearchCase::IgnoreCase))
			{
				return &Prop;
			}
		}
		return nullptr;
	}
};
