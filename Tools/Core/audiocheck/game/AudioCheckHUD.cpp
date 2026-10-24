#include "PCH/PCH.h"
#include "AudioCheckHUD.h"
#include "UI/GameUI.h"
#include "Platform/JWindow.h"
#include <chrono>

namespace
{
	const char* const EFFECT_SOUND = "/JGGame/Sounds/Effect.wav";   // make_sounds.py — 0.2초 880Hz
	const char* const MUSIC_A      = "/JGGame/Sounds/MusicA.wav";   // make_sounds.py — 6초 아르페지오
	const char* const MUSIC_B      = "/JGGame/Sounds/MusicB.ogg";   // Android SDK fallbackring.ogg 사본(검증 게임에만)
	const char* const RAIN         = "/JGGame/Sounds/Rain.wav";     // make_sounds.py — 4초 잡음

	const float32 BURST_INTERVAL = 0.04f;

	// -audiocheck 자동 순서(초). 녹음 도구의 시각과 맞춰 본다(analyze_audiocheck.py 가 단계 이름으로 구간을 찾는다).
	// 크기 비율은 일정한 곡 A(2초 주기)와 같은 효과음으로 잰다. 곡 B(ogg)는 울림과 쉼이 번갈아 와서 "들린다"만 본다.
	struct HScriptStep
	{
		float32     Time;
		const char* Name;
	};

	const HScriptStep SCRIPT[] =
	{
		{ 1.0f,  "effect x8" },
		{ 2.5f,  "music B" },
		{ 5.0f,  "music A" },
		{ 8.0f,  "music A steady" },
		{ 10.0f, "music bus 0.25" },
		{ 12.0f, "music bus 1.0" },
		{ 12.5f, "rain on" },
		{ 14.0f, "master 0" },
		{ 15.0f, "master 1" },
		{ 16.0f, "stop music and rain" },
		{ 17.5f, "effects 0.5 and effect" },
		{ 18.5f, "effects 1.0 and effect" },
		{ 19.5f, "background mute" },           // 곡 A 다시 + 모드 Mute + 앱 비활성(WM_ACTIVATEAPP FALSE)
		{ 21.5f, "background active" },         // 앱 활성
		{ 23.5f, "background lower" },          // 모드 Lower(0.3) + 앱 비활성
		{ 25.5f, "background active again" },   // 앱 활성
		{ 27.5f, "done" },
	};

	// 창 비활성 처리 확인: OS 가 보내는 것과 같은 WM_ACTIVATEAPP 를 주 창에 보낸다(엔진 WndProc → 오디오 모듈)
	void sendAppActivation(bool bActive)
	{
		PJWindow* window = GCoreSystem::GetGlobalValues().MainWindow;
		if (window == nullptr)
		{
			return;
		}
		::SendMessageW((HWND)window->GetHandle(), WM_ACTIVATEAPP, bActive ? TRUE : FALSE, 0);
	}

	bool hasCommandLineSwitch(const wchar_t* name)
	{
		int32 argc = 0;
		LPWSTR* argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
		if (argv == nullptr)
		{
			return false;
		}

		bool bFound = false;
		for (int32 i = 1; i < argc; ++i)
		{
			if (_wcsicmp(argv[i], name) == 0)
			{
				bFound = true;
				break;
			}
		}

		::LocalFree(argv);
		return bFound;
	}

	int64 epochMilliseconds()
	{
		return (int64)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}

	const char* backgroundModeName(EAudioBackgroundMode mode)
	{
		if (mode == EAudioBackgroundMode::Lower)
		{
			return "Lower";
		}
		if (mode == EAudioBackgroundMode::Mute)
		{
			return "Mute";
		}
		return "Keep";
	}
}

void JGAudioCheckHUD::OnInitialize()
{
	_status = AddChild<PGameUIText>(PName("Status"));
	_status->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(40.0f, 20.0f), HVector2(1840.0f, 100.0f));
	_status->SetFontSize(30.0f);
	_status->SetAlignment(EGameUIHorizontalAlign::Left, EGameUIVerticalAlign::Middle);

	addButton("Effect", PString("Effect · 효과음"), 0)->OnClicked.AddLambda([this]()
		{
			playEffect();
		});
	addButton("Burst", PString("Effect x8 · 8번 겹치기"), 1)->OnClicked.AddLambda([this]()
		{
			startBurst();
		});
	addButton("MusicA", PString("Music A (wav) · 곡 A"), 2)->OnClicked.AddLambda([this]()
		{
			playMusic(false);
		});
	addButton("MusicB", PString("Music B (ogg) · 곡 B"), 3)->OnClicked.AddLambda([this]()
		{
			playMusic(true);
		});
	addButton("StopMusic", PString("Stop music · 음악 멈춤"), 4)->OnClicked.AddLambda([this]()
		{
			stopMusic();
		});
	_pauseButton = addButton("PauseMusic", PString(), 5);
	_pauseButton->OnClicked.AddLambda([this]()
		{
			toggleMusicPause();
		});
	addButton("Rain", PString("Rain on/off · 분위기 소리"), 6)->OnClicked.AddLambda([this]()
		{
			toggleRain();
		});
	_effectsButton = addButton("EffectsBus", PString(), 7);
	_effectsButton->OnClicked.AddLambda([this]()
		{
			cycleBus(EAudioBus::Effects);
		});
	_musicButton = addButton("MusicBus", PString(), 8);
	_musicButton->OnClicked.AddLambda([this]()
		{
			cycleBus(EAudioBus::Music);
		});
	_masterButton = addButton("MasterBus", PString(), 9);
	_masterButton->OnClicked.AddLambda([this]()
		{
			cycleBus(EAudioBus::Master);
		});
	_backgroundButton = addButton("Background", PString(), 10);
	_backgroundButton->OnClicked.AddLambda([this]()
		{
			cycleBackgroundMode();
		});

	HAudio::Preload(EFFECT_SOUND);
	_bScript = hasCommandLineSwitch(L"-audiocheck");
	updateLabels();
	JG_LOG(AudioCheck, ELogLevel::Info, "AudioCheck HUD initialized (device %d, script %d)", HAudio::IsDeviceActive() ? 1 : 0, _bScript ? 1 : 0);
}

void JGAudioCheckHUD::OnUpdate(float32 deltaSeconds)
{
	_clock += deltaSeconds;

	if (_burstRemaining > 0)
	{
		_burstTimer -= deltaSeconds;
		while (_burstRemaining > 0 && _burstTimer <= 0.0f)
		{
			playEffect();
			--_burstRemaining;
			_burstTimer += BURST_INTERVAL;
			if (_burstRemaining == 0)
			{
				logStep("burst done");
			}
		}
	}

	if (_bScript == true)
	{
		runScript();
	}

	_status->SetText(PString::Format("Device: %s · Playing: %d · Master %d%% · Music %d%% · Effects %d%% · Background: %s",
		HAudio::IsDeviceActive() ? "real output" : "silent (null device)",
		HAudio::GetPlayingCount(),
		(int32)(HAudio::GetBusVolume(EAudioBus::Master) * 100.0f + 0.5f),
		(int32)(HAudio::GetBusVolume(EAudioBus::Music) * 100.0f + 0.5f),
		(int32)(HAudio::GetBusVolume(EAudioBus::Effects) * 100.0f + 0.5f),
		backgroundModeName(_backgroundMode)));
}

PSharedPtr<PGameUIButton> JGAudioCheckHUD::addButton(const char* name, const PString& label, int32 row)
{
	PSharedPtr<PGameUIButton> button = AddChild<PGameUIButton>(PName(name));
	button->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(40.0f, 140.0f + (float32)row * 80.0f), HVector2(560.0f, 64.0f));
	button->SetLabel(label);
	return button;
}

void JGAudioCheckHUD::playEffect()
{
	HAudio::Play(EFFECT_SOUND);
}

void JGAudioCheckHUD::startBurst()
{
	_burstRemaining = 8;
	_burstTimer     = 0.0f;
}

void JGAudioCheckHUD::playMusic(bool bTrackB)
{
	// 처음 곡은 1초 동안 커지고, 다른 곡으로 바꾸면 2초 동안 서로 겹치며 바뀐다
	const float32 fadeSeconds = HAudio::IsPlaying(_music) ? 2.0f : 1.0f;
	_music        = HAudio::PlayMusic(bTrackB ? MUSIC_B : MUSIC_A, fadeSeconds);
	_bMusicPaused = false;
	updateLabels();
}

void JGAudioCheckHUD::stopMusic()
{
	HAudio::StopMusic(1.0f);
	_music        = HAudioHandle();
	_bMusicPaused = false;
	updateLabels();
}

void JGAudioCheckHUD::toggleRain()
{
	if (HAudio::IsPlaying(_rain) == true)
	{
		HAudio::Stop(_rain, 1.0f);
		_rain = HAudioHandle();
		return;
	}

	HAudioPlayArguments args;
	args.Bus           = EAudioBus::Music;
	args.Volume        = 0.8f;
	args.bLoop         = true;
	args.bStream       = true;
	args.FadeInSeconds = 1.0f;
	_rain = HAudio::Play(RAIN, args);
}

void JGAudioCheckHUD::toggleMusicPause()
{
	if (_bMusicPaused == true)
	{
		HAudio::Resume(_music);
		_bMusicPaused = false;
	}
	else if (HAudio::IsPlaying(_music) == true)
	{
		HAudio::Pause(_music);
		_bMusicPaused = true;
	}
	updateLabels();
}

void JGAudioCheckHUD::cycleBus(EAudioBus bus)
{
	const float32 volume = HAudio::GetBusVolume(bus);
	float32 next = 1.0f;
	if (volume > 0.75f)
	{
		next = 0.5f;
	}
	else if (volume > 0.25f)
	{
		next = 0.0f;
	}
	HAudio::SetBusVolume(bus, next);
	updateLabels();
}

void JGAudioCheckHUD::cycleBackgroundMode()
{
	if (_backgroundMode == EAudioBackgroundMode::Keep)
	{
		_backgroundMode = EAudioBackgroundMode::Lower;
	}
	else if (_backgroundMode == EAudioBackgroundMode::Lower)
	{
		_backgroundMode = EAudioBackgroundMode::Mute;
	}
	else
	{
		_backgroundMode = EAudioBackgroundMode::Keep;
	}
	HAudio::SetBackgroundMode(_backgroundMode, 0.3f);
	updateLabels();
}

void JGAudioCheckHUD::updateLabels()
{
	_effectsButton->SetLabel(PString::Format("Effects %d%% · 효과음 크기", (int32)(HAudio::GetBusVolume(EAudioBus::Effects) * 100.0f + 0.5f)));
	_musicButton->SetLabel(PString::Format("Music %d%% · 음악 크기", (int32)(HAudio::GetBusVolume(EAudioBus::Music) * 100.0f + 0.5f)));
	_masterButton->SetLabel(PString::Format("Master %d%% · 전체 크기", (int32)(HAudio::GetBusVolume(EAudioBus::Master) * 100.0f + 0.5f)));
	_backgroundButton->SetLabel(PString::Format("Background: %s · 창이 뒤로 가면", backgroundModeName(_backgroundMode)));
	_pauseButton->SetLabel(PString(_bMusicPaused ? "Resume music · 음악 이어" : "Pause music · 음악 일시 멈춤"));
}

void JGAudioCheckHUD::runScript()
{
	const int32 stepCount = (int32)(sizeof(SCRIPT) / sizeof(SCRIPT[0]));
	while (_scriptStep < stepCount && _clock >= SCRIPT[_scriptStep].Time)
	{
		const HScriptStep& step = SCRIPT[_scriptStep];
		++_scriptStep;

		switch (_scriptStep - 1)
		{
		case 0:
			startBurst();
			break;
		case 1:
			playMusic(true);
			break;
		case 2:
			playMusic(false);
			break;
		case 4:
			HAudio::SetBusVolume(EAudioBus::Music, 0.25f);
			break;
		case 5:
			HAudio::SetBusVolume(EAudioBus::Music, 1.0f);
			break;
		case 6:
			toggleRain();
			break;
		case 7:
			HAudio::SetBusVolume(EAudioBus::Master, 0.0f);
			break;
		case 8:
			HAudio::SetBusVolume(EAudioBus::Master, 1.0f);
			break;
		case 9:
			stopMusic();
			toggleRain();
			break;
		case 10:
			HAudio::SetBusVolume(EAudioBus::Effects, 0.5f);
			playEffect();
			break;
		case 11:
			HAudio::SetBusVolume(EAudioBus::Effects, 1.0f);
			playEffect();
			break;
		case 12:
			_music = HAudio::PlayMusic(MUSIC_A, 0.0f);
			_backgroundMode = EAudioBackgroundMode::Mute;
			HAudio::SetBackgroundMode(_backgroundMode, 0.3f);
			sendAppActivation(false);
			break;
		case 13:
			sendAppActivation(true);
			break;
		case 14:
			_backgroundMode = EAudioBackgroundMode::Lower;
			HAudio::SetBackgroundMode(_backgroundMode, 0.3f);
			sendAppActivation(false);
			break;
		case 15:
			sendAppActivation(true);
			break;
		case 16:
			HAudio::StopAll(0.0f);
			break;
		default:
			break;
		}

		updateLabels();
		logStep(step.Name);

		if (_scriptStep == stepCount)
		{
			JG_LOG(AudioCheck, ELogLevel::Warning, "AUDIOCHECK finished - exiting");
			GCoreSystem::RequestExit(0);
		}
	}
}

void JGAudioCheckHUD::logStep(const char* step)
{
	JG_LOG(AudioCheck, ELogLevel::Info, "AUDIOCHECK %lld t=%.3f %s playing=%d device=%d master=%.2f music=%.2f effects=%.2f musicPlaying=%d rainPlaying=%d",
		epochMilliseconds(), _clock, step, HAudio::GetPlayingCount(), HAudio::IsDeviceActive() ? 1 : 0,
		HAudio::GetBusVolume(EAudioBus::Master), HAudio::GetBusVolume(EAudioBus::Music), HAudio::GetBusVolume(EAudioBus::Effects),
		HAudio::IsPlaying(_music) ? 1 : 0, HAudio::IsPlaying(_rain) ? 1 : 0);
}
