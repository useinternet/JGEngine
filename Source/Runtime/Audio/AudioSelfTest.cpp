#include "PCH/PCH.h"
#include "Audio.h"
#include "AudioEngine.h"
#include "AudioGlobalSystem.h"
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"
#include "FileIO/FileHelper.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>

// audio.selftest [-ogg=<파일 경로>] — 오디오 모듈 자체 검사(오디오 재생, D-37).
//   장치 없이(Manual 출력) 섞은 결과를 직접 읽어 확인한다: 재생 · 겹치기 · 크기 · 높낮이 · 묶음 크기 · 되풀이 · 페이드 · 일시 멈춤 ·
//   배경음 교체 · 흘려 풀기 · 경로 오류 · 종료, 그리고 무음 장치 시작. 시험음은 코드로 만든 wav 를 메모리에 올려 쓴다(디스크 · 장치 불필요).
//   -ogg= 를 주면 그 파일을 메모리에 올려 ogg 풀기도 본다(저장소에는 ogg 시험 파일을 두지 않는다).
namespace
{
	int32 GCheckCount   = 0;
	int32 GFailureCount = 0;

	void check(bool bCondition, const char* caseName)
	{
		++GCheckCount;
		if (bCondition == false)
		{
			++GFailureCount;
			JG_LOG(Audio, ELogLevel::Error, "audio.selftest FAILED: %s", caseName);
		}
	}

	const uint32  SAMPLE_RATE = 48000;
	const uint32  CHANNELS    = 2;
	const float64 TWO_PI      = 6.283185307179586;

	const char* const SHORT_TONE  = "/JGEngine/__AudioSelfTest/short.wav";       // 440Hz 0.1초(44주기 — 되풀이 이음매가 맞는다)
	const char* const LONG_TONE   = "/JGEngine/__AudioSelfTest/long.wav";        // 440Hz 0.5초
	const char* const MUSIC_A     = "/JGEngine/__AudioSelfTest/music_a.wav";     // 220Hz 3초
	const char* const MUSIC_B     = "/JGEngine/__AudioSelfTest/music_b.wav";     // 660Hz 3초
	const char* const AMBIENCE    = "/JGEngine/__AudioSelfTest/ambience.wav";    // 1000Hz 3초
	const char* const LOOP_STREAM = "/JGEngine/__AudioSelfTest/loop_stream.wav"; // 330Hz 0.3초(99주기)
	const char* const MISSING     = "/JGEngine/__AudioSelfTest/missing.wav";     // 등록하지 않는다(디스크에도 없다)
	const char* const OGG_SAMPLE  = "/JGEngine/__AudioSelfTest/sample.ogg";

	void writeUint32(std::vector<uint8>& bytes, size_t offset, uint32 value)
	{
		bytes[offset + 0] = (uint8)(value & 0xFF);
		bytes[offset + 1] = (uint8)((value >> 8) & 0xFF);
		bytes[offset + 2] = (uint8)((value >> 16) & 0xFF);
		bytes[offset + 3] = (uint8)((value >> 24) & 0xFF);
	}

	void writeUint16(std::vector<uint8>& bytes, size_t offset, uint16 value)
	{
		bytes[offset + 0] = (uint8)(value & 0xFF);
		bytes[offset + 1] = (uint8)((value >> 8) & 0xFF);
	}

	// 16비트 PCM 모노 wav(사인파)를 메모리에 만든다
	std::vector<uint8> makeToneWav(float64 frequency, float64 seconds, float64 amplitude)
	{
		const uint32 frameCount = (uint32)(seconds * SAMPLE_RATE + 0.5);
		const uint32 dataSize   = frameCount * 2;
		std::vector<uint8> bytes(44 + dataSize, 0);

		std::memcpy(&bytes[0], "RIFF", 4);
		writeUint32(bytes, 4, 36 + dataSize);
		std::memcpy(&bytes[8], "WAVE", 4);
		std::memcpy(&bytes[12], "fmt ", 4);
		writeUint32(bytes, 16, 16);              // fmt 크기
		writeUint16(bytes, 20, 1);               // PCM
		writeUint16(bytes, 22, 1);               // 모노
		writeUint32(bytes, 24, SAMPLE_RATE);
		writeUint32(bytes, 28, SAMPLE_RATE * 2); // 초당 바이트
		writeUint16(bytes, 32, 2);               // 블록 정렬
		writeUint16(bytes, 34, 16);              // 표본 비트
		std::memcpy(&bytes[36], "data", 4);
		writeUint32(bytes, 40, dataSize);

		for (uint32 i = 0; i < frameCount; ++i)
		{
			const float64 sample = amplitude * std::sin(TWO_PI * frequency * (float64)i / (float64)SAMPLE_RATE);
			const int16 value = (int16)std::lround(sample * 32767.0);
			writeUint16(bytes, 44 + (size_t)i * 2, (uint16)value);
		}
		return bytes;
	}

	uint64 toFrames(float64 seconds)
	{
		return (uint64)(seconds * SAMPLE_RATE + 0.5);
	}

	// 섞은 결과를 seconds 만큼 읽는다(10ms 씩). bPace 면 사이에 1ms 쉰다 — 흘려 풀기 작업 스레드가 다음 쪽을 풀 시간
	std::vector<float32> render(HAudioEngine& engine, float64 seconds, bool bPace = false)
	{
		const uint64 totalFrames = toFrames(seconds);
		const uint64 chunkFrames = SAMPLE_RATE / 100;
		std::vector<float32> frames((size_t)(totalFrames * CHANNELS), 0.0f);
		for (uint64 done = 0; done < totalFrames;)
		{
			const uint64 count = (std::min)(chunkFrames, totalFrames - done);
			engine.ReadOutput(frames.data() + done * CHANNELS, count);
			done += count;
			if (bPace == true)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
		return frames;
	}

	// 0번 채널의 [start, start + length) 구간(초)
	void range(const std::vector<float32>& frames, float64 startSeconds, float64 lengthSeconds, uint64* outBegin, uint64* outEnd)
	{
		const uint64 totalFrames = frames.size() / CHANNELS;
		*outBegin = (std::min)(toFrames(startSeconds), totalFrames);
		*outEnd   = (std::min)(*outBegin + toFrames(lengthSeconds), totalFrames);
	}

	float32 peak(const std::vector<float32>& frames, float64 startSeconds, float64 lengthSeconds)
	{
		uint64 begin = 0;
		uint64 end   = 0;
		range(frames, startSeconds, lengthSeconds, &begin, &end);
		float32 result = 0.0f;
		for (uint64 i = begin; i < end; ++i)
		{
			result = (std::max)(result, std::fabs(frames[(size_t)(i * CHANNELS)]));
		}
		return result;
	}

	float32 rms(const std::vector<float32>& frames, float64 startSeconds, float64 lengthSeconds)
	{
		uint64 begin = 0;
		uint64 end   = 0;
		range(frames, startSeconds, lengthSeconds, &begin, &end);
		if (end <= begin)
		{
			return 0.0f;
		}

		float64 sum = 0.0;
		for (uint64 i = begin; i < end; ++i)
		{
			const float64 value = frames[(size_t)(i * CHANNELS)];
			sum += value * value;
		}
		return (float32)std::sqrt(sum / (float64)(end - begin));
	}

	// 한 주파수의 진폭(Goertzel). 두 곡이 겹쳐 있어도 곡마다 크기를 따로 본다
	float32 toneLevel(const std::vector<float32>& frames, float64 startSeconds, float64 lengthSeconds, float64 frequency)
	{
		uint64 begin = 0;
		uint64 end   = 0;
		range(frames, startSeconds, lengthSeconds, &begin, &end);
		const uint64 count = end - begin;
		if (count == 0)
		{
			return 0.0f;
		}

		const float64 coefficient = 2.0 * std::cos(TWO_PI * frequency / (float64)SAMPLE_RATE);
		float64 previous1 = 0.0;
		float64 previous2 = 0.0;
		for (uint64 i = begin; i < end; ++i)
		{
			const float64 current = frames[(size_t)(i * CHANNELS)] + coefficient * previous1 - previous2;
			previous2 = previous1;
			previous1 = current;
		}

		const float64 power = previous1 * previous1 + previous2 * previous2 - coefficient * previous1 * previous2;
		return (float32)(2.0 * std::sqrt((std::max)(power, 0.0)) / (float64)count);
	}

	// 10ms 창마다 RMS 의 가장 작은 값(되풀이 이음매에 빈 곳이 있나)
	float32 minimumWindowRms(const std::vector<float32>& frames, float64 startSeconds, float64 endSeconds)
	{
		float32 result = 1000.0f;
		for (float64 t = startSeconds; t + 0.01 <= endSeconds + 1e-9; t += 0.01)
		{
			result = (std::min)(result, rms(frames, t, 0.01));
		}
		return result;
	}

	HAudioPlayArguments makeArguments(EAudioBus bus, float32 volume, float32 pitch, bool bLoop, float32 fadeInSeconds, bool bStream)
	{
		HAudioPlayArguments args;
		args.Bus           = bus;
		args.Volume        = volume;
		args.Pitch         = pitch;
		args.bLoop         = bLoop;
		args.FadeInSeconds = fadeInSeconds;
		args.bStream       = bStream;
		return args;
	}

	bool startManual(HAudioEngine& engine)
	{
		HAudioEngineArguments args;
		args.Output     = EAudioOutput::Manual;
		args.SampleRate = SAMPLE_RATE;
		args.Channels   = CHANNELS;
		return engine.Initialize(args);
	}

	void registerTestSounds(HAudioEngine& engine)
	{
		bool bRegistered = true;
		bRegistered &= engine.RegisterMemoryFile(SHORT_TONE, makeToneWav(440.0, 0.1, 0.2));
		bRegistered &= engine.RegisterMemoryFile(LONG_TONE, makeToneWav(440.0, 0.5, 0.2));
		bRegistered &= engine.RegisterMemoryFile(MUSIC_A, makeToneWav(220.0, 3.0, 0.3));
		bRegistered &= engine.RegisterMemoryFile(MUSIC_B, makeToneWav(660.0, 3.0, 0.3));
		bRegistered &= engine.RegisterMemoryFile(AMBIENCE, makeToneWav(1000.0, 3.0, 0.3));
		bRegistered &= engine.RegisterMemoryFile(LOOP_STREAM, makeToneWav(330.0, 0.3, 0.3));
		check(bRegistered, "files: test sounds are served from memory under content paths");
	}

	// 한 번 재생 → 섞인 소리 크기(기준값)를 돌려준다
	float32 testBasic(HAudioEngine& engine)
	{
		const HAudioHandle handle = engine.Play(SHORT_TONE, HAudioPlayArguments());
		check(handle.IsValid() && engine.GetPlayingCount() == 1, "play: a short sound starts");

		const std::vector<float32> frames = render(engine, 0.05);
		const float32 referencePeak = peak(frames, 0.0, 0.05);
		check(referencePeak > 0.05f, "play: the sound is in the mix");

		render(engine, 0.08);
		engine.Update();
		check(engine.GetPlayingCount() == 0 && engine.IsPlaying(handle) == false, "play: a finished sound is released by Update");
		return referencePeak;
	}

	void testOverlap(HAudioEngine& engine, float32 referencePeak)
	{
		for (int32 i = 0; i < 4; ++i)
		{
			engine.Play(SHORT_TONE, HAudioPlayArguments());
		}
		check(engine.GetPlayingCount() == 4, "overlap: four copies of the same sound play at once");
		const std::vector<float32> frames = render(engine, 0.05);
		check(peak(frames, 0.0, 0.05) > referencePeak * 3.5f, "overlap: four copies started together add up (about 4x)");
		render(engine, 0.1);
		engine.Update();
		check(engine.GetPlayingCount() == 0, "overlap: all copies are released when they end");

		// 10ms 간격으로 빠르게 여덟 번(요청서 완료 기준 (a))
		int32 maximumCount = 0;
		for (int32 i = 0; i < 8; ++i)
		{
			engine.Play(SHORT_TONE, HAudioPlayArguments());
			render(engine, 0.01);
			maximumCount = (std::max)(maximumCount, engine.GetPlayingCount());
		}
		check(maximumCount == 8, "overlap: eight rapid plays (10 ms apart) overlap");
		render(engine, 0.15);
		engine.Update();
		check(engine.GetPlayingCount() == 0, "overlap: rapid plays are released when they end");
	}

	void testVolumeAndPitch(HAudioEngine& engine, float32 referencePeak)
	{
		engine.Play(SHORT_TONE, makeArguments(EAudioBus::Effects, 0.5f, 1.0f, false, 0.0f, false));
		const std::vector<float32> frames = render(engine, 0.05);
		const float32 ratio = peak(frames, 0.0, 0.05) / referencePeak;
		check(std::fabs(ratio - 0.5f) < 0.02f, "volume: a sound at volume 0.5 is half as loud");
		render(engine, 0.1);
		engine.Update();

		const HAudioHandle fast   = engine.Play(SHORT_TONE, makeArguments(EAudioBus::Effects, 1.0f, 2.0f, false, 0.0f, false));
		const HAudioHandle normal = engine.Play(SHORT_TONE, HAudioPlayArguments());
		render(engine, 0.07);
		engine.Update();
		check(engine.IsPlaying(fast) == false && engine.IsPlaying(normal) == true, "pitch: pitch 2 plays twice as fast (0.1 s sound ends before 0.07 s)");
		render(engine, 0.05);
		engine.Update();
	}

	void testBuses(HAudioEngine& engine, float32 referencePeak)
	{
		engine.SetBusVolume(EAudioBus::Effects, 0.5f);
		engine.Play(SHORT_TONE, HAudioPlayArguments());
		std::vector<float32> frames = render(engine, 0.05);
		check(std::fabs(peak(frames, 0.0, 0.05) / referencePeak - 0.5f) < 0.02f, "bus: effects volume 0.5 halves effect sounds");
		render(engine, 0.1);
		engine.Update();

		engine.SetBusVolume(EAudioBus::Effects, 0.0f);
		engine.Play(SHORT_TONE, HAudioPlayArguments());
		frames = render(engine, 0.05);
		check(peak(frames, 0.0, 0.05) < 1e-4f, "bus: effects volume 0 silences effect sounds");
		const HAudioHandle musicBusSound = engine.Play(SHORT_TONE, makeArguments(EAudioBus::Music, 1.0f, 1.0f, false, 0.0f, false));
		frames = render(engine, 0.05);
		check(std::fabs(peak(frames, 0.0, 0.05) / referencePeak - 1.0f) < 0.02f, "bus: a music-bus sound is not affected by the effects volume");
		render(engine, 0.1);
		engine.Update();
		check(engine.IsPlaying(musicBusSound) == false, "bus: the music-bus sound ends normally");

		engine.SetBusVolume(EAudioBus::Effects, 1.0f);
		engine.SetBusVolume(EAudioBus::Master, 0.0f);
		engine.Play(SHORT_TONE, HAudioPlayArguments());
		frames = render(engine, 0.05);
		check(peak(frames, 0.0, 0.05) < 1e-4f, "bus: master volume 0 silences everything");
		render(engine, 0.1);
		engine.Update();
		engine.SetBusVolume(EAudioBus::Master, 1.0f);
		check(engine.GetBusVolume(EAudioBus::Master) == 1.0f && engine.GetBusVolume(EAudioBus::Effects) == 1.0f, "bus: GetBusVolume returns what was set");

		// 소리가 나는 도중에 바꾸면 다음 읽기에 바로 들어간다(요청서 완료 기준 (c))
		const HAudioHandle longSound = engine.Play(LONG_TONE, HAudioPlayArguments());
		frames = render(engine, 0.05);
		const float32 before = peak(frames, 0.04, 0.01);
		engine.SetBusVolume(EAudioBus::Effects, 0.25f);
		frames = render(engine, 0.02);
		const float32 after = peak(frames, 0.0, 0.02);
		check(before > 0.0f && std::fabs(after / before - 0.25f) < 0.03f, "bus: a volume change during a sound applies to the next block");
		engine.SetBusVolume(EAudioBus::Effects, 1.0f);
		engine.Stop(longSound, 0.0f);
		engine.Update();
	}

	void testLoopFadePause(HAudioEngine& engine, float32 referencePeak)
	{
		const float32 referenceRms = referencePeak / 1.41421356f;

		const HAudioHandle loop = engine.Play(SHORT_TONE, makeArguments(EAudioBus::Effects, 1.0f, 1.0f, true, 0.0f, false));
		std::vector<float32> frames = render(engine, 0.35);
		check(engine.IsPlaying(loop) && minimumWindowRms(frames, 0.0, 0.35) > referenceRms * 0.8f, "loop: a looping sound keeps playing with no gap (0.35 s of a 0.1 s sound)");
		engine.Stop(loop, 0.0f);
		check(engine.IsPlaying(loop) == false && engine.GetPlayingCount() == 0, "stop: Stop without a fade releases at once");

		const HAudioHandle fadeIn = engine.Play(LONG_TONE, makeArguments(EAudioBus::Effects, 1.0f, 1.0f, false, 0.2f, false));
		frames = render(engine, 0.25);
		const float32 start  = rms(frames, 0.0, 0.01);
		const float32 middle = rms(frames, 0.095, 0.01);
		const float32 end    = rms(frames, 0.21, 0.01);
		check(start < referenceRms * 0.2f && start < middle && middle < end && end > referenceRms * 0.9f, "fade: fade-in rises from silence to full over 0.2 s");
		engine.Stop(fadeIn, 0.0f);

		const HAudioHandle fadeOut = engine.Play(SHORT_TONE, makeArguments(EAudioBus::Effects, 1.0f, 1.0f, true, 0.0f, false));
		render(engine, 0.05);
		engine.Stop(fadeOut, 0.2f);
		check(engine.IsPlaying(fadeOut) == true, "fade: a sound stopping with a fade is still playing at first");
		frames = render(engine, 0.3);
		const float32 first  = rms(frames, 0.0, 0.02);
		const float32 second = rms(frames, 0.1, 0.02);
		const float32 last   = rms(frames, 0.25, 0.05);
		check(first > second && second > last && last < 1e-4f, "fade: Stop with a 0.2 s fade lowers the sound and then stops it");
		engine.Update();
		check(engine.IsPlaying(fadeOut) == false && engine.GetPlayingCount() == 0, "fade: the faded-out sound is released");

		const HAudioHandle paused = engine.Play(LONG_TONE, HAudioPlayArguments());
		render(engine, 0.05);
		engine.Pause(paused);
		frames = render(engine, 0.05);
		// 멈춘 직후 10ms 안에는 짧은 꼬리가 남는다(2026-10-11 측정 최대 0.034, 소리 크기 0.2 — 10ms 뒤로는 0)
		check(peak(frames, 0.01, 0.04) < 1e-4f && engine.IsPlaying(paused) == true, "pause: a paused sound is silent (after a short tail under 10 ms) and still alive");
		engine.Resume(paused);
		frames = render(engine, 0.05);
		check(peak(frames, 0.0, 0.05) > referencePeak * 0.8f, "pause: Resume continues the sound");
		engine.Stop(paused, 0.0f);
		engine.Update();
	}

	void testMusic(HAudioEngine& engine)
	{
		const HAudioHandle musicA = engine.PlayMusic(MUSIC_A, 0.2f, 1.0f);
		check(musicA.IsValid(), "music: PlayMusic starts the first track (streamed, looping)");
		std::vector<float32> frames = render(engine, 0.3, true);
		const float32 full = toneLevel(frames, 0.25, 0.05, 220.0);
		check(full > 0.1f && toneLevel(frames, 0.25, 0.05, 660.0) < full * 0.05f, "music: the first track is heard after its fade-in");
		check(engine.PlayMusic(MUSIC_A, 0.2f, 1.0f) == musicA, "music: PlayMusic with the same track keeps it playing");

		const HAudioHandle musicB = engine.PlayMusic(MUSIC_B, 0.3f, 1.0f);
		frames = render(engine, 0.15, true);
		const float32 oldMiddle = toneLevel(frames, 0.1, 0.05, 220.0);
		const float32 newMiddle = toneLevel(frames, 0.1, 0.05, 660.0);
		check(musicB.IsValid() && oldMiddle > full * 0.2f && newMiddle > full * 0.2f, "music: during the cross-fade both tracks are heard");
		frames = render(engine, 0.3, true);
		check(toneLevel(frames, 0.25, 0.05, 220.0) < full * 0.05f && toneLevel(frames, 0.25, 0.05, 660.0) > full * 0.9f, "music: the old track fades out and the new one fades in (0.3 s)");
		engine.Update();
		check(engine.GetMusicHandle() == musicB && engine.IsPlaying(musicA) == false && engine.GetPlayingCount() == 1, "music: the old track is released after its fade");

		const HAudioHandle ambience = engine.Play(AMBIENCE, makeArguments(EAudioBus::Music, 1.0f, 1.0f, true, 0.0f, true));
		frames = render(engine, 0.1, true);
		check(toneLevel(frames, 0.05, 0.05, 1000.0) > full * 0.9f && toneLevel(frames, 0.05, 0.05, 660.0) > full * 0.9f, "music: two looping background sounds play at once");

		engine.StopMusic(0.2f);
		frames = render(engine, 0.3, true);
		check(toneLevel(frames, 0.25, 0.05, 660.0) < full * 0.05f && toneLevel(frames, 0.25, 0.05, 1000.0) > full * 0.9f, "music: StopMusic fades out only the music track");
		engine.Update();
		check(engine.GetPlayingCount() == 1 && engine.GetMusicHandle().IsValid() == false, "music: after StopMusic only the other background sound remains");
		engine.Stop(ambience, 0.0f);

		const HAudioHandle loopStream = engine.Play(LOOP_STREAM, makeArguments(EAudioBus::Effects, 1.0f, 1.0f, true, 0.0f, true));
		frames = render(engine, 0.8, true);
		const float32 streamRms = full / 1.41421356f;   // 같은 진폭(0.3) 사인파의 RMS
		check(engine.IsPlaying(loopStream) && minimumWindowRms(frames, 0.01, 0.8) > streamRms * 0.8f, "stream: a looping streamed sound has no gap at the loop point (0.8 s of a 0.3 s file)");
		engine.Stop(loopStream, 0.0f);
		engine.Update();
	}

	void testErrors(HAudioEngine& engine)
	{
		JG_LOG(Audio, ELogLevel::Info, "audio.selftest: the next two error lines are expected (bad path and missing file checks)");
		check(engine.Play("C:/NotAContentPath/sound.wav", HAudioPlayArguments()).IsValid() == false, "path: a path without /JGGame/ or /JGEngine/ is refused");
		check(engine.Play(MISSING, HAudioPlayArguments()).IsValid() == false, "path: a missing file is refused");
		check(engine.Preload(SHORT_TONE) == true && engine.Preload(MISSING) == false, "preload: Preload decodes an existing sound and refuses a missing one");
	}

	void testOgg(HAudioEngine& engine, const PString& oggPath)
	{
		if (oggPath.Length() == 0)
		{
			JG_LOG(Audio, ELogLevel::Info, "audio.selftest: ogg check skipped (give -ogg=<file path> to decode an ogg file)");
			return;
		}

		std::vector<uint8> bytes;
		check(HFileHelper::ReadAllBytes(oggPath, bytes) && engine.RegisterMemoryFile(OGG_SAMPLE, bytes), "ogg: the given ogg file is read");

		const HAudioHandle decoded = engine.Play(OGG_SAMPLE, HAudioPlayArguments());
		std::vector<float32> frames = render(engine, 0.5);
		check(decoded.IsValid() && rms(frames, 0.0, 0.5) > 0.001f, "ogg: an ogg file decodes and plays");
		engine.Stop(decoded, 0.0f);

		const HAudioHandle streamed = engine.Play(OGG_SAMPLE, makeArguments(EAudioBus::Music, 1.0f, 1.0f, true, 0.0f, true));
		frames = render(engine, 0.5, true);
		check(streamed.IsValid() && rms(frames, 0.0, 0.5) > 0.001f, "ogg: an ogg file streams and plays");
		engine.Stop(streamed, 0.0f);
		engine.Update();
	}

	void testShutdown(HAudioEngine& engine)
	{
		engine.Play(SHORT_TONE, makeArguments(EAudioBus::Effects, 1.0f, 1.0f, true, 0.0f, false));
		engine.Play(LONG_TONE, makeArguments(EAudioBus::Effects, 1.0f, 1.0f, true, 0.0f, false));
		engine.PlayMusic(MUSIC_A, 0.0f, 1.0f);
		check(engine.GetPlayingCount() == 3, "stop all: three sounds are playing");
		engine.StopAll(0.0f);
		check(engine.GetPlayingCount() == 0 && engine.GetMusicHandle().IsValid() == false, "stop all: StopAll releases every sound");

		engine.Play(SHORT_TONE, makeArguments(EAudioBus::Effects, 1.0f, 1.0f, true, 0.0f, false));
		engine.PlayMusic(MUSIC_B, 0.5f, 1.0f);
		render(engine, 0.05, true);
		engine.Shutdown();
		check(engine.IsInitialized() == false && engine.GetPlayingCount() == 0, "shutdown: Shutdown with sounds playing releases everything");
		check(engine.Play(SHORT_TONE, HAudioPlayArguments()).IsValid() == false, "shutdown: calls after Shutdown do nothing");
		engine.Shutdown();
	}

	// 장치가 없는 PC 를 흉내 낸다: 무음 장치로 시작해 실제 시간대로 돈다(요청서 완료 기준 (e))
	void testNullDevice()
	{
		HAudioEngine engine;
		HAudioEngineArguments args;
		args.Output = EAudioOutput::NullDevice;
		check(engine.Initialize(args) == true && engine.IsDeviceActive() == false, "null device: the engine starts on the null device and reports no real output");

		engine.RegisterMemoryFile(SHORT_TONE, makeToneWav(440.0, 0.1, 0.2));
		const HAudioHandle handle = engine.Play(SHORT_TONE, HAudioPlayArguments());
		check(handle.IsValid(), "null device: Play works without a real device");

		bool bEnded = false;
		for (int32 i = 0; i < 100 && bEnded == false; ++i)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			engine.Update();
			bEnded = (engine.IsPlaying(handle) == false);
		}
		check(bEnded, "null device: the sound advances in real time and ends");
		engine.Shutdown();
	}

	bool executeSelfTest(const HConsoleCommandArgs& args)
	{
		GCheckCount   = 0;
		GFailureCount = 0;

		PString oggPath;
		args.TryGetString("ogg", oggPath);

		{
			HAudioEngine engine;
			check(startManual(engine), "engine: starts without a device (manual output)");
			check(engine.GetSampleRate() == SAMPLE_RATE && engine.GetChannels() == CHANNELS, "engine: manual output uses 48000 Hz stereo");
			registerTestSounds(engine);

			const float32 referencePeak = testBasic(engine);
			testOverlap(engine, referencePeak);
			testVolumeAndPitch(engine, referencePeak);
			testBuses(engine, referencePeak);
			testLoopFadePause(engine, referencePeak);
			testMusic(engine);
			testErrors(engine);
			testOgg(engine, oggPath);
			testShutdown(engine);
		}

		testNullDevice();

		// 이 실행의 전역 오디오(모듈): 창이 없으면 무음 장치로 돈다
		if (GCoreSystem::GetGlobalValues().MainWindow == nullptr)
		{
			const bool bActive = GAudioGlobalSystem::IsValid() && GAudioGlobalSystem::GetInstance().IsActive();
			check(bActive && HAudio::IsDeviceActive() == false, "module: without a window the module runs on the null device");
		}

		if (GFailureCount == 0)
		{
			JG_LOG(Audio, ELogLevel::Info, "audio.selftest: OK (%d/%d)", GCheckCount, GCheckCount);
		}
		else
		{
			JG_LOG(Audio, ELogLevel::Error, "audio.selftest: FAILED (%d/%d)", GCheckCount - GFailureCount, GCheckCount);
		}
		return GFailureCount == 0;
	}

	HAutoConsoleCommand AudioSelfTestCommand(
		"audio.selftest",
		"audio.selftest [-ogg=<file path>]",
		"Run the Audio module self tests (mixing without a device: overlap, volume, pitch, buses, loop, fades, pause, music cross-fade, streaming, null device)",
		&executeSelfTest);
}
