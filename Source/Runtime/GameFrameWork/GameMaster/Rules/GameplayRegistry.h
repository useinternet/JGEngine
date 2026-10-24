#pragma once
#include "GameMaster/GameMasterDefines.h"
#include <algorithm>

// 종류(PName) → 구현 객체. 핸들러 · 효과 · 트리거 · 수정자에 공통으로 쓴다.
// T 는 PName GetKind() const 를 가진 JGObject 파생이다. 조회는 선형 (개수가 수십 규모).
template<class T>
class PGameplayRegistry
{
	HList<PSharedPtr<T>> _all;

public:
	bool Register(PSharedPtr<T> item)
	{
		if (item == nullptr)
		{
			return false;
		}

		PName kind = item->GetKind();
		if (kind != NAME_NONE && Find(kind) != nullptr)
		{
			JG_LOG(GameMaster, ELogLevel::Warning, "PGameplayRegistry: duplicate kind %s ignored", kind.ToString());
			return false;
		}

		_all.push_back(item);
		return true;
	}

	PSharedPtr<T> Find(const PName& kind) const
	{
		if (kind == NAME_NONE)
		{
			return nullptr;
		}
		for (const PSharedPtr<T>& item : _all)
		{
			if (item->GetKind() == kind)
			{
				return item;
			}
		}
		return nullptr;
	}

	bool Has(const PName& kind) const
	{
		return Find(kind) != nullptr;
	}

	const HList<PSharedPtr<T>>& All() const
	{
		return _all;
	}

	int32 Count() const
	{
		return (int32)_all.size();
	}

	void Clear()
	{
		_all.clear();
	}

	// 종류 이름의 사전순으로 고정한다. 등록 순서(리플렉션 열거 순서)에 결과가 좌우되지 않게 하기 위함.
	void SortByKind()
	{
		std::stable_sort(_all.begin(), _all.end(), [](const PSharedPtr<T>& lhs, const PSharedPtr<T>& rhs)
		{
			return lhs->GetKind().ToString().GetRawString() < rhs->GetKind().ToString().GetRawString();
		});
	}
};
