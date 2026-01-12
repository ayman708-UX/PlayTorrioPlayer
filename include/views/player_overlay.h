// Copyright (c) 2022-2025 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include "view.h"

namespace ImPlay::Views {
class PlayerOverlay : public View {
 public:
  PlayerOverlay(Config *config, Mpv *mpv);

  void draw() override;
  void show() override { m_visible = true; }
  void hide() { m_visible = false; }
  void toggle() { m_visible = !m_visible; }
  bool isVisible() const { return m_visible; }

  void setShowControls(bool show) { m_showControls = show; }
  bool getShowControls() const { return m_showControls; }

  // Draw idle screen when no media is playing
  void drawIdleScreen();

 private:
  void drawTopBar();
  void drawBottomBar();
  void drawProgressBar();
  void drawControlButtons();
  void drawVolumeControl();
  void drawSubtitleMenu();
  void drawAudioMenu();
  void drawSettingsMenu();

  void openSubtitleFile();
  void openMediaFile();
  void openURL();

  bool m_visible = true;
  bool m_showControls = true;
  float m_controlsAlpha = 1.0f;
  double m_lastActivityTime = 0;

  bool m_showSubtitleMenu = false;
  bool m_showAudioMenu = false;
  bool m_showSettingsMenu = false;
  bool m_showURLDialog = false;

  // Progress bar state
  bool m_seeking = false;
  float m_seekPos = 0.0f;

  // Colors matching PlayTorrio design
  ImVec4 m_primaryPurple = ImVec4(0.616f, 0.306f, 0.867f, 1.0f);   // #9d4edd
  ImVec4 m_darkPurple = ImVec4(0.353f, 0.094f, 0.604f, 1.0f);      // #5a189a
  ImVec4 m_accentPurple = ImVec4(0.780f, 0.490f, 1.0f, 1.0f);      // #c77dff
  ImVec4 m_bgDark = ImVec4(0.039f, 0.0f, 0.102f, 0.95f);           // #0a001a
};
}  // namespace ImPlay::Views
