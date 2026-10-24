#include "PCH/PCH.h"
#include "AudioEngine.h"
#include "FileIO/FileHelper.h"
#include <algorithm>
#include <cstring>

#pragma warning(push, 0)
#include "miniaudio/miniaudio.h"
#pragma warning(pop)

namespace
{
	// Content 토큰(Asset/AssetDefines.h 의 JG_ASSET_*_PATH_RECOGNITION_TOEKN 과 같은 값). Audio 가 Core 만 의존하는 따로 모듈이던 때 둔 사본이다.
	const char* const ENGINE_TOKEN = "/JGEngine/";
	const char* const GAME_TOKEN   = "/JGGame/";

	const uint32 MANUAL_DEFAULT_SAMPLE_RATE = 48000;
	const uint32 MANUAL_DEFAULT_CHANNELS    = 2;

	// 파일 갈고리(VFS). miniaudio 가 여는 파일을 메모리(Files)에서 읽는다 — 파일은 메인 스레드가 HFileHelper::ReadAllBytes 로 미리 채운다.
	// miniaudio 작업 스레드(흘려 풀기)에서도 불리므로 엔진 컨테이너 · 로그 · 엔진 할당을 쓰지 않는다.
	struct HMemoryVfs
	{
		ma_vfs_callbacks Callbacks;   // 첫 멤버여야 한다: miniaudio 는 ma_vfs* 를 ma_vfs_callbacks* 로 본다
		std::mutex       Mutex;       // Files 를 지킨다(메인 스레드 추가 · 작업 스레드 찾기)
		std::unordered_map<std::string, std::shared_ptr<const std::vector<uint8>>> Files;
	};

	// 열린 파일 하나. 내용은 공유하고 읽는 자리는 따로 갖는다.
	struct HMemoryFile
	{
		std::shared_ptr<const std::vector<uint8>> Bytes;
		size_t Cursor = 0;
	};

	ma_result vfsOpen(ma_vfs* pVFS, const char* pFilePath, ma_uint32 openMode, ma_vfs_file* pFile)
	{
		if (pVFS == nullptr || pFilePath == nullptr || pFile == nullptr)
		{
			return MA_INVALID_ARGS;
		}

		*pFile = nullptr;
		if ((openMode & MA_OPEN_MODE_WRITE) != 0)
		{
			return MA_ACCESS_DENIED;
		}

		HMemoryVfs* vfs = (HMemoryVfs*)pVFS;
		std::shared_ptr<const std::vector<uint8>> bytes;
		{
			std::lock_guard<std::mutex> lock(vfs->Mutex);
			auto found = vfs->Files.find(pFilePath);
			if (found == vfs->Files.end())
			{
				return MA_DOES_NOT_EXIST;
			}
			bytes = found->second;
		}

		HMemoryFile* file = new HMemoryFile();
		file->Bytes = bytes;
		*pFile = (ma_vfs_file)file;
		return MA_SUCCESS;
	}

	ma_result vfsOpenW(ma_vfs* pVFS, const wchar_t* pFilePath, ma_uint32 openMode, ma_vfs_file* pFile)
	{
		// 경로는 늘 char 로 넘긴다(엔진 경로는 ASCII — 사용자 결정 D-26)
		return MA_NOT_IMPLEMENTED;
	}

	ma_result vfsClose(ma_vfs* pVFS, ma_vfs_file file)
	{
		delete (HMemoryFile*)file;
		return MA_SUCCESS;
	}

	ma_result vfsRead(ma_vfs* pVFS, ma_vfs_file file, void* pDst, size_t sizeInBytes, size_t* pBytesRead)
	{
		HMemoryFile* memoryFile = (HMemoryFile*)file;
		if (memoryFile == nullptr)
		{
			return MA_INVALID_ARGS;
		}

		const size_t size      = memoryFile->Bytes->size();
		const size_t available = (memoryFile->Cursor < size) ? (size - memoryFile->Cursor) : 0;
		const size_t count     = (std::min)(sizeInBytes, available);
		if (count > 0)
		{
			std::memcpy(pDst, memoryFile->Bytes->data() + memoryFile->Cursor, count);
			memoryFile->Cursor += count;
		}

		if (pBytesRead != nullptr)
		{
			*pBytesRead = count;
		}

		if (count == 0 && sizeInBytes > 0)
		{
			return MA_AT_END;
		}
		return MA_SUCCESS;
	}

	ma_result vfsWrite(ma_vfs* pVFS, ma_vfs_file file, const void* pSrc, size_t sizeInBytes, size_t* pBytesWritten)
	{
		return MA_ACCESS_DENIED;
	}

	ma_result vfsSeek(ma_vfs* pVFS, ma_vfs_file file, ma_int64 offset, ma_seek_origin origin)
	{
		HMemoryFile* memoryFile = (HMemoryFile*)file;
		if (memoryFile == nullptr)
		{
			return MA_INVALID_ARGS;
		}

		const ma_int64 size = (ma_int64)memoryFile->Bytes->size();
		ma_int64 position = offset;
		if (origin == ma_seek_origin_current)
		{
			position = (ma_int64)memoryFile->Cursor + offset;
		}
		else if (origin == ma_seek_origin_end)
		{
			position = size + offset;
		}

		if (position < 0)
		{
			return MA_INVALID_ARGS;
		}

		memoryFile->Cursor = (size_t)(std::min)(position, size);
		return MA_SUCCESS;
	}

	ma_result vfsTell(ma_vfs* pVFS, ma_vfs_file file, ma_int64* pCursor)
	{
		HMemoryFile* memoryFile = (HMemoryFile*)file;
		if (memoryFile == nullptr || pCursor == nullptr)
		{
			return MA_INVALID_ARGS;
		}

		*pCursor = (ma_int64)memoryFile->Cursor;
		return MA_SUCCESS;
	}

	ma_result vfsInfo(ma_vfs* pVFS, ma_vfs_file file, ma_file_info* pInfo)
	{
		HMemoryFile* memoryFile = (HMemoryFile*)file;
		if (memoryFile == nullptr || pInfo == nullptr)
		{
			return MA_INVALID_ARGS;
		}

		pInfo->sizeInBytes = (ma_uint64)memoryFile->Bytes->size();
		return MA_SUCCESS;
	}

	ma_uint64 toMilliseconds(float32 seconds)
	{
		if (seconds <= 0.0f)
		{
			return 0;
		}
		return (ma_uint64)(seconds * 1000.0f + 0.5f);
	}

	// "/JGGame/a/b.wav" → "<게임 Content>/a/b.wav". 토큰 경로가 아니거나 그 Content 가 없으면(엔진 단독 실행의 /JGGame/) false.
	bool resolveSoundPath(const PString& soundPath, std::string* outKey)
	{
		const std::string& path = soundPath.GetRawString();
		const PString* contentDirectory = nullptr;
		size_t tokenLength = 0;
		if (path.rfind(ENGINE_TOKEN, 0) == 0)
		{
			contentDirectory = &HFileHelper::EngineContentDirectory();
			tokenLength = std::strlen(ENGINE_TOKEN);
		}
		else if (path.rfind(GAME_TOKEN, 0) == 0)
		{
			contentDirectory = &HFileHelper::GameContentDirectory();
			tokenLength = std::strlen(GAME_TOKEN);
		}

		if (contentDirectory == nullptr || contentDirectory->Length() == 0 || path.size() <= tokenLength)
		{
			return false;
		}

		std::string relativePath = path.substr(tokenLength);
		std::replace(relativePath.begin(), relativePath.end(), '\\', '/');

		PString combined;
		HFileHelper::CombinePath(*contentDirectory, PString(relativePath.c_str()), &combined);
		*outKey = combined.GetRawString();
		return true;
	}
}

struct HSoundInstance
{
	uint64    ID        = 0;
	ma_sound  Sound;
	EAudioBus Bus       = EAudioBus::Effects;
	bool      bStopping = false;   // 페이드로 멈추는 중 — 멈추면 Update 가 놓는다
	bool      bPaused   = false;
	bool      bLooping  = false;
};

struct HAudioEngine::HImpl
{
	using HInstanceMap = std::unordered_map<uint64, std::unique_ptr<HSoundInstance>>;

	HMemoryVfs     Vfs;
	ma_context     Context;
	ma_engine      Engine;
	ma_sound_group Groups[(int32)EAudioBus::Count];   // Master(엔진 끝점) ← Music · Effects
	float32        BusVolumes[(int32)EAudioBus::Count] = { 1.0f, 1.0f, 1.0f };
	int32          GroupCount          = 0;
	bool           bContextInitialized = false;
	bool           bEngineInitialized  = false;
	bool           bInitialized        = false;
	bool           bDeviceActive       = false;
	EAudioOutput   Output              = EAudioOutput::Device;
	std::string    DeviceDescription;

	uint64       NextID = 1;
	HInstanceMap Instances;
	// 통째로 풀어 둔 효과음(경로마다 하나). 재생은 이것의 복사본이라 풀린 데이터를 같이 쓴다.
	std::unordered_map<std::string, std::unique_ptr<ma_sound>> Prototypes;
	// 오류를 경로마다 한 번만 남긴다(같은 소리를 매 프레임 불러도 로그가 넘치지 않게)
	std::unordered_set<std::string> ReportedPaths;

	HAudioHandle Music;      // PlayMusic 의 지금 곡
	std::string  MusicKey;

	HImpl()
	{
		std::memset(&Vfs.Callbacks, 0, sizeof(Vfs.Callbacks));
		Vfs.Callbacks.onOpen  = &vfsOpen;
		Vfs.Callbacks.onOpenW = &vfsOpenW;
		Vfs.Callbacks.onClose = &vfsClose;
		Vfs.Callbacks.onRead  = &vfsRead;
		Vfs.Callbacks.onWrite = &vfsWrite;
		Vfs.Callbacks.onSeek  = &vfsSeek;
		Vfs.Callbacks.onTell  = &vfsTell;
		Vfs.Callbacks.onInfo  = &vfsInfo;
	}

	static bool IsFinished(const HSoundInstance& sound)
	{
		if (sound.bPaused == true)
		{
			return false;
		}

		if (sound.bStopping == true)
		{
			return ma_sound_is_playing(&sound.Sound) == MA_FALSE;
		}
		return ma_sound_at_end(&sound.Sound) == MA_TRUE;
	}

	void ReportOnce(const PString& soundPath, const char* reason)
	{
		if (ReportedPaths.insert(soundPath.GetRawString()).second == false)
		{
			return;
		}
		JG_LOG(Audio, ELogLevel::Error, "Audio: cannot play '%s' - %s", soundPath, reason);
	}

	// 소리 파일을 메모리에 올린다(처음 한 번). 패키지 읽기 검사를 거치도록 HFileHelper 로 읽는다.
	bool PrepareFile(const PString& soundPath, std::string* outKey)
	{
		if (resolveSoundPath(soundPath, outKey) == false)
		{
			ReportOnce(soundPath, "not a content path (use /JGGame/... or /JGEngine/... with the file extension)");
			return false;
		}

		{
			std::lock_guard<std::mutex> lock(Vfs.Mutex);
			if (Vfs.Files.find(*outKey) != Vfs.Files.end())
			{
				return true;
			}
		}

		std::vector<uint8> bytes;
		if (HFileHelper::ReadAllBytes(PString(outKey->c_str()), bytes) == false)
		{
			ReportOnce(soundPath, "file not found or empty");
			return false;
		}

		std::shared_ptr<const std::vector<uint8>> shared = std::make_shared<std::vector<uint8>>(std::move(bytes));
		std::lock_guard<std::mutex> lock(Vfs.Mutex);
		Vfs.Files[*outKey] = shared;
		return true;
	}

	ma_sound* FindOrLoadPrototype(const std::string& key)
	{
		auto found = Prototypes.find(key);
		if (found != Prototypes.end())
		{
			return found->second.get();
		}

		std::unique_ptr<ma_sound> prototype = std::make_unique<ma_sound>();
		const ma_uint32 flags = MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_NO_DEFAULT_ATTACHMENT;
		if (ma_sound_init_from_file(&Engine, key.c_str(), flags, nullptr, nullptr, prototype.get()) != MA_SUCCESS)
		{
			return nullptr;
		}

		ma_sound* rawPrototype = prototype.get();
		Prototypes.emplace(key, std::move(prototype));
		return rawPrototype;
	}

	HInstanceMap::iterator ReleaseInstance(HInstanceMap::iterator iterator)
	{
		ma_sound_uninit(&iterator->second->Sound);
		if (Music.ID == iterator->first)
		{
			Music = HAudioHandle();
			MusicKey.clear();
		}
		return Instances.erase(iterator);
	}

	void ReleaseAll()
	{
		if (bEngineInitialized == true)
		{
			// 오디오 스레드를 먼저 멈춘다(Manual 출력이면 할 일이 없다)
			ma_engine_stop(&Engine);
		}

		for (auto& pair : Instances)
		{
			ma_sound_uninit(&pair.second->Sound);
		}
		Instances.clear();

		for (auto& pair : Prototypes)
		{
			ma_sound_uninit(pair.second.get());
		}
		Prototypes.clear();

		// 자식 묶음(Music · Effects)부터
		for (int32 i = GroupCount - 1; i >= 0; --i)
		{
			ma_sound_group_uninit(&Groups[i]);
		}
		GroupCount = 0;

		if (bEngineInitialized == true)
		{
			ma_engine_uninit(&Engine);
			bEngineInitialized = false;
		}

		if (bContextInitialized == true)
		{
			ma_context_uninit(&Context);
			bContextInitialized = false;
		}

		{
			std::lock_guard<std::mutex> lock(Vfs.Mutex);
			Vfs.Files.clear();
		}
		ReportedPaths.clear();
		Music = HAudioHandle();
		MusicKey.clear();
		DeviceDescription.clear();
		bDeviceActive = false;
		bInitialized  = false;
	}
};

HAudioEngine::HAudioEngine()
	: _impl(new HImpl())
{
}

HAudioEngine::~HAudioEngine()
{
	Shutdown();
	delete _impl;
	_impl = nullptr;
}

bool HAudioEngine::Initialize(const HAudioEngineArguments& args)
{
	HImpl& impl = *_impl;
	if (impl.bInitialized == true)
	{
		return true;
	}

	impl.Output = args.Output;

	ma_engine_config config = ma_engine_config_init();
	config.pResourceManagerVFS = &impl.Vfs;

	if (args.Output == EAudioOutput::Manual)
	{
		config.noDevice   = MA_TRUE;
		config.channels   = (args.Channels > 0) ? args.Channels : MANUAL_DEFAULT_CHANNELS;
		config.sampleRate = (args.SampleRate > 0) ? args.SampleRate : MANUAL_DEFAULT_SAMPLE_RATE;
	}
	else if (args.Output == EAudioOutput::NullDevice)
	{
		ma_backend backends[] = { ma_backend_null };
		ma_context_config contextConfig = ma_context_config_init();
		if (ma_context_init(backends, 1, &contextConfig, &impl.Context) != MA_SUCCESS)
		{
			return false;
		}
		impl.bContextInitialized = true;
		config.pContext = &impl.Context;
	}

	if (ma_engine_init(&config, &impl.Engine) != MA_SUCCESS)
	{
		impl.ReleaseAll();
		return false;
	}
	impl.bEngineInitialized = true;

	// 묶음: Master 는 엔진 끝점에, Music · Effects 는 Master 에 붙는다
	for (int32 i = 0; i < (int32)EAudioBus::Count; ++i)
	{
		ma_sound_group* parent = (i == (int32)EAudioBus::Master) ? nullptr : &impl.Groups[(int32)EAudioBus::Master];
		if (ma_sound_group_init(&impl.Engine, MA_SOUND_FLAG_NO_SPATIALIZATION, parent, &impl.Groups[i]) != MA_SUCCESS)
		{
			impl.ReleaseAll();
			return false;
		}
		impl.GroupCount = i + 1;
		ma_sound_group_set_volume(&impl.Groups[i], impl.BusVolumes[i]);
	}

	if (args.Output != EAudioOutput::Manual)
	{
		ma_device* device = ma_engine_get_device(&impl.Engine);
		if (device != nullptr)
		{
			char name[256] = {};
			ma_device_get_name(device, ma_device_type_playback, name, sizeof(name), nullptr);
			const ma_backend backend = device->pContext->backend;
			impl.DeviceDescription = std::string(name) + " (" + ma_get_backend_name(backend) + ")";
			impl.bDeviceActive = (backend != ma_backend_null);
		}
	}

	impl.bInitialized = true;
	return true;
}

void HAudioEngine::Shutdown()
{
	if (_impl == nullptr)
	{
		return;
	}
	_impl->ReleaseAll();
}

bool HAudioEngine::IsInitialized() const
{
	return _impl->bInitialized;
}

bool HAudioEngine::IsDeviceActive() const
{
	return _impl->bInitialized == true && _impl->bDeviceActive == true;
}

PString HAudioEngine::GetDeviceDescription() const
{
	return PString(_impl->DeviceDescription.c_str());
}

HAudioHandle HAudioEngine::Play(const PString& soundPath, const HAudioPlayArguments& args)
{
	HImpl& impl = *_impl;
	if (impl.bInitialized == false)
	{
		return HAudioHandle();
	}

	std::string key;
	if (impl.PrepareFile(soundPath, &key) == false)
	{
		return HAudioHandle();
	}

	const EAudioBus bus = (args.Bus == EAudioBus::Music) ? EAudioBus::Music : EAudioBus::Effects;
	ma_sound_group* group = &impl.Groups[(int32)bus];

	std::unique_ptr<HSoundInstance> instance = std::make_unique<HSoundInstance>();
	ma_uint32 flags = MA_SOUND_FLAG_NO_SPATIALIZATION;
	ma_result result = MA_ERROR;
	if (args.bStream == true)
	{
		flags |= MA_SOUND_FLAG_STREAM;
		if (args.bLoop == true)
		{
			flags |= MA_SOUND_FLAG_LOOPING;
		}
		result = ma_sound_init_from_file(&impl.Engine, key.c_str(), flags, group, nullptr, &instance->Sound);
	}
	else
	{
		ma_sound* prototype = impl.FindOrLoadPrototype(key);
		if (prototype != nullptr)
		{
			result = ma_sound_init_copy(&impl.Engine, prototype, flags, group, &instance->Sound);
			if (result == MA_SUCCESS && args.bLoop == true)
			{
				ma_sound_set_looping(&instance->Sound, MA_TRUE);
			}
		}
	}

	if (result != MA_SUCCESS)
	{
		impl.ReportOnce(soundPath, "cannot decode the file (wav, ogg, mp3, flac)");
		return HAudioHandle();
	}

	ma_sound_set_volume(&instance->Sound, (std::max)(args.Volume, 0.0f));
	if (args.Pitch > 0.0f && args.Pitch != 1.0f)
	{
		ma_sound_set_pitch(&instance->Sound, args.Pitch);
	}
	if (args.FadeInSeconds > 0.0f)
	{
		ma_sound_set_fade_in_milliseconds(&instance->Sound, 0.0f, 1.0f, toMilliseconds(args.FadeInSeconds));
	}

	if (ma_sound_start(&instance->Sound) != MA_SUCCESS)
	{
		ma_sound_uninit(&instance->Sound);
		impl.ReportOnce(soundPath, "cannot start the sound");
		return HAudioHandle();
	}

	instance->ID       = impl.NextID++;
	instance->Bus      = bus;
	instance->bLooping = args.bLoop;

	HAudioHandle handle;
	handle.ID = instance->ID;
	impl.Instances.emplace(instance->ID, std::move(instance));
	return handle;
}

void HAudioEngine::Stop(HAudioHandle handle, float32 fadeSeconds)
{
	HImpl& impl = *_impl;
	auto found = impl.Instances.find(handle.ID);
	if (impl.bInitialized == false || found == impl.Instances.end())
	{
		return;
	}

	HSoundInstance& sound = *found->second;
	if (fadeSeconds > 0.0f && sound.bPaused == false && HImpl::IsFinished(sound) == false)
	{
		ma_sound_stop_with_fade_in_milliseconds(&sound.Sound, toMilliseconds(fadeSeconds));
		sound.bStopping = true;
		return;
	}

	impl.ReleaseInstance(found);
}

void HAudioEngine::Pause(HAudioHandle handle)
{
	HImpl& impl = *_impl;
	auto found = impl.Instances.find(handle.ID);
	if (impl.bInitialized == false || found == impl.Instances.end())
	{
		return;
	}

	HSoundInstance& sound = *found->second;
	if (sound.bPaused == true || sound.bStopping == true)
	{
		return;
	}

	ma_sound_stop(&sound.Sound);
	sound.bPaused = true;
}

void HAudioEngine::Resume(HAudioHandle handle)
{
	HImpl& impl = *_impl;
	auto found = impl.Instances.find(handle.ID);
	if (impl.bInitialized == false || found == impl.Instances.end())
	{
		return;
	}

	HSoundInstance& sound = *found->second;
	if (sound.bPaused == false)
	{
		return;
	}

	ma_sound_start(&sound.Sound);
	sound.bPaused = false;
}

bool HAudioEngine::IsPlaying(HAudioHandle handle) const
{
	const HImpl& impl = *_impl;
	auto found = impl.Instances.find(handle.ID);
	if (impl.bInitialized == false || found == impl.Instances.end())
	{
		return false;
	}
	return HImpl::IsFinished(*found->second) == false;
}

void HAudioEngine::SetVolume(HAudioHandle handle, float32 volume)
{
	HImpl& impl = *_impl;
	auto found = impl.Instances.find(handle.ID);
	if (impl.bInitialized == false || found == impl.Instances.end())
	{
		return;
	}
	ma_sound_set_volume(&found->second->Sound, (std::max)(volume, 0.0f));
}

HAudioHandle HAudioEngine::PlayMusic(const PString& soundPath, float32 fadeSeconds, float32 volume)
{
	HImpl& impl = *_impl;
	if (impl.bInitialized == false)
	{
		return HAudioHandle();
	}

	// 같은 곡이 돌고 있으면 그대로 둔다
	std::string key;
	if (resolveSoundPath(soundPath, &key) == true && impl.Music.IsValid() == true && key == impl.MusicKey)
	{
		auto found = impl.Instances.find(impl.Music.ID);
		if (found != impl.Instances.end() && found->second->bStopping == false && HImpl::IsFinished(*found->second) == false)
		{
			return impl.Music;
		}
	}

	HAudioPlayArguments args;
	args.Bus           = EAudioBus::Music;
	args.Volume        = volume;
	args.bLoop         = true;
	args.bStream       = true;
	args.FadeInSeconds = fadeSeconds;

	const HAudioHandle previous = impl.Music;
	const HAudioHandle next     = Play(soundPath, args);
	if (next.IsValid() == false)
	{
		// 새 곡을 못 내면 앞 곡을 그대로 둔다
		return HAudioHandle();
	}

	if (previous.IsValid() == true)
	{
		Stop(previous, fadeSeconds);
	}

	impl.Music    = next;
	impl.MusicKey = key;
	return next;
}

void HAudioEngine::StopMusic(float32 fadeSeconds)
{
	HImpl& impl = *_impl;
	if (impl.Music.IsValid() == false)
	{
		return;
	}

	const HAudioHandle music = impl.Music;
	impl.Music = HAudioHandle();
	impl.MusicKey.clear();
	Stop(music, fadeSeconds);
}

HAudioHandle HAudioEngine::GetMusicHandle() const
{
	return _impl->Music;
}

void HAudioEngine::SetBusVolume(EAudioBus bus, float32 volume)
{
	HImpl& impl = *_impl;
	const int32 index = (int32)bus;
	if (index < 0 || index >= (int32)EAudioBus::Count)
	{
		return;
	}

	impl.BusVolumes[index] = (std::max)(volume, 0.0f);
	if (impl.bInitialized == true)
	{
		ma_sound_group_set_volume(&impl.Groups[index], impl.BusVolumes[index]);
	}
}

float32 HAudioEngine::GetBusVolume(EAudioBus bus) const
{
	const int32 index = (int32)bus;
	if (index < 0 || index >= (int32)EAudioBus::Count)
	{
		return 0.0f;
	}
	return _impl->BusVolumes[index];
}

void HAudioEngine::FadeOutputGain(float32 gain, float32 seconds)
{
	HImpl& impl = *_impl;
	if (impl.bInitialized == false)
	{
		return;
	}

	// 시작 크기 -1 = 지금 크기에서. 묶음 크기(Master 볼륨)와 따로 곱해진다.
	ma_sound_group_set_fade_in_milliseconds(&impl.Groups[(int32)EAudioBus::Master], -1.0f, (std::max)(gain, 0.0f), toMilliseconds(seconds));
}

bool HAudioEngine::Preload(const PString& soundPath)
{
	HImpl& impl = *_impl;
	if (impl.bInitialized == false)
	{
		return false;
	}

	std::string key;
	if (impl.PrepareFile(soundPath, &key) == false)
	{
		return false;
	}

	if (impl.FindOrLoadPrototype(key) == nullptr)
	{
		impl.ReportOnce(soundPath, "cannot decode the file (wav, ogg, mp3, flac)");
		return false;
	}
	return true;
}

void HAudioEngine::StopAll(float32 fadeSeconds)
{
	HImpl& impl = *_impl;
	if (impl.bInitialized == false)
	{
		return;
	}

	std::vector<uint64> ids;
	ids.reserve(impl.Instances.size());
	for (const auto& pair : impl.Instances)
	{
		ids.push_back(pair.first);
	}

	impl.Music = HAudioHandle();
	impl.MusicKey.clear();
	for (uint64 id : ids)
	{
		HAudioHandle handle;
		handle.ID = id;
		Stop(handle, fadeSeconds);
	}
}

int32 HAudioEngine::GetPlayingCount() const
{
	int32 count = 0;
	for (const auto& pair : _impl->Instances)
	{
		if (HImpl::IsFinished(*pair.second) == false)
		{
			++count;
		}
	}
	return count;
}

void HAudioEngine::Update()
{
	HImpl& impl = *_impl;
	if (impl.bInitialized == false)
	{
		return;
	}

	for (auto iterator = impl.Instances.begin(); iterator != impl.Instances.end();)
	{
		if (HImpl::IsFinished(*iterator->second) == true)
		{
			iterator = impl.ReleaseInstance(iterator);
			continue;
		}
		++iterator;
	}
}

bool HAudioEngine::ReadOutput(float32* outFrames, uint64 frameCount)
{
	HImpl& impl = *_impl;
	if (impl.bInitialized == false || impl.Output != EAudioOutput::Manual || outFrames == nullptr)
	{
		return false;
	}

	ma_uint64 framesRead = 0;
	const ma_result result = ma_engine_read_pcm_frames(&impl.Engine, outFrames, frameCount, &framesRead);
	return result == MA_SUCCESS && framesRead == frameCount;
}

uint32 HAudioEngine::GetSampleRate() const
{
	if (_impl->bEngineInitialized == false)
	{
		return 0;
	}
	return ma_engine_get_sample_rate(&_impl->Engine);
}

uint32 HAudioEngine::GetChannels() const
{
	if (_impl->bEngineInitialized == false)
	{
		return 0;
	}
	return ma_engine_get_channels(&_impl->Engine);
}

bool HAudioEngine::RegisterMemoryFile(const PString& soundPath, const std::vector<uint8>& bytes)
{
	std::string key;
	if (resolveSoundPath(soundPath, &key) == false || bytes.empty() == true)
	{
		return false;
	}

	std::shared_ptr<const std::vector<uint8>> shared = std::make_shared<std::vector<uint8>>(bytes);
	std::lock_guard<std::mutex> lock(_impl->Vfs.Mutex);
	_impl->Vfs.Files[key] = shared;
	return true;
}
