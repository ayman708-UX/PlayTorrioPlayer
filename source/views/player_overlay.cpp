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
  m_controlsAlpha = 1.0f;
  m_targetAlpha = 1.0f;
}

void PlayerOverlay::draw() {
  if (!m_visible) return;
  if (!mpv) return;  // Safety check

  if (m_showURLDialog) openURL();

  ImGuiIO& io = ImGui::GetIO();
  double now = ImGui::GetTime();
  
  bool inMenu = m_showSubtitleMenu || m_showAudioMenu || m_showSettingsMenu;
  bool mouseActive = !inMenu && (io.MouseDelta.x != 0 || io.MouseDelta.y != 0);
  bool hasActivity = mouseActive || io.MouseDown[0] || io.MouseDown[1] || inMenu;
  
  if (hasActivity) {
    m_lastActivityTime = now;
    m_targetAlpha = 1.0f;
  } else if (now - m_lastActivityTime > 3.0) {
    m_targetAlpha = 0.0f;
  }

  float lerpSpeed = 0.12f;
  m_controlsAlpha += (m_targetAlpha - m_controlsAlpha) * lerpSpeed;
  if (std::abs(m_targetAlpha - m_controlsAlpha) < 0.005f) 
    m_controlsAlpha = m_targetAlpha;

  if (m_controlsAlpha < 0.01f) return;

  drawTopBar();
  drawBottomControls();

  if (m_showSubtitleMenu) drawSubtitleMenu();
  if (m_showAudioMenu) drawAudioMenu();
  if (m_showSettingsMenu) drawSettingsMenu();
}

void PlayerOverlay::drawIdleScreen() {
  if (!mpv) return;  // Safety check
  
  auto vp = ImGui::GetMainViewport();
  ImVec2 wPos = vp->WorkPos;
  ImVec2 wSize = vp->WorkSize;

  if (m_showURLDialog) openURL();

  ImGui::SetNextWindowPos(wPos);
  ImGui::SetNextWindowSize(wSize);

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.01f, 0.05f, 1.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

  if (ImGui::Begin("##IdleScreen", nullptr, flags)) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    
    // Beautiful gradient background
    ImU32 topCol = IM_COL32(20, 8, 45, 255);
    ImU32 botCol = IM_COL32(5, 2, 15, 255);
    dl->AddRectFilledMultiColor(wPos, ImVec2(wPos.x + wSize.x, wPos.y + wSize.y), topCol, topCol, botCol, botCol);

    // Subtle glow effect
    ImVec2 center(wPos.x + wSize.x / 2, wPos.y + wSize.y * 0.4f);
    for (int i = 5; i >= 0; i--) {
      float r = 120 + i * 40;
      int alpha = 8 - i;
      dl->AddCircleFilled(center, r, IM_COL32(157, 78, 221, alpha), 64);
    }

    // Main play circle
    float circleR = 70;
    dl->AddCircleFilled(center, circleR, IM_COL32(157, 78, 221, 30), 64);
    dl->AddCircle(center, circleR - 5, IM_COL32(199, 125, 255, 200), 64, 3.0f);
    
    // Play triangle
    float triSize = 28;
    ImVec2 p1(center.x - triSize * 0.4f, center.y - triSize);
    ImVec2 p2(center.x - triSize * 0.4f, center.y + triSize);
    ImVec2 p3(center.x + triSize * 0.8f, center.y);
    dl->AddTriangleFilled(p1, p2, p3, IM_COL32(255, 255, 255, 230));

    // Title
    float titleY = center.y - wPos.y + 110;
    ImGui::SetWindowFontScale(2.8f);
    const char* title = "PlayTorrio";
    ImVec2 ts = ImGui::CalcTextSize(title);
    ImGui::SetCursorPos(ImVec2((wSize.x - ts.x) / 2, titleY));
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.95f), "%s", title);
    ImGui::SetWindowFontScale(1.0f);

    // Subtitle
    ImGui::SetWindowFontScale(1.2f);
    const char* sub = "Modern Media Experience";
    ImVec2 ss = ImGui::CalcTextSize(sub);
    ImGui::SetCursorPos(ImVec2((wSize.x - ss.x) / 2, titleY + 55));
    ImGui::TextColored(ImVec4(0.6f, 0.5f, 0.7f, 0.8f), "%s", sub);
    ImGui::SetWindowFontScale(1.0f);

    // Buttons
    float btnW = 200, btnH = 56, gap = 30;
    float btnY = titleY + 120;
    float btnX = (wSize.x - btnW * 2 - gap) / 2;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 28);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    
    // Open File button - primary purple
    ImGui::SetCursorPos(ImVec2(btnX, btnY));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.62f, 0.31f, 0.87f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.72f, 0.45f, 0.95f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.52f, 0.25f, 0.75f, 1.0f));
    ImGui::SetWindowFontScale(1.15f);
    if (ImGui::Button(ICON_FA_FOLDER_OPEN "   Open File", ImVec2(btnW, btnH))) openMediaFile();
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor(3);

    // Open URL button - outline style
    ImGui::SameLine(0, gap);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.10f, 0.25f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.18f, 0.40f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.25f, 0.55f, 1.0f));
    ImGui::SetWindowFontScale(1.15f);
    if (ImGui::Button(ICON_FA_LINK "   Open URL", ImVec2(btnW, btnH))) m_showURLDialog = true;
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor(3);
    
    ImGui::PopStyleVar(2);

    // Hint text
    ImGui::SetWindowFontScale(1.0f);
    const char* hint = "Drag and drop files to play";
    ImVec2 hs = ImGui::CalcTextSize(hint);
    ImGui::SetCursorPos(ImVec2((wSize.x - hs.x) / 2, btnY + 85));
    ImGui::TextColored(ImVec4(0.4f, 0.35f, 0.5f, 0.6f), "%s", hint);
  }
  ImGui::End();
  ImGui::PopStyleVar();
  ImGui::PopStyleColor();
}

void PlayerOverlay::drawTopBar() {
  if (!mpv) return;  // Safety check
  
  auto vp = ImGui::GetMainViewport();
  ImVec2 wPos = vp->WorkPos;
  ImVec2 wSize = vp->WorkSize;

  float barH = 70;
  ImGui::SetNextWindowPos(wPos);
  ImGui::SetNextWindowSize(ImVec2(wSize.x, barH));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
                           ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

  ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_controlsAlpha);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

  if (ImGui::Begin("##TopBar", nullptr, flags)) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    
    // Smooth gradient from top
    ImU32 topCol = IM_COL32(0, 0, 0, (int)(220 * m_controlsAlpha));
    ImU32 botCol = IM_COL32(0, 0, 0, 0);
    dl->AddRectFilledMultiColor(wPos, ImVec2(wPos.x + wSize.x, wPos.y + barH), topCol, topCol, botCol, botCol);

    // Back button - clean circle
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 1, 1, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.12f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.4f, 1.0f, 0.2f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 25);

    ImGui::SetCursorPos(ImVec2(18, 15));
    ImGui::SetWindowFontScale(1.4f);
    if (ImGui::Button(ICON_FA_CHEVRON_LEFT "##back", ImVec2(50, 50))) mpv->command("quit");
    ImGui::SetWindowFontScale(1.0f);

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    // Title - clean and big
    ImGui::SameLine();
    ImGui::SetCursorPos(ImVec2(80, 22));
    
    std::string title = mpv->property("media-title");
    if (title.empty()) title = "PlayTorrio";
    
    float maxTitleW = wSize.x - 120;
    ImGui::SetWindowFontScale(1.35f);
    ImVec2 titleSize = ImGui::CalcTextSize(title.c_str());
    if (titleSize.x > maxTitleW) {
      float ratio = maxTitleW / titleSize.x;
      size_t len = (size_t)(title.length() * ratio);
      if (len > 3) title = title.substr(0, len - 3) + "...";
    }
    ImGui::TextColored(ImVec4(1, 1, 1, 0.95f), "%s", title.c_str());
    ImGui::SetWindowFontScale(1.0f);
  }
  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar(2);
}

void PlayerOverlay::drawBottomControls() {
  if (!mpv) return;  // Safety check
  
  auto vp = ImGui::GetMainViewport();
  ImVec2 wPos = vp->WorkPos;
  ImVec2 wSize = vp->WorkSize;

  float barH = 130;
  ImGui::SetNextWindowPos(ImVec2(wPos.x, wPos.y + wSize.y - barH));
  ImGui::SetNextWindowSize(ImVec2(wSize.x, barH));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
                           ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

  ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_controlsAlpha);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

  if (ImGui::Begin("##BottomControls", nullptr, flags)) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 barPos = ImVec2(wPos.x, wPos.y + wSize.y - barH);
    
    // Smooth gradient from bottom
    ImU32 topCol = IM_COL32(0, 0, 0, 0);
    ImU32 botCol = IM_COL32(0, 0, 0, (int)(230 * m_controlsAlpha));
    dl->AddRectFilledMultiColor(barPos, ImVec2(barPos.x + wSize.x, barPos.y + barH), topCol, topCol, botCol, botCol);

    // Progress bar
    ImGui::SetCursorPos(ImVec2(30, 20));
    drawProgressBar();

    // Control buttons
    ImGui::SetCursorPos(ImVec2(30, 60));
    drawControlButtons();
  }
  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar(2);
}

void PlayerOverlay::drawProgressBar() {
  if (!mpv) return;  // Safety check
  
  auto vp = ImGui::GetMainViewport();
  ImVec2 wPos = vp->WorkPos;
  float barWidth = vp->WorkSize.x - 60;
  float barHeight = 6;
  float barY = ImGui::GetCursorScreenPos().y;
  float barX = wPos.x + 30;

  double duration = mpv->property<double, MPV_FORMAT_DOUBLE>("duration");
  double position = (double)mpv->timePos;
  float progress = duration > 0 ? (float)(position / duration) : 0.0f;
  if (m_seeking) progress = m_seekPos;

  ImDrawList* dl = ImGui::GetWindowDrawList();
  
  // Track background
  dl->AddRectFilled(
    ImVec2(barX, barY), 
    ImVec2(barX + barWidth, barY + barHeight),
    IM_COL32(100, 100, 110, (int)(120 * m_controlsAlpha)), 
    3.0f
  );
  
  // Progress fill - purple gradient
  float progressW = barWidth * progress;
  if (progressW > 0) {
    dl->AddRectFilled(
      ImVec2(barX, barY), 
      ImVec2(barX + progressW, barY + barHeight),
      IM_COL32(180, 100, 255, (int)(255 * m_controlsAlpha)), 
      3.0f
    );
  }
  
  // Seek handle
  float handleX = barX + progressW;
  float handleY = barY + barHeight / 2;
  
  // Invisible hit area
  ImGui::SetCursorScreenPos(ImVec2(barX - 10, barY - 15));
  ImGui::InvisibleButton("##seekbar", ImVec2(barWidth + 20, barHeight + 30));
  
  bool hovered = ImGui::IsItemHovered();
  bool active = ImGui::IsItemActive();
  float handleR = (hovered || active) ? 10.0f : 7.0f;
  
  if (hovered || active) {
    ImVec2 mousePos = ImGui::GetMousePos();
    float seekProgress = std::clamp((mousePos.x - barX) / barWidth, 0.0f, 1.0f);
    
    if (active) {
      m_seeking = true;
      m_seekPos = seekProgress;
    }
    
    // Time tooltip
    double seekTime = seekProgress * duration;
    int h = (int)(seekTime / 3600);
    int m = (int)((seekTime - h * 3600) / 60);
    int s = (int)(seekTime - h * 3600 - m * 60);
    std::string timeStr = h > 0 ? fmt::format("{:d}:{:02d}:{:02d}", h, m, s) : fmt::format("{:d}:{:02d}", m, s);
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 6));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.1f, 0.08f, 0.15f, 0.95f));
    ImGui::SetTooltip("%s", timeStr.c_str());
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
  }
  
  if (m_seeking && !active) {
    mpv->commandv("seek", std::to_string(m_seekPos * duration).c_str(), "absolute", nullptr);
    m_seeking = false;
  }
  
  // Draw handle
  dl->AddCircleFilled(ImVec2(handleX, handleY), handleR, IM_COL32(255, 255, 255, (int)(255 * m_controlsAlpha)), 32);
  if (hovered || active) {
    dl->AddCircle(ImVec2(handleX, handleY), handleR + 4, IM_COL32(180, 100, 255, (int)(100 * m_controlsAlpha)), 32, 2.5f);
  }
}

void PlayerOverlay::drawControlButtons() {
  if (!mpv) return;  // Safety check
  
  auto vp = ImGui::GetMainViewport();
  float windowW = vp->WorkSize.x;
  
  float btnSize = 52.0f;
  float playBtnSize = 64.0f;
  float smallBtnSize = 48.0f;
  float y = ImGui::GetCursorPosY();

  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.4f, 1.0f, 0.2f));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 32);
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 0));

  // === LEFT SIDE: Playback controls ===
  
  // Play/Pause - big purple button
  bool paused = mpv->pause;
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.62f, 0.31f, 0.87f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.72f, 0.45f, 0.95f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.52f, 0.25f, 0.75f, 1.0f));
  ImGui::SetCursorPosY(y);
  ImGui::SetWindowFontScale(1.6f);
  if (ImGui::Button(paused ? ICON_FA_PLAY "##play" : ICON_FA_PAUSE "##pause", ImVec2(playBtnSize, playBtnSize))) 
    mpv->command("cycle pause");
  ImGui::SetWindowFontScale(1.0f);
  ImGui::PopStyleColor(3);

  // Skip backward
  ImGui::SameLine(0, 12);
  ImGui::SetCursorPosY(y + (playBtnSize - btnSize) / 2);
  ImGui::SetWindowFontScale(1.4f);
  if (ImGui::Button(ICON_FA_BACKWARD "##back10", ImVec2(btnSize, btnSize))) 
    mpv->command("seek -10");
  
  // Skip forward
  ImGui::SameLine(0, 4);
  ImGui::SetCursorPosY(y + (playBtnSize - btnSize) / 2);
  if (ImGui::Button(ICON_FA_FORWARD "##fwd10", ImVec2(btnSize, btnSize))) 
    mpv->command("seek 10");
  ImGui::SetWindowFontScale(1.0f);

  // === VOLUME ===
  ImGui::SameLine(0, 20);
  ImGui::SetCursorPosY(y + (playBtnSize - btnSize) / 2);
  
  bool muted = mpv->mute;
  int vol = (int)mpv->volume;
  const char* volIcon = muted ? ICON_FA_VOLUME_MUTE : 
                        (vol > 60 ? ICON_FA_VOLUME_UP : 
                         vol > 20 ? ICON_FA_VOLUME_DOWN : ICON_FA_VOLUME_OFF);
  
  ImGui::SetWindowFontScale(1.3f);
  if (ImGui::Button(volIcon, ImVec2(btnSize, btnSize))) 
    mpv->command("cycle mute");
  ImGui::SetWindowFontScale(1.0f);
  
  // Volume slider
  ImGui::SameLine(0, 4);
  ImGui::SetCursorPosY(y + (playBtnSize - 20) / 2);
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1, 1, 1, 0.1f));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(1, 1, 1, 0.15f));
  ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1, 1, 1, 0.9f));
  ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.7f, 0.4f, 1.0f, 1.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4);
  ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 12);
  ImGui::SetNextItemWidth(80);
  if (ImGui::SliderInt("##volume", &vol, 0, 100, "")) {
    mpv->commandv("set", "volume", std::to_string(vol).c_str(), nullptr);
  }
  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor(4);

  // === TIME DISPLAY ===
  double dur = mpv->property<double, MPV_FORMAT_DOUBLE>("duration");
  double pos = (double)mpv->timePos;
  
  auto formatTime = [](double t) -> std::string {
    if (t < 0) t = 0;
    int h = (int)(t / 3600);
    int m = (int)((t - h * 3600) / 60);
    int s = (int)(t - h * 3600 - m * 60);
    return h > 0 ? fmt::format("{:d}:{:02d}:{:02d}", h, m, s) : fmt::format("{:d}:{:02d}", m, s);
  };

  std::string timeStr = formatTime(pos) + " / " + formatTime(dur);
  ImGui::SameLine(0, 20);
  ImGui::SetCursorPosY(y + (playBtnSize - ImGui::GetTextLineHeight()) / 2);
  ImGui::SetWindowFontScale(1.1f);
  ImGui::TextColored(ImVec4(1, 1, 1, 0.85f), "%s", timeStr.c_str());
  ImGui::SetWindowFontScale(1.0f);

  // === RIGHT SIDE: Settings buttons ===
  float rightX = windowW - 240;
  ImGui::SameLine();
  ImGui::SetCursorPosX(rightX);
  ImGui::SetCursorPosY(y + (playBtnSize - smallBtnSize) / 2);

  ImGui::SetWindowFontScale(1.25f);
  
  // Subtitles
  if (ImGui::Button(ICON_FA_CLOSED_CAPTIONING "##subs", ImVec2(smallBtnSize, smallBtnSize))) {
    m_showSubtitleMenu = !m_showSubtitleMenu;
    m_showAudioMenu = false;
    m_showSettingsMenu = false;
  }
  
  // Audio
  ImGui::SameLine(0, 6);
  ImGui::SetCursorPosY(y + (playBtnSize - smallBtnSize) / 2);
  if (ImGui::Button(ICON_FA_HEADPHONES "##audio", ImVec2(smallBtnSize, smallBtnSize))) {
    m_showAudioMenu = !m_showAudioMenu;
    m_showSubtitleMenu = false;
    m_showSettingsMenu = false;
  }
  
  // Settings
  ImGui::SameLine(0, 6);
  ImGui::SetCursorPosY(y + (playBtnSize - smallBtnSize) / 2);
  if (ImGui::Button(ICON_FA_SLIDERS_H "##settings", ImVec2(smallBtnSize, smallBtnSize))) {
    m_showSettingsMenu = !m_showSettingsMenu;
    m_showSubtitleMenu = false;
    m_showAudioMenu = false;
  }
  
  // Fullscreen
  ImGui::SameLine(0, 6);
  ImGui::SetCursorPosY(y + (playBtnSize - smallBtnSize) / 2);
  const char* fsIcon = mpv->fullscreen ? ICON_FA_COMPRESS : ICON_FA_EXPAND;
  if (ImGui::Button(fsIcon, ImVec2(smallBtnSize, smallBtnSize))) {
    mpv->command("cycle fullscreen");
  }
  
  ImGui::SetWindowFontScale(1.0f);
  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor(3);
}


void PlayerOverlay::drawSubtitleMenu() {
  if (!mpv) return;  // Safety check
  
  auto vp = ImGui::GetMainViewport();
  float menuW = 340, menuH = 420;
  ImVec2 menuPos(vp->WorkPos.x + vp->WorkSize.x - menuW - 25, vp->WorkPos.y + vp->WorkSize.y - menuH - 145);
  
  ImGui::SetNextWindowPos(menuPos);
  ImGui::SetNextWindowSize(ImVec2(menuW, menuH));

  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.04f, 0.12f, 0.97f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.3f, 0.8f, 0.3f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 16));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
  
  if (ImGui::Begin("##SubtitleMenu", &m_showSubtitleMenu, flags)) {
    // Header
    ImGui::SetWindowFontScale(1.3f);
    ImGui::TextColored(ImVec4(0.8f, 0.5f, 1.0f, 1.0f), ICON_FA_CLOSED_CAPTIONING);
    ImGui::SameLine(0, 12);
    ImGui::TextColored(ImVec4(1, 1, 1, 0.95f), "Subtitles");
    ImGui::SetWindowFontScale(1.0f);
    
    // Close button
    ImGui::SameLine(menuW - 50);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
    ImGui::SetWindowFontScale(1.2f);
    if (ImGui::Button(ICON_FA_TIMES "##closeSubMenu", ImVec2(30, 30))) m_showSubtitleMenu = false;
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor(2);
    
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.5f, 0.3f, 0.8f, 0.3f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // Provider tabs
    int numProviders = 1 + (int)m_externalProviders.size();
    float tabW = (menuW - 40) / std::min(numProviders, 3);
    
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 6));
    
    // Built-in tab
    bool isBuiltIn = (m_selectedProviderTab == 0);
    ImGui::PushStyleColor(ImGuiCol_Button, isBuiltIn ? ImVec4(0.5f, 0.28f, 0.78f, 1.0f) : ImVec4(0.15f, 0.1f, 0.25f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.38f, 0.88f, 1.0f));
    ImGui::SetWindowFontScale(1.05f);
    if (ImGui::Button("Embedded##tab0", ImVec2(tabW - 3, 34))) m_selectedProviderTab = 0;
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor(2);
    
    // External provider tabs
    for (int i = 0; i < (int)m_externalProviders.size() && i < 2; i++) {
      ImGui::SameLine();
      bool isSelected = (m_selectedProviderTab == i + 1);
      ImGui::PushStyleColor(ImGuiCol_Button, isSelected ? ImVec4(0.5f, 0.28f, 0.78f, 1.0f) : ImVec4(0.15f, 0.1f, 0.25f, 0.9f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.38f, 0.88f, 1.0f));
      
      std::string tabName = m_externalProviders[i].name;
      if (tabName.length() > 12) tabName = tabName.substr(0, 11) + "..";
      ImGui::SetWindowFontScale(1.05f);
      if (ImGui::Button((tabName + "##provtab" + std::to_string(i)).c_str(), ImVec2(tabW - 3, 34))) 
        m_selectedProviderTab = i + 1;
      ImGui::SetWindowFontScale(1.0f);
      ImGui::PopStyleColor(2);
    }
    
    ImGui::PopStyleVar(2);
    ImGui::Spacing();

    // Content
    float listH = menuH - 220;
    
    if (m_selectedProviderTab == 0) {
      ImGui::TextColored(ImVec4(0.6f, 0.5f, 0.75f, 0.9f), "Video Tracks");
      ImGui::Spacing();
      
      ImGui::BeginChild("##SubList", ImVec2(menuW - 40, listH), false);
      std::string currentSid = mpv->sid;
      
      ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.5f, 0.3f, 0.8f, 0.25f));
      ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.6f, 0.4f, 0.9f, 0.35f));
      ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0, 0.5f));
      
      bool hasEmbedded = false;
      for (auto &track : mpv->tracks) {
        if (track.type != "sub") continue;
        hasEmbedded = true;
        
        std::string label = track.title.empty() ? fmt::format("Track {}", track.id) : track.title;
        if (!track.lang.empty()) label += "  [" + track.lang + "]";
        
        bool selected = currentSid == std::to_string(track.id);
        ImGui::SetWindowFontScale(1.05f);
        if (ImGui::Selectable(label.c_str(), selected, 0, ImVec2(0, 32)))
          mpv->property<int64_t, MPV_FORMAT_INT64>("sid", track.id);
        ImGui::SetWindowFontScale(1.0f);
      }
      
      if (!hasEmbedded) {
        ImGui::TextColored(ImVec4(0.5f, 0.45f, 0.6f, 0.7f), "No embedded subtitles");
      }
      
      ImGui::Spacing();
      ImGui::SetWindowFontScale(1.05f);
      if (ImGui::Selectable("Disable Subtitles", currentSid == "no", 0, ImVec2(0, 32)))
        mpv->commandv("set", "sid", "no", nullptr);
      ImGui::SetWindowFontScale(1.0f);
      
      ImGui::PopStyleVar();
      ImGui::PopStyleColor(2);
      ImGui::EndChild();
    } else {
      int provIdx = m_selectedProviderTab - 1;
      if (provIdx >= 0 && provIdx < (int)m_externalProviders.size()) {
        auto& provider = m_externalProviders[provIdx];
        ImGui::TextColored(ImVec4(0.6f, 0.5f, 0.75f, 0.9f), "%s  (%d available)", provider.name.c_str(), (int)provider.subtitles.size());
        ImGui::Spacing();
        
        ImGui::BeginChild("##ExtSubList", ImVec2(menuW - 40, listH), false);
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.5f, 0.3f, 0.8f, 0.25f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.6f, 0.4f, 0.9f, 0.35f));
        ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0, 0.5f));
        
        for (int i = 0; i < (int)provider.subtitles.size(); i++) {
          auto& sub = provider.subtitles[i];
          ImGui::SetWindowFontScale(1.05f);
          if (ImGui::Selectable((sub.name + "##extsub" + std::to_string(i)).c_str(), false, 0, ImVec2(0, 32))) {
            mpv->commandv("sub-add", sub.url.c_str(), "select", nullptr);
          }
          ImGui::SetWindowFontScale(1.0f);
        }
        
        if (provider.subtitles.empty()) {
          ImGui::TextColored(ImVec4(0.5f, 0.45f, 0.6f, 0.7f), "No subtitles available");
        }
        
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
        ImGui::EndChild();
      }
    }

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.5f, 0.3f, 0.8f, 0.3f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();
    
    // Load file button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.28f, 0.78f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.38f, 0.88f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10);
    ImGui::SetWindowFontScale(1.1f);
    if (ImGui::Button(ICON_FA_FILE_UPLOAD "   Load Subtitle File", ImVec2(menuW - 40, 40))) openSubtitleFile();
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
  }
  ImGui::End();
  ImGui::PopStyleVar(3);
  ImGui::PopStyleColor(2);
}


void PlayerOverlay::drawAudioMenu() {
  if (!mpv) return;  // Safety check
  
  auto vp = ImGui::GetMainViewport();
  float menuW = 320, menuH = 280;
  ImVec2 menuPos(vp->WorkPos.x + vp->WorkSize.x - menuW - 25, vp->WorkPos.y + vp->WorkSize.y - menuH - 145);
  
  ImGui::SetNextWindowPos(menuPos);
  ImGui::SetNextWindowSize(ImVec2(menuW, menuH));

  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.04f, 0.12f, 0.97f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.3f, 0.8f, 0.3f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 16));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
  
  if (ImGui::Begin("##AudioMenu", &m_showAudioMenu, flags)) {
    // Header
    ImGui::SetWindowFontScale(1.3f);
    ImGui::TextColored(ImVec4(0.8f, 0.5f, 1.0f, 1.0f), ICON_FA_HEADPHONES);
    ImGui::SameLine(0, 12);
    ImGui::TextColored(ImVec4(1, 1, 1, 0.95f), "Audio");
    ImGui::SetWindowFontScale(1.0f);
    
    // Close button
    ImGui::SameLine(menuW - 50);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
    ImGui::SetWindowFontScale(1.2f);
    if (ImGui::Button(ICON_FA_TIMES "##closeAudioMenu", ImVec2(30, 30))) m_showAudioMenu = false;
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor(2);
    
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.5f, 0.3f, 0.8f, 0.3f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.6f, 0.5f, 0.75f, 0.9f), "Audio Tracks");
    ImGui::Spacing();

    ImGui::BeginChild("##AudioList", ImVec2(menuW - 40, menuH - 100), false);
    std::string currentAid = mpv->aid;
    
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.5f, 0.3f, 0.8f, 0.25f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.6f, 0.4f, 0.9f, 0.35f));
    ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0, 0.5f));
    
    bool hasAudio = false;
    for (auto &track : mpv->tracks) {
      if (track.type != "audio") continue;
      hasAudio = true;
      
      std::string label = track.title.empty() ? fmt::format("Track {}", track.id) : track.title;
      if (!track.lang.empty()) label += "  [" + track.lang + "]";
      
      bool selected = currentAid == std::to_string(track.id);
      ImGui::SetWindowFontScale(1.05f);
      if (ImGui::Selectable(label.c_str(), selected, 0, ImVec2(0, 32)))
        mpv->property<int64_t, MPV_FORMAT_INT64>("aid", track.id);
      ImGui::SetWindowFontScale(1.0f);
    }
    
    if (!hasAudio) {
      ImGui::TextColored(ImVec4(0.5f, 0.45f, 0.6f, 0.7f), "No audio tracks");
    }
    
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
    ImGui::EndChild();
  }
  ImGui::End();
  ImGui::PopStyleVar(3);
  ImGui::PopStyleColor(2);
}

void PlayerOverlay::drawSettingsMenu() {
  if (!mpv) return;  // Safety check
  
  auto vp = ImGui::GetMainViewport();
  float menuW = 360, menuH = 520;
  ImVec2 menuPos(vp->WorkPos.x + vp->WorkSize.x - menuW - 25, vp->WorkPos.y + vp->WorkSize.y - menuH - 145);
  
  ImGui::SetNextWindowPos(menuPos);
  ImGui::SetNextWindowSize(ImVec2(menuW, menuH));

  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.04f, 0.12f, 0.97f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.3f, 0.8f, 0.3f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 16));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
  
  if (ImGui::Begin("##SettingsMenu", &m_showSettingsMenu, flags)) {
    // Header
    ImGui::SetWindowFontScale(1.3f);
    ImGui::TextColored(ImVec4(0.8f, 0.5f, 1.0f, 1.0f), ICON_FA_SLIDERS_H);
    ImGui::SameLine(0, 12);
    ImGui::TextColored(ImVec4(1, 1, 1, 0.95f), "Settings");
    ImGui::SetWindowFontScale(1.0f);
    
    // Close button
    ImGui::SameLine(menuW - 50);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
    ImGui::SetWindowFontScale(1.2f);
    if (ImGui::Button(ICON_FA_TIMES "##closeSettingsMenu", ImVec2(30, 30))) m_showSettingsMenu = false;
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor(2);
    
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.5f, 0.3f, 0.8f, 0.3f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::BeginChild("##SettingsContent", ImVec2(menuW - 40, menuH - 80), false);
    
    float labelW = 110;
    float controlW = menuW - labelW - 60;
    
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.08f, 0.18f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.15f, 0.12f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.6f, 0.35f, 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.7f, 0.45f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.7f, 0.45f, 1.0f, 1.0f));

    // Playback Speed
    ImGui::SetWindowFontScale(1.05f);
    ImGui::TextColored(ImVec4(0.8f, 0.75f, 0.9f, 0.9f), "Speed");
    ImGui::SameLine(labelW);
    float speed = (float)mpv->property<double, MPV_FORMAT_DOUBLE>("speed");
    ImGui::SetNextItemWidth(controlW);
    if (ImGui::SliderFloat("##speed", &speed, 0.25f, 4.0f, "%.2fx"))
      mpv->commandv("set", "speed", fmt::format("{:.2f}", speed).c_str(), nullptr);
    ImGui::SetWindowFontScale(1.0f);
    
    ImGui::Spacing();
    ImGui::Spacing();

    // Aspect Ratio
    ImGui::SetWindowFontScale(1.05f);
    ImGui::TextColored(ImVec4(0.8f, 0.75f, 0.9f, 0.9f), "Aspect");
    ImGui::SameLine(labelW);
    const char* aspects[] = {"Auto", "16:9", "4:3", "21:9", "1:1"};
    static int aspectIdx = 0;
    ImGui::SetNextItemWidth(controlW);
    if (ImGui::Combo("##aspect", &aspectIdx, aspects, IM_ARRAYSIZE(aspects)))
      mpv->commandv("set", "video-aspect-override", aspectIdx == 0 ? "-1" : aspects[aspectIdx], nullptr);
    ImGui::SetWindowFontScale(1.0f);
    
    ImGui::Spacing();
    ImGui::Spacing();

    // Hardware Decoding
    ImGui::SetWindowFontScale(1.05f);
    ImGui::TextColored(ImVec4(0.8f, 0.75f, 0.9f, 0.9f), "HW Decode");
    ImGui::SameLine(labelW);
    std::string hwdec = mpv->property("hwdec");
    bool hwEnabled = hwdec != "no";
    if (ImGui::Checkbox("##hwdec", &hwEnabled))
      mpv->commandv("set", "hwdec", hwEnabled ? "auto" : "no", nullptr);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 0.8f), hwEnabled ? "GPU" : "CPU");
    ImGui::SetWindowFontScale(1.0f);
    
    ImGui::Spacing();
    ImGui::Spacing();

    // Loop
    ImGui::SetWindowFontScale(1.05f);
    ImGui::TextColored(ImVec4(0.8f, 0.75f, 0.9f, 0.9f), "Loop");
    ImGui::SameLine(labelW);
    std::string loopFile = mpv->property("loop-file");
    bool loopEnabled = loopFile == "inf";
    if (ImGui::Checkbox("##loop", &loopEnabled))
      mpv->commandv("set", "loop-file", loopEnabled ? "inf" : "no", nullptr);
    ImGui::SetWindowFontScale(1.0f);
    
    ImGui::Spacing();
    ImGui::Spacing();

    // Cache
    ImGui::SetWindowFontScale(1.05f);
    ImGui::TextColored(ImVec4(0.8f, 0.75f, 0.9f, 0.9f), "Cache");
    ImGui::SameLine(labelW);
    static int cacheSize = 150;
    ImGui::SetNextItemWidth(controlW);
    if (ImGui::SliderInt("##cache", &cacheSize, 16, 512, "%d MB"))
      mpv->commandv("set", "demuxer-max-bytes", fmt::format("{}MiB", cacheSize).c_str(), nullptr);
    ImGui::SetWindowFontScale(1.0f);

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.5f, 0.3f, 0.8f, 0.2f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();
    ImGui::Spacing();

    // Subtitle section header
    ImGui::SetWindowFontScale(1.15f);
    ImGui::TextColored(ImVec4(0.8f, 0.5f, 1.0f, 1.0f), ICON_FA_CLOSED_CAPTIONING);
    ImGui::SameLine(0, 10);
    ImGui::TextColored(ImVec4(1, 1, 1, 0.9f), "Subtitle Options");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Spacing();
    ImGui::Spacing();

    // Subtitle Size
    ImGui::SetWindowFontScale(1.05f);
    ImGui::TextColored(ImVec4(0.8f, 0.75f, 0.9f, 0.9f), "Size");
    ImGui::SameLine(labelW);
    static int subSize = 55;
    ImGui::SetNextItemWidth(controlW);
    if (ImGui::SliderInt("##subsize", &subSize, 20, 100, "%d"))
      mpv->commandv("set", "sub-font-size", std::to_string(subSize).c_str(), nullptr);
    ImGui::SetWindowFontScale(1.0f);
    
    ImGui::Spacing();
    ImGui::Spacing();

    // Subtitle Position
    ImGui::SetWindowFontScale(1.05f);
    ImGui::TextColored(ImVec4(0.8f, 0.75f, 0.9f, 0.9f), "Position");
    ImGui::SameLine(labelW);
    static int subPos = 100;
    ImGui::SetNextItemWidth(controlW);
    if (ImGui::SliderInt("##subpos", &subPos, 0, 150, "%d%%"))
      mpv->commandv("set", "sub-pos", std::to_string(subPos).c_str(), nullptr);
    ImGui::SetWindowFontScale(1.0f);
    
    ImGui::Spacing();
    ImGui::Spacing();

    // Subtitle Delay
    ImGui::SetWindowFontScale(1.05f);
    ImGui::TextColored(ImVec4(0.8f, 0.75f, 0.9f, 0.9f), "Delay");
    ImGui::SameLine(labelW);
    double subDelay = mpv->property<double, MPV_FORMAT_DOUBLE>("sub-delay");
    float subDelayF = (float)subDelay;
    ImGui::SetNextItemWidth(controlW);
    if (ImGui::SliderFloat("##subdelay", &subDelayF, -5.0f, 5.0f, "%.1f s"))
      mpv->commandv("set", "sub-delay", fmt::format("{:.2f}", subDelayF).c_str(), nullptr);
    ImGui::SetWindowFontScale(1.0f);

    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar();
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
  if (auto res = NFD::openFile(filters))
    mpv->commandv("sub-add", res->string().c_str(), "select", nullptr);
  mpv->command("set pause no");
}

void PlayerOverlay::openMediaFile() {
  std::vector<std::pair<std::string, std::string>> filters = {
      {"Video Files", "mp4,mkv,avi,mov,wmv,flv,webm,m4v,mpg,mpeg,ts,m2ts,vob"},
      {"Audio Files", "mp3,flac,wav,aac,ogg,m4a,wma,opus"},
      {"All Files", "*"},
  };
  if (auto res = NFD::openFile(filters))
    mpv->commandv("loadfile", res->string().c_str(), nullptr);
}

void PlayerOverlay::openURL() {
  ImGui::OpenPopup("##OpenURLPopup");
  
  ImVec2 windowSize = ImGui::GetMainViewport()->WorkSize;
  float popupW = std::min(windowSize.x * 0.4f, 420.0f);
  ImGui::SetNextWindowSize(ImVec2(popupW, 0), ImGuiCond_Always);
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

  ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.06f, 0.04f, 0.12f, 0.98f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.3f, 0.8f, 0.4f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 20));
  ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1);

  if (ImGui::BeginPopupModal("##OpenURLPopup", &m_showURLDialog, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
    // Header
    ImGui::SetWindowFontScale(1.4f);
    ImGui::TextColored(ImVec4(0.8f, 0.5f, 1.0f, 1.0f), ICON_FA_LINK);
    ImGui::SameLine(0, 14);
    ImGui::TextColored(ImVec4(1, 1, 1, 0.95f), "Open URL");
    ImGui::SetWindowFontScale(1.0f);
    
    ImGui::Spacing();
    ImGui::Spacing();

    // URL input
    static char urlBuffer[2048] = {0};
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.08f, 0.05f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.12f, 0.08f, 0.2f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14, 12));
    ImGui::SetNextItemWidth(-1);
    ImGui::SetWindowFontScale(1.1f);
    bool enterPressed = ImGui::InputTextWithHint("##urlInput", "https://...", urlBuffer, IM_ARRAYSIZE(urlBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
    
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();

    // Buttons
    float btnW = 110;
    float btnH = 44;
    ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - btnW * 2 - 12);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10);

    // Cancel button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.08f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.12f, 0.28f, 1.0f));
    ImGui::SetWindowFontScale(1.1f);
    if (ImGui::Button("Cancel", ImVec2(btnW, btnH))) {
      m_showURLDialog = false;
      urlBuffer[0] = '\0';
      ImGui::CloseCurrentPopup();
    }
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor(2);

    ImGui::SameLine(0, 12);
    
    // Play button
    bool hasUrl = urlBuffer[0] != '\0';
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.3f, 0.82f, hasUrl ? 1.0f : 0.4f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.65f, 0.4f, 0.92f, 1.0f));
    if (!hasUrl) ImGui::BeginDisabled();
    ImGui::SetWindowFontScale(1.1f);
    if (ImGui::Button(ICON_FA_PLAY "  Play", ImVec2(btnW, btnH)) || (enterPressed && hasUrl)) {
      mpv->commandv("loadfile", urlBuffer, nullptr);
      m_showURLDialog = false;
      urlBuffer[0] = '\0';
      ImGui::CloseCurrentPopup();
    }
    ImGui::SetWindowFontScale(1.0f);
    if (!hasUrl) ImGui::EndDisabled();
    ImGui::PopStyleColor(2);
    
    ImGui::PopStyleVar();
    ImGui::EndPopup();
  }
  ImGui::PopStyleVar(3);
  ImGui::PopStyleColor(2);
}

}  // namespace ImPlay::Views
