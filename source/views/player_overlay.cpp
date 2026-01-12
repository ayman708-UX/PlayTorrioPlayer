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

  bool shouldShow = m_showControls || (currentTime - m_lastActivityTime < 2.5);
  float targetAlpha = shouldShow ? 1.0f : 0.0f;
  m_controlsAlpha = m_controlsAlpha + (targetAlpha - m_controlsAlpha) * 0.15f;

  if (m_controlsAlpha < 0.01f) return;

  ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_controlsAlpha);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

  drawTopBar();
  drawBottomBar();

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

  if (m_showURLDialog) {
    openURL();
  }

  ImGui::SetNextWindowPos(wPos);
  ImGui::SetNextWindowSize(wSize);

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

  // Gradient-like dark background
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.01f, 0.06f, 1.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

  if (ImGui::Begin("##IdleScreen", nullptr, flags)) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    // Draw subtle gradient overlay
    ImU32 topColor = IM_COL32(20, 5, 40, 255);
    ImU32 bottomColor = IM_COL32(5, 0, 15, 255);
    drawList->AddRectFilledMultiColor(wPos, ImVec2(wPos.x + wSize.x, wPos.y + wSize.y),
                                       topColor, topColor, bottomColor, bottomColor);

    // Centered content
    float centerY = wSize.y * 0.4f;
    
    // App icon/logo area - draw a stylized play button
    float iconSize = 80.0f;
    ImVec2 iconCenter(wPos.x + wSize.x / 2, wPos.y + centerY - 60);
    drawList->AddCircleFilled(iconCenter, iconSize / 2, IM_COL32(157, 78, 221, 40), 64);
    drawList->AddCircle(iconCenter, iconSize / 2, IM_COL32(199, 125, 255, 100), 64, 2.0f);
    
    // Play triangle inside
    ImVec2 p1(iconCenter.x - 15, iconCenter.y - 20);
    ImVec2 p2(iconCenter.x - 15, iconCenter.y + 20);
    ImVec2 p3(iconCenter.x + 20, iconCenter.y);
    drawList->AddTriangleFilled(p1, p2, p3, IM_COL32(199, 125, 255, 200));

    // Title
    const char* title = "PlayTorrio";
    ImVec2 titleSize = ImGui::CalcTextSize(title);
    ImGui::SetCursorPos(ImVec2((wSize.x - titleSize.x * 2.5f) / 2, centerY + 20));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.78f, 0.49f, 1.0f, 1.0f));
    ImGui::SetWindowFontScale(2.5f);
    ImGui::TextUnformatted(title);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    // Subtitle
    const char* subtitle = "Modern Media Player";
    ImVec2 subSize = ImGui::CalcTextSize(subtitle);
    ImGui::SetCursorPos(ImVec2((wSize.x - subSize.x) / 2, centerY + 75));
    ImGui::TextColored(ImVec4(0.6f, 0.55f, 0.7f, 1.0f), "%s", subtitle);

    // Buttons
    float btnWidth = 180;
    float btnHeight = 48;
    float btnSpacing = 24;
    float totalWidth = btnWidth * 2 + btnSpacing;
    float startX = (wSize.x - totalWidth) / 2;

    ImGui::SetCursorPos(ImVec2(startX, centerY + 130));

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.616f, 0.306f, 0.867f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.78f, 0.49f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.353f, 0.094f, 0.604f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 24);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 12));

    if (ImGui::Button(ICON_FA_FOLDER_OPEN "  Open File", ImVec2(btnWidth, btnHeight))) {
      openMediaFile();
    }

    ImGui::SameLine(0, btnSpacing);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.08f, 0.15f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.616f, 0.306f, 0.867f, 0.6f));
    if (ImGui::Button(ICON_FA_LINK "  Open URL", ImVec2(btnWidth, btnHeight))) {
      m_showURLDialog = true;
    }
    ImGui::PopStyleColor(2);

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);

    // Hint text
    const char* hint = "or drag and drop a file here";
    ImVec2 hintSize = ImGui::CalcTextSize(hint);
    ImGui::SetCursorPos(ImVec2((wSize.x - hintSize.x) / 2, centerY + 200));
    ImGui::TextColored(ImVec4(0.4f, 0.35f, 0.5f, 0.8f), "%s", hint);

    // Footer
    const char* footer = "Powered by mpv";
    ImVec2 footerSize = ImGui::CalcTextSize(footer);
    ImGui::SetCursorPos(ImVec2((wSize.x - footerSize.x) / 2, wSize.y - 40));
    ImGui::TextColored(ImVec4(0.3f, 0.25f, 0.4f, 0.6f), "%s", footer);
  }
  ImGui::End();

  ImGui::PopStyleVar();
  ImGui::PopStyleColor();
}

void PlayerOverlay::drawTopBar() {
  auto vp = ImGui::GetMainViewport();
  ImVec2 wPos = vp->WorkPos;
  ImVec2 wSize = vp->WorkSize;

  float barHeight = 70;
  ImGui::SetNextWindowPos(wPos);
  ImGui::SetNextWindowSize(ImVec2(wSize.x, barHeight));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

  // Gradient top bar
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

  if (ImGui::Begin("##TopBar", nullptr, flags)) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImU32 topColor = IM_COL32(0, 0, 0, (int)(200 * m_controlsAlpha));
    ImU32 bottomColor = IM_COL32(0, 0, 0, 0);
    drawList->AddRectFilledMultiColor(wPos, ImVec2(wPos.x + wSize.x, wPos.y + barHeight),
                                       topColor, topColor, bottomColor, bottomColor);

    ImGui::SetCursorPos(ImVec2(20, 18));

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 0.9f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 20);

    if (ImGui::Button(ICON_FA_CHEVRON_LEFT, ImVec2(36, 36))) {
      mpv->command("quit");
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Close");

    ImGui::SameLine();
    ImGui::SetCursorPosX(70);

    // Video title with ellipsis
    std::string title = mpv->property("media-title");
    if (title.empty()) title = "PlayTorrio";
    
    ImGui::SetCursorPosY(22);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 0.95f));
    
    float maxWidth = wSize.x - 150;
    ImVec2 textSize = ImGui::CalcTextSize(title.c_str());
    if (textSize.x > maxWidth) {
      // Truncate with ellipsis
      std::string truncated = title.substr(0, (int)(title.length() * maxWidth / textSize.x) - 3) + "...";
      ImGui::TextUnformatted(truncated.c_str());
    } else {
      ImGui::TextUnformatted(title.c_str());
    }
    ImGui::PopStyleColor();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
  }
  ImGui::End();
  ImGui::PopStyleColor();
}

void PlayerOverlay::drawBottomBar() {
  auto vp = ImGui::GetMainViewport();
  ImVec2 wPos = vp->WorkPos;
  ImVec2 wSize = vp->WorkSize;

  float barHeight = 120;
  ImGui::SetNextWindowPos(ImVec2(wPos.x, wPos.y + wSize.y - barHeight));
  ImGui::SetNextWindowSize(ImVec2(wSize.x, barHeight));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

  if (ImGui::Begin("##BottomBar", nullptr, flags)) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 barPos = ImVec2(wPos.x, wPos.y + wSize.y - barHeight);
    ImU32 topColor = IM_COL32(0, 0, 0, 0);
    ImU32 bottomColor = IM_COL32(0, 0, 0, (int)(220 * m_controlsAlpha));
    drawList->AddRectFilledMultiColor(barPos, ImVec2(barPos.x + wSize.x, barPos.y + barHeight),
                                       topColor, topColor, bottomColor, bottomColor);

    ImGui::SetCursorPos(ImVec2(30, 15));
    drawProgressBar();

    ImGui::SetCursorPos(ImVec2(30, 60));
    drawControlButtons();
  }
  ImGui::End();
  ImGui::PopStyleColor();
}

void PlayerOverlay::drawProgressBar() {
  auto vp = ImGui::GetMainViewport();
  float width = vp->WorkSize.x - 60;

  double duration = mpv->property<double, MPV_FORMAT_DOUBLE>("duration");
  double position = (double)mpv->timePos;
  float progress = duration > 0 ? (float)(position / duration) : 0.0f;

  // Custom progress bar styling
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1, 1, 1, 0.12f));
  ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.78f, 0.49f, 1.0f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.9f, 0.7f, 1.0f, 1.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 14);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 4));

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

  // Time tooltip on hover
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

    std::string timeStr = hours > 0 ? fmt::format("{:02d}:{:02d}:{:02d}", hours, mins, secs)
                                    : fmt::format("{:02d}:{:02d}", mins, secs);
    ImGui::SetTooltip("%s", timeStr.c_str());
  }

  ImGui::PopStyleVar(3);
  ImGui::PopStyleColor(3);
}

void PlayerOverlay::drawControlButtons() {
  auto vp = ImGui::GetMainViewport();

  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.78f, 0.49f, 1.0f, 0.3f));
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 0.95f));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 24);

  ImVec2 btnSize(44, 44);
  ImVec2 smallBtn(38, 38);

  // Play/Pause - larger center button
  bool paused = mpv->pause;
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.78f, 0.49f, 1.0f, 0.15f));
  if (ImGui::Button(paused ? ICON_FA_PLAY : ICON_FA_PAUSE, ImVec2(52, 52))) {
    mpv->command("cycle pause");
  }
  ImGui::PopStyleColor();
  if (ImGui::IsItemHovered()) ImGui::SetTooltip(paused ? "Play" : "Pause");

  ImGui::SameLine(0, 16);

  // Skip backward
  if (ImGui::Button(ICON_FA_BACKWARD, smallBtn)) {
    mpv->command("seek -10");
  }
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Back 10s");

  ImGui::SameLine(0, 8);

  // Skip forward
  if (ImGui::Button(ICON_FA_FORWARD, smallBtn)) {
    mpv->command("seek 10");
  }
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Forward 10s");

  ImGui::SameLine(0, 20);

  // Volume control
  drawVolumeControl();

  ImGui::SameLine(0, 24);

  // Time display
  double duration = mpv->property<double, MPV_FORMAT_DOUBLE>("duration");
  double position = (double)mpv->timePos;

  auto formatTime = [](double t) -> std::string {
    int hours = (int)(t / 3600);
    int mins = (int)((t - hours * 3600) / 60);
    int secs = (int)(t - hours * 3600 - mins * 60);
    return hours > 0 ? fmt::format("{:02d}:{:02d}:{:02d}", hours, mins, secs)
                     : fmt::format("{:02d}:{:02d}", mins, secs);
  };

  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
  ImGui::TextColored(ImVec4(1, 1, 1, 0.7f), "%s", formatTime(position).c_str());
  ImGui::SameLine(0, 4);
  ImGui::TextColored(ImVec4(1, 1, 1, 0.4f), "/");
  ImGui::SameLine(0, 4);
  ImGui::TextColored(ImVec4(1, 1, 1, 0.5f), "%s", formatTime(duration).c_str());

  // Right side controls
  float rightX = vp->WorkSize.x - 220;
  ImGui::SameLine();
  ImGui::SetCursorPosX(rightX);
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 10);

  // Subtitles
  if (ImGui::Button(ICON_FA_CLOSED_CAPTIONING, btnSize)) {
    m_showSubtitleMenu = !m_showSubtitleMenu;
    m_showAudioMenu = false;
    m_showSettingsMenu = false;
  }
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Subtitles");

  ImGui::SameLine(0, 8);

  // Audio
  if (ImGui::Button(ICON_FA_HEADPHONES, btnSize)) {
    m_showAudioMenu = !m_showAudioMenu;
    m_showSubtitleMenu = false;
    m_showSettingsMenu = false;
  }
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Audio");

  ImGui::SameLine(0, 8);

  // Settings
  if (ImGui::Button(ICON_FA_SLIDERS_H, btnSize)) {
    m_showSettingsMenu = !m_showSettingsMenu;
    m_showSubtitleMenu = false;
    m_showAudioMenu = false;
  }
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Settings");

  ImGui::SameLine(0, 8);

  // Fullscreen
  bool fullscreen = mpv->fullscreen;
  if (ImGui::Button(fullscreen ? ICON_FA_COMPRESS : ICON_FA_EXPAND, btnSize)) {
    mpv->command("cycle fullscreen");
  }
  if (ImGui::IsItemHovered()) ImGui::SetTooltip(fullscreen ? "Exit Fullscreen" : "Fullscreen");

  ImGui::PopStyleVar();
  ImGui::PopStyleColor(4);
}

void PlayerOverlay::drawVolumeControl() {
  bool muted = mpv->mute;
  int volume = (int)mpv->volume;
  
  const char* volIcon = muted ? ICON_FA_VOLUME_MUTE :
                        (volume > 60 ? ICON_FA_VOLUME_UP : 
                         volume > 20 ? ICON_FA_VOLUME_DOWN : ICON_FA_VOLUME_OFF);

  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
  if (ImGui::Button(volIcon, ImVec2(38, 38))) {
    mpv->command("cycle mute");
  }
  ImGui::PopStyleColor();
  if (ImGui::IsItemHovered()) ImGui::SetTooltip(muted ? "Unmute" : "Mute");

  ImGui::SameLine(0, 4);

  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1, 1, 1, 0.1f));
  ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1, 1, 1, 0.9f));
  ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.78f, 0.49f, 1.0f, 1.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4);
  ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 10);

  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);
  ImGui::SetNextItemWidth(80);
  if (ImGui::SliderInt("##vol", &volume, 0, 100, "")) {
    mpv->commandv("set", "volume", std::to_string(volume).c_str(), nullptr);
  }

  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor(3);
}

void PlayerOverlay::drawSubtitleMenu() {
  auto vp = ImGui::GetMainViewport();
  ImVec2 wPos = vp->WorkPos;
  ImVec2 wSize = vp->WorkSize;

  float menuWidth = 320;
  float menuHeight = 420;
  ImGui::SetNextWindowPos(ImVec2(wPos.x + wSize.x - menuWidth - 30, wPos.y + wSize.y - menuHeight - 140));
  ImGui::SetNextWindowSize(ImVec2(menuWidth, menuHeight));

  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.03f, 0.1f, 0.95f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.78f, 0.49f, 1.0f, 0.3f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

  if (ImGui::Begin("##SubtitleMenu", &m_showSubtitleMenu, flags)) {
    // Header
    ImGui::TextColored(ImVec4(0.78f, 0.49f, 1.0f, 1.0f), ICON_FA_CLOSED_CAPTIONING "  Subtitles");
    
    ImGui::SameLine(menuWidth - 50);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    if (ImGui::Button(ICON_FA_TIMES "##closeSubs", ImVec2(24, 24))) {
      m_showSubtitleMenu = false;
    }
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Subtitle list
    ImGui::BeginChild("##SubList", ImVec2(menuWidth - 40, 200), false);

    std::string currentSid = mpv->sid;
    bool hasSubtitles = false;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8);
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.78f, 0.49f, 1.0f, 0.3f));

    for (auto &track : mpv->tracks) {
      if (track.type != "sub") continue;
      hasSubtitles = true;

      std::string title = track.title.empty() ? fmt::format("Track {}", track.id) : track.title;
      if (!track.lang.empty()) title += fmt::format(" [{}]", track.lang);

      bool selected = currentSid == std::to_string(track.id);
      if (ImGui::Selectable(title.c_str(), selected, 0, ImVec2(0, 32))) {
        mpv->property<int64_t, MPV_FORMAT_INT64>("sid", track.id);
      }
    }

    if (ImGui::Selectable("Disable Subtitles", currentSid == "no", 0, ImVec2(0, 32))) {
      mpv->commandv("set", "sid", "no", nullptr);
    }

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    if (!hasSubtitles) {
      ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No subtitles available");
    }

    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Upload button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.616f, 0.306f, 0.867f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.78f, 0.49f, 1.0f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8);

    if (ImGui::Button(ICON_FA_UPLOAD "  Load Subtitle File", ImVec2(menuWidth - 40, 36))) {
      openSubtitleFile();
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    // Delay control
    ImGui::Spacing();
    ImGui::Text("Delay");
    ImGui::SameLine(100);
    float delay = (float)mpv->subDelay;
    ImGui::SetNextItemWidth(menuWidth - 140);
    if (ImGui::InputFloat("##subDelay", &delay, 0.1f, 0.5f, "%.1f s")) {
      mpv->commandv("set", "sub-delay", fmt::format("{:.1f}", delay).c_str(), nullptr);
    }
  }
  ImGui::End();

  ImGui::PopStyleVar(3);
  ImGui::PopStyleColor(2);
}

void PlayerOverlay::drawAudioMenu() {
  auto vp = ImGui::GetMainViewport();
  ImVec2 wPos = vp->WorkPos;
  ImVec2 wSize = vp->WorkSize;

  float menuWidth = 320;
  float menuHeight = 320;
  ImGui::SetNextWindowPos(ImVec2(wPos.x + wSize.x - menuWidth - 30, wPos.y + wSize.y - menuHeight - 140));
  ImGui::SetNextWindowSize(ImVec2(menuWidth, menuHeight));

  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.03f, 0.1f, 0.95f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.78f, 0.49f, 1.0f, 0.3f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

  if (ImGui::Begin("##AudioMenu", &m_showAudioMenu, flags)) {
    ImGui::TextColored(ImVec4(0.78f, 0.49f, 1.0f, 1.0f), ICON_FA_HEADPHONES "  Audio Tracks");

    ImGui::SameLine(menuWidth - 50);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    if (ImGui::Button(ICON_FA_TIMES "##closeAudio", ImVec2(24, 24))) {
      m_showAudioMenu = false;
    }
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::BeginChild("##AudioList", ImVec2(menuWidth - 40, menuHeight - 100), false);

    std::string currentAid = mpv->aid;
    bool hasAudio = false;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8);
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.78f, 0.49f, 1.0f, 0.3f));

    for (auto &track : mpv->tracks) {
      if (track.type != "audio") continue;
      hasAudio = true;

      std::string title = track.title.empty() ? fmt::format("Track {}", track.id) : track.title;
      if (!track.lang.empty()) title += fmt::format(" [{}]", track.lang);

      bool selected = currentAid == std::to_string(track.id);
      if (ImGui::Selectable(title.c_str(), selected, 0, ImVec2(0, 32))) {
        mpv->property<int64_t, MPV_FORMAT_INT64>("aid", track.id);
      }
    }

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

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

  float menuWidth = 320;
  float menuHeight = 340;
  ImGui::SetNextWindowPos(ImVec2(wPos.x + wSize.x - menuWidth - 30, wPos.y + wSize.y - menuHeight - 140));
  ImGui::SetNextWindowSize(ImVec2(menuWidth, menuHeight));

  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.03f, 0.1f, 0.95f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.78f, 0.49f, 1.0f, 0.3f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

  if (ImGui::Begin("##SettingsMenu", &m_showSettingsMenu, flags)) {
    ImGui::TextColored(ImVec4(0.78f, 0.49f, 1.0f, 1.0f), ICON_FA_SLIDERS_H "  Settings");

    ImGui::SameLine(menuWidth - 50);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    if (ImGui::Button(ICON_FA_TIMES "##closeSettings", ImVec2(24, 24))) {
      m_showSettingsMenu = false;
    }
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::BeginChild("##SettingsList", ImVec2(menuWidth - 40, menuHeight - 100), false);

    // Playback speed
    ImGui::Text("Speed");
    ImGui::SameLine(120);
    float speed = (float)mpv->property<double, MPV_FORMAT_DOUBLE>("speed");
    ImGui::SetNextItemWidth(menuWidth - 160);
    if (ImGui::SliderFloat("##speed", &speed, 0.25f, 4.0f, "%.2fx")) {
      mpv->commandv("set", "speed", fmt::format("{:.2f}", speed).c_str(), nullptr);
    }

    ImGui::Spacing();

    // Aspect ratio
    ImGui::Text("Aspect");
    const char* ratios[] = {"Auto", "16:9", "4:3", "21:9"};
    static int currentRatio = 0;
    ImGui::SameLine(120);
    ImGui::SetNextItemWidth(menuWidth - 160);
    if (ImGui::Combo("##aspect", &currentRatio, ratios, IM_ARRAYSIZE(ratios))) {
      if (currentRatio == 0)
        mpv->commandv("set", "video-aspect-override", "-1", nullptr);
      else
        mpv->commandv("set", "video-aspect-override", ratios[currentRatio], nullptr);
    }

    ImGui::Spacing();

    // Hardware decoding
    ImGui::Text("HW Decode");
    ImGui::SameLine(120);
    std::string hwdec = mpv->property("hwdec");
    bool hwEnabled = hwdec != "no";
    if (ImGui::Checkbox("##hwdec", &hwEnabled)) {
      mpv->commandv("set", "hwdec", hwEnabled ? "auto" : "no", nullptr);
    }

    ImGui::Spacing();

    // Loop
    ImGui::Text("Loop");
    ImGui::SameLine(120);
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
  ImGui::SetNextWindowSize(ImVec2(std::min(wSize.x * 0.5f, 450.0f), 0), ImGuiCond_Always);
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

  ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.05f, 0.03f, 0.1f, 0.98f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.78f, 0.49f, 1.0f, 0.4f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(28, 24));
  ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1);

  if (ImGui::BeginPopupModal("Open URL##PlayTorrio", &m_showURLDialog, ImGuiWindowFlags_NoResize)) {
    ImGui::TextColored(ImVec4(0.78f, 0.49f, 1.0f, 1.0f), ICON_FA_LINK "  Stream from URL");
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.7f, 1.0f), "Enter a video URL to stream");
    ImGui::Spacing();
    ImGui::Spacing();

    static char urlBuffer[1024] = {0};

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.08f, 0.15f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 10));
    ImGui::SetNextItemWidth(-1);

    bool enterPressed = ImGui::InputTextWithHint("##url", "https://...", urlBuffer,
                                                  IM_ARRAYSIZE(urlBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Spacing();

    float btnWidth = 100;
    ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - btnWidth * 2 - 12);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.15f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.25f, 0.4f, 1.0f));
    if (ImGui::Button("Cancel", ImVec2(btnWidth, 36))) {
      m_showURLDialog = false;
      urlBuffer[0] = '\0';
      ImGui::CloseCurrentPopup();
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine(0, 12);

    bool canPlay = urlBuffer[0] != '\0';
    
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.616f, 0.306f, 0.867f, canPlay ? 1.0f : 0.4f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.78f, 0.49f, 1.0f, 1.0f));
    
    if (!canPlay) ImGui::BeginDisabled();
    if (ImGui::Button(ICON_FA_PLAY "  Play", ImVec2(btnWidth, 36)) || (enterPressed && canPlay)) {
      mpv->commandv("loadfile", urlBuffer, nullptr);
      m_showURLDialog = false;
      urlBuffer[0] = '\0';
      ImGui::CloseCurrentPopup();
    }
    if (!canPlay) ImGui::EndDisabled();
    
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();

    ImGui::EndPopup();
  }

  ImGui::PopStyleVar(3);
  ImGui::PopStyleColor(2);
}

}  // namespace ImPlay::Views
