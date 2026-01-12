// Copyright (c) 2022-2025 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <fonts/fontawesome.h>
#include "helpers/utils.h"
#include "helpers/imgui.h"
#include "helpers/nfd.h"
#include "views/player_overlay.h"

namespace ImPlay::Views {
PlayerOverlay::PlayerOverlay(Config *config, Mpv *mpv) : View(config, mpv) {
  m_lastActivityTime = 0;
}

void PlayerOverlay::draw() {
  if (!m_visible) return;

  // Draw URL dialog if open
  if (m_showURLDialog) {
    openURL();
  }

  auto vp = ImGui::GetMainViewport();
  ImVec2 wPos = vp->WorkPos;
  ImVec2 wSize = vp->WorkSize;

  // Auto-hide controls after inactivity
  double currentTime = ImGui::GetTime();
  if (ImGui::GetIO().MouseDelta.x != 0 || ImGui::GetIO().MouseDelta.y != 0 ||
      ImGui::IsAnyMouseDown() || ImGui::IsKeyDown(ImGuiKey_Space)) {
    m_lastActivityTime = currentTime;
    m_showControls = true;
  }

  bool shouldShow = m_showControls || (currentTime - m_lastActivityTime < 3.0);
  float targetAlpha = shouldShow ? 1.0f : 0.0f;
  m_controlsAlpha = m_controlsAlpha + (targetAlpha - m_controlsAlpha) * 0.1f;

  if (m_controlsAlpha < 0.01f) return;

  ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_controlsAlpha);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

  // Draw top bar
  drawTopBar();

  // Draw bottom bar with controls
  drawBottomBar();

  // Draw popup menus
  if (m_showSubtitleMenu) drawSubtitleMenu();
  if (m_showAudioMenu) drawAudioMenu();
  if (m_showSettingsMenu) drawSettingsMenu();

  ImGui::PopStyleColor();
  ImGui::PopStyleVar(3);
}

void PlayerOverlay::drawIdleScreen() {
  auto vp = ImGui::GetMainViewport();
  ImVec2 wPos = vp->WorkPos;
  ImVec2 wSize = vp->WorkSize;

  // Draw URL dialog if open
  if (m_showURLDialog) {
    openURL();
  }

  // Full screen dark purple background
  ImGui::SetNextWindowPos(wPos);
  ImGui::SetNextWindowSize(wSize);

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

  ImGui::PushStyleColor(ImGuiCol_WindowBg, m_bgDark);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

  if (ImGui::Begin("##IdleScreen", nullptr, flags)) {
    ImVec2 center = ImVec2(wPos.x + wSize.x / 2, wPos.y + wSize.y / 2);

    // Title
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    const char* title = "PlayTorrio Player";
    ImVec2 titleSize = ImGui::CalcTextSize(title);
    ImGui::SetCursorPos(ImVec2((wSize.x - titleSize.x * 2) / 2, wSize.y / 2 - 100));
    ImGui::PushStyleColor(ImGuiCol_Text, m_accentPurple);
    ImGui::SetWindowFontScale(2.0f);
    ImGui::TextUnformatted(title);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();
    ImGui::PopFont();

    // Subtitle
    const char* subtitle = "Drop a file or click below to start";
    ImVec2 subSize = ImGui::CalcTextSize(subtitle);
    ImGui::SetCursorPos(ImVec2((wSize.x - subSize.x) / 2, wSize.y / 2 - 30));
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", subtitle);

    // Buttons
    float btnWidth = 200;
    float btnHeight = 50;
    float btnSpacing = 20;
    float totalWidth = btnWidth * 2 + btnSpacing;
    float startX = (wSize.x - totalWidth) / 2;

    ImGui::SetCursorPos(ImVec2(startX, wSize.y / 2 + 30));

    ImGui::PushStyleColor(ImGuiCol_Button, m_primaryPurple);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, m_accentPurple);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, m_darkPurple);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12);

    if (ImGui::Button(ICON_FA_FOLDER_OPEN " Open File", ImVec2(btnWidth, btnHeight))) {
      openMediaFile();
    }

    ImGui::SameLine(0, btnSpacing);

    if (ImGui::Button(ICON_FA_GLOBE " Open URL", ImVec2(btnWidth, btnHeight))) {
      m_showURLDialog = true;
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    // Version info at bottom
    const char* version = "Powered by mpv";
    ImVec2 verSize = ImGui::CalcTextSize(version);
    ImGui::SetCursorPos(ImVec2((wSize.x - verSize.x) / 2, wSize.y - 50));
    ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "%s", version);
  }
  ImGui::End();

  ImGui::PopStyleVar();
  ImGui::PopStyleColor();
}

void PlayerOverlay::drawTopBar() {
  auto vp = ImGui::GetMainViewport();
  ImVec2 wPos = vp->WorkPos;
  ImVec2 wSize = vp->WorkSize;

  ImGui::SetNextWindowPos(wPos);
  ImGui::SetNextWindowSize(ImVec2(wSize.x, 80));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.7f * m_controlsAlpha));

  if (ImGui::Begin("##TopBar", nullptr, flags)) {
    ImGui::SetCursorPos(ImVec2(25, 20));

    // Back button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(m_primaryPurple.x, m_primaryPurple.y, m_primaryPurple.z, 0.25f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));

    if (ImGui::Button(ICON_FA_ARROW_LEFT "##back", ImVec2(40, 40))) {
      mpv->command("quit");
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Exit Player");

    ImGui::SameLine();
    ImGui::SetCursorPosX(80);

    // Video title
    std::string title = mpv->property("media-title");
    if (title.empty()) title = "PlayTorrio Player";
    ImGui::PushStyleColor(ImGuiCol_Text, m_accentPurple);
    ImGui::SetCursorPosY(28);
    ImGui::TextUnformatted(title.c_str());
    ImGui::PopStyleColor();

    ImGui::PopStyleColor(3);
  }
  ImGui::End();
  ImGui::PopStyleColor();
}

void PlayerOverlay::drawBottomBar() {
  auto vp = ImGui::GetMainViewport();
  ImVec2 wPos = vp->WorkPos;
  ImVec2 wSize = vp->WorkSize;

  float barHeight = 140;
  ImGui::SetNextWindowPos(ImVec2(wPos.x, wPos.y + wSize.y - barHeight));
  ImGui::SetNextWindowSize(ImVec2(wSize.x, barHeight));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.85f * m_controlsAlpha));

  if (ImGui::Begin("##BottomBar", nullptr, flags)) {
    ImGui::SetCursorPos(ImVec2(40, 20));

    // Progress bar
    drawProgressBar();

    ImGui::SetCursorPos(ImVec2(40, 70));

    // Control buttons
    drawControlButtons();
  }
  ImGui::End();
  ImGui::PopStyleColor();
}

void PlayerOverlay::drawProgressBar() {
  auto vp = ImGui::GetMainViewport();
  float width = vp->WorkSize.x - 80;

  double duration = mpv->property<double, MPV_FORMAT_DOUBLE>("duration");
  double position = (double)mpv->timePos;
  float progress = duration > 0 ? (float)(position / duration) : 0.0f;

  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1, 1, 1, 0.15f));
  ImGui::PushStyleColor(ImGuiCol_SliderGrab, m_primaryPurple);
  ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, m_accentPurple);

  ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 16);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3);

  ImGui::SetNextItemWidth(width);

  float seekValue = m_seeking ? m_seekPos : progress;
  if (ImGui::SliderFloat("##progress", &seekValue, 0.0f, 1.0f, "")) {
    m_seeking = true;
    m_seekPos = seekValue;
  }

  if (ImGui::IsItemDeactivatedAfterEdit() && m_seeking) {
    double seekTime = m_seekPos * duration;
    mpv->commandv("seek", std::to_string(seekTime).c_str(), "absolute", nullptr);
    m_seeking = false;
  }

  // Show time tooltip on hover
  if (ImGui::IsItemHovered()) {
    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 itemPos = ImGui::GetItemRectMin();
    ImVec2 itemSize = ImGui::GetItemRectSize();
    float hoverProgress = (mousePos.x - itemPos.x) / itemSize.x;
    hoverProgress = std::clamp(hoverProgress, 0.0f, 1.0f);
    double hoverTime = hoverProgress * duration;

    int hours = (int)(hoverTime / 3600);
    int mins = (int)((hoverTime - hours * 3600) / 60);
    int secs = (int)(hoverTime - hours * 3600 - mins * 60);

    std::string timeStr;
    if (hours > 0)
      timeStr = fmt::format("{:02d}:{:02d}:{:02d}", hours, mins, secs);
    else
      timeStr = fmt::format("{:02d}:{:02d}", mins, secs);

    ImGui::SetTooltip("%s", timeStr.c_str());
  }

  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor(3);
}

void PlayerOverlay::drawControlButtons() {
  auto vp = ImGui::GetMainViewport();

  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(m_primaryPurple.x, m_primaryPurple.y, m_primaryPurple.z, 0.25f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, m_primaryPurple);
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));

  ImVec2 btnSize(48, 48);

  // Play/Pause button
  bool paused = mpv->pause;
  const char* playIcon = paused ? ICON_FA_PLAY : ICON_FA_PAUSE;
  if (ImGui::Button(playIcon, btnSize)) {
    mpv->command("cycle pause");
  }
  if (ImGui::IsItemHovered()) ImGui::SetTooltip(paused ? "Play" : "Pause");

  ImGui::SameLine();

  // Volume control
  drawVolumeControl();

  ImGui::SameLine();
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);

  // Time display
  double duration = mpv->property<double, MPV_FORMAT_DOUBLE>("duration");
  double position = (double)mpv->timePos;

  auto formatTime = [](double t) -> std::string {
    int hours = (int)(t / 3600);
    int mins = (int)((t - hours * 3600) / 60);
    int secs = (int)(t - hours * 3600 - mins * 60);
    if (hours > 0)
      return fmt::format("{:02d}:{:02d}:{:02d}", hours, mins, secs);
    return fmt::format("{:02d}:{:02d}", mins, secs);
  };

  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 12);
  ImGui::TextColored(ImVec4(0.87f, 0.87f, 0.87f, 1.0f), "%s / %s",
                     formatTime(position).c_str(), formatTime(duration).c_str());

  // Right side controls
  float rightX = vp->WorkSize.x - 300;
  ImGui::SameLine();
  ImGui::SetCursorPosX(rightX);
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 12);

  // Audio tracks button
  if (ImGui::Button(ICON_FA_MUSIC "##audio", btnSize)) {
    m_showAudioMenu = !m_showAudioMenu;
    m_showSubtitleMenu = false;
    m_showSettingsMenu = false;
  }
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Audio Tracks");

  ImGui::SameLine();

  // Settings button
  if (ImGui::Button(ICON_FA_COG "##settings", btnSize)) {
    m_showSettingsMenu = !m_showSettingsMenu;
    m_showSubtitleMenu = false;
    m_showAudioMenu = false;
  }
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Settings");

  ImGui::SameLine();

  // Subtitles button
  if (ImGui::Button(ICON_FA_CLOSED_CAPTIONING "##subs", btnSize)) {
    m_showSubtitleMenu = !m_showSubtitleMenu;
    m_showAudioMenu = false;
    m_showSettingsMenu = false;
  }
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Subtitles");

  ImGui::SameLine();

  // Fullscreen button
  bool fullscreen = mpv->fullscreen;
  const char* fsIcon = fullscreen ? ICON_FA_COMPRESS : ICON_FA_EXPAND;
  if (ImGui::Button(fsIcon, btnSize)) {
    mpv->command("cycle fullscreen");
  }
  if (ImGui::IsItemHovered()) ImGui::SetTooltip(fullscreen ? "Exit Fullscreen" : "Fullscreen");

  ImGui::PopStyleColor(4);
}

void PlayerOverlay::drawVolumeControl() {
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1, 1, 1, 0.08f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(m_primaryPurple.x, m_primaryPurple.y, m_primaryPurple.z, 0.2f));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 30);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 6));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);

  ImGui::BeginGroup();

  // Mute button
  bool muted = mpv->mute;
  int volume = (int)mpv->volume;
  const char* volIcon = muted ? ICON_FA_VOLUME_MUTE :
                        (volume > 50 ? ICON_FA_VOLUME_UP : ICON_FA_VOLUME_DOWN);

  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
  if (ImGui::Button(volIcon, ImVec2(30, 30))) {
    mpv->command("cycle mute");
  }
  ImGui::PopStyleColor();

  ImGui::SameLine();

  // Volume slider
  ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1, 1, 1, 1));
  ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, m_accentPurple);
  ImGui::SetNextItemWidth(100);
  if (ImGui::SliderInt("##volume", &volume, 0, 100, "")) {
    mpv->commandv("set", "volume", std::to_string(volume).c_str(), nullptr);
  }
  ImGui::PopStyleColor(2);

  ImGui::EndGroup();

  ImGui::PopStyleVar(3);
  ImGui::PopStyleColor(2);
}

void PlayerOverlay::drawSubtitleMenu() {
  auto vp = ImGui::GetMainViewport();
  ImVec2 wPos = vp->WorkPos;
  ImVec2 wSize = vp->WorkSize;

  float menuWidth = 360;
  float menuHeight = 500;
  ImGui::SetNextWindowPos(ImVec2(wPos.x + wSize.x - menuWidth - 40, wPos.y + wSize.y - menuHeight - 160));
  ImGui::SetNextWindowSize(ImVec2(menuWidth, menuHeight));

  ImGui::PushStyleColor(ImGuiCol_WindowBg, m_bgDark);
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(m_primaryPurple.x, m_primaryPurple.y, m_primaryPurple.z, 0.4f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 24);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

  if (ImGui::Begin("##SubtitleMenu", &m_showSubtitleMenu, flags)) {
    // Header
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    ImGui::SetCursorPos(ImVec2(30, 25));
    ImGui::TextColored(m_accentPurple, "SUBTITLES");

    ImGui::SameLine(menuWidth - 50);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    if (ImGui::Button(ICON_FA_TIMES "##closeSubs", ImVec2(30, 30))) {
      m_showSubtitleMenu = false;
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();

    ImGui::SetCursorPos(ImVec2(15, 70));

    // Subtitle list
    ImGui::BeginChild("##SubList", ImVec2(menuWidth - 30, 250), false);

    std::string currentSid = mpv->sid;
    bool hasSubtitles = false;

    for (auto &track : mpv->tracks) {
      if (track.type != "sub") continue;
      hasSubtitles = true;

      std::string title = track.title.empty() ? fmt::format("Track {}", track.id) : track.title;
      if (!track.lang.empty()) title += fmt::format(" [{}]", track.lang);

      bool selected = currentSid == std::to_string(track.id);

      ImGui::PushStyleColor(ImGuiCol_Header, m_primaryPurple);
      ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 14);

      if (ImGui::Selectable(title.c_str(), selected, 0, ImVec2(0, 40))) {
        mpv->property<int64_t, MPV_FORMAT_INT64>("sid", track.id);
      }

      ImGui::PopStyleVar();
      ImGui::PopStyleColor();
    }

    // Disable subtitles option
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 14);
    if (ImGui::Selectable("Disable Subtitles", currentSid == "no", 0, ImVec2(0, 40))) {
      mpv->commandv("set", "sid", "no", nullptr);
    }
    ImGui::PopStyleVar();

    if (!hasSubtitles) {
      ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No subtitles available");
    }

    ImGui::EndChild();

    // Footer with Upload Subtitles button
    ImGui::SetCursorPos(ImVec2(0, menuHeight - 120));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0.3f));
    ImGui::BeginChild("##SubFooter", ImVec2(menuWidth, 120), false);

    ImGui::SetCursorPos(ImVec2(30, 20));

    // Upload Subtitles button
    ImGui::PushStyleColor(ImGuiCol_Button, m_primaryPurple);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, m_accentPurple);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12);

    if (ImGui::Button(ICON_FA_UPLOAD " Upload Subtitles", ImVec2(menuWidth - 60, 40))) {
      openSubtitleFile();
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    // Subtitle delay control
    ImGui::SetCursorPos(ImVec2(30, 75));
    ImGui::Text("Delay (s)");
    ImGui::SameLine(150);
    float delay = (float)mpv->subDelay;
    ImGui::SetNextItemWidth(150);
    if (ImGui::InputFloat("##subDelay", &delay, 0.1f, 0.5f, "%.1f")) {
      mpv->commandv("set", "sub-delay", fmt::format("{:.1f}", delay).c_str(), nullptr);
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
  }
  ImGui::End();

  ImGui::PopStyleVar(3);
  ImGui::PopStyleColor(2);
}

void PlayerOverlay::drawAudioMenu() {
  auto vp = ImGui::GetMainViewport();
  ImVec2 wPos = vp->WorkPos;
  ImVec2 wSize = vp->WorkSize;

  float menuWidth = 360;
  float menuHeight = 400;
  ImGui::SetNextWindowPos(ImVec2(wPos.x + wSize.x - menuWidth - 40, wPos.y + wSize.y - menuHeight - 160));
  ImGui::SetNextWindowSize(ImVec2(menuWidth, menuHeight));

  ImGui::PushStyleColor(ImGuiCol_WindowBg, m_bgDark);
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(m_primaryPurple.x, m_primaryPurple.y, m_primaryPurple.z, 0.4f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 24);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

  if (ImGui::Begin("##AudioMenu", &m_showAudioMenu, flags)) {
    // Header
    ImGui::SetCursorPos(ImVec2(30, 25));
    ImGui::TextColored(m_accentPurple, "AUDIO TRACKS");

    ImGui::SameLine(menuWidth - 50);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    if (ImGui::Button(ICON_FA_TIMES "##closeAudio", ImVec2(30, 30))) {
      m_showAudioMenu = false;
    }
    ImGui::PopStyleColor();

    ImGui::SetCursorPos(ImVec2(15, 70));

    // Audio track list
    ImGui::BeginChild("##AudioList", ImVec2(menuWidth - 30, menuHeight - 100), false);

    std::string currentAid = mpv->aid;
    bool hasAudio = false;

    for (auto &track : mpv->tracks) {
      if (track.type != "audio") continue;
      hasAudio = true;

      std::string title = track.title.empty() ? fmt::format("Track {}", track.id) : track.title;
      if (!track.lang.empty()) title += fmt::format(" [{}]", track.lang);

      bool selected = currentAid == std::to_string(track.id);

      ImGui::PushStyleColor(ImGuiCol_Header, m_primaryPurple);
      ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 14);

      if (ImGui::Selectable(title.c_str(), selected, 0, ImVec2(0, 40))) {
        mpv->property<int64_t, MPV_FORMAT_INT64>("aid", track.id);
      }

      ImGui::PopStyleVar();
      ImGui::PopStyleColor();
    }

    if (!hasAudio) {
      ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No audio tracks available");
    }

    ImGui::EndChild();
  }
  ImGui::End();

  ImGui::PopStyleVar(3);
  ImGui::PopStyleColor(2);
}

void PlayerOverlay::drawSettingsMenu() {
  auto vp = ImGui::GetMainViewport();
  ImVec2 wPos = vp->WorkPos;
  ImVec2 wSize = vp->WorkSize;

  float menuWidth = 360;
  float menuHeight = 350;
  ImGui::SetNextWindowPos(ImVec2(wPos.x + wSize.x - menuWidth - 40, wPos.y + wSize.y - menuHeight - 160));
  ImGui::SetNextWindowSize(ImVec2(menuWidth, menuHeight));

  ImGui::PushStyleColor(ImGuiCol_WindowBg, m_bgDark);
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(m_primaryPurple.x, m_primaryPurple.y, m_primaryPurple.z, 0.4f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 24);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

  if (ImGui::Begin("##SettingsMenu", &m_showSettingsMenu, flags)) {
    // Header
    ImGui::SetCursorPos(ImVec2(30, 25));
    ImGui::TextColored(m_accentPurple, "SETTINGS");

    ImGui::SameLine(menuWidth - 50);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    if (ImGui::Button(ICON_FA_TIMES "##closeSettings", ImVec2(30, 30))) {
      m_showSettingsMenu = false;
    }
    ImGui::PopStyleColor();

    ImGui::SetCursorPos(ImVec2(15, 70));

    // Settings list
    ImGui::BeginChild("##SettingsList", ImVec2(menuWidth - 30, menuHeight - 100), false);

    // Playback speed
    ImGui::Text("Playback Speed");
    ImGui::SameLine(180);
    float speed = (float)mpv->property<double, MPV_FORMAT_DOUBLE>("speed");
    ImGui::SetNextItemWidth(140);
    if (ImGui::SliderFloat("##speed", &speed, 0.25f, 4.0f, "%.2fx")) {
      mpv->commandv("set", "speed", fmt::format("{:.2f}", speed).c_str(), nullptr);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Video aspect ratio
    ImGui::Text("Aspect Ratio");
    const char* ratios[] = {"Auto", "16:9", "4:3", "21:9"};
    static int currentRatio = 0;
    ImGui::SameLine(180);
    ImGui::SetNextItemWidth(140);
    if (ImGui::Combo("##aspect", &currentRatio, ratios, IM_ARRAYSIZE(ratios))) {
      if (currentRatio == 0)
        mpv->commandv("set", "video-aspect-override", "-1", nullptr);
      else
        mpv->commandv("set", "video-aspect-override", ratios[currentRatio], nullptr);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Hardware decoding toggle
    ImGui::Text("Hardware Decoding");
    ImGui::SameLine(180);
    std::string hwdec = mpv->property("hwdec");
    bool hwEnabled = hwdec != "no";
    if (ImGui::Checkbox("##hwdec", &hwEnabled)) {
      mpv->commandv("set", "hwdec", hwEnabled ? "auto" : "no", nullptr);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Loop toggle
    ImGui::Text("Loop Video");
    ImGui::SameLine(180);
    std::string loopFile = mpv->property("loop-file");
    bool loopEnabled = loopFile == "inf";
    if (ImGui::Checkbox("##loop", &loopEnabled)) {
      mpv->commandv("set", "loop-file", loopEnabled ? "inf" : "no", nullptr);
    }

    ImGui::EndChild();
  }
  ImGui::End();

  ImGui::PopStyleVar(3);
  ImGui::PopStyleColor(2);
}

void PlayerOverlay::openSubtitleFile() {
  m_showSubtitleMenu = false;

  std::vector<std::pair<std::string, std::string>> filters = {
      {"Subtitle Files", "srt,ass,idx,sub,sup,ttxt,txt,ssa,smi,mks,vtt"},
  };

  mpv->command("set pause yes");
  if (auto res = NFD::openFile(filters)) {
    mpv->commandv("sub-add", res->string().c_str(), "select", nullptr);
  }
  mpv->command("set pause no");
}

void PlayerOverlay::openMediaFile() {
  std::vector<std::pair<std::string, std::string>> filters = {
      {"Video Files", "mp4,mkv,avi,mov,wmv,flv,webm,m4v,mpg,mpeg,ts,m2ts,vob"},
      {"Audio Files", "mp3,flac,wav,aac,ogg,m4a,wma,opus"},
      {"All Files", "*"},
  };

  if (auto res = NFD::openFile(filters)) {
    mpv->commandv("loadfile", res->string().c_str(), nullptr);
  }
}

void PlayerOverlay::openURL() {
  ImGui::OpenPopup("Open URL##PlayTorrio");

  ImVec2 wSize = ImGui::GetMainViewport()->WorkSize;
  ImGui::SetNextWindowSize(ImVec2(std::min(wSize.x * 0.6f, 500.0f), 0), ImGuiCond_Always);
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

  ImGui::PushStyleColor(ImGuiCol_PopupBg, m_bgDark);
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(m_primaryPurple.x, m_primaryPurple.y, m_primaryPurple.z, 0.4f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(30, 20));

  if (ImGui::BeginPopupModal("Open URL##PlayTorrio", &m_showURLDialog, ImGuiWindowFlags_NoResize)) {
    ImGui::TextColored(m_accentPurple, "Stream from URL");
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Enter a video URL (HTTP, HTTPS, RTMP, etc.)");
    ImGui::Spacing();
    ImGui::Spacing();

    static char urlBuffer[1024] = {0};

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.15f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8);
    ImGui::SetNextItemWidth(-1);

    bool enterPressed = ImGui::InputTextWithHint("##url", "https://example.com/video.mp4", urlBuffer,
                                                  IM_ARRAYSIZE(urlBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Button, m_primaryPurple);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, m_accentPurple);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8);

    float btnWidth = 120;
    ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - btnWidth * 2 - 10);

    if (ImGui::Button("Cancel", ImVec2(btnWidth, 35))) {
      m_showURLDialog = false;
      urlBuffer[0] = '\0';
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    bool canPlay = urlBuffer[0] != '\0';
    if (!canPlay) ImGui::BeginDisabled();

    if (ImGui::Button(ICON_FA_PLAY " Play", ImVec2(btnWidth, 35)) || (enterPressed && canPlay)) {
      mpv->commandv("loadfile", urlBuffer, nullptr);
      m_showURLDialog = false;
      urlBuffer[0] = '\0';
      ImGui::CloseCurrentPopup();
    }

    if (!canPlay) ImGui::EndDisabled();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    ImGui::EndPopup();
  }

  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor(2);
}

}  // namespace ImPlay::Views
