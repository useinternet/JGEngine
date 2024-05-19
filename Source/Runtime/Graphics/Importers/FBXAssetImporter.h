#pragma once
#include "AssetImporter.h"
#include "JGGraphicsDefine.h"

struct aiScene;
struct aiMesh;
struct aiAnimation;
struct aiNode;
struct aiTexture;

JG_ENUM_FLAG(EFBXAssetImportFlags)
enum class EFBXAssetImportFlags
{
	None = 0x000,
	Import_Mesh = 0x001,
	Import_Skeletal = 0x002,
	Import_AnimationClip = 0x004,
	Import_Texture = 0x008,
};

class JGFBXAssetImporter;
class GRAPHICS_API PFBXAssetImportArguments : public PAssetImportArguments
{
public:
	EFBXAssetImportFlags Flags;

public:
	virtual JGType GetArgumentType() const { return JGTYPE(PFBXAssetImportArguments); }
	virtual JGType GetImporterType() const { return JGTYPE(JGFBXAssetImporter); }
	virtual ~PFBXAssetImportArguments() = default;
};


JGCLASS()
class GRAPHICS_API JGFBXAssetImporter : public JGAssetImporter
{
	JG_GENERATED_CLASS_BODY
public:
	struct HMeshStock
	{
		PString Name;
		HList<PName> SubMeshNames;
		HList<HList<HVertex>>  Vertices;
		HList<HList<HBoneVertex>> BoneVertices;
		HList<HList<HBoneOffsetData>> BoneOffsetDatas;
		HList<HList<uint32>> Indices;
		HBBox BoundingBox;
	};

	struct HTextureStock
	{
	public:
		PString Name;
		int32 Width = 0;
		int32 Height = 0;
		int32 Channels = 0;
		uint32 PixelPerUnit = 100;
		uint32 OriginPixelSize = 0;
		HList<uint8> Pixels;
	};

	class HSkeletalStock
	{
	public:
		struct BoneNode
		{
			uint32 ID;
			PString Name;

			uint32 ParentNode;
			HList<uint32> ChildNodes;

			HMatrix Transform;
		};
	public:
		PString Name;
		uint32	RootBoneNode;
		HList<BoneNode> BoneNodes;
		HMatrix RootOffset;
	};

	struct HAnimationClipStock
	{
	public:
		struct HAnimationNode
		{
			PString NodeName;

			HList<HVector3>	  LocationValues;
			HList<HQuaternion> RotationValues;
			HList<HVector3>    ScaleValues;

			HList<float32> LocationTimes;
			HList<float32> RotationTimes;
			HList<float32> ScaleTimes;
		};
	public:
		PString Name;
		float32 Duration;
		float32 TicksPerSecond;
		HHashMap<PString, HAnimationNode> AnimationNodes;
	};

	struct HSceneHierarchyInfo
	{
		HHashMap<PString, uint32> NodeIDMap;
		HHashSet<PString> MeshNodeSet;
	};

	PFBXAssetImportArguments _args;

public:
	virtual ~JGFBXAssetImporter() = default;

	virtual bool Import(PSharedPtr<PAssetImportArguments> inArgs);
	virtual bool IsSupportedExtension(const PString& inExtension) const;

private:
	void ReadMesh(const aiScene* scene, const aiMesh* mesh, HMeshStock* inOutStock);
	void ReadSkeletal(const aiScene* scene, HSkeletalStock* outStock, HSceneHierarchyInfo* outSceneHierarchyInfo);
	void ReadSkeletalNodeHierarchy(const aiNode* node, HSceneHierarchyInfo* outSceneHierarchyInfo, HSkeletalStock* outStock);
	void ReadAnimation(const aiAnimation* anim, HAnimationClipStock* outStock);
	void ReadTexture(const aiTexture* tex, HTextureStock* outStock);

private:
	void WriteMesh(const HMeshStock& inStock);
	void WriteTexture(const HTextureStock& inStock);
};