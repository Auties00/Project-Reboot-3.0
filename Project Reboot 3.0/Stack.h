#pragma once

#include "OutputDevice.h"
#include "Class.h"

#define RESULT_DECL void*const RESULT_PARAM

struct FFrame : public FOutputDevice // https://github.com/EpicGames/UnrealEngine/blob/7acbae1c8d1736bb5a0da4f6ed21ccb237bc8851/Engine/Source/Runtime/CoreUObject/Public/UObject/Stack.h#L83
{
public:
	void** VFT; // 10

	// Variables.
	UFunction* Node; // 16
	UObject* Object; // 24 // 0x18
	uint8* Code; // 32 // 0x20
	uint8* Locals; // 40

	// MORE STUFF HERE

	void Step(UObject* Context, RESULT_DECL)
	{
		static void (*StepOriginal)(__int64 frame, UObject* Context, RESULT_DECL) = decltype(StepOriginal)(Addresses::FrameStep);
		StepOriginal(__int64(this), Context, RESULT_PARAM);

		// int32 B = *Code++;
		// (GNatives[B])(Context, *this, RESULT_PARAM);
	}
};