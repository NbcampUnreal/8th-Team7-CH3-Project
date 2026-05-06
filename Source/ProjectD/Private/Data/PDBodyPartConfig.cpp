#include "Data/PDBodyPartConfig.h"

EBodyPart UPDBodyPartConfig::GetBodyPartFromName(FName InName) const
{
	for (const FBodyPartMapping& Mapping:Mappings)
	{
		if (Mapping.HitBoxName==InName)
		{
			return Mapping.BodyPart;
		}
	}
	return EBodyPart::None;
}
