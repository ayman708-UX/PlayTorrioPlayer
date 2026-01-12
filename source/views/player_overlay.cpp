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
}

void PlayerOverlay::draw() {
  if (!m_visible) return;

  if (m_showURLDialog) openURL();

  double currentTime = ImGui::GetTime();
  ImGuiIO& io = ImGui::GetIO();
  
  bool hasActivity = (io.MouseDelta.x != 0 || io.MouseDelta.y != 0 ||
                      ImGui::IsAnyMouseDown() || 
                      m_showSubtitleMenu || m_showAudioMenu || m_showSettingsMenu);
  
  if (hasActivity) m_lastActivityTime = currentTime;

  float timeSinceActivity = (float)(currentTime - m_lastActivityTime);
  float targetAlpha = (timeSinceActivity < 2.5f) ? 1.0f : 0.0f;
  
  float speed = targetAlpha > m_controlsAlpha ? 0.15f : 0.05f;
  m_controlsAlpha += (targetAlpha - m_controlsAlpha) * speed;

  if (m_controlsAlpha < 0.01f) return;

  drawTopBar();
  drawBottomBar();

  if (m_showSubtitleMenu) drawSubtitleMenu();
  if (m_showAudioMenu) drawAudioMenu();
  if (m_showSettingsMenu) drawSettingsMenu();
}

void PlayerOverlay::drawIdleScreen() {
  auto vp = ImGui::GetMainViewport();
  ImVec2 wPos = vp->WorkPos;
  ImVec2 wSize = vp->WorkSize;

  if (m_showURLDialog) openURL();

  ImGui::SetNextWindowPos(wPos);
  ImGui::SetNextWindowSize(wSize);

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.012f, 0.004f, 0.035f, 1.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

  if (ImGui::Begin("##IdleScreen", nullptr, flags)) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    
    ImU32 top = IM_COL32(15, 5, 35, 255);
    ImU32 bot = IM_COL32(3, 0, 12, 255);
    dl->AddRectFilledMultiColor(wPos, ImVec2(wPos.x + wSize.x, wPos.y + wSize.y), top, top, bot, bot);

    float cy = wSize.y * 0.38f;
    
    ImVec2 center(wPos.x + wSize.x / 2, wPos.y + cy);
    for (int i = 3; i >= 0; i--) {
      float r = 55 + i * 12;
      int alpha = 15 - i * 3;
      dl->AddCircleFilled(center, r, IM_COL32(157, 78, 221, alpha), 64);
    }
    dl->AddCircle(center, 55, IM_COL32(199, 125, 255, 180), 64, 2.5f);
    
    ImVec2 p1(center.x - 15, center.y - 22);
    ImVec2 p2(center.x - 15, center.y + 22);
    ImVec2 p3(center.x + 22, center.y);
    dl->AddTriangleFilled(p1, p2, p3, IM_COL32(220, 180, 255, 240));

    ImGui::SetWindowFontScale(2.2f);
    const char* title = "PlayTorrioPlayer";
    ImVec2 ts = ImGui::CalcTextSize(title);
    ImGui::SetCursorPos(ImVec2((wSize.x - ts.x) / 2, cy + 85));
    ImGui::TextColored(ImVec4(0.86f, 0.70f, 1.0f, 1.0f), "%s", title);
    ImGui::SetWindowFontScale(1.0f);

    const char* sub = "Modern Media Experience";
    ImVec2 ss = ImGui::CalcTextSize(sub);
    ImGui::SetCursorPos(ImVec2((wSize.x - ss.x) / 2, cy + 135));
    ImGui::TextColored(ImVec4(0.5f, 0.45f, 0.6f, 0.9f), "%s", sub);

    float bw = 180, bh = 50, gap = 24;
    float sx = (wSize.x - bw * 2 - gap) / 2;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 25);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 14));
    
    ImGui::SetCursorPos(ImVec2(sx, cy + 190));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.616f, 0.306f, 0.867f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.50f, 0.95f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.25f, 0.7f, 1.0f));
    if (ImGui::Button(ICON_FA_FOLDER_OPEN "  Open File", ImVec2(bw, bh))) openMediaFile();
    ImGui::PopStyleColor(3);

    ImGui::SameLine(0, gap);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.08f, 0.22f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.18f, 0.40f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.25f, 0.55f, 1.0f));
    if (ImGui::Button(ICON_FA_LINK "  Open URL", ImVec2(bw, bh))) m_showURLDialog = true;
    ImGui::PopStyleColor(3);
    
    ImGui::PopStyleVar(2);

    const char* hint = "Drop files here to play";
    ImVec2 hs = ImGui::CalcTextSize(hint);
    ImGui::SetCursorPos(ImVec2((wSize.x - hs.x) / 2, cy + 265));
    ImGui::TextColored(ImVec4(0.35f, 0.30f, 0.45f, 0.7f), "%s", hint);
  }
  ImGui::End();
  ImGui::PopStyleVar();
  ImGui::PopStyleColor();
}

void PlayerOverlay::drawTopBar() {
  auto vp = ImGui::GetMainViewport();
  ImVec2 wPos = vp->WorkPos;
  ImVec2 wSize = vp->WorkSize;

  float h = 65;
  ImGui::SetNextWindowPos(wPos);
  ImGui::SetNextWindowSize(ImVec2(wSize.x, h));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
                           ImGuiWindowFlags_NoFocusOnAppearing;

  ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_controlsAlpha);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

  if (ImGui::Begin("##TopBar", nullptr, flags)) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    
    ImU32 top = IM_COL32(0, 0, 0, (int)(200 * m_controlsAlpha));
    ImU32 bot = IM_COL32(0, 0, 0, 0);
    dl->AddRectFilledMultiColor(wPos, ImVec2(wPos.x + wSize.x, wPos.y + h), top, top, bot, bot);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.2f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 20);

    ImGui::SetCursorPos(ImVec2(16, 12));
    if (ImGui::Button(ICON_FA_ARROW_LEFT, ImVec2(40, 40))) {
      mpv->command("quit");
    }

    ImGui::SameLine();
    ImGui::SetCursorPosY(20);
    
    std::string title = mpv->property("media-title");
    if (title.empty()) title = "PlayTorrioPlayer";
    
    float maxW = wSize.x - 80;
    ImVec2 ts = ImGui::CalcTextSize(title.c_str());
    if (ts.x > maxW) {
      size_t len = (size_t)(title.length() * maxW / ts.x);
      if (len > 3) title = title.substr(0, len - 3) + "...";
    }
    
    ImGui::TextColored(ImVec4(1, 1, 1, 0.92f), "%s", title.c_str());

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
  }
  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar(2);
}

void PlayerOverlay::drawBottomBar() {
  auto vp = ImGui::GetMainViewport();
  ImVec2 wPos = vp->WorkPos;
  ImVec2 wSize = vp->WorkSize;

  float h = 100;
  ImGui::SetNextWindowPos(ImVec2(wPos.x, wPos.y + wSize.y - h));
  ImGui::SetNextWindowSize(ImVec2(wSize.x, h));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
                           ImGuiWindowFlags_NoFocusOnAppearing;

  ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_controlsAlpha);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

  if (ImGui::Begin("##BottomBar", nullptr, flags)) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 barPos = ImVec2(wPos.x, wPos.y + wSize.y - h);
    
    ImU32 top = IM_COL32(0, 0, 0, 0);
    ImU32 bot = IM_COL32(0, 0, 0, (int)(220 * m_controlsAlpha));
    dl->AddRectFilledMultiColor(barPos, ImVec2(barPos.x + wSize.x, barPos.y + h), top, top, bot, bot);

    // Progress bar with watched color
    ImGui::SetCursorPos(ImVec2(24, 15));
    drawProgressBar();

    // All controls on ONE LINE
    ImGui::SetCursorPos(ImVec2(24, 50));
    drawControlButtons();
  }
  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar(2);
}

void PlayerOverlay::drawProgressBar() {
  auto vp = ImGui::GetMainViewport();
  ImVec2 wPos = vp->WorkPos;
  float barWidth = vp->WorkSize.x - 48;
  float barHeight = 6;
  float barY = ImGui::GetCursorScreenPos().y;
  float barX = wPos.x + 24;

  double duration = mpv->property<double, MPV_FORMAT_DOUBLE>("duration");
  double position = (double)mpv->timePos;
  float progress = duration > 0 ? (float)(position / duration) : 0.0f;
  if (m_seeking) progress = m_seekPos;

  ImDrawList* dl = ImGui::GetWindowDrawList();
  
  // Background track (unwatched - dark gray)
  dl->AddRectFilled(
    ImVec2(barX, barY),
    ImVec2(barX + barWidth, barY + barHeight),
    IM_COL32(60, 60, 70, (int)(200 * m_controlsAlpha)),
    3.0f
  );
  
  // Watched portion (purple)
  float watchedWidth = barWidth * progress;
  if (watchedWidth > 0) {
    dl->AddRectFilled(
      ImVec2(barX, barY),
      ImVec2(barX + watchedWidth, barY + barHeight),
      IM_COL32(180, 100, 255, (int)(255 * m_controlsAlpha)),
      3.0f
    );
  }
  
  // Scrubber handle (circle)
  float handleX = barX + watchedWidth;
  float handleY = barY + barHeight / 2;
  float handleRadius = 8;
  
  // Invisible button for interaction
  ImGui::SetCursorScreenPos(ImVec2(barX - 5, barY - 10));
  ImGui::InvisibleButton("##progressBar", ImVec2(barWidth + 10, barHeight + 20));
  
  bool hovered = ImGui::IsItemHovered();
  bool active = ImGui::IsItemActive();
  
  if (hovered || active) {
    handleRadius = 10;
    
    ImVec2 mousePos = ImGui::GetMousePos();
    float hoverProgress = std::clamp((mousePos.x - barX) / barWidth, 0.0f, 1.0f);
    
    if (active) {
      m_seeking = true;
      m_seekPos = hoverProgress;
    }
    
    // Tooltip with time
    double hoverTime = hoverProgress * duration;
    int hrs = (int)(hoverTime / 3600);
    int min = (int)((hoverTime - hrs * 3600) / 60);
    int sec = (int)(hoverTime - hrs * 3600 - min * 60);
    std::string timeStr = hrs > 0 ? fmt::format("{:02d}:{:02d}:{:02d}", hrs, min, sec)
                                  : fmt::format("{:02d}:{:02d}", min, sec);
    ImGui::SetTooltip("%s", timeStr.c_str());
  }
  
  if (m_seeking && !active) {
    double seekTime = m_seekPos * duration;
    mpv->commandv("seek", std::to_string(seekTime).c_str(), "absolute", nullptr);
    m_seeking = false;
  }
  
  // Draw handle
  dl->AddCircleFilled(
    ImVec2(handleX, handleY),
    handleRadius,
    IM_COL32(255, 255, 255, (int)(255 * m_controlsAlpha)),
    32
  );
  
  // Handle glow when hovered
  if (hovered || active) {
    dl->AddCircle(
      ImVec2(handleX, handleY),
      handleRadius + 3,
      IM_COL32(180, 100, 255, (int)(100 * m_controlsAlpha)),
      32, 2.0f
    );
  }
}

void PlayerOverlay::drawControlButtons() {
  auto vp = ImGui::GetMainViewport();
  float ww = vp->WorkSize.x;
  float controlY = ImGui::GetCursorPosY();
  float btnSize = 40.0f;
  float playBtnSize = 48.0f;

  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.12f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.78f, 0.49f, 1.0f, 0.3f));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 24);
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 0));

  // ALL BUTTONS ON SAME Y LEVEL
  float centerY = controlY + (playBtnSize - btnSize) / 2;

  // Play/Pause - purple, slightly larger
  bool paused = mpv->pause;
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.616f, 0.306f, 0.867f, 0.95f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.50f, 0.95f, 1.0f));
  ImGui::SetCursorPosY(controlY);
  if (ImGui::Button(paused ? ICON_FA_PLAY : ICON_FA_PAUSE, ImVec2(playBtnSize, playBtnSize))) {
    mpv->command("cycle pause");
  }
  ImGui::PopStyleColor(2);

  // Skip backward - same Y as other buttons
  ImGui::SameLine();
  ImGui::SetCursorPosY(centerY);
  if (ImGui::Button(ICON_FA_BACKWARD, ImVec2(btnSize, btnSize))) mpv->command("seek -10");
  
  // Skip forward
  ImGui::SameLine();
  ImGui::SetCursorPosY(centerY);
  if (ImGui::Button(ICON_FA_FORWARD, ImVec2(btnSize, btnSize))) mpv->command("seek 10");

  // Volume section
  ImGui::SameLine();
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
  ImGui::SetCursorPosY(centerY);
  
  bool muted = mpv->mute;
  int vol = (int)mpv->volume;
  const char* volIcon = muted ? ICON_FA_VOLUME_MUTE :
                        (vol > 60 ? ICON_FA_VOLUME_UP : 
                         vol > 20 ? ICON_FA_VOLUME_DOWN : ICON_FA_VOLUME_OFF);
  if (ImGui::Button(volIcon, ImVec2(btnSize, btnSize))) mpv->command("cycle mute");
  
  ImGui::SameLine();
  ImGui::SetCursorPosY(centerY + (btnSize - 16) / 2);
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1, 1, 1, 0.1f));
  ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1, 1, 1, 0.9f));
  ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.78f, 0.49f, 1.0f, 1.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3);
  ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 8);
  ImGui::SetNextItemWidth(70);
  if (ImGui::SliderInt("##vol", &vol, 0, 100, "")) {
    mpv->commandv("set", "volume", std::to_string(vol).c_str(), nullptr);
  }
  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor(3);

  // Time display - same Y level
  ImGui::SameLine();
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 12);
  ImGui::SetCursorPosY(centerY + (btnSize - ImGui::GetTextLineHeight()) / 2);
  
  double dur = mpv->property<double, MPV_FORMAT_DOUBLE>("duration");
  double pos = (double)mpv->timePos;

  auto fmtTime = [](double t) -> std::string {
    if (t < 0) t = 0;
    int h = (int)(t / 3600);
    int m = (int)((t - h * 3600) / 60);
    int s = (int)(t - h * 3600 - m * 60);
    return h > 0 ? fmt::format("{:02d}:{:02d}:{:02d}", h, m, s)
                 : fmt::format("{:02d}:{:02d}", m, s);
  };

  ImGui::TextColored(ImVec4(1, 1, 1, 0.9f), "%s", fmtTime(pos).c_str());
  ImGui::SameLine(0, 4);
  ImGui::TextColored(ImVec4(1, 1, 1, 0.4f), "/");
  ImGui::SameLine(0, 4);
  ImGui::TextColored(ImVec4(1, 1, 1, 0.55f), "%s", fmtTime(dur).c_str());

  // Right side controls - same Y level
  float rightX = ww - 200;
  ImGui::SameLine();
  ImGui::SetCursorPosX(rightX);
  ImGui::SetCursorPosY(centerY);

  if (ImGui::Button(ICON_FA_CLOSED_CAPTIONING, ImVec2(btnSize, btnSize))) {
    m_showSubtitleMenu = !m_showSubtitleMenu;
    m_showAudioMenu = false;
    m_showSettingsMenu = false;
  }
  
  ImGui::SameLine();
  ImGui::SetCursorPosY(centerY);
  if (ImGui::Button(ICON_FA_HEADPHONES, ImVec2(btnSize, btnSize))) {
    m_showAudioMenu = !m_showAudioMenu;
    m_showSubtitleMenu = false;
    m_showSettingsMenu = false;
  }
  
  ImGui::SameLine();
  ImGui::SetCursorPosY(centerY);
  if (ImGui::Button(ICON_FA_COG, ImVec2(btnSize, btnSize))) {
    m_showSettingsMenu = !m_showSettingsMenu;
    m_showSubtitleMenu = false;
    m_showAudioMenu = false;
  }
  
  ImGui::SameLine();
  ImGui::SetCursorPosY(centerY);
  bool fs = mpv->fullscreen;
  if (ImGui::Button(fs ? ICON_FA_COMPRESS : ICON_FA_EXPAND, ImVec2(btnSize, btnSize))) {
    mpv->command("cycle fullscreen");
  }

  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor(3);
}


void PlayerOverlay::drawSubtitleMenu() {
  auto vp = ImGui::GetMainViewport();
  ImVec2 wPos = vp->WorkPos;
  ImVec2 wSize = vp->WorkSize;

  float mw = 280, mh = 320;
  ImGui::SetNextWindowPos(ImVec2(wPos.x + wSize.x - mw - 24, wPos.y + wSize.y - mh - 120));
  ImGui::SetNextWindowSize(ImVec2(mw, mh));

  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.02f, 0.10f, 0.96f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.78f, 0.49f, 1.0f, 0.25f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 14));

  if (ImGui::Begin("##SubMenu", &m_showSubtitleMenu, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {
    ImGui::TextColored(ImVec4(0.78f, 0.49f, 1.0f, 1.0f), ICON_FA_CLOSED_CAPTIONING "  Subtitles");
    ImGui::SameLine(mw - 44);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    if (ImGui::Button(ICON_FA_TIMES "##cs", ImVec2(24, 24))) m_showSubtitleMenu = false;
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::BeginChild("##SubList", ImVec2(mw - 32, 140), false);
    std::string sid = mpv->sid;
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6);
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.78f, 0.49f, 1.0f, 0.25f));

    bool hasSubs = false;
    for (auto &t : mpv->tracks) {
      if (t.type != "sub") continue;
      hasSubs = true;
      std::string name = t.title.empty() ? fmt::format("Track {}", t.id) : t.title;
      if (!t.lang.empty()) name += fmt::format(" [{}]", t.lang);
      if (ImGui::Selectable(name.c_str(), sid == std::to_string(t.id), 0, ImVec2(0, 28))) {
        mpv->property<int64_t, MPV_FORMAT_INT64>("sid", t.id);
      }
    }
    if (ImGui::Selectable("Disable", sid == "no", 0, ImVec2(0, 28))) {
      mpv->commandv("set", "sid", "no", nullptr);
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    if (!hasSubs) ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No subtitles");
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.616f, 0.306f, 0.867f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.50f, 0.95f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6);
    if (ImGui::Button(ICON_FA_UPLOAD "  Load", ImVec2(mw - 32, 32))) openSubtitleFile();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
  }
  ImGui::End();
  ImGui::PopStyleVar(3);
  ImGui::PopStyleColor(2);
}

void PlayerOverlay::drawAudioMenu() {
  auto vp = ImGui::GetMainViewport();
  ImVec2 wPos = vp->WorkPos;
  ImVec2 wSize = vp->WorkSize;

  float mw = 280, mh = 240;
  ImGui::SetNextWindowPos(ImVec2(wPos.x + wSize.x - mw - 24, wPos.y + wSize.y - mh - 120));
  ImGui::SetNextWindowSize(ImVec2(mw, mh));

  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.02f, 0.10f, 0.96f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.78f, 0.49f, 1.0f, 0.25f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 14));

  if (ImGui::Begin("##AudioMenu", &m_showAudioMenu, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {
    ImGui::TextColored(ImVec4(0.78f, 0.49f, 1.0f, 1.0f), ICON_FA_HEADPHONES "  Audio");
    ImGui::SameLine(mw - 44);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    if (ImGui::Button(ICON_FA_TIMES "##ca", ImVec2(24, 24))) m_showAudioMenu = false;
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::BeginChild("##AudioList", ImVec2(mw - 32, mh - 70), false);
    std::string aid = mpv->aid;
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6);
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.78f, 0.49f, 1.0f, 0.25f));

    bool hasAudio = false;
    for (auto &t : mpv->tracks) {
      if (t.type != "audio") continue;
      hasAudio = true;
      std::string name = t.title.empty() ? fmt::format("Track {}", t.id) : t.title;
      if (!t.lang.empty()) name += fmt::format(" [{}]", t.lang);
      if (ImGui::Selectable(name.c_str(), aid == std::to_string(t.id), 0, ImVec2(0, 28))) {
        mpv->property<int64_t, MPV_FORMAT_INT64>("aid", t.id);
      }
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    if (!hasAudio) ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No audio tracks");
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

  float mw = 280, mh = 220;
  ImGui::SetNextWindowPos(ImVec2(wPos.x + wSize.x - mw - 24, wPos.y + wSize.y - mh - 120));
  ImGui::SetNextWindowSize(ImVec2(mw, mh));

  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.02f, 0.10f, 0.96f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.78f, 0.49f, 1.0f, 0.25f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 14));

  if (ImGui::Begin("##SetMenu", &m_showSettingsMenu, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {
    ImGui::TextColored(ImVec4(0.78f, 0.49f, 1.0f, 1.0f), ICON_FA_COG "  Playback");
    ImGui::SameLine(mw - 44);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    if (ImGui::Button(ICON_FA_TIMES "##cset", ImVec2(24, 24))) m_showSettingsMenu = false;
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Speed");
    ImGui::SameLine(70);
    float spd = (float)mpv->property<double, MPV_FORMAT_DOUBLE>("speed");
    ImGui::SetNextItemWidth(mw - 100);
    if (ImGui::SliderFloat("##spd", &spd, 0.25f, 4.0f, "%.2fx")) {
      mpv->commandv("set", "speed", fmt::format("{:.2f}", spd).c_str(), nullptr);
    }

    ImGui::Spacing();
    ImGui::Text("Loop");
    ImGui::SameLine(70);
    std::string lf = mpv->property("loop-file");
    bool loop = lf == "inf";
    if (ImGui::Checkbox("##loop", &loop)) {
      mpv->commandv("set", "loop-file", loop ? "inf" : "no", nullptr);
    }
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
  ImGui::OpenPopup("Open URL##PTP");

  ImVec2 ws = ImGui::GetMainViewport()->WorkSize;
  ImGui::SetNextWindowSize(ImVec2(std::min(ws.x * 0.4f, 380.0f), 0), ImGuiCond_Always);
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

  ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.05f, 0.02f, 0.10f, 0.98f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.78f, 0.49f, 1.0f, 0.3f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 16));
  ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1);

  if (ImGui::BeginPopupModal("Open URL##PTP", &m_showURLDialog, ImGuiWindowFlags_NoResize)) {
    ImGui::TextColored(ImVec4(0.78f, 0.49f, 1.0f, 1.0f), ICON_FA_LINK "  Stream URL");
    ImGui::Spacing();

    static char url[1024] = {0};

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.08f, 0.04f, 0.15f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 8));
    ImGui::SetNextItemWidth(-1);
    bool enter = ImGui::InputTextWithHint("##url", "https://...", url, IM_ARRAYSIZE(url), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();

    ImGui::Spacing();

    float bw = 90;
    ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - bw * 2 - 8);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.08f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.14f, 0.32f, 1.0f));
    if (ImGui::Button("Cancel", ImVec2(bw, 32))) {
      m_showURLDialog = false;
      url[0] = '\0';
      ImGui::CloseCurrentPopup();
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine(0, 8);
    bool ok = url[0] != '\0';
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.616f, 0.306f, 0.867f, ok ? 1.0f : 0.4f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.50f, 0.95f, 1.0f));
    if (!ok) ImGui::BeginDisabled();
    if (ImGui::Button(ICON_FA_PLAY " Play", ImVec2(bw, 32)) || (enter && ok)) {
      mpv->commandv("loadfile", url, nullptr);
      m_showURLDialog = false;
      url[0] = '\0';
      ImGui::CloseCurrentPopup();
    }
    if (!ok) ImGui::EndDisabled();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();

    ImGui::EndPopup();
  }
  ImGui::PopStyleVar(3);
  ImGui::PopStyleColor(2);
}

}  // namespace ImPlay::Views
