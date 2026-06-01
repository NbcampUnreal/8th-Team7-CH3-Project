#include "Data/PDBodyPartConfig.h"

EBodyPart UPDBodyPartConfig::GetBodyPartFromName(FName InName) const
{
	if (InName.IsNone())
	{
		return EBodyPart::None;
	}


	for (int32 MappingIndex = 0; MappingIndex < Mappings.Num(); ++MappingIndex)
	{
		const FBodyPartMapping& Mapping = Mappings[MappingIndex];
		if (Mapping.HitBoxName==InName)
		{
			return Mapping.BodyPart;
		}
	}

	return EBodyPart::None;
}
