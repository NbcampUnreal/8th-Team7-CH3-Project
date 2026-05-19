#include "Data/PDItemGradeColorData.h"

FLinearColor UPDItemGradeColorData::ResolveColor(EPDItemGrade Grade) const
{
	if (const FLinearColor* Found = GradeColors.Find(Grade))
	{
		return *Found;
	}
	return FallbackColor;
}