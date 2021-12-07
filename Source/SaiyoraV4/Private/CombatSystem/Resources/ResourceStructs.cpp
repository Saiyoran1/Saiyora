#include "ResourceStructs.h"

FResourceState::FResourceState(float const Min, float const Max, float const Value)
{
	Minimum = Min;
	Maximum = Max;
	CurrentValue = Value;
}
