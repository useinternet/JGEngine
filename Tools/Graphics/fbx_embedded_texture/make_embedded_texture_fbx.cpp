// 5-2 검증용 FBX 생성기 (2026-09-29)
// 임베디드 텍스처 3장을 가진 쿼드 메시 FBX를 assimp 익스포터로 만들고,
// 엔진 임포터(JGFBXAssetImporter::Import)와 같은 플래그로 다시 읽어 텍스처가 들어갔는지, 디코딩한 픽셀이 기대값과 같은지 확인한다.
//
//   textures/Checker.png  300 x 150 RGBA  (행 바이트 1200, 256 정렬 아님. 알파 반투명 절반)  -> 정확 비교
//   textures/Noise.png   1024 x 1024 RGBA (LCG 노이즈. 압축해도 약 4MB, 옛 JSON 2MB 한도를 넘는다) -> 정확 비교
//   C:\Art\Photo.jpg        64 x 64 RGB   (JPEG 손실. 평균 오차만 본다)
//
// 빌드: build.bat (VS 2022 vcvars64, assimp-mt.lib). 실행: make_embedded_texture_fbx.exe <출력 .fbx>
// 기대 픽셀 함수(checkerPixel, noisePixels, photoPixel)는 엔진 쪽 검증 코드와 같아야 한다.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <string>
#include <vector>

#include "assimp/Exporter.hpp"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

namespace
{
	constexpr int CheckerWidth  = 300;
	constexpr int CheckerHeight = 150;
	constexpr int NoiseSize     = 1024;
	constexpr int PhotoSize     = 64;

	void checkerPixel(int x, int y, unsigned char out[4])
	{
		out[0] = (unsigned char)(x * 255 / (CheckerWidth - 1));         // R: 왼쪽 -> 오른쪽 증가
		out[1] = (unsigned char)(y * 255 / (CheckerHeight - 1));        // G: 위 -> 아래 증가
		out[2] = (((x / 30) + (y / 30)) % 2) ? 255 : 0;                 // B: 30px 체커
		out[3] = (x < CheckerWidth / 2) ? 255 : 128;                    // A: 오른쪽 절반 반투명
	}

	std::vector<unsigned char> noisePixels()
	{
		std::vector<unsigned char> pixels((size_t)NoiseSize * NoiseSize * 4);
		uint32_t state = 12345u;
		for (size_t i = 0; i < pixels.size(); i += 4)
		{
			state = state * 1664525u + 1013904223u;
			pixels[i + 0] = (unsigned char)(state >> 24);
			pixels[i + 1] = (unsigned char)(state >> 16);
			pixels[i + 2] = (unsigned char)(state >> 8);
			pixels[i + 3] = (unsigned char)(state);
		}
		return pixels;
	}

	void photoPixel(int x, int y, unsigned char out[3])
	{
		out[0] = (unsigned char)(x * 4);
		out[1] = (unsigned char)(y * 4);
		out[2] = 128;
	}

	void appendToVector(void* context, void* data, int size)
	{
		std::vector<unsigned char>* buffer = static_cast<std::vector<unsigned char>*>(context);
		const unsigned char* bytes = static_cast<const unsigned char*>(data);
		buffer->insert(buffer->end(), bytes, bytes + size);
	}

	std::vector<unsigned char> makeCheckerPng()
	{
		std::vector<unsigned char> pixels((size_t)CheckerWidth * CheckerHeight * 4);
		for (int y = 0; y < CheckerHeight; ++y)
		{
			for (int x = 0; x < CheckerWidth; ++x)
			{
				checkerPixel(x, y, &pixels[((size_t)y * CheckerWidth + x) * 4]);
			}
		}
		std::vector<unsigned char> png;
		stbi_write_png_to_func(appendToVector, &png, CheckerWidth, CheckerHeight, 4, pixels.data(), CheckerWidth * 4);
		return png;
	}

	std::vector<unsigned char> makeNoisePng()
	{
		const std::vector<unsigned char> pixels = noisePixels();
		std::vector<unsigned char> png;
		stbi_write_png_to_func(appendToVector, &png, NoiseSize, NoiseSize, 4, pixels.data(), NoiseSize * 4);
		return png;
	}

	std::vector<unsigned char> makePhotoJpg()
	{
		std::vector<unsigned char> pixels((size_t)PhotoSize * PhotoSize * 3);
		for (int y = 0; y < PhotoSize; ++y)
		{
			for (int x = 0; x < PhotoSize; ++x)
			{
				photoPixel(x, y, &pixels[((size_t)y * PhotoSize + x) * 3]);
			}
		}
		std::vector<unsigned char> jpg;
		stbi_write_jpg_to_func(appendToVector, &jpg, PhotoSize, PhotoSize, 3, pixels.data(), 95);
		return jpg;
	}

	aiTexture* makeCompressedTexture(const std::vector<unsigned char>& bytes, const char* hint, const char* filename)
	{
		aiTexture* tex = new aiTexture();
		tex->mWidth  = (unsigned int)bytes.size();   // 압축 텍스처: mWidth = 바이트 수, mHeight = 0
		tex->mHeight = 0;
		strncpy_s(tex->achFormatHint, sizeof(tex->achFormatHint), hint, _TRUNCATE);
		tex->pcData = new aiTexel[(bytes.size() + sizeof(aiTexel) - 1) / sizeof(aiTexel)];
		memcpy(tex->pcData, bytes.data(), bytes.size());
		tex->mFilename.Set(filename);
		return tex;
	}

	aiScene* makeScene(const char* paths[3], const std::vector<unsigned char>* images[3], const char* hints[3])
	{
		aiScene* scene = new aiScene();

		scene->mRootNode = new aiNode("Root");
		scene->mRootNode->mNumMeshes = 1;
		scene->mRootNode->mMeshes = new unsigned int[1] { 0 };

		aiMesh* mesh = new aiMesh();
		mesh->mName = "EmbeddedTextureQuad";
		mesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
		mesh->mMaterialIndex = 0;
		mesh->mNumVertices = 4;
		mesh->mVertices = new aiVector3D[4] { { -100.0f, 0.0f, 0.0f }, { 100.0f, 0.0f, 0.0f }, { 100.0f, 100.0f, 0.0f }, { -100.0f, 100.0f, 0.0f } };
		mesh->mNormals  = new aiVector3D[4] { { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } };
		mesh->mTextureCoords[0] = new aiVector3D[4] { { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
		mesh->mNumUVComponents[0] = 2;
		mesh->mNumFaces = 2;
		mesh->mFaces = new aiFace[2];
		mesh->mFaces[0].mNumIndices = 3;
		mesh->mFaces[0].mIndices = new unsigned int[3] { 0, 1, 2 };
		mesh->mFaces[1].mNumIndices = 3;
		mesh->mFaces[1].mIndices = new unsigned int[3] { 0, 2, 3 };

		scene->mNumMeshes = 1;
		scene->mMeshes = new aiMesh*[1] { mesh };

		aiMaterial* material = new aiMaterial();
		aiString materialName("EmbeddedTextureMaterial");
		material->AddProperty(&materialName, AI_MATKEY_NAME);
		aiString path0(paths[0]);
		aiString path1(paths[1]);
		aiString path2(paths[2]);
		material->AddProperty(&path0, AI_MATKEY_TEXTURE_DIFFUSE(0));
		material->AddProperty(&path1, AI_MATKEY_TEXTURE_NORMALS(0));
		material->AddProperty(&path2, AI_MATKEY_TEXTURE_SPECULAR(0));

		scene->mNumMaterials = 1;
		scene->mMaterials = new aiMaterial*[1] { material };

		scene->mNumTextures = 3;
		scene->mTextures = new aiTexture*[3];
		for (int i = 0; i < 3; ++i)
		{
			scene->mTextures[i] = makeCompressedTexture(*images[i], hints[i], paths[i]);
		}
		return scene;
	}

	// 엔진 JGFBXAssetImporter::Import 와 같은 플래그
	constexpr unsigned int EngineImportFlags =
		aiProcess_JoinIdenticalVertices | aiProcess_ValidateDataStructure | aiProcess_ImproveCacheLocality |
		aiProcess_RemoveRedundantMaterials | aiProcess_GenUVCoords | aiProcess_TransformUVCoords |
		aiProcess_FindInstances | aiProcess_LimitBoneWeights | aiProcess_OptimizeMeshes |
		aiProcess_GenSmoothNormals | aiProcess_SplitLargeMeshes | aiProcess_Triangulate |
		aiProcess_ConvertToLeftHanded | aiProcess_SortByPType | aiProcess_CalcTangentSpace;

	int verifyDecoded(const char* filename, const unsigned char* rgba, int w, int h)
	{
		const char* shortName = aiScene::GetShortFilename(filename);
		if (strcmp(shortName, "Checker.png") == 0)
		{
			if (w != CheckerWidth || h != CheckerHeight)
			{
				printf("    FAIL size %dx%d (expected %dx%d)\n", w, h, CheckerWidth, CheckerHeight);
				return 1;
			}
			int mismatches = 0;
			for (int y = 0; y < h; ++y)
			{
				for (int x = 0; x < w; ++x)
				{
					unsigned char expected[4];
					checkerPixel(x, y, expected);
					if (memcmp(expected, rgba + ((size_t)y * w + x) * 4, 4) != 0)
					{
						++mismatches;
					}
				}
			}
			printf("    Checker exact compare: %d mismatched pixels\n", mismatches);
			return mismatches == 0 ? 0 : 1;
		}
		if (strcmp(shortName, "Noise.png") == 0)
		{
			if (w != NoiseSize || h != NoiseSize)
			{
				printf("    FAIL size %dx%d\n", w, h);
				return 1;
			}
			const std::vector<unsigned char> expected = noisePixels();
			const bool same = memcmp(expected.data(), rgba, expected.size()) == 0;
			printf("    Noise exact compare: %s\n", same ? "same" : "DIFFERENT");
			return same ? 0 : 1;
		}
		if (strcmp(shortName, "Photo.jpg") == 0)
		{
			if (w != PhotoSize || h != PhotoSize)
			{
				printf("    FAIL size %dx%d\n", w, h);
				return 1;
			}
			double errorSum = 0.0;
			for (int y = 0; y < h; ++y)
			{
				for (int x = 0; x < w; ++x)
				{
					unsigned char expected[3];
					photoPixel(x, y, expected);
					const unsigned char* actual = rgba + ((size_t)y * w + x) * 4;
					for (int c = 0; c < 3; ++c)
					{
						errorSum += fabs((double)expected[c] - (double)actual[c]);
					}
				}
			}
			const double meanError = errorSum / ((double)w * h * 3);
			printf("    Photo mean abs error: %.2f (alpha of pixel 0 = %d)\n", meanError, rgba[3]);
			return meanError < 4.0 ? 0 : 1;
		}
		printf("    (unknown texture name, not compared)\n");
		return 1;
	}
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("usage: make_embedded_texture_fbx <out.fbx>\n");
		return 2;
	}
	const char* outPath = argv[1];

	const std::vector<unsigned char> checkerPng = makeCheckerPng();
	const std::vector<unsigned char> noisePng   = makeNoisePng();
	const std::vector<unsigned char> photoJpg   = makePhotoJpg();
	printf("encoded: Checker.png %zu bytes, Noise.png %zu bytes, Photo.jpg %zu bytes\n", checkerPng.size(), noisePng.size(), photoJpg.size());

	const char* paths[3] = { "textures/Checker.png", "textures/Noise.png", "C:\\Art\\Photo.jpg" };
	const std::vector<unsigned char>* images[3] = { &checkerPng, &noisePng, &photoJpg };
	const char* hints[3] = { "png", "png", "jpg" };

	aiScene* scene = makeScene(paths, images, hints);

	Assimp::Exporter exporter;
	const aiReturn exportResult = exporter.Export(scene, "fbx", outPath);
	delete scene;
	if (exportResult != aiReturn_SUCCESS)
	{
		printf("export failed: %s\n", exporter.GetErrorString());
		return 3;
	}
	printf("exported: %s\n", outPath);

	Assimp::Importer importer;
	const aiScene* imported = importer.ReadFile(outPath, EngineImportFlags);
	if (imported == nullptr)
	{
		printf("re-import failed: %s\n", importer.GetErrorString());
		return 4;
	}

	printf("re-import: meshes %u, materials %u, textures %u\n", imported->mNumMeshes, imported->mNumMaterials, imported->mNumTextures);
	for (unsigned int m = 0; m < imported->mNumMaterials; ++m)
	{
		const aiMaterial* material = imported->mMaterials[m];
		const aiTextureType types[3] = { aiTextureType_DIFFUSE, aiTextureType_NORMALS, aiTextureType_SPECULAR };
		for (aiTextureType type : types)
		{
			aiString path;
			if (material->GetTexture(type, 0, &path) == aiReturn_SUCCESS)
			{
				printf("  material %u texture(type %d) path = \"%s\"\n", m, (int)type, path.C_Str());
			}
		}
	}

	int failures = 0;
	for (unsigned int i = 0; i < imported->mNumTextures; ++i)
	{
		const aiTexture* tex = imported->mTextures[i];
		printf("  texture %u: mWidth %u, mHeight %u, hint \"%s\", filename \"%s\"\n", i, tex->mWidth, tex->mHeight, tex->achFormatHint, tex->mFilename.C_Str());
		if (tex->mHeight != 0)
		{
			printf("    uncompressed texture (not expected from FBX)\n");
			++failures;
			continue;
		}
		int w = 0;
		int h = 0;
		int channels = 0;
		unsigned char* rgba = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(tex->pcData), (int)tex->mWidth, &w, &h, &channels, 4);
		if (rgba == nullptr)
		{
			printf("    decode failed: %s\n", stbi_failure_reason());
			++failures;
			continue;
		}
		printf("    decoded %dx%d, channels in file %d\n", w, h, channels);
		failures += verifyDecoded(tex->mFilename.C_Str(), rgba, w, h);
		stbi_image_free(rgba);
	}

	if (imported->mNumTextures != 3)
	{
		printf("FAIL: expected 3 embedded textures\n");
		return 5;
	}
	printf(failures == 0 ? "RESULT: PASS\n" : "RESULT: FAIL (%d)\n", failures);
	return failures == 0 ? 0 : 6;
}
