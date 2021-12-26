#include "ResourceStructs.h"

FResourceState::FResourceState()
{
	//Needs to exist to prevent compile errors.
}

FResourceState::FResourceState(float const Min, float const Max, float const Value)
{
	Minimum = Min;
	Maximum = Max;
	CurrentValue = Value;
}
