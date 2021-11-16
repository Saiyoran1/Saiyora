#include "SaiyoraThreatFunctions.h"

FThreatFromDamage USaiyoraThreatFunctions::MakeThreatFromDamage(FThreatModCondition const& SourceModifier,
	bool const bGeneratesThreat, bool const bSeparateBaseThreat, float const BaseThreat, bool const bIgnoreModifiers,
	bool const bIgnoreRestrictions)
{
	FThreatFromDamage New;
	New.GeneratesThreat = bGeneratesThreat;
	New.SeparateBaseThreat = bSeparateBaseThreat;
	New.BaseThreat = BaseThreat;
	New.IgnoreModifiers = bIgnoreModifiers;
	New.IgnoreRestrictions = bIgnoreRestrictions;
	New.SourceModifier = SourceModifier;
	return New;
}
