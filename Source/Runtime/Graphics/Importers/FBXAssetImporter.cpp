#include "PCH/PCH.h"
#include "FBXAssetImporter.h"
#include "Classes/StaticMesh.h"
#include "Classes/Texture.h"
#include "stb/stb_image.h"
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

			if (_args.Flags & EFBXAssetImportFlags::Import_Skeletal)
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
			if (_args.Flags & EFBXAssetImportFlags::Import_Mesh)
			{
				HMeshStock mestStock;
				mestStock.Name = scene->mName.C_Str();
				uint32 meshCount = scene->mNumMeshes;
				for (uint32 i = 0; i < meshCount; ++i)
				{
					auto mesh = scene->mMeshes[i];
					ReadMesh(scene, mesh, &mestStock);
				}
				//WriteMesh(setting.OutputPath, mestStock);
			}
		}
		if (scene->HasAnimations() == true && _args.Flags & EFBXAssetImportFlags::Import_AnimationClip)
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
		if (scene->HasTextures() == true && _args.Flags & EFBXAssetImportFlags::Import_Texture)
		{
			// Texture
			uint32 texCnt = scene->mNumTextures;
			for (uint32 i = 0; i < texCnt; ++i)
			{
				HTextureStock texStock;
				aiTexture* tex = scene->mTextures[i];
				ReadTexture(tex, &texStock);
				//WriteTexture(setting.OutputPath, texInfo);
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
			JG_LOG(Asset, ELogLevel::Warning, "This Mesh is multiple texcoord : {0}", inOutStock->Name);
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

void JGFBXAssetImporter::ReadTexture(const aiTexture* tex, HTextureStock* outStock)
{
	if (outStock == nullptr || tex == nullptr)
	{
		return;
	}

	outStock->Name   = tex->mFilename.C_Str();
	outStock->Width  = tex->mWidth;
	outStock->Height = tex->mHeight;
	outStock->Channels = 4;

	uint32 pixelSize = outStock->Width * outStock->Height * outStock->Channels;
	outStock->OriginPixelSize = pixelSize;

	outStock->Pixels.resize(pixelSize);
	HPlatform::MemCopy(tex->pcData, outStock->Pixels.data(), pixelSize);
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
		args.Name = inStock.Name;
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

	HTextureConstructArguments args;
	args.TextureInfo.Name = inStock.Name;
	args.TextureInfo.Width = inStock.Width;
	args.TextureInfo.Height = inStock.Height;
	args.TextureInfo.PixelPerUnit = inStock.PixelPerUnit;
	args.TextureInfo.Format = ETextureFormat::R16G16B16A16_Float;
	
	args.Pixels = std::move(inStock.Pixels);

	PSharedPtr<JGTexture> texture = GetGraphicsAPI().CreateTexture(args);
	if (SaveObject(destPath, texture.GetRawPointer()))
	{
		JG_LOG(Asset, ELogLevel::Trace, "%s : Success Save Texture", destPath);
	}
	else
	{
		JG_LOG(Asset, ELogLevel::Error, "%s : Fail Save Texture", destPath);
	}
}
