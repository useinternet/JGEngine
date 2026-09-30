#include "PCH/PCH.h"
#include "GameMaster/Rules/GameplayValuePipeline.h"

void PGameplayValuePipeline::DefineStages(const PName& valueKind, const HList<PName>& stages)
{
	for (HPair<PName, HList<PName>>& entry : _stagesByKind)
	{
		if (entry.first == valueKind)
		{
			entry.second = stages;
			return;
		}
	}
	_stagesByKind.push_back(HPair<PName, HList<PName>>(valueKind, stages));
}

const HList<PName>* PGameplayValuePipeline::FindStages(const PName& valueKind) const
{
	for (const HPair<PName, HList<PName>>& entry : _stagesByKind)
	{
		if (entry.first == valueKind)
		{
			return &entry.second;
		}
	}
	return nullptr;
}

const HList<HPair<PName, HList<PName>>>& PGameplayValuePipeline::All() const
{
	return _stagesByKind;
}

void PGameplayValuePipeline::Clear()
{
	_stagesByKind.clear();
}

int32 PGameplayValuePipeline::Compute(const HGameplayContext& ctx, const PGameplayRegistry<JGGameplayModifier>& modifiers, const HGameplayValueQuery& query) const
{
	int32 value = query.Base;
	const HList<PName>* stages = FindStages(query.Kind);

	if (stages == nullptr)
	{
		for (const PSharedPtr<JGGameplayModifier>& modifier : modifiers.All())
		{
			if (modifier->GetValueKind() != query.Kind)
			{
				continue;
			}
			if (modifier->Applies(ctx, query) == true)
			{
				value = modifier->Apply(ctx, query, value);
			}
		}
		return value;
	}

	for (const PName& stage : *stages)
	{
		for (const PSharedPtr<JGGameplayModifier>& modifier : modifiers.All())
		{
			if (modifier->GetValueKind() != query.Kind)
			{
				continue;
			}
			if (modifier->GetStage() != stage)
			{
				continue;
			}
			if (modifier->Applies(ctx, query) == true)
			{
				value = modifier->Apply(ctx, query, value);
			}
		}
	}
	return value;
}
