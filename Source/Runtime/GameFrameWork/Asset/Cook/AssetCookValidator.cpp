#include "PCH/PCH.h"
#include "Asset/Cook/AssetCookValidator.h"
#include <algorithm>
#include <cstdint>
#include <cstring>

namespace
{
	HRawString toLowerAscii(const HRawString& inText)
	{
		HRawString result = inText;
		for (char& c : result)
		{
			if (c >= 'A' && c <= 'Z')
			{
				c = (char)(c - 'A' + 'a');
			}
		}
		return result;
	}

	// 절대 경로, 구분자 '/', . · .. 정리
	HRawString absoluteGenericPath(const PString& inPath)
	{
		std::error_code errCode;
		fs::path path = fs::absolute(fs::path(inPath.GetRawString()), errCode);
		if (errCode)
		{
			path = fs::path(inPath.GetRawString());
		}
		return path.lexically_normal().generic_string();
	}

	bool hasAssetExtension(const HRawString& inPath)
	{
		return toLowerAscii(fs::path(inPath).extension().generic_string()) == JG_ASSET_FORMAT;
	}

	// HAssetPath::setupAssetPath 와 같은 규칙: 토큰 뒤의 역슬래시를 '/' 로, 앞쪽 '/' 를 지우고, 확장자가 없으면 .jgasset 을 붙인다.
	// inTokens 의 어느 토큰으로도 시작하지 않으면 빈 글
	HRawString normalizeTokenPath(const HRawString& inPath, const HList<HRawString>& inTokens)
	{
		for (const HRawString& token : inTokens)
		{
			if (inPath.compare(0, token.size(), token) != 0)
			{
				continue;
			}

			HRawString relative = inPath.substr(token.size());
			std::replace(relative.begin(), relative.end(), '\\', '/');
			const size_t first = relative.find_first_not_of('/');
			relative = (first == HRawString::npos) ? HRawString() : relative.substr(first);
			if (fs::path(relative).has_extension() == false)
			{
				relative += JG_ASSET_FORMAT;
			}
			return token + relative;
		}
		return HRawString();
	}

	struct HMountRoot
	{
		HRawString Token;
		HRawString Directory;        // 절대 경로, '/' 로 끝남
		HRawString LowerDirectory;   // 비교용(Windows 경로는 대소문자를 가리지 않는다)
	};

	// 파일의 토큰 경로. 마운트 폴더가 겹치면 더 깊은 쪽. 어느 마운트에도 없으면 false
	bool toTokenPath(const HRawString& inAbsoluteFile, const HList<HMountRoot>& inRoots, HRawString* outTokenPath)
	{
		const HRawString lowerFile = toLowerAscii(inAbsoluteFile);
		const HMountRoot* best = nullptr;
		for (const HMountRoot& root : inRoots)
		{
			if (lowerFile.compare(0, root.LowerDirectory.size(), root.LowerDirectory) != 0)
			{
				continue;
			}
			if (best == nullptr || root.LowerDirectory.size() > best->LowerDirectory.size())
			{
				best = &root;
			}
		}

		if (best == nullptr)
		{
			return false;
		}
		*outTokenPath = best->Token + inAbsoluteFile.substr(best->Directory.size());
		return true;
	}

	// _guid 글("d1-d2-d3-b0-…-b7", 10진수 11개)이 HGuid 형식인지 보고 비교용 정규형(앞 0 없는 수)을 만든다.
	// HGuid::FromString 은 숫자가 아니면 std::stoull 예외를 던지고 범위를 넘으면 잘라 버려서 쓰지 않는다
	bool parseGuidText(const char* inText, size_t inLength, HRawString* outCanonical)
	{
		HList<uint64> numbers;
		uint64 current = 0;
		size_t digits  = 0;
		for (size_t i = 0; i <= inLength; ++i)
		{
			const char c = (i < inLength) ? inText[i] : '-';
			if (c == '-')
			{
				if (digits == 0)
				{
					return false;
				}
				numbers.push_back(current);
				current = 0;
				digits  = 0;
				continue;
			}
			if (c < '0' || c > '9')
			{
				return false;
			}

			const uint64 digit = (uint64)(c - '0');
			if (current > (UINT64_MAX - digit) / 10)
			{
				return false;
			}
			current = current * 10 + digit;
			++digits;
		}

		// HGuid: _data1(0 이면 무효) · _data2 · _data3(16비트) · _data4[8](8비트)
		if (numbers.size() != 11 || numbers[0] == 0 || numbers[1] > 0xFFFF || numbers[2] > 0xFFFF)
		{
			return false;
		}
		for (size_t i = 3; i < numbers.size(); ++i)
		{
			if (numbers[i] > 0xFF)
			{
				return false;
			}
		}

		outCanonical->clear();
		for (size_t i = 0; i < numbers.size(); ++i)
		{
			if (i > 0)
			{
				outCanonical->push_back('-');
			}
			outCanonical->append(std::to_string(numbers[i]));
		}
		return true;
	}

	const rapidjson::Value* findMember(const rapidjson::Value& inObject, const char* inName)
	{
		if (inObject.IsObject() == false)
		{
			return nullptr;
		}
		rapidjson::Value::ConstMemberIterator iter = inObject.FindMember(inName);
		return (iter == inObject.MemberEnd()) ? nullptr : &iter->value;
	}

	PString toText(const rapidjson::Value& inString)
	{
		return PString(HRawString(inString.GetString(), inString.GetStringLength()).c_str());
	}

	// 에셋 글 안의 마운트 토큰 문자열. 같은 대상은 한 번만(처음 위치 + 횟수)
	struct HFoundReference
	{
		HRawString Target;   // 글 그대로
		HRawString At;       // 처음 나온 위치(JGObject.JGDataTable.Rows[2].Mesh)
		int32      Count = 0;
	};

	class HReferenceCollector
	{
	private:
		struct HSegment
		{
			const char* Name   = nullptr;   // 객체 멤버면 이름, 배열 원소면 nullptr
			size_t      Length = 0;
			size_t      Index  = 0;
		};

		const HList<HRawString>&   _tokens;
		HList<HSegment>            _path;
		HHashMap<HRawString, int32> _indexByTarget;

	public:
		HList<HFoundReference> References;

		explicit HReferenceCollector(const HList<HRawString>& inTokens)
			: _tokens(inTokens)
		{
		}

		void Collect(const rapidjson::Value& inValue)
		{
			if (inValue.IsString())
			{
				collectString(inValue);
			}
			else if (inValue.IsObject())
			{
				for (rapidjson::Value::ConstMemberIterator iter = inValue.MemberBegin(); iter != inValue.MemberEnd(); ++iter)
				{
					HSegment segment;
					segment.Name   = iter->name.GetString();
					segment.Length = iter->name.GetStringLength();
					_path.push_back(segment);
					Collect(iter->value);
					_path.pop_back();
				}
			}
			else if (inValue.IsArray())
			{
				for (rapidjson::SizeType i = 0; i < inValue.Size(); ++i)
				{
					HSegment segment;
					segment.Index = i;
					_path.push_back(segment);
					Collect(inValue[i]);
					_path.pop_back();
				}
			}
		}

	private:
		// JGObject.JGAsset.AssetPath.AssetPath — 에셋 자기 경로. 에셋 DB 는 파일 위치로 경로를 정하므로 참조가 아니다
		bool isSelfAssetPath() const
		{
			static const char* const keys[] = { "JGObject", "JGAsset", "AssetPath", "AssetPath" };
			if (_path.size() != 4)
			{
				return false;
			}
			for (size_t i = 0; i < 4; ++i)
			{
				if (_path[i].Name == nullptr || HRawString(_path[i].Name, _path[i].Length) != keys[i])
				{
					return false;
				}
			}
			return true;
		}

		HRawString pathText() const
		{
			HRawString text;
			for (const HSegment& segment : _path)
			{
				if (segment.Name != nullptr)
				{
					if (text.empty() == false)
					{
						text.push_back('.');
					}
					text.append(segment.Name, segment.Length);
				}
				else
				{
					text.append("[" + std::to_string(segment.Index) + "]");
				}
			}
			return text;
		}

		void collectString(const rapidjson::Value& inValue)
		{
			const char*  text   = inValue.GetString();
			const size_t length = inValue.GetStringLength();
			bool bToken = false;
			for (const HRawString& token : _tokens)
			{
				if (length >= token.size() && std::memcmp(text, token.data(), token.size()) == 0)
				{
					bToken = true;
					break;
				}
			}
			if (bToken == false || isSelfAssetPath())
			{
				return;
			}

			const HRawString target(text, length);
			HHashMap<HRawString, int32>::iterator iter = _indexByTarget.find(target);
			if (iter != _indexByTarget.end())
			{
				++References[iter->second].Count;
				return;
			}

			HFoundReference reference;
			reference.Target = target;
			reference.At     = pathText();
			reference.Count  = 1;
			_indexByTarget[target] = (int32)References.size();
			References.push_back(reference);
		}
	};

	// 검사가 채우는 문맥(HAssetCookContext 는 타입별 검사에 읽기 전용으로 넘긴다)
	class HAssetCookContextBuilder final : public HAssetCookContext
	{
	public:
		HList<HRawString>& Tokens() { return _tokens; }
		HHashSet<HRawString>& Files() { return _files; }
		HHashMap<HRawString, HRawString>& FilesByLowerPath() { return _filesByLowerPath; }

		void AddAsset(const HAssetCookAsset& inAsset)
		{
			_assetIndexByPath[inAsset.TokenPath.GetRawString()] = (int32)_assets.size();
			_assets.push_back(inAsset);
		}
	};

	struct HCookFile
	{
		HRawString File;        // 절대 경로
		HRawString TokenPath;
	};
}

PString HAssetCookContext::NormalizeTokenPath(const PString& inPath) const
{
	return PString(normalizeTokenPath(inPath.GetRawString(), _tokens).c_str());
}

bool HAssetCookContext::HasFile(const PString& inPath) const
{
	const HRawString path = normalizeTokenPath(inPath.GetRawString(), _tokens);
	return path.empty() == false && _files.contains(path);
}

const HAssetCookAsset* HAssetCookContext::FindAsset(const PString& inPath) const
{
	const HRawString path = normalizeTokenPath(inPath.GetRawString(), _tokens);
	HHashMap<HRawString, int32>::const_iterator iter = _assetIndexByPath.find(path);
	if (iter == _assetIndexByPath.end())
	{
		return nullptr;
	}
	return &_assets[iter->second];
}

bool ValidateAssetsForCook(const HList<PString>& inFiles, const HList<HAssetCookMount>& inMounts,
	const HList<HAssetCookTypeCheck>& inTypeChecks, HList<HAssetCookIssue>& outIssues)
{
	HAssetCookContextBuilder context;
	HList<HAssetCookIssue> issues;
	const auto addError = [&issues](const HRawString& inFile, const PString& inMessage)
		{
			HAssetCookIssue issue;
			issue.Severity = EAssetCookIssueSeverity::Error;
			issue.File     = PString(inFile.c_str());
			issue.Message  = inMessage;
			issues.push_back(issue);
		};

	// 마운트
	HList<HMountRoot> roots;
	for (const HAssetCookMount& mount : inMounts)
	{
		const HRawString& token = mount.Token.GetRawString();
		if (token.size() < 2 || token.front() != '/' || token.back() != '/')
		{
			addError(token, PString::Format("mount token '%s' must start and end with '/'", mount.Token));
			continue;
		}
		if (std::find(context.Tokens().begin(), context.Tokens().end(), token) != context.Tokens().end())
		{
			addError(token, PString::Format("mount token '%s' is given twice", mount.Token));
			continue;
		}

		HMountRoot root;
		root.Token     = token;
		root.Directory = absoluteGenericPath(mount.Directory);
		if (root.Directory.empty() || root.Directory.back() != '/')
		{
			root.Directory.push_back('/');
		}
		root.LowerDirectory = toLowerAscii(root.Directory);
		roots.push_back(root);
		context.Tokens().push_back(token);
	}

	// 파일 → 토큰 경로(같은 파일은 한 번, 토큰 경로 순)
	HList<HCookFile> files;
	HHashSet<HRawString> seenPaths;
	for (const PString& file : inFiles)
	{
		HCookFile cookFile;
		cookFile.File = absoluteGenericPath(file);
		if (toTokenPath(cookFile.File, roots, &cookFile.TokenPath) == false)
		{
			addError(cookFile.File, "file is outside the content mounts");
			continue;
		}
		if (seenPaths.insert(cookFile.TokenPath).second == false)
		{
			continue;
		}
		files.push_back(cookFile);
	}
	std::sort(files.begin(), files.end(), [](const HCookFile& a, const HCookFile& b)
		{
			return a.TokenPath < b.TokenPath;
		});
	for (const HCookFile& file : files)
	{
		context.Files().insert(file.TokenPath);
		context.FilesByLowerPath()[toLowerAscii(file.TokenPath)] = file.TokenPath;
	}

	// .jgasset: 1 형식 · 2 GUID · 3 참조
	HHashMap<HRawString, HRawString> firstPathByGuid;
	for (const HCookFile& file : files)
	{
		if (hasAssetExtension(file.TokenPath) == false)
		{
			continue;
		}

		PString text;
		if (HFileHelper::ReadAllText(PString(file.File.c_str()), &text) == false)
		{
			addError(file.TokenPath, "cannot read the file");
			continue;
		}

		PJson json;
		PString parseError;
		if (PJson::ToObjectWithError(text, &json, &parseError) == false)
		{
			addError(file.TokenPath, PString::Format("JSON %s", parseError));
			continue;
		}

		const rapidjson::Document& document = json.GetDocument();
		const rapidjson::Value* objectType = findMember(document, "JGObjectType");
		if (objectType == nullptr || objectType->IsString() == false || objectType->GetStringLength() == 0)
		{
			addError(file.TokenPath, "JGObjectType is missing");
			continue;
		}
		const rapidjson::Value* object = findMember(document, "JGObject");
		if (object == nullptr || object->IsObject() == false)
		{
			addError(file.TokenPath, "JGObject is missing");
			continue;
		}

		const rapidjson::Value* assetSection = findMember(*object, "JGAsset");
		const rapidjson::Value* guid = (assetSection != nullptr) ? findMember(*assetSection, "_guid") : nullptr;
		HRawString canonicalGuid;
		if (guid == nullptr || guid->IsString() == false)
		{
			addError(file.TokenPath, "JGAsset._guid is missing");
		}
		else if (parseGuidText(guid->GetString(), guid->GetStringLength(), &canonicalGuid) == false)
		{
			addError(file.TokenPath, PString::Format("JGAsset._guid '%s' is not a GUID", toText(*guid)));
		}
		else
		{
			HHashMap<HRawString, HRawString>::const_iterator first = firstPathByGuid.find(canonicalGuid);
			if (first != firstPathByGuid.end())
			{
				addError(file.TokenPath, PString::Format("GUID %s is also used by %s (the asset database keeps only the first one; give the copy a new _guid)",
					PString(canonicalGuid.c_str()), PString(first->second.c_str())));
			}
			else
			{
				firstPathByGuid[canonicalGuid] = file.TokenPath;
			}
		}

		HReferenceCollector collector(context.Tokens());
		collector.Collect(document);
		for (const HFoundReference& reference : collector.References)
		{
			const HRawString target = normalizeTokenPath(reference.Target, context.Tokens());
			if (context.Files().contains(target))
			{
				continue;
			}

			const PString where = (reference.Count > 1)
				? PString::Format("%s and %d more place(s)", PString(reference.At.c_str()), reference.Count - 1)
				: PString(reference.At.c_str());
			HHashMap<HRawString, HRawString>::iterator caseMatch = context.FilesByLowerPath().find(toLowerAscii(target));
			if (caseMatch != context.FilesByLowerPath().end())
			{
				addError(file.TokenPath, PString::Format("reference '%s' at %s differs in case from '%s' (asset lookup is case-sensitive)",
					PString(reference.Target.c_str()), where, PString(caseMatch->second.c_str())));
			}
			else
			{
				addError(file.TokenPath, PString::Format("reference '%s' at %s is not in the cook set", PString(reference.Target.c_str()), where));
			}
		}

		HAssetCookAsset asset;
		asset.File       = PString(file.File.c_str());
		asset.TokenPath  = PString(file.TokenPath.c_str());
		asset.ObjectType = toText(*objectType);
		for (const HAssetCookTypeCheck& typeCheck : inTypeChecks)
		{
			if (typeCheck.ObjectType.GetRawString() == asset.ObjectType.GetRawString())
			{
				asset.JsonText = text;
				break;
			}
		}
		context.AddAsset(asset);
	}

	// 타입별 검사(모든 에셋을 읽은 뒤 — 다른 에셋을 찾을 수 있게)
	for (const HAssetCookAsset& asset : context.GetAssets())
	{
		for (const HAssetCookTypeCheck& typeCheck : inTypeChecks)
		{
			if (typeCheck.Check && typeCheck.ObjectType.GetRawString() == asset.ObjectType.GetRawString())
			{
				typeCheck.Check(context, asset, issues);
			}
		}
	}

	// 파일별로 모은다(같은 파일 안은 나온 순서)
	std::stable_sort(issues.begin(), issues.end(), [](const HAssetCookIssue& a, const HAssetCookIssue& b)
		{
			return a.File.GetRawString() < b.File.GetRawString();
		});

	bool bOk = true;
	for (const HAssetCookIssue& issue : issues)
	{
		if (issue.Severity == EAssetCookIssueSeverity::Error)
		{
			bOk = false;
		}
		outIssues.push_back(issue);
	}
	return bOk;
}
