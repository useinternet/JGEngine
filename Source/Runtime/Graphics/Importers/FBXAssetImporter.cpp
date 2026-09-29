#include "PCH/PCH.h"
#include "FBXAssetImporter.h"
#include "Classes/StaticMesh.h"
#include "Classes/Texture.h"
// 임베디드 텍스처(PNG, JPG 등 원본 파일 바이트) 디코딩. Graphics 모듈에서 stb_image 구현은 이 파일 하나에만 둔다.
#pragma warning(push)
#pragma warning(disable : 4996)
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#pragma warning(pop)
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "JGGraphics.h"

HMatrix ToHMatrix(const aiMatrix4x4& aiMatrix)
{
	return HMatrix::Transpose(HMatrix(
		aiMatrix.a1, aiMatrix.a2, aiMatrix.a3, aiMatrix.a4,
		aiMatrix.b1, aiMatrix.b2, aiMatrix.b3, aiMatrix.b4,
		aiMatrix.c1, aiMatrix.c2, aiMatrix.c3, aiMatrix.c4,
		aiMatrix.d1, aiMatrix.d2, aiMatrix.d3, aiMatrix.d4));
};

namespace
{
	// 임베디드 텍스처의 에셋 이름. FBX는 원본 경로("textures/Albedo.png", "C:\Art\Albedo.png")를 주므로 파일 이름만 쓰고 확장자는 뗀다.
	// 이름이 없거나 "*0" 같은 인덱스 참조면 "<FBX 파일 이름>_Texture<인덱스>"로 짓는다.
	PString makeTextureAssetName(const aiTexture* inTexture, uint32 inTextureIndex, const PString& inSrcPath)
	{
		const char* fullPath = inTexture->mFilename.C_Str();
		const char* fileName = fullPath;
		for (const char* c = fullPath; *c != '\0'; ++c)
		{
			if (*c == '/' || *c == '\\')
			{
				fileName = c + 1;
			}
		}

		const char* extension = strrchr(fileName, '.');
		const uint64 nameLength = (extension != nullptr) ? (uint64)(extension - fileName) : (uint64)strlen(fileName);
		if (nameLength == 0 || fileName[0] == '*')
		{
			PString srcName;
			HFileHelper::FileNameOnly(inSrcPath, &srcName);
			return PString::Format("%s_Texture%d", srcName, (int32)inTextureIndex);
		}

		// 파일 이름에 못 쓰는 문자는 '_'로 바꾼다. PString은 만들 때 해시를 계산하므로 버퍼에서 고친 뒤 한 번에 만든다.
		HList<char> buffer(fileName, fileName + nameLength);
		for (char& c : buffer)
		{
			if ((uint8)c < 32 || strchr("<>:\"|?*", c) != nullptr)
			{
				c = '_';
			}
		}
		buffer.push_back('\0');
		return PString(buffer.data());
	}
}

bool JGFBXAssetImporter::Import(PSharedPtr<PAssetImportArguments> inArgs)
{
	if (inArgs == nullptr)
	{
		return false;
	}

	if (inArgs->GetArgumentType() != JGTYPE(PFBXAssetImportArguments))
	{
		JG_LOG(Asset, ELogLevel::Error, "Import : dismatch import arguments type");
		return false;
	}

	if (inArgs->GetImporterType() != JGTYPE(JGFBXAssetImporter))
	{
		JG_LOG(Asset, ELogLevel::Error, "Import : dismatch importer type");
		return false;
	}

	if (HFileHelper::Exists(inArgs->SrcPath) == false)
	{
		JG_LOG(Asset, ELogLevel::Error, "Import : not exist src file");
		return false;
	}

	_args = *(Cast<PFBXAssetImportArguments>(inArgs));

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(inArgs->SrcPath.GetCStr(),
		aiProcess_JoinIdenticalVertices |     // 동일한 꼭지점 결합, 인덱싱 최적화
		aiProcess_ValidateDataStructure |     // 로더의 출력을 검증
		aiProcess_ImproveCacheLocality |      // 출력 정점의 캐쉬위치를 개선
		aiProcess_RemoveRedundantMaterials |  // 중복된 매터리얼 제거
		aiProcess_GenUVCoords |               // 구형, 원통형, 상자 및 평면 매핑을 적절한 UV로 변환
		aiProcess_TransformUVCoords |         // UV 변환 처리기 (스케일링, 변환...)
		aiProcess_FindInstances |             // 인스턴스된 매쉬를 검색하여 하나의 마스터에 대한 참조로 제거
		aiProcess_LimitBoneWeights |          // 정점당 뼈의 가중치를 최대 4개로 제한
		aiProcess_OptimizeMeshes |            // 가능한 경우 작은 매쉬를 조인
		aiProcess_GenSmoothNormals |          // 부드러운 노말벡터(법선벡터) 생성
		aiProcess_SplitLargeMeshes |          // 거대한 하나의 매쉬를 하위매쉬들로 분활(나눔)
		aiProcess_Triangulate |               // 3개 이상의 모서리를 가진 다각형 면을 삼각형으로 만듬(나눔)
		aiProcess_ConvertToLeftHanded |       // D3D의 왼손좌표계로 변환
		aiProcess_SortByPType |               // 단일타입의  프리미티브로 구성된 '깨끗한' 매쉬를 만듬
		aiProcess_CalcTangentSpace            // 탄젠트 공간 계산 )
	);
	if (scene != nullptr)
	{
		if (scene->HasMeshes() == true)
		{
			
			if (EnumHasAnyFlags(_args.Flags, EFBXAssetImportFlags::Import_Skeletal))
			{
				uint32 meshCount = scene->mNumMeshes;
				for (uint32 i = 0; i < meshCount; ++i)
				{
					aiMesh* mesh = scene->mMeshes[i];
					if (mesh->HasBones() == true)
					{
						HSceneHierarchyInfo hierarchyInfo;
						HSkeletalStock skeletalInfo;
						skeletalInfo.Name = PString(mesh->mName.C_Str()) + PString("_Skeletal");

						ReadSkeletal(scene, &skeletalInfo, &hierarchyInfo);
						//WriteSkeletal(setting.OutputPath, skeletalInfo);
					}
				}
			}
			if (EnumHasAnyFlags(_args.Flags, EFBXAssetImportFlags::Import_Mesh))
			{
				HMeshStock meshStock;
				meshStock.Name = scene->mName.C_Str();
				uint32 meshCount = scene->mNumMeshes;
				for (uint32 i = 0; i < meshCount; ++i)
				{
					const aiMesh* mesh = scene->mMeshes[i];
					if(meshStock.Name.Empty())
					{
						meshStock.Name = mesh->mName.C_Str();
					}
					
					ReadMesh(scene, mesh, &meshStock);
				}
				
				WriteMesh(meshStock);
			}
		}
		if (scene->HasAnimations() == true && EnumHasAnyFlags(_args.Flags, EFBXAssetImportFlags::Import_AnimationClip))
		{
			uint32 animCount = scene->mNumAnimations;
			for (uint32 i = 0; i < animCount; ++i)
			{
				HAnimationClipStock animClipStock;
				aiAnimation* anim = scene->mAnimations[i];

				ReadAnimation(anim, &animClipStock);
				//WriteAnimation(setting.OutputPath, animInfo);
			}
		}
		if (scene->HasMaterials() == true)
		{
			uint32 materialCount = scene->mNumMaterials;
			for (uint32 i = 0; i < materialCount; ++i)
			{
				//MaterialAssetStock materialInfo;

				//aiMaterial* mat = scene->mMaterials[i];

				//ReadMaterial(mat, &materialInfo);

			}
		}
		if (scene->HasTextures() == true && EnumHasAnyFlags(_args.Flags, EFBXAssetImportFlags::Import_Texture))
		{
			// FBX 안에 들어 있는(embedded) 텍스처만 온다. 외부 파일로 참조된 텍스처는 mTextures에 없다.
			HHashSet<PString> writtenNames;
			uint32 texCnt = scene->mNumTextures;
			for (uint32 i = 0; i < texCnt; ++i)
			{
				HTextureStock texStock;
				const aiTexture* tex = scene->mTextures[i];
				if (ReadTexture(tex, i, &texStock) == false)
				{
					continue;
				}

				// 폴더만 다르고 파일 이름이 같은 텍스처가 서로를 덮어쓰지 않게 한다.
				if (writtenNames.contains(texStock.Name))
				{
					const PString originName = texStock.Name;
					texStock.Name = PString::Format("%s_%d", originName, (int32)i);
					JG_LOG(Asset, ELogLevel::Warning, "%s : Embedded texture name %s is duplicated. Saved as %s", _args.SrcPath, originName, texStock.Name);
				}
				writtenNames.insert(texStock.Name);

				WriteTexture(texStock);
			}
		}

	}
	else
	{
		JG_LOG(Asset, ELogLevel::Error, "Assimp Importer ReadFiles Error : {%s}", importer.GetErrorString());
		return false;
	}

	return true;
}

bool JGFBXAssetImporter::IsSupportedExtension(const PString& inExtension) const
{
	if (PString::ToLower(".FBX") == PString::ToLower(inExtension))
	{
		return true;
	}

	return false;
}


void JGFBXAssetImporter::ReadMesh(const aiScene* scene, const aiMesh* mesh, HMeshStock* inOutStock)
{
	if (inOutStock == nullptr || mesh == nullptr)
	{
		return;
	}

	HList<HVertex>     vertices;
	HList<HBoneVertex> boneVertices;
	HList<HBoneOffsetData> boneOffsetDatas;
	HList<uint32>   indices;
	inOutStock->SubMeshNames.push_back(PName(mesh->mName.C_Str()));

	vertices.resize(mesh->mNumVertices);

	for (uint32 i = 0; i < mesh->mNumVertices; ++i)
	{
		HVertex v;
		if (mesh->HasPositions() == true)
		{
			auto& ai_pos = mesh->mVertices[i];
			v.Position.x = ai_pos.x;
			v.Position.y = ai_pos.y;
			v.Position.z = ai_pos.z;

			for (int32 i = 0; i < 3; ++i)
			{
				inOutStock->BoundingBox.min[i] = std::min<float>(inOutStock->BoundingBox.min[i], v.Position[i]);
				inOutStock->BoundingBox.max[i] = std::max<float>(inOutStock->BoundingBox.max[i], v.Position[i]);
			}
		}

		int32 multipleTex = 0;
		int32 texCount = 0;
		while (mesh->HasTextureCoords(texCount) == true)
		{
			texCount++;
			multipleTex++;
		}

		if (multipleTex >= 2)
		{
			JG_LOG(Asset, ELogLevel::Warning, "This Mesh is multiple texcoord : %s", inOutStock->Name);
		}


		if (mesh->HasTextureCoords(0))
		{
			auto& ai_tex = mesh->mTextureCoords[0][i];
			v.Texcoord.x = ai_tex.x;
			v.Texcoord.y = ai_tex.y;
		}

		if (mesh->HasNormals() == true)
		{
			auto& ai_nor = mesh->mNormals[i];

			v.Normal.x = ai_nor.x;
			v.Normal.y = ai_nor.y;
			v.Normal.z = ai_nor.z;
		}


		if (mesh->HasTangentsAndBitangents() == true)
		{
			auto& ai_tan = mesh->mTangents[i];
			v.Tangent.x = ai_tan.x;
			v.Tangent.y = ai_tan.y;
			v.Tangent.z = ai_tan.z;

			auto& ai_bit = mesh->mBitangents[i];
			v.Bitangent.x = ai_bit.x;
			v.Bitangent.y = ai_bit.y;
			v.Bitangent.z = ai_bit.z;
		}


		vertices[i] = v;
	}

	inOutStock->Vertices.push_back(vertices);

	if (mesh->HasFaces() == true)
	{
		for (uint32 i = 0; i < mesh->mNumFaces; ++i)
		{
			auto& face = mesh->mFaces[i];

			for (uint32 j = 0; j < face.mNumIndices; ++j)
			{
				indices.push_back(face.mIndices[j]);
			}
		}
	}
	inOutStock->Indices.push_back(indices);
	
	if(EnumHasAnyFlags(_args.Flags, EFBXAssetImportFlags::Import_Skeletal))
	{
		if (mesh->HasBones() == true)
		{
			boneVertices.resize(vertices.size());

			HSceneHierarchyInfo info;
			ReadSkeletal(scene, nullptr, &info);
		
			uint32 boneCount = mesh->mNumBones;
			for (uint32 i = 0; i < boneCount; ++i)
			{
				aiBone* bone = mesh->mBones[i];
				// Bone

				// Weight
				uint32 weightCnt = bone->mNumWeights;
				for (uint32 j = 0; j < weightCnt; ++j)
				{
					uint32 vertexID = bone->mWeights[j].mVertexId;
					float32 vertexWeight = bone->mWeights[j].mWeight;

					for (uint32 k = 0; k < 4; ++k)
					{
						HBoneVertex& boneVertex = boneVertices[vertexID];
						if (boneVertex.BoneWeights[k] == 0.0f)
						{
							boneVertex.BoneIDs[k] = info.NodeIDMap[bone->mName.C_Str()];
							boneVertex.BoneWeights[k] = vertexWeight;
							break;
						}
					}
				}

				// Offset
				HBoneOffsetData offsetData;
				offsetData.ID = info.NodeIDMap[bone->mName.C_Str()];
				offsetData.Offset = ToHMatrix(bone->mOffsetMatrix);
				boneOffsetDatas.push_back(offsetData);
			}

			inOutStock->BoneOffsetDatas.push_back(boneOffsetDatas);
			inOutStock->BoneVertices.push_back(boneVertices);
		}
	}
}

void JGFBXAssetImporter::ReadSkeletal(const aiScene* scene, HSkeletalStock* outStock, HSceneHierarchyInfo* outSceneHierarchyInfo)
{
	if (outStock)
	{
		outStock->RootOffset = ToHMatrix(scene->mRootNode->mTransformation.Inverse());
		outStock->RootBoneNode = -1;
	}

	for (uint32 i = 0; i < scene->mNumMeshes; ++i)
	{
		outSceneHierarchyInfo->MeshNodeSet.insert(scene->mMeshes[i]->mName.C_Str());
	}

	for (uint32 i = 0; i < scene->mRootNode->mNumChildren; ++i)
	{
		ReadSkeletalNodeHierarchy(scene->mRootNode->mChildren[i], outSceneHierarchyInfo, outStock);
	}
}

void JGFBXAssetImporter::ReadSkeletalNodeHierarchy(const aiNode* node, JGFBXAssetImporter::HSceneHierarchyInfo* outSceneHierarchyInfo, JGFBXAssetImporter::HSkeletalStock* outStock)
{
	if (node == nullptr)
	{
		return;
	}

	if (outSceneHierarchyInfo->MeshNodeSet.find(node->mName.C_Str()) != outSceneHierarchyInfo->MeshNodeSet.end())
	{
		return;
	}

	if (outSceneHierarchyInfo->NodeIDMap.find(node->mName.C_Str()) == outSceneHierarchyInfo->NodeIDMap.end())
	{
		if (outStock != nullptr)
		{
			HSkeletalStock::BoneNode boneNode;
			boneNode.ID = (uint32)outSceneHierarchyInfo->NodeIDMap.size();
			boneNode.Name = node->mName.C_Str();


			outStock->BoneNodes.push_back(boneNode);
		}

		outSceneHierarchyInfo->NodeIDMap.emplace(PString(node->mName.C_Str()), (uint32)outSceneHierarchyInfo->NodeIDMap.size());
	}

	uint32 boneID = outSceneHierarchyInfo->NodeIDMap[node->mName.C_Str()];

	if (outStock)
	{
		HSkeletalStock::BoneNode& boneNode = outStock->BoneNodes[boneID];
		if (outStock->RootBoneNode == -1)
		{
			outStock->RootBoneNode = boneID;
			boneNode.ParentNode = -1;
		}
		else
		{
			aiNode* parentNode = node->mParent;
			uint32 parentBoneID = outSceneHierarchyInfo->NodeIDMap[parentNode->mName.C_Str()];
			HSkeletalStock::BoneNode& parentBoneNode = outStock->BoneNodes[parentBoneID];

			boneNode.ParentNode = parentBoneID;
			parentBoneNode.ChildNodes.push_back(boneID);
		}
		boneNode.Transform = ToHMatrix(node->mTransformation);
	}

	for (uint32 i = 0; i < node->mNumChildren; ++i)
	{
		ReadSkeletalNodeHierarchy(node->mChildren[i], outSceneHierarchyInfo, outStock);
	}
}

void JGFBXAssetImporter::ReadAnimation(const aiAnimation* anim, HAnimationClipStock* outStock)
{
	JG_LOG(Asset, ELogLevel::Error, "%s not support Read Animation", _args.SrcPath);
	JG_CHECK(false);
}

bool JGFBXAssetImporter::ReadTexture(const aiTexture* tex, uint32 inTextureIndex, HTextureStock* outStock)
{
	if (outStock == nullptr || tex == nullptr || tex->pcData == nullptr)
	{
		return false;
	}

	outStock->Name     = makeTextureAssetName(tex, inTextureIndex, _args.SrcPath);
	outStock->Channels = 4;

	if (tex->mHeight == 0)
	{
		// 압축 텍스처: pcData는 원본 파일(PNG, JPG 등) 바이트이고 mWidth가 그 바이트 수다. FBX의 임베디드 텍스처는 항상 이 경우다.
		// (이전 코드는 비압축으로 가정해 Width * 0 * 4 = 0 바이트를 복사했다)
		int32 width = 0;
		int32 height = 0;
		int32 channelsInFile = 0;
		stbi_uc* decoded = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(tex->pcData), (int32)tex->mWidth, &width, &height, &channelsInFile, 4);
		if (decoded == nullptr)
		{
			JG_LOG(Asset, ELogLevel::Error, "%s : Fail decode embedded texture %s (format hint \"%s\", %d bytes) : %s",
				_args.SrcPath, tex->mFilename.C_Str(), tex->achFormatHint, (int32)tex->mWidth, stbi_failure_reason());
			return false;
		}

		outStock->Width  = width;
		outStock->Height = height;

		const uint64 pixelSize = (uint64)width * height * 4;
		outStock->Pixels.resize(pixelSize);
		HPlatform::MemCopy(decoded, outStock->Pixels.data(), pixelSize);
		stbi_image_free(decoded);
	}
	else
	{
		// 비압축 텍스처: mWidth x mHeight 개의 aiTexel(ARGB8888, 메모리 순서 B G R A). 필드 이름으로 읽어 RGBA로 담는다.
		// FBX 로더는 만들지 않지만 assimp는 파일 내용으로 포맷을 고르므로 다른 포맷이 들어와도 맞게 둔다.
		outStock->Width  = tex->mWidth;
		outStock->Height = tex->mHeight;

		const uint64 texelCount = (uint64)tex->mWidth * tex->mHeight;
		outStock->Pixels.resize(texelCount * 4);
		for (uint64 i = 0; i < texelCount; ++i)
		{
			const aiTexel& texel = tex->pcData[i];
			outStock->Pixels[i * 4 + 0] = texel.r;
			outStock->Pixels[i * 4 + 1] = texel.g;
			outStock->Pixels[i * 4 + 2] = texel.b;
			outStock->Pixels[i * 4 + 3] = texel.a;
		}
	}

	outStock->OriginPixelSize = (uint32)outStock->Pixels.size();
	return true;
}

void JGFBXAssetImporter::WriteMesh(const HMeshStock& inStock)
{
	PString destPath;
	HFileHelper::CombinePath(_args.DestPath, inStock.Name + JG_ASSET_FORMAT, &destPath);

	// Mesh 만들기
	const bool bIsSkelMesh = inStock.BoneVertices.empty() == false;
	if (bIsSkelMesh == false)
	{
		HStaticMeshConstructArguments args;
		args.Name = HAssetPath(destPath);
		args.SubMeshNames = inStock.SubMeshNames;
		args.Verties = inStock.Vertices;
		args.Indeies = inStock.Indices;
		PSharedPtr<JGStaticMesh> staticMesh = GetGraphicsAPI().CreateStaticMesh(args);
		if (SaveObject(destPath, staticMesh.GetRawPointer()))
		{
			JG_LOG(Asset, ELogLevel::Trace, "%s : Success Save Mesh", destPath);
		}
		else
		{
			JG_LOG(Asset, ELogLevel::Error, "%s : Fail Save Mesh", destPath);
		}
	}
	else
	{
		JG_LOG(Asset, ELogLevel::Error, "%s not support Write Mesh", _args.SrcPath);
		JG_CHECK(false);
	}

}

void JGFBXAssetImporter::WriteTexture(const HTextureStock& inStock)
{
	PString destPath;
	HFileHelper::CombinePath(_args.DestPath, inStock.Name + JG_ASSET_FORMAT, &destPath);

	// 메시(WriteMesh)처럼 오브젝트 이름을 에셋 경로로 준다. JGAsset::SetName이 이름으로 AssetPath를 채우므로
	// 짧은 이름을 주면 "NOT Support Asset Path" 경고와 함께 AssetPath가 "(null)"로 저장된다(5-20). DestPath가 Content 밖이면 짧은 이름을 쓴다.
	const HAssetPath assetPath(destPath);

	HTextureConstructArguments args;
	args.TextureInfo.Name = assetPath.IsValid() ? assetPath.GetAssetPath().ToString() : inStock.Name;
	args.TextureInfo.Width = inStock.Width;
	args.TextureInfo.Height = inStock.Height;
	args.TextureInfo.PixelPerUnit = inStock.PixelPerUnit;
	// ReadTexture가 항상 RGBA8로 풀어 둔다. (이전에는 R16G16B16A16_Float라 픽셀당 8바이트로 읽어 4바이트 버퍼를 넘었다)
	args.TextureInfo.Format = ETextureFormat::R8G8B8A8_Unorm;
	// 업로드는 밉 0만 한다. MipLevel 0은 D3D12에서 전체 밉 체인이라 나머지 밉이 빈 채로 샘플링된다.
	args.TextureInfo.MipLevel  = 1;
	args.TextureInfo.ArraySize = 1;

	// CreateTexture가 const 인자를 받으므로 복사한다. (이전의 std::move는 const 참조라 어차피 복사였다)
	args.Pixels = inStock.Pixels;

	// 저장(WriteJson)은 방금 요청한 업로드를 반영한 뒤 GPU에서 픽셀을 다시 읽어 압축한다. (ReadbackTextureImmediate, 메인 스레드)
	PSharedPtr<JGTexture> texture = GetGraphicsAPI().CreateTexture(args);
	if (SaveObject(destPath, texture.GetRawPointer()))
	{
		JG_LOG(Asset, ELogLevel::Info, "%s : Success Save Texture (%dx%d, R8G8B8A8_Unorm)", destPath, inStock.Width, inStock.Height);
	}
	else
	{
		JG_LOG(Asset, ELogLevel::Error, "%s : Fail Save Texture", destPath);
	}
}
