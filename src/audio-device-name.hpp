#pragma once

#include <QString>

// OBS 오디오 소스의 장치가 "default"일 때 실제로 잡히는 장치 이름 (Windows 전용, 실패·기타 OS는 빈 문자열)
QString default_audio_device_name(bool input);
