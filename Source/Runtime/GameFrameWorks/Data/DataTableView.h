#pragma once
#include "Data/DataTable.h"
#include "AssetPath.h"
#include "Object/ObjectGlobalSystem.h"
#include <functional>
#include <type_traits>

// 테이블을 C++ 구조체 배열로 읽는다. 구조체가 필요한 열만 멤버 포인터로 묶고, Bind 가 열 이름 · 타입을 검사한다.
//
//   struct HSampleRow
//   {
//       int32      Count = 0;
//       HAssetPath Mesh;
//       static void BindColumns(HDataTableRowBinder<HSampleRow>& binder)
//       {
//           binder.Bind("Count", &HSampleRow::Count);
//           binder.Bind("Mesh",  &HSampleRow::Mesh);
//       }
//   };
//   HDataTableView<HSampleRow> view;
//   if (view.Bind(table, &issues) == false) { /* 없는 열 · 다른 타입 — issues 와 로그에 */ }
//   const HSampleRow* row = view.Find(PName("Row_A"));
//
// 멤버 타입 → 받는 열 타입: bool → Bool, int32 · int64 → Int, float32 · float64 → Float · Int,
// PString · PName → String · Enum · AssetRef · RowRef, 리플렉션 열거형 → Enum(이름으로), HAssetPath → AssetRef · String.
// 테이블에만 있는 열은 무시한다. 순회는 행 순서(파일 순서)이고 Find 는 키 색인이다.
// 테이블이 바뀌면(에디터 저장) IsUpToDate() 가 false 가 된다 — JGDataTable::OnChanged 에서 다시 Bind 한다.

namespace DataTableBindingPrivate
{
	inline bool IsTextColumn(EDataTableColumnType inType)
	{
		return DataTableValueKindOf(inType) == EDataTableValueKind::Text;
	}

	template<class M, class Enable = void>
	struct HMemberTraits
	{
		static constexpr bool bSupported = false;
	};

	template<>
	struct HMemberTraits<bool>
	{
		static constexpr bool bSupported = true;
		static const char* Expected() { return "Bool"; }
		static bool Accepts(EDataTableColumnType inType) { return inType == EDataTableColumnType::Bool; }
		static bool Assign(const HDataTableValue& inValue, bool& outMember, PString* outError)
		{
			outMember = inValue.GetBool();
			return true;
		}
	};

	template<>
	struct HMemberTraits<int32>
	{
		static constexpr bool bSupported = true;
		static const char* Expected() { return "Int"; }
		static bool Accepts(EDataTableColumnType inType) { return inType == EDataTableColumnType::Int; }
		static bool Assign(const HDataTableValue& inValue, int32& outMember, PString* outError)
		{
			const int64 value = inValue.GetInt();
			if (value < (int64)INT32_MIN || value > (int64)INT32_MAX)
			{
				if (outError != nullptr)
				{
					*outError = PString::Format("%s does not fit in int32", PString::FromInt64(value));
				}
				return false;
			}
			outMember = (int32)value;
			return true;
		}
	};

	template<>
	struct HMemberTraits<int64>
	{
		static constexpr bool bSupported = true;
		static const char* Expected() { return "Int"; }
		static bool Accepts(EDataTableColumnType inType) { return inType == EDataTableColumnType::Int; }
		static bool Assign(const HDataTableValue& inValue, int64& outMember, PString* outError)
		{
			outMember = inValue.GetInt();
			return true;
		}
	};

	template<class F>
	struct HFloatMemberTraits
	{
		static constexpr bool bSupported = true;
		static const char* Expected() { return "Float"; }
		static bool Accepts(EDataTableColumnType inType) { return inType == EDataTableColumnType::Float || inType == EDataTableColumnType::Int; }
		static bool Assign(const HDataTableValue& inValue, F& outMember, PString* outError)
		{
			outMember = (inValue.GetKind() == EDataTableValueKind::Int) ? (F)inValue.GetInt() : (F)inValue.GetFloat();
			return true;
		}
	};

	template<>
	struct HMemberTraits<float32> : public HFloatMemberTraits<float32>
	{
	};

	template<>
	struct HMemberTraits<float64> : public HFloatMemberTraits<float64>
	{
	};

	template<>
	struct HMemberTraits<PString>
	{
		static constexpr bool bSupported = true;
		static const char* Expected() { return "String"; }
		static bool Accepts(EDataTableColumnType inType) { return IsTextColumn(inType); }
		static bool Assign(const HDataTableValue& inValue, PString& outMember, PString* outError)
		{
			outMember = inValue.GetText();
			return true;
		}
	};

	template<>
	struct HMemberTraits<PName>
	{
		static constexpr bool bSupported = true;
		static const char* Expected() { return "String"; }
		static bool Accepts(EDataTableColumnType inType) { return IsTextColumn(inType); }
		static bool Assign(const HDataTableValue& inValue, PName& outMember, PString* outError)
		{
			outMember = inValue.GetText().Empty() ? PName() : PName(inValue.GetText());
			return true;
		}
	};

	template<>
	struct HMemberTraits<HAssetPath>
	{
		static constexpr bool bSupported = true;
		static const char* Expected() { return "AssetRef"; }
		static bool Accepts(EDataTableColumnType inType) { return inType == EDataTableColumnType::AssetRef || inType == EDataTableColumnType::String; }
		static bool Assign(const HDataTableValue& inValue, HAssetPath& outMember, PString* outError)
		{
			outMember = inValue.GetText().Empty() ? HAssetPath() : HAssetPath(inValue.GetText());
			return true;
		}
	};

	// 리플렉션 열거형. 셀의 이름을 리플렉션 열거형의 값으로 바꾼다.
	template<class M>
	struct HMemberTraits<M, std::enable_if_t<std::is_enum_v<M>>>
	{
		static constexpr bool bSupported = true;
		static const char* Expected() { return "Enum"; }
		static bool Accepts(EDataTableColumnType inType) { return inType == EDataTableColumnType::Enum; }
		static bool Assign(const HDataTableValue& inValue, M& outMember, PString* outError)
		{
			PSharedPtr<JGEnum> reflectedEnum = StaticEnum<M>();
			if (reflectedEnum == nullptr)
			{
				if (outError != nullptr)
				{
					*outError = PString::Format("%s is not a reflected enum", JGTYPE(M).GetName().ToString());
				}
				return false;
			}

			// JGEnum::GetIndexByEnumName 은 못 찾으면 -1 이 아니라 항목 수를 돌려준다 — 그 자리의 이름으로 확인한다
			const PName name(inValue.GetText());
			const int32 index = reflectedEnum->GetIndexByEnumName(name);
			if (reflectedEnum->GetEnumNameByIndex(index) != name)
			{
				if (outError != nullptr)
				{
					*outError = PString::Format("'%s' is not a value of %s", inValue.GetText(), JGTYPE(M).GetName().ToString());
				}
				return false;
			}

			// JGEnum::GetValueByEnumName 은 값이 0 부터 이어질 때만 맞다 — 되돌려 이름이 같은지 확인한다
			const int32 value = reflectedEnum->GetValueByEnumName(name);
			if (reflectedEnum->GetEnumNameByValue(value) != name)
			{
				if (outError != nullptr)
				{
					*outError = PString::Format("%s has explicit values. Binding by name supports 0, 1, 2 ... enums only", JGTYPE(M).GetName().ToString());
				}
				return false;
			}

			outMember = (M)value;
			return true;
		}
	};
}

template<class T>
class HDataTableRowBinder
{
public:
	struct HBinding
	{
		PString                                                                 Column;
		const char*                                                             Expected = "";
		std::function<bool(EDataTableColumnType)>                               Accepts;
		std::function<bool(const HDataTableValue&, T&, PString*)>               Assign;
	};

private:
	HList<HBinding> _bindings;

public:
	template<class M>
	void Bind(const PString& inColumn, M T::* inMember)
	{
		using HTraits = DataTableBindingPrivate::HMemberTraits<M>;
		static_assert(HTraits::bSupported, "HDataTableRowBinder::Bind: unsupported member type (bool, int32, int64, float32, float64, PString, PName, HAssetPath, reflected enum)");

		HBinding binding;
		binding.Column   = inColumn;
		binding.Expected = HTraits::Expected();
		binding.Accepts  = [](EDataTableColumnType inType)
		{
			return HTraits::Accepts(inType);
		};
		binding.Assign   = [inMember](const HDataTableValue& inValue, T& outRow, PString* outError)
		{
			return HTraits::Assign(inValue, outRow.*inMember, outError);
		};
		_bindings.push_back(binding);
	}

	const HList<HBinding>& GetBindings() const { return _bindings; }
};

template<class T>
class HDataTableView
{
	PWeakPtr<JGDataTable>  _table;
	uint64                 _revision = 0;
	bool                   _bBound   = false;
	HList<PName>           _keys;
	HList<T>               _rows;
	HHashMap<PName, int32> _indexByKey;

public:
	// 모든 행을 T 로 바꾼다. 없는 열 · 다른 타입 열 · 바꿀 수 없는 값(int32 넘침, 열거형에 없는 이름)이 하나라도 있으면
	// 비운 채 false — 문제는 outIssues 와 로그에. (Row = 행 번호, Column = 테이블 열 번호)
	bool Bind(const PSharedPtr<JGDataTable>& inTable, HList<HDataTableIssue>* outIssues = nullptr)
	{
		reset();

		HList<HDataTableIssue> issues;
		if (inTable == nullptr)
		{
			addIssue(issues, -1, -1, "no table");
			return finish(issues, outIssues);
		}

		HDataTableRowBinder<T> binder;
		T::BindColumns(binder);

		const HDataTableSchema& schema = inTable->GetSchema();
		const HList<typename HDataTableRowBinder<T>::HBinding>& bindings = binder.GetBindings();

		HList<int32> columnIndices;
		for (const typename HDataTableRowBinder<T>::HBinding& binding : bindings)
		{
			const int32 columnIndex = schema.FindColumn(binding.Column);
			if (columnIndex < 0)
			{
				addIssue(issues, -1, -1, PString::Format("no column '%s' (C++ expects %s)", binding.Column, PString(binding.Expected)));
			}
			else if (binding.Accepts(schema.Columns[columnIndex].Type) == false)
			{
				addIssue(issues, -1, columnIndex, PString::Format("column '%s' is %s but C++ expects %s", binding.Column, PString(DataTableColumnTypeToString(schema.Columns[columnIndex].Type)), PString(binding.Expected)));
			}
			columnIndices.push_back(columnIndex);
		}
		if (issues.empty() == false)
		{
			return finish(issues, outIssues);
		}

		const int32 rowCount = inTable->GetRowCount();
		_keys.reserve(rowCount);
		_rows.reserve(rowCount);
		for (int32 r = 0; r < rowCount; ++r)
		{
			const PString& key = inTable->GetRowKey(r);
			if (key.Empty())
			{
				addIssue(issues, r, -1, "row has an empty key");
				continue;
			}

			T row{};
			const int32 bindingCount = (int32)bindings.size();
			for (int32 b = 0; b < bindingCount; ++b)
			{
				PString error;
				if (bindings[b].Assign(inTable->GetValue(r, columnIndices[b]), row, &error) == false)
				{
					addIssue(issues, r, columnIndices[b], PString::Format("row '%s' column '%s': %s", key, bindings[b].Column, error));
				}
			}

			const PName name(key);
			if (_indexByKey.contains(name))
			{
				addIssue(issues, r, -1, PString::Format("key '%s' is a duplicate", key));
				continue;
			}
			_indexByKey[name] = (int32)_rows.size();
			_keys.push_back(name);
			_rows.push_back(row);
		}

		if (issues.empty() == false)
		{
			reset();
			return finish(issues, outIssues);
		}

		_table    = inTable;
		_revision = inTable->GetRevision();
		_bBound   = true;
		return true;
	}

	bool IsBound() const { return _bBound; }

	// 묶은 뒤 테이블이 바뀌지 않았나
	bool IsUpToDate() const
	{
		if (_bBound == false)
		{
			return false;
		}
		PSharedPtr<JGDataTable> table = _table.Pin();
		return table != nullptr && table->GetRevision() == _revision;
	}

	int32 GetCount() const { return (int32)_rows.size(); }
	const PName& GetKey(int32 inIndex) const { return _keys[inIndex]; }
	const T& Get(int32 inIndex) const { return _rows[inIndex]; }

	const T* Find(const PName& inKey) const
	{
		typename HHashMap<PName, int32>::const_iterator iter = _indexByKey.find(inKey);
		if (iter == _indexByKey.end())
		{
			return nullptr;
		}
		return &_rows[iter->second];
	}

private:
	void reset()
	{
		_table.Reset();
		_revision = 0;
		_bBound   = false;
		_keys.clear();
		_rows.clear();
		_indexByKey.clear();
	}

	static void addIssue(HList<HDataTableIssue>& issues, int32 inRow, int32 inColumn, const PString& inMessage)
	{
		HDataTableIssue issue;
		issue.Severity = EDataTableIssueSeverity::Error;
		issue.Row      = inRow;
		issue.Column   = inColumn;
		issue.Message  = inMessage;
		issues.push_back(issue);
	}

	static bool finish(const HList<HDataTableIssue>& issues, HList<HDataTableIssue>* outIssues)
	{
		LogDataTableIssues(PString::Format("HDataTableView<%s>::Bind", JGTYPE(T).GetName().ToString()), issues);
		if (outIssues != nullptr)
		{
			outIssues->insert(outIssues->end(), issues.begin(), issues.end());
		}
		return false;
	}
};
