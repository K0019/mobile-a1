#pragma once
#include "Engine/Engine.h"

struct Context;
struct FrameData;

class Application
{
public:
	void Initialize(Context& context);
	void Update(Context& context, FrameData& frame);
	void Shutdown(Context& context);

private:
	MagicEngine magicEngine;

};
