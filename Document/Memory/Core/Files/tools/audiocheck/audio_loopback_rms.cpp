// audio_loopback_rms — 기본 출력 장치로 실제로 나가는 소리의 크기(RMS)를 잰다(오디오 재생 D-37 검증 도구, 엔진 밖).
// WASAPI loopback 으로 시스템 출력 전체를 받아 window 밀리초마다 "RMS <epoch ms> <경과 초> <rms>" 한 줄을 쓴다.
// 소리 자체는 저장하지 않는다(수치만). 다른 앱 소리도 섞이므로 "났다 · 안 났다 · 크기 비율" 판정에만 쓴다.
// 빌드: build.bat (miniaudio.h 는 엔진 Source/ThirdParty/miniaudio). 실행: audio_loopback_rms.exe <초=20> <window ms=50>
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <thread>

namespace
{
	struct HState
	{
		std::mutex Mutex;
		double     Sum   = 0.0;
		unsigned long long Count = 0;
	};

	void onData(ma_device* device, void* output, const void* input, ma_uint32 frameCount)
	{
		HState* state = (HState*)device->pUserData;
		const float* samples = (const float*)input;
		const unsigned long long count = (unsigned long long)frameCount * device->capture.channels;
		double sum = 0.0;
		for (unsigned long long i = 0; i < count; ++i)
		{
			sum += (double)samples[i] * (double)samples[i];
		}

		std::lock_guard<std::mutex> lock(state->Mutex);
		state->Sum   += sum;
		state->Count += count;
	}

	long long epochMilliseconds()
	{
		return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}
}

int main(int argc, char** argv)
{
	const double seconds  = (argc > 1) ? std::atof(argv[1]) : 20.0;
	const int    windowMs = (argc > 2) ? std::atoi(argv[2]) : 50;

	ma_context context;
	if (ma_context_init(NULL, 0, NULL, &context) == MA_SUCCESS)
	{
		ma_device_info* playbackInfos = NULL;
		ma_uint32       playbackCount = 0;
		ma_device_info* captureInfos  = NULL;
		ma_uint32       captureCount  = 0;
		if (ma_context_get_devices(&context, &playbackInfos, &playbackCount, &captureInfos, &captureCount) == MA_SUCCESS)
		{
			for (ma_uint32 i = 0; i < playbackCount; ++i)
			{
				std::printf("PLAYBACK %u %s%s\n", i, playbackInfos[i].name, playbackInfos[i].isDefault ? " (default)" : "");
			}
		}
		ma_context_uninit(&context);
	}

	HState state;
	ma_backend backends[] = { ma_backend_wasapi };
	ma_device_config config = ma_device_config_init(ma_device_type_loopback);
	config.capture.format   = ma_format_f32;
	config.capture.channels = 2;
	config.sampleRate       = 48000;
	config.dataCallback     = onData;
	config.pUserData        = &state;

	ma_device device;
	if (ma_device_init_ex(backends, 1, NULL, &config, &device) != MA_SUCCESS)
	{
		std::printf("LOOPBACK init failed\n");
		return 2;
	}
	if (ma_device_start(&device) != MA_SUCCESS)
	{
		std::printf("LOOPBACK start failed\n");
		ma_device_uninit(&device);
		return 3;
	}

	std::printf("LOOPBACK start %lld seconds %.1f window %d\n", epochMilliseconds(), seconds, windowMs);
	std::fflush(stdout);

	const auto start = std::chrono::steady_clock::now();
	for (;;)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(windowMs));
		double sum = 0.0;
		unsigned long long count = 0;
		{
			std::lock_guard<std::mutex> lock(state.Mutex);
			sum = state.Sum;
			count = state.Count;
			state.Sum = 0.0;
			state.Count = 0;
		}

		const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
		const double rms = (count > 0) ? std::sqrt(sum / (double)count) : 0.0;
		std::printf("RMS %lld %.3f %.6f\n", epochMilliseconds(), elapsed, rms);
		std::fflush(stdout);
		if (elapsed >= seconds)
		{
			break;
		}
	}

	ma_device_uninit(&device);
	std::printf("LOOPBACK end %lld\n", epochMilliseconds());
	return 0;
}
