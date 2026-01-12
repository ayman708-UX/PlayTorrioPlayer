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

  if (m_showURLDialog) openURL();

  // Efficient activity detection
  ImGuiIO& io = ImGui::GetIO();
  double now = ImGui::GetTime();
  
  // Only check mouse movement if not in a menu
  bool inMenu = m_showSubtitleMenu || m_showAudioMenu || m_showSettingsMenu;
  bool mouseActive = !inMenu && (io.MouseDelta.x != 0 || io.MouseDelta.y != 0);
  bool hasActivity = mouseActive || io.MouseDown[0] || io.MouseDown[1] || inMenu;
  
  if (hasActivity) {
    m_lastActivityTime = now;
    m_targetAlpha = 1.0f;
  } else if (now - m_lastActivityTime > 2.5) {
    m_targetAlpha = 0.0f;
  }

  // Simple alpha lerp
  if (m_controlsAlpha != m_targetAlpha) {
    m_controlsAlpha += (m_targetAlpha - m_controlsAlpha) * 0.2f;
    if (std::abs(m_targetAlpha - m_controlsAlpha) < 0.01f) 
      m_controlsAlpha = m_targetAlpha;
  }

  // Skip drawing if fully hidden
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
    dl->AddCircleFilled(center, 60, IM_COL32(157, 78, 221, 20), 48);
    dl->AddCircle(center, 55, IM_COL32(199, 125, 255, 180), 48, 2.5f);
    
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
    
    ImGui::PopStyleVar();

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

  float h = 60;
  ImGui::SetNextWindowPos(wPos);
  ImGui::SetNextWindowSize(ImVec2(wSize.x, h));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
                           ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

  ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_controlsAlpha);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

  if (ImGui::Begin("##TopBar", nullptr, flags)) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 top = IM_COL32(0, 0, 0, (int)(180 * m_controlsAlpha));
    ImU32 bot = IM_COL32(0, 0, 0, 0);
    dl->AddRectFilledMultiColor(wPos, ImVec2(wPos.x + wSize.x, wPos.y + h), top, top, bot, bot);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 20);

    ImGui::SetCursorPos(ImVec2(14, 10));
    if (ImGui::Button(ICON_FA_ARROW_LEFT, ImVec2(40, 40))) mpv->command("quit");

    ImGui::SameLine();
    ImGui::SetCursorPosY(18);
    
    std::string title = mpv->property("media-title");
    if (title.empty()) title = "PlayTorrioPlayer";
    float maxW = wSize.x - 80;
    ImVec2 ts = ImGui::CalcTextSize(title.c_str());
    if (ts.x > maxW) {
      size_t len = (size_t)(title.length() * maxW / ts.x);
      if (len > 3) title = title.substr(0, len - 3) + "...";
    }
    ImGui::TextColored(ImVec4(1, 1, 1, 0.9f), "%s", title.c_str());

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
  }
  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar(2);
}

void PlayerOverlay::drawBottomBar() {
  auto vp = ImGui::GetMainViewport();
  ImVec2 wPos = vp->WorkPos;
  ImVec2 wSize = vp->WorkSize;

  float h = 95;
  ImGui::SetNextWindowPos(ImVec2(wPos.x, wPos.y + wSize.y - h));
  ImGui::SetNextWindowSize(ImVec2(wSize.x, h));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
                           ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

  ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_controlsAlpha);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

  if (ImGui::Begin("##BottomBar", nullptr, flags)) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 barPos = ImVec2(wPos.x, wPos.y + wSize.y - h);
    ImU32 top = IM_COL32(0, 0, 0, 0);
    ImU32 bot = IM_COL32(0, 0, 0, (int)(200 * m_controlsAlpha));
    dl->AddRectFilledMultiColor(barPos, ImVec2(barPos.x + wSize.x, barPos.y + h), top, top, bot, bot);

    ImGui::SetCursorPos(ImVec2(20, 12));
    drawProgressBar();

    ImGui::SetCursorPos(ImVec2(20, 48));
    drawControlButtons();
  }
  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar(2);
}

void PlayerOverlay::drawProgressBar() {
  auto vp = ImGui::GetMainViewport();
  ImVec2 wPos = vp->WorkPos;
  float barWidth = vp->WorkSize.x - 40;
  float barHeight = 5;
  float barY = ImGui::GetCursorScreenPos().y;
  float barX = wPos.x + 20;

  double duration = mpv->property<double, MPV_FORMAT_DOUBLE>("duration");
  double position = (double)mpv->timePos;
  float progress = duration > 0 ? (float)(position / duration) : 0.0f;
  if (m_seeking) progress = m_seekPos;

  ImDrawList* dl = ImGui::GetWindowDrawList();
  
  // Background (unwatched)
  dl->AddRectFilled(ImVec2(barX, barY), ImVec2(barX + barWidth, barY + barHeight),
                    IM_COL32(80, 80, 90, (int)(180 * m_controlsAlpha)), 2.5f);
  
  // Watched (purple)
  float watchedW = barWidth * progress;
  if (watchedW > 0) {
    dl->AddRectFilled(ImVec2(barX, barY), ImVec2(barX + watchedW, barY + barHeight),
                      IM_COL32(180, 100, 255, (int)(255 * m_controlsAlpha)), 2.5f);
  }
  
  // Handle
  float hx = barX + watchedW;
  float hy = barY + barHeight / 2;
  
  ImGui::SetCursorScreenPos(ImVec2(barX - 8, barY - 12));
  ImGui::InvisibleButton("##prog", ImVec2(barWidth + 16, barHeight + 24));
  
  bool hov = ImGui::IsItemHovered();
  bool act = ImGui::IsItemActive();
  float hr = (hov || act) ? 9.0f : 7.0f;
  
  if (hov || act) {
    ImVec2 mp = ImGui::GetMousePos();
    float hp = std::clamp((mp.x - barX) / barWidth, 0.0f, 1.0f);
    if (act) { m_seeking = true; m_seekPos = hp; }
    
    double ht = hp * duration;
    int hh = (int)(ht / 3600), hm = (int)((ht - hh * 3600) / 60), hs = (int)(ht - hh * 3600 - hm * 60);
    ImGui::SetTooltip("%s", (hh > 0 ? fmt::format("{:02d}:{:02d}:{:02d}", hh, hm, hs) : fmt::format("{:02d}:{:02d}", hm, hs)).c_str());
  }
  
  if (m_seeking && !act) {
    mpv->commandv("seek", std::to_string(m_seekPos * duration).c_str(), "absolute", nullptr);
    m_seeking = false;
  }
  
  dl->AddCircleFilled(ImVec2(hx, hy), hr, IM_COL32(255, 255, 255, (int)(255 * m_controlsAlpha)), 24);
  if (hov || act) {
    dl->AddCircle(ImVec2(hx, hy), hr + 3, IM_COL32(180, 100, 255, (int)(80 * m_controlsAlpha)), 24, 2.0f);
  }
}

void PlayerOverlay::drawControlButtons() {
  auto vp = ImGui::GetMainViewport();
  float ww = vp->WorkSize.x;
  
  // BIGGER button sizes
  float bs = 44.0f;
  float pbs = 52.0f;  // play button
  float y = ImGui::GetCursorPosY();
  float centerY = y;  // All buttons same Y

  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.4f, 1.0f, 0.25f));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 26);
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 0));

  // Play button - purple, bigger
  bool paused = mpv->pause;
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.616f, 0.306f, 0.867f, 0.9f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.72f, 0.45f, 0.92f, 1.0f));
  ImGui::SetCursorPosY(centerY);
  ImGui::SetWindowFontScale(1.3f);
  if (ImGui::Button(paused ? ICON_FA_PLAY : ICON_FA_PAUSE, ImVec2(pbs, pbs))) mpv->command("cycle pause");
  ImGui::SetWindowFontScale(1.0f);
  ImGui::PopStyleColor(2);

  ImGui::SameLine();
  ImGui::SetCursorPosY(centerY + (pbs - bs) / 2);
  ImGui::SetWindowFontScale(1.2f);
  if (ImGui::Button(ICON_FA_BACKWARD, ImVec2(bs, bs))) mpv->command("seek -10");
  ImGui::SameLine();
  ImGui::SetCursorPosY(centerY + (pbs - bs) / 2);
  if (ImGui::Button(ICON_FA_FORWARD, ImVec2(bs, bs))) mpv->command("seek 10");
  ImGui::SetWindowFontScale(1.0f);

  // Volume
  ImGui::SameLine();
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6);
  ImGui::SetCursorPosY(centerY + (pbs - bs) / 2);
  
  bool muted = mpv->mute;
  int vol = (int)mpv->volume;
  const char* vi = muted ? ICON_FA_VOLUME_MUTE : (vol > 60 ? ICON_FA_VOLUME_UP : vol > 20 ? ICON_FA_VOLUME_DOWN : ICON_FA_VOLUME_OFF);
  ImGui::SetWindowFontScale(1.2f);
  if (ImGui::Button(vi, ImVec2(bs, bs))) mpv->command("cycle mute");
  ImGui::SetWindowFontScale(1.0f);
  
  ImGui::SameLine();
  ImGui::SetCursorPosY(centerY + (pbs - 16) / 2);
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1, 1, 1, 0.08f));
  ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1, 1, 1, 0.85f));
  ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.7f, 0.4f, 1.0f, 1.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3);
  ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 8);
  ImGui::SetNextItemWidth(65);
  if (ImGui::SliderInt("##vol", &vol, 0, 100, "")) {
    mpv->commandv("set", "volume", std::to_string(vol).c_str(), nullptr);
  }
  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor(3);

  // Time - ALL ON SAME LINE, SAME Y
  double dur = mpv->property<double, MPV_FORMAT_DOUBLE>("duration");
  double pos = (double)mpv->timePos;
  auto fmt = [](double t) -> std::string {
    if (t < 0) t = 0;
    int h = (int)(t / 3600), m = (int)((t - h * 3600) / 60), s = (int)(t - h * 3600 - m * 60);
    return h > 0 ? fmt::format("{:02d}:{:02d}:{:02d}", h, m, s) : fmt::format("{:02d}:{:02d}", m, s);
  };

  std::string timeStr = fmt(pos) + " / " + fmt(dur);
  ImGui::SameLine();
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
  ImGui::SetCursorPosY(centerY + (pbs - ImGui::GetTextLineHeight()) / 2);
  ImGui::TextColored(ImVec4(1, 1, 1, 0.85f), "%s", timeStr.c_str());

  // Right side - same Y
  float rx = ww - 195;
  ImGui::SameLine();
  ImGui::SetCursorPosX(rx);
  ImGui::SetCursorPosY(centerY + (pbs - bs) / 2);

  ImGui::SetWindowFontScale(1.2f);
  if (ImGui::Button(ICON_FA_CLOSED_CAPTIONING, ImVec2(bs, bs))) {
    m_showSubtitleMenu = !m_showSubtitleMenu; m_showAudioMenu = false; m_showSettingsMenu = false;
  }
  ImGui::SameLine();
  ImGui::SetCursorPosY(centerY + (pbs - bs) / 2);
  if (ImGui::Button(ICON_FA_HEADPHONES, ImVec2(bs, bs))) {
    m_showAudioMenu = !m_showAudioMenu; m_showSubtitleMenu = false; m_showSettingsMenu = false;
  }
  ImGui::SameLine();
  ImGui::SetCursorPosY(centerY + (pbs - bs) / 2);
  if (ImGui::Button(ICON_FA_COG, ImVec2(bs, bs))) {
    m_showSettingsMenu = !m_showSettingsMenu; m_showSubtitleMenu = false; m_showAudioMenu = false;
  }
  ImGui::SameLine();
  ImGui::SetCursorPosY(centerY + (pbs - bs) / 2);
  if (ImGui::Button(mpv->fullscreen ? ICON_FA_COMPRESS : ICON_FA_EXPAND, ImVec2(bs, bs))) {
    mpv->command("cycle fullscreen");
  }
  ImGui::SetWindowFontScale(1.0f);

  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor(3);
}


void PlayerOverlay::drawSubtitleMenu() {
  auto vp = ImGui::GetMainViewport();
  float mw = 320, mh = 380;
  ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + vp->WorkSize.x - mw - 20, vp->WorkPos.y + vp->WorkSize.y - mh - 110));
  ImGui::SetNextWindowSize(ImVec2(mw, mh));

  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.02f, 0.08f, 0.95f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.6f, 0.35f, 0.85f, 0.2f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 12));

  if (ImGui::Begin("##Sub", &m_showSubtitleMenu, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {
    ImGui::TextColored(ImVec4(0.75f, 0.45f, 1.0f, 1.0f), ICON_FA_CLOSED_CAPTIONING "  Subtitles");
    ImGui::SameLine(mw - 40);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    if (ImGui::Button(ICON_FA_TIMES "##cs", ImVec2(22, 22))) m_showSubtitleMenu = false;
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    // Provider tabs
    int numTabs = 1 + (int)m_externalProviders.size();
    float tabWidth = (mw - 28) / std::min(numTabs, 4);  // Max 4 tabs visible
    
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
    
    // Built-in tab
    bool isBuiltIn = (m_selectedProviderTab == 0);
    ImGui::PushStyleColor(ImGuiCol_Button, isBuiltIn ? ImVec4(0.5f, 0.25f, 0.75f, 0.9f) : ImVec4(0.15f, 0.08f, 0.25f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.35f, 0.85f, 1.0f));
    if (ImGui::Button("Built-in", ImVec2(tabWidth - 2, 26))) m_selectedProviderTab = 0;
    ImGui::PopStyleColor(2);
    
    // External provider tabs
    for (int i = 0; i < (int)m_externalProviders.size(); i++) {
      ImGui::SameLine();
      bool isSelected = (m_selectedProviderTab == i + 1);
      ImGui::PushStyleColor(ImGuiCol_Button, isSelected ? ImVec4(0.5f, 0.25f, 0.75f, 0.9f) : ImVec4(0.15f, 0.08f, 0.25f, 0.8f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.35f, 0.85f, 1.0f));
      
      std::string tabName = m_externalProviders[i].name;
      if (tabName.length() > 10) tabName = tabName.substr(0, 9) + "..";
      if (ImGui::Button((tabName + "##tab" + std::to_string(i)).c_str(), ImVec2(tabWidth - 2, 26))) 
        m_selectedProviderTab = i + 1;
      ImGui::PopStyleColor(2);
    }
    
    ImGui::PopStyleVar(2);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Content based on selected tab
    float listHeight = mh - 180;
    
    if (m_selectedProviderTab == 0) {
      // Built-in subtitles (from the video file)
      ImGui::TextColored(ImVec4(0.6f, 0.5f, 0.7f, 0.9f), "Embedded Tracks");
      ImGui::BeginChild("##SL", ImVec2(mw - 28, listHeight), false);
      std::string sid = mpv->sid;
      ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.6f, 0.35f, 0.85f, 0.2f));
      
      bool hasBuiltIn = false;
      for (auto &t : mpv->tracks) {
        if (t.type != "sub") continue;
        hasBuiltIn = true;
        std::string n = t.title.empty() ? fmt::format("Track {}", t.id) : t.title;
        if (!t.lang.empty()) n += " [" + t.lang + "]";
        if (ImGui::Selectable(n.c_str(), sid == std::to_string(t.id), 0, ImVec2(0, 24)))
          mpv->property<int64_t, MPV_FORMAT_INT64>("sid", t.id);
      }
      
      if (!hasBuiltIn) {
        ImGui::TextColored(ImVec4(0.5f, 0.4f, 0.6f, 0.7f), "No embedded subtitles");
      }
      
      if (ImGui::Selectable("Disable", sid == "no", 0, ImVec2(0, 24)))
        mpv->commandv("set", "sid", "no", nullptr);
      ImGui::PopStyleColor();
      ImGui::EndChild();
    } else {
      // External provider subtitles
      int provIdx = m_selectedProviderTab - 1;
      if (provIdx >= 0 && provIdx < (int)m_externalProviders.size()) {
        auto& provider = m_externalProviders[provIdx];
        ImGui::TextColored(ImVec4(0.6f, 0.5f, 0.7f, 0.9f), "%s (%d)", provider.name.c_str(), (int)provider.subtitles.size());
        
        ImGui::BeginChild("##ExtSL", ImVec2(mw - 28, listHeight), false);
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.6f, 0.35f, 0.85f, 0.2f));
        
        for (int i = 0; i < (int)provider.subtitles.size(); i++) {
          auto& sub = provider.subtitles[i];
          if (ImGui::Selectable((sub.name + "##ext" + std::to_string(i)).c_str(), false, 0, ImVec2(0, 24))) {
            // Load this subtitle URL
            mpv->commandv("sub-add", sub.url.c_str(), "select", nullptr);
          }
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Click to load: %s", sub.url.c_str());
          }
        }
        
        if (provider.subtitles.empty()) {
          ImGui::TextColored(ImVec4(0.5f, 0.4f, 0.6f, 0.7f), "No subtitles from this provider");
        }
        
        ImGui::PopStyleColor();
        ImGui::EndChild();
      }
    }

    ImGui::Separator();
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.25f, 0.75f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.65f, 0.4f, 0.9f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5);
    if (ImGui::Button(ICON_FA_UPLOAD " Load File", ImVec2(mw - 28, 28))) openSubtitleFile();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
  }
  ImGui::End();
  ImGui::PopStyleVar(3);
  ImGui::PopStyleColor(2);
}

void PlayerOverlay::drawAudioMenu() {
  auto vp = ImGui::GetMainViewport();
  float mw = 280, mh = 220;
  ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + vp->WorkSize.x - mw - 20, vp->WorkPos.y + vp->WorkSize.y - mh - 110));
  ImGui::SetNextWindowSize(ImVec2(mw, mh));

  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.02f, 0.08f, 0.95f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.6f, 0.35f, 0.85f, 0.2f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 12));

  if (ImGui::Begin("##Aud", &m_showAudioMenu, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {
    ImGui::TextColored(ImVec4(0.75f, 0.45f, 1.0f, 1.0f), ICON_FA_HEADPHONES "  Audio");
    ImGui::SameLine(mw - 40);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    if (ImGui::Button(ICON_FA_TIMES "##ca", ImVec2(22, 22))) m_showAudioMenu = false;
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::BeginChild("##AL", ImVec2(mw - 28, mh - 60), false);
    std::string aid = mpv->aid;
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.6f, 0.35f, 0.85f, 0.2f));
    for (auto &t : mpv->tracks) {
      if (t.type != "audio") continue;
      std::string n = t.title.empty() ? fmt::format("Track {}", t.id) : t.title;
      if (!t.lang.empty()) n += " [" + t.lang + "]";
      if (ImGui::Selectable(n.c_str(), aid == std::to_string(t.id), 0, ImVec2(0, 24)))
        mpv->property<int64_t, MPV_FORMAT_INT64>("aid", t.id);
    }
    ImGui::PopStyleColor();
    ImGui::EndChild();
  }
  ImGui::End();
  ImGui::PopStyleVar(3);
  ImGui::PopStyleColor(2);
}

void PlayerOverlay::drawSettingsMenu() {
  auto vp = ImGui::GetMainViewport();
  float mw = 300, mh = 480;  // Taller for subtitle options
  ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + vp->WorkSize.x - mw - 20, vp->WorkPos.y + vp->WorkSize.y - mh - 110));
  ImGui::SetNextWindowSize(ImVec2(mw, mh));

  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.02f, 0.08f, 0.95f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.6f, 0.35f, 0.85f, 0.2f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 12));

  if (ImGui::Begin("##Set", &m_showSettingsMenu, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {
    ImGui::TextColored(ImVec4(0.75f, 0.45f, 1.0f, 1.0f), ICON_FA_COG "  Settings");
    ImGui::SameLine(mw - 40);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    if (ImGui::Button(ICON_FA_TIMES "##cset", ImVec2(22, 22))) m_showSettingsMenu = false;
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    // Speed
    ImGui::Text("Speed");
    ImGui::SameLine(90);
    float spd = (float)mpv->property<double, MPV_FORMAT_DOUBLE>("speed");
    ImGui::SetNextItemWidth(mw - 120);
    if (ImGui::SliderFloat("##spd", &spd, 0.25f, 4.0f, "%.2fx"))
      mpv->commandv("set", "speed", fmt::format("{:.2f}", spd).c_str(), nullptr);

    ImGui::Spacing();

    // Aspect Ratio
    ImGui::Text("Aspect");
    ImGui::SameLine(90);
    const char* aspects[] = {"Auto", "16:9", "4:3", "21:9", "1:1"};
    static int asp = 0;
    ImGui::SetNextItemWidth(mw - 120);
    if (ImGui::Combo("##asp", &asp, aspects, IM_ARRAYSIZE(aspects)))
      mpv->commandv("set", "video-aspect-override", asp == 0 ? "-1" : aspects[asp], nullptr);

    ImGui::Spacing();

    // Hardware Decoding
    ImGui::Text("HW Decode");
    ImGui::SameLine(90);
    std::string hwdec = mpv->property("hwdec");
    bool hwOn = hwdec != "no";
    if (ImGui::Checkbox("##hw", &hwOn))
      mpv->commandv("set", "hwdec", hwOn ? "auto" : "no", nullptr);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), hwOn ? "(faster)" : "(software)");

    ImGui::Spacing();

    // Loop
    ImGui::Text("Loop");
    ImGui::SameLine(90);
    std::string lf = mpv->property("loop-file");
    bool loop = lf == "inf";
    if (ImGui::Checkbox("##loop", &loop))
      mpv->commandv("set", "loop-file", loop ? "inf" : "no", nullptr);

    ImGui::Spacing();

    // Cache/Buffer for streaming
    ImGui::Text("Cache");
    ImGui::SameLine(90);
    static int cache = 150;  // MB
    ImGui::SetNextItemWidth(mw - 120);
    if (ImGui::SliderInt("##cache", &cache, 16, 512, "%d MB"))
      mpv->commandv("set", "demuxer-max-bytes", fmt::format("{}MiB", cache).c_str(), nullptr);

    ImGui::Spacing();

    // Deinterlace
    ImGui::Text("Deinterlace");
    ImGui::SameLine(90);
    std::string deint = mpv->property("deinterlace");
    bool deintOn = deint == "yes";
    if (ImGui::Checkbox("##deint", &deintOn))
      mpv->commandv("set", "deinterlace", deintOn ? "yes" : "no", nullptr);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Subtitle section
    ImGui::TextColored(ImVec4(0.75f, 0.45f, 1.0f, 1.0f), ICON_FA_CLOSED_CAPTIONING "  Subtitles");
    ImGui::Spacing();

    // Subtitle Size
    ImGui::Text("Sub Size");
    ImGui::SameLine(90);
    static int subSize = 55;  // Default mpv sub-font-size
    ImGui::SetNextItemWidth(mw - 120);
    if (ImGui::SliderInt("##subsize", &subSize, 20, 100, "%d"))
      mpv->commandv("set", "sub-font-size", std::to_string(subSize).c_str(), nullptr);

    ImGui::Spacing();

    // Subtitle Position (vertical)
    ImGui::Text("Sub Pos");
    ImGui::SameLine(90);
    static int subPos = 100;  // 100 = bottom, 0 = top
    ImGui::SetNextItemWidth(mw - 120);
    if (ImGui::SliderInt("##subpos", &subPos, 0, 150, "%d%%"))
      mpv->commandv("set", "sub-pos", std::to_string(subPos).c_str(), nullptr);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("100 = bottom, lower = higher on screen");

    ImGui::Spacing();

    // Subtitle Scale
    ImGui::Text("Sub Scale");
    ImGui::SameLine(90);
    static float subScale = 1.0f;
    ImGui::SetNextItemWidth(mw - 120);
    if (ImGui::SliderFloat("##subscale", &subScale, 0.5f, 2.0f, "%.1fx"))
      mpv->commandv("set", "sub-scale", fmt::format("{:.2f}", subScale).c_str(), nullptr);

    ImGui::Spacing();

    // Subtitle Delay
    ImGui::Text("Sub Delay");
    ImGui::SameLine(90);
    double subDelay = mpv->property<double, MPV_FORMAT_DOUBLE>("sub-delay");
    float subDelayF = (float)subDelay;
    ImGui::SetNextItemWidth(mw - 120);
    if (ImGui::SliderFloat("##subdelay", &subDelayF, -5.0f, 5.0f, "%.1fs"))
      mpv->commandv("set", "sub-delay", fmt::format("{:.2f}", subDelayF).c_str(), nullptr);
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
  ImGui::OpenPopup("Open URL##PTP");
  ImVec2 ws = ImGui::GetMainViewport()->WorkSize;
  ImGui::SetNextWindowSize(ImVec2(std::min(ws.x * 0.4f, 360.0f), 0), ImGuiCond_Always);
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

  ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.04f, 0.02f, 0.08f, 0.97f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.6f, 0.35f, 0.85f, 0.25f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18, 14));
  ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1);

  if (ImGui::BeginPopupModal("Open URL##PTP", &m_showURLDialog, ImGuiWindowFlags_NoResize)) {
    ImGui::TextColored(ImVec4(0.75f, 0.45f, 1.0f, 1.0f), ICON_FA_LINK "  Stream URL");
    ImGui::Spacing();

    static char url[1024] = {0};
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.06f, 0.03f, 0.12f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5);
    ImGui::SetNextItemWidth(-1);
    bool enter = ImGui::InputTextWithHint("##url", "https://...", url, IM_ARRAYSIZE(url), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    float bw = 85;
    ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - bw * 2 - 6);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.06f, 0.18f, 1.0f));
    if (ImGui::Button("Cancel", ImVec2(bw, 30))) {
      m_showURLDialog = false; url[0] = '\0'; ImGui::CloseCurrentPopup();
    }
    ImGui::PopStyleColor();

    ImGui::SameLine(0, 6);
    bool ok = url[0] != '\0';
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.25f, 0.75f, ok ? 1.0f : 0.4f));
    if (!ok) ImGui::BeginDisabled();
    if (ImGui::Button(ICON_FA_PLAY " Play", ImVec2(bw, 30)) || (enter && ok)) {
      mpv->commandv("loadfile", url, nullptr);
      m_showURLDialog = false; url[0] = '\0'; ImGui::CloseCurrentPopup();
    }
    if (!ok) ImGui::EndDisabled();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::EndPopup();
  }
  ImGui::PopStyleVar(3);
  ImGui::PopStyleColor(2);
}

}  // namespace ImPlay::Views
