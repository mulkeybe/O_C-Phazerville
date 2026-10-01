// Copyright (c) 2026, _________

//

// Permission is hereby granted, free of charge, to any person obtaining a copy

// of this software and associated documentation files (the "Software"), to deal

// in the Software without restriction, including without limitation the rights

// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell

// copies of the Software, and to permit persons to whom the Software is

// furnished to do so, subject to the following conditions:

//

// The above copyright notice and this permission notice shall be included in

// all copies or substantial portions of the Software.

//

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR

// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,

// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE

// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER

// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,

// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE

// SOFTWARE.


static const uint8_t frog_bitmap[9][12] = {
  {0,1,0,0,1,1,1,1,0,0,1,0},
  {1,1,0,1,0,1,1,0,1,0,1,1},
  {0,1,0,1,1,1,1,1,1,0,1,0},
  {0,0,1,1,1,1,1,1,1,1,0,0},
  {0,0,0,1,1,1,1,1,1,0,0,0},
  {0,0,1,1,1,1,1,1,1,1,0,0},
  {0,1,0,1,1,1,1,1,1,0,1,0},
  {1,1,0,1,1,1,1,1,1,0,1,1},
  {0,1,0,0,1,1,1,1,0,0,1,0}
};

class FrogSeq : public HemisphereApplet {

public:

  static constexpr int FROGSEQ_STEPS = 16;

  const char * applet_name() {
    return "FrogSeq";
  }

  const uint8_t* applet_icon() {
    return ZAP_ICON;
  }

  enum Page {
    MAIN_PAGE,
    NOTE_SEQ_PAGE
  };

  enum MainCursor {
    FROG_SELECT,
    SEMITONE_SELECT,
    RANDOM_SELECT,
    MAIN_CURSOR_LAST = RANDOM_SELECT
  };

  enum NoteSeqCursor {
    NOTE_STEP_FIRST = 0,
    NOTE_STEP_LAST = FROGSEQ_STEPS - 1,

    NOTE_STEPS = FROGSEQ_STEPS,

    LANE1_DIRECTION,
    LANE1_SPEED,
    LANE1_FLOW,

    LANE2_DIRECTION,
    LANE2_SPEED,
    LANE2_FLOW,

    LANE3_DIRECTION,
    LANE3_SPEED,
    LANE3_FLOW,

    NOTE_SEQ_CURSOR_LAST = LANE3_FLOW
  };

private:

  // --------------------------------------------------------------------------
  // State
  // --------------------------------------------------------------------------

  Page page = MAIN_PAGE;

  int cursor = FROG_SELECT;
  bool q_select = false;
  int qselect = 0;

  int8_t frog_x = 26;
  int8_t frog_y = 14;

  bool frog_horizontal = true;

  // Frog positions: Safe Zone, Lane 1, Lane 2, Lane 3.
  static constexpr int FROG_Y[4] = {13, 28, 41, 53};
  int frog_lane = 0;

  // --------------------------------------------------------------------------
  // Traffic
  // --------------------------------------------------------------------------

  static constexpr int TRAFFIC_LANES = 3;
  int sequence_length = 16;

  // Saved sequence memory.
  static constexpr int SAVED_SEQUENCES = 8;
  static constexpr int FROGSEQ_DATA_START = 0;
  static constexpr int FROGSEQ_DATA_SLOTS = SAVED_SEQUENCES * 2;


  uint8_t current_sequence = 0;
  bool sequence_menu = false;

  enum SequenceMenuMode {
    SEQUENCE_LOAD = 0,
    SEQUENCE_MOVE,
    SEQUENCE_RESET
  };

  uint8_t sequence_menu_mode = SEQUENCE_LOAD;
bool sequence_reset_confirm = false;
bool sequence_reset_yes = false;
  bool sequence_slot_mode = false;
  uint8_t selected_sequence = 0;

  static constexpr int TRAFFIC_OBJECTS = 3;
  static constexpr int TRAFFIC_WIDTH = 10;
  static constexpr int TRAFFIC_MOVE_PIXELS = 4;
  static constexpr int TRAFFIC_MIN_SPAWN_GAP =
    TRAFFIC_MOVE_PIXELS * 4;
  static constexpr int TRAFFIC_LEFT_BOUNDARY = -TRAFFIC_WIDTH;
  static constexpr int TRAFFIC_RIGHT_BOUNDARY = 64;

  enum TrafficRate {
    TRAFFIC_DIV_2,
    TRAFFIC_X1,
    TRAFFIC_X2,
    TRAFFIC_X3,
    TRAFFIC_X4
  };
  struct TrafficObject {
    int x;
    bool active;
  };

  TrafficObject traffic[TRAFFIC_LANES][TRAFFIC_OBJECTS];

  bool lane_reverse[TRAFFIC_LANES] = {
    false,
    true,
    false
  };

  TrafficRate lane_rate[TRAFFIC_LANES] = {
    TRAFFIC_X4,
    TRAFFIC_X1,
    TRAFFIC_DIV_2
  };  int lane_flow[TRAFFIC_LANES] = {
    5,
    5,
    5
  };

  uint32_t traffic_last_clock_tick = 0;
  uint32_t traffic_last_move_tick[TRAFFIC_LANES] = {0, 0, 0};
  uint32_t traffic_clock_ticks = 1;
  bool traffic_clock_valid = false;
  bool traffic_initialized = false;

  int8_t sequence_notes[FROGSEQ_STEPS];
  bool sequence_mutes[FROGSEQ_STEPS];
  bool sequence_bursts[FROGSEQ_STEPS];
  int step = 0;
  bool reset = true;

  int GetFrogNote(int step) {
    return sequence_notes[step];
  }

  void SetFrogNote(int note, int step) {
    sequence_notes[step] = constrain(note, -24, 35);
  }

  bool muted(int step) {
    return sequence_mutes[step];
  }

  void Unmute(int step) {
    sequence_mutes[step] = false;
  }

  void SetMute(int step, bool on) {
    sequence_mutes[step] = on;
  }

  void ToggleMute(int step) {
    sequence_mutes[step] = !sequence_mutes[step];
  }

  bool BurstEnabled(int step) {
    return sequence_bursts[step];
  }

  void SetBurst(int step, bool on = true) {
    sequence_bursts[step] = on;
  }

  void ToggleBurst(int step) {
    sequence_bursts[step] = !sequence_bursts[step];
  }


  int current_note = 0;
  uint32_t click_tick = 0;
  int edit_ticker = 0;

  // Collision display state.
  bool frog_hit = false;
  uint32_t modifier_display_tick = 0;
  int modifier_step = -1;
  int modifier_value = 0;
  bool modifier_gate = false;
  const uint8_t *modifier_icon = nullptr;
  bool modifier_clear_on_next_step = false;

  // Runtime collision Burst state.
  bool collision_burst_armed = false;
  uint8_t collision_bursts_to_go = 0;
  uint8_t collision_burst_count = 0;
  uint32_t collision_burst_countdown = 0;
  bool collision_burst_zap = false;
  uint32_t collision_burst_spacing = 0;

  // --------------------------------------------------------------------------
  // Drawing helpers
  // --------------------------------------------------------------------------

  int SafeZoneModifierX() {
    return 0;
  }

  int SafeZoneModifierY() {
    return FROG_Y[0];
  }

  void DrawFrog() {

    if (frog_hit) {
      gfxIcon(frog_x + 2, frog_y + 1, BURST_ICON);
      return;
    }

    for (int y = 0; y < 9; y++) {
      for (int x = 0; x < 12; x++) {

        if (frog_bitmap[y][x])
          gfxPixel(frog_x + x, frog_y + y);

      }
    }
  }

  bool FrogCollides(int lane, int car_x) {

    if (frog_lane != lane + 1)
      return false;

    const int frog_left = frog_x;
    const int frog_right = frog_x + 12;
    const int car_left = car_x;
    const int car_right = car_x + TRAFFIC_WIDTH;

    return frog_left < car_right && frog_right > car_left;
  }

  void ApplyCollisionModifier(int lane) {
    modifier_step = step;

    const bool can_burst =
      lane_rate[lane] == TRAFFIC_X2 ||
      lane_rate[lane] == TRAFFIC_X3 ||
      lane_rate[lane] == TRAFFIC_X4;

    // Each car gets exactly one collision result.
    // ×1 and ÷2: Note or Gate.
    // ×2, ×3 and ×4: Burst, Note or Gate.
    const int result = random(can_burst ? 3 : 2);

    // Clear any previous runtime Burst before applying the new result.
    collision_burst_armed = false;
    collision_bursts_to_go = 0;
    collision_burst_count = 0;
    collision_burst_countdown = 0;
    collision_burst_spacing = 0;

    if (result == 0 && can_burst) {
      collision_burst_armed = true;
      collision_burst_count = 0;
      collision_burst_countdown = 0;

      switch (lane_rate[lane]) {
        case TRAFFIC_X2:
          collision_bursts_to_go = 2;
          collision_burst_spacing =
            max(1u, ClockCycleTicks(0) / 2);
          break;

        case TRAFFIC_X3:
          collision_bursts_to_go = 3;
          collision_burst_spacing =
            max(1u, ClockCycleTicks(0) / 3);
          break;

        case TRAFFIC_X4:
          collision_bursts_to_go = 4;
          collision_burst_spacing =
            max(1u, ClockCycleTicks(0) / 4);
          break;

        default:
          collision_burst_armed = false;
          break;
      }

      modifier_gate = false;
      modifier_icon = BURST_ICON;
      modifier_value = collision_bursts_to_go;
      modifier_clear_on_next_step = true;
    }
    else if (result == (can_burst ? 1 : 0)) {
      modifier_clear_on_next_step = false;
      modifier_gate = false;
      modifier_icon = NOTE_ICON;
      modifier_value = random(-12, 13);
      SetFrogNote(
        GetFrogNote(step) + modifier_value,
        step
      );
    }
    else {
      modifier_clear_on_next_step = false;
      ToggleMute(step);
      modifier_gate = true;
      modifier_icon = GATE_ICON;
      modifier_value = muted(step) ? 0 : 1;
    }

    modifier_display_tick = OC::CORE::ticks;
  }

  void CheckTrafficCollisions() {

    if (frog_lane == 0 || frog_hit)
      return;

    for (int lane = 0; lane < TRAFFIC_LANES; ++lane) {
      for (int i = 0; i < TRAFFIC_OBJECTS; ++i) {

        if (!traffic[lane][i].active)
          continue;

        if (FrogCollides(lane, traffic[lane][i].x)) {
          traffic[lane][i].active = false;
          frog_hit = true;

          // The individual car determines exactly one collision result.
          ApplyCollisionModifier(lane);
          return;
        }
      }
    }
  }

  void MoveTraffic(bool clocked) {
    const uint32_t now = OC::CORE::ticks;

    if (clocked) {
      traffic_clock_ticks = max(1u, ClockCycleTicks(0));
      traffic_clock_valid = true;
      traffic_last_clock_tick = now;
    }

    if (!traffic_clock_valid)
      return;

    for (int lane = 0; lane < TRAFFIC_LANES; ++lane) {
      uint32_t move_ticks = traffic_clock_ticks;

      switch (lane_rate[lane]) {
        case TRAFFIC_X4:
          move_ticks = max(1u, traffic_clock_ticks / 4);
          break;
        case TRAFFIC_X3:
          move_ticks = max(1u, traffic_clock_ticks / 3);
          break;
        case TRAFFIC_X2:
          move_ticks = max(1u, traffic_clock_ticks / 2);
          break;
        case TRAFFIC_X1:
          move_ticks = traffic_clock_ticks;
          break;
        case TRAFFIC_DIV_2:
          move_ticks = traffic_clock_ticks * 2;
          break;
      }

      if (traffic_last_move_tick[lane] == 0)
        traffic_last_move_tick[lane] = now - move_ticks;

      if (now - traffic_last_move_tick[lane] < move_ticks)
        continue;

      traffic_last_move_tick[lane] = now;

      for (int i = 0; i < TRAFFIC_OBJECTS; ++i) {
        if (!traffic[lane][i].active)
          continue;

        if (lane_reverse[lane]) {
          traffic[lane][i].x -= TRAFFIC_MOVE_PIXELS;
          if (traffic[lane][i].x + TRAFFIC_WIDTH <= 0)
            traffic[lane][i].active = false;
        }
        else {
          traffic[lane][i].x += TRAFFIC_MOVE_PIXELS;
          if (traffic[lane][i].x >= TRAFFIC_RIGHT_BOUNDARY)
            traffic[lane][i].active = false;
        }
      }

      const int spawn_x = lane_reverse[lane]
        ? TRAFFIC_RIGHT_BOUNDARY
        : TRAFFIC_LEFT_BOUNDARY;

      bool spawn_clear = true;
      for (int i = 0; i < TRAFFIC_OBJECTS; ++i) {
        if (!traffic[lane][i].active)
          continue;

        const int car_left = traffic[lane][i].x;
        const int car_right = traffic[lane][i].x + TRAFFIC_WIDTH;

        if (lane_reverse[lane]) {
          if (car_right > TRAFFIC_RIGHT_BOUNDARY - TRAFFIC_MIN_SPAWN_GAP) {
            spawn_clear = false;
            break;
          }
        }
        else {
          if (car_left < TRAFFIC_MIN_SPAWN_GAP) {
            spawn_clear = false;
            break;
          }
        }
      }
      if (spawn_clear &&
          random(100) < FLOW_CHANCE[lane_flow[lane]]) {

        for (int i = 0; i < TRAFFIC_OBJECTS; ++i) {
          if (!traffic[lane][i].active) {
            traffic[lane][i].active = true;
            traffic[lane][i].x = spawn_x;
            break;
          }
        }
      }
    }

    CheckTrafficCollisions();
  }

  static constexpr int FLOW_CHANCE[7] = {
    0,
    2,
    5,
    12,
    25,
    50,
    100
  };

  void ResetTraffic() {
    for (int lane = 0; lane < TRAFFIC_LANES; ++lane) {
      for (int i = 0; i < TRAFFIC_OBJECTS; ++i) {
        traffic[lane][i].active = (i == 0);
        traffic[lane][i].x = lane_reverse[lane] ? 64 : -TRAFFIC_WIDTH;
      }
    }
  }

  void DrawTrafficObject(int lane, int x) {

    const int y = FROG_Y[lane + 1] + 3;

    const int draw_x = max(x, 0);
    const int draw_right = min(x + TRAFFIC_WIDTH, 64);
    const int draw_width = draw_right - draw_x;

    if (draw_width > 0)
      gfxFrame(draw_x, y, draw_width, 5);

    const int wheel1_x = x + 2;
    const int wheel2_x = x + 8;

    if (wheel1_x >= 0 && wheel1_x < 64)
      gfxPixel(wheel1_x, y + 5);

    if (wheel2_x >= 0 && wheel2_x < 64)
      gfxPixel(wheel2_x, y + 5);

  }

  void DrawStepCounter() {

    const int x0 = 1;
    const int y = 25;
    const int gap = 4;

    for (int i = 0; i < FROGSEQ_STEPS; ++i) {

      if (i == step) {
        gfxRect(x0 + i * gap - 1, y - 2, 3, 5);
      }
      else if (!muted(i)) {
        gfxPixel(x0 + i * gap, y);
      }

    }
  }
  void DrawCurrentNote() {
    if (q_select) {
      char q_label[] = { 'Q', char('1' + qselect), '\\0' };
      gfxPrint(42, 15, q_label);
      return;
    }

    const int semitone = (current_note % 12 + 12) % 12;
    const int notenum = current_note + 36;

    const int octave = (notenum / 12) - 3;

    gfxBitmap(42, 13, 8, NOTE_NAMES + semitone * 8);

    if (octave == -2)
      gfxBitmap(51, 16, 3, SUB_TWO);   // C1-B1
    else if (octave == -1)
      gfxBitmap(51, 19, 3, SUP_ONE);   // C2-B2
    else if (octave == 1)
      gfxBitmap(51, 11, 3, SUP_ONE);   // C4-B4
    else if (octave == 2)
      gfxBitmap(51, 8, 3, SUB_TWO);    // C5-B5
  }

  void DrawMainCursor() {

    if (cursor == FROG_SELECT) {

      if (!EditMode() && CursorBlink()) {
        gfxLine(frog_x, frog_y + 9, frog_x + 11, frog_y + 9);
        gfxPixel(frog_x, frog_y + 8);
        gfxPixel(frog_x + 11, frog_y + 8);
      }

    }
    else if (cursor == SEMITONE_SELECT) {

      if (q_select) {
        gfxSpicyCursor(42, 23, 12);
        SetLabel("Q-engine");
        SetAux(true);
      } else {
        gfxCursor(42, 23, 12);
      }

    }
    else if (cursor == RANDOM_SELECT) {

      gfxCursor(54, 23, 10);

    }

  }

  void DrawMainPage() {
    if (q_select) {
      SetLabel("Q-engine");
    } else {
      SetLabel("");
    }

    if (cursor == FROG_SELECT && EditMode()) {
      if (frog_horizontal) {
        gfxIcon(25, 1, LEFT_ICON);
        gfxIcon(35, 1, RIGHT_ICON);
      } else {
        gfxIcon(25, 1, UP_ICON);
        gfxIcon(35, 1, DOWN_ICON);
      }
    }

    SetAux(
      cursor == FROG_SELECT ||
      q_select ||
      (page == NOTE_SEQ_PAGE &&
       cursor >= 0 &&
       cursor < FROGSEQ_STEPS)
    );

    DrawFrog();

    // Runtime Collision Burst display in the Safe Zone.
    // Burst display replaces the normal modifier information while active.
    if (collision_burst_zap || collision_bursts_to_go > 0) {
      const int x = SafeZoneModifierX();
      const int y = SafeZoneModifierY();

      const int display_bursts =
        constrain(collision_bursts_to_go, 0, 4);

      if (collision_burst_zap) {
        const int total_bursts =
          constrain(
            collision_burst_count + collision_bursts_to_go,
            1,
            4
          );

        gfxIcon(
          max(0, x + 1 + ((total_bursts - 1) * 5) - 2),
          y,
          ZAP_ICON
        );
      }

      for (int i = 0; i < display_bursts; i++)
        gfxFrame(x + 1 + (i * 5), y + 3, 3, 3);
    }
    else if (modifier_icon &&
             OC::CORE::ticks - modifier_display_tick <
               HEMISPHERE_CURSOR_TICKS * 6 &&
             modifier_step >= 0) {

      const int x = SafeZoneModifierX();
      const int y = SafeZoneModifierY();

      // Fixed two-digit step field: 01-16.
      const int step_number = modifier_step + 1;
      gfxBitmap(x, y, 8, TEENS_8X8 + (step_number / 10) * 8);
      gfxBitmap(x + 8, y, 8, TEENS_8X8 + (step_number % 10) * 8);

      // Collision modifier icon.
      gfxIcon(x + 16, y, modifier_icon);

      if (modifier_gate) {
        // Gate state is represented entirely by its bitmap.
        gfxIcon(x + 24, y, modifier_value ? BTN_ON_ICON : BTN_OFF_ICON);
      }
      else if (modifier_icon == BURST_ICON) {
        // Burst count.
        const int amount = modifier_value;
        gfxBitmap(x + 24, y, 8, TEENS_8X8 + (amount / 10) * 8);
        gfxBitmap(x + 32, y, 8, TEENS_8X8 + (amount % 10) * 8);
      }
      else {
        // Note amount is absolute; negative values invert the value bitmap.
        const int amount = abs(modifier_value);
        const int value_x = x + 24;

        gfxBitmap(value_x, y, 8, TEENS_8X8 + (amount / 10) * 8);
        gfxBitmap(value_x + 8, y, 8, TEENS_8X8 + (amount % 10) * 8);

        if (modifier_value < 0)
          gfxInvert(value_x, y, 16, 8);
      }
    }

    for (int lane = 0; lane < TRAFFIC_LANES; ++lane) {
      for (int i = 0; i < TRAFFIC_OBJECTS; ++i) {
        if (traffic[lane][i].active)
          DrawTrafficObject(lane, traffic[lane][i].x);
      }
    }


    DrawCurrentNote();
    gfxIcon(56, 13, RANDOM_ICON);

// Live 16-note sequencer readout.
DrawStepCounter();

    DrawMainCursor();

    // Frogger-style playfield.


    for (int x = 0; x < 64; x += 8)
      gfxLine(x, 38, x + 3, 38);

    for (int x = 0; x < 64; x += 8)
      gfxLine(x, 51, x + 3, 51);

  }

  void DrawNoteSequencerStep(int step_index) {
    const int col = step_index & 7;
    const int row = step_index >> 3;

    const int x = 1 + col * 8;
    const int y = 16 + row * 12;

    const int note = GetFrogNote(step_index);
    const int height = constrain((note + 32) / 8, 1, 6);

    if (!muted(step_index)) {
      if (BurstEnabled(step_index))
        gfxRect(x, y + 6 - height, 6, height);
      else
        gfxFrame(x, y + 6 - height, 6, height);
    }

    // Active step indicator.
    if (step == step_index)
      gfxIcon(x + 1, y - 8, DOWN_BTN_ICON);

    // Step cursor.
    if (cursor == step_index) {
      gfxFrame(x - 1, y - 1, 8, 8);
      if (EditMode())
        gfxInvert(x - 1, y - 1, 8, 8);
    }
  }

  void DrawLaneCursor(int lane, int y) {

    const int direction_cursor = LANE1_DIRECTION + lane * 3;
    const int speed_cursor = LANE1_SPEED + lane * 3;
    const int flow_cursor = LANE1_FLOW + lane * 3;

    if (cursor == direction_cursor) {
      SetLabel(lane_reverse[lane] ? "Left" : "Right");
      gfxLine(12, y + 8, 20, y + 8);

      if (EditMode())
        gfxInvert(12, y, 10, 8);
    }
    else if (cursor == speed_cursor) {
      SetLabel("Speed");
      gfxLine(24, y + 8, 43, y + 8);

      if (EditMode())
        gfxInvert(32, y, 15, 8);
    }
    else if (cursor == flow_cursor) {
      SetLabel("Flow");
      gfxLine(48, y + 8, 63, y + 8);

      if (EditMode())
        gfxInvert(57, y, 7, 8);
    }
  }

  void DrawSequenceMenu() {

    const int px = 5;
    const int py = 13;
    const int pw = 54;
    const int ph = 38;

    gfxRect(px, py, pw, ph);
    gfxFrame(px, py, pw, ph);

    gfxPrint(px + 5, py + 5, "Load");
    gfxPrint(px + 5, py + 15, "Move");
    gfxPrint(px + 5, py + 25, "Reset");

    if (sequence_menu_mode == SEQUENCE_LOAD)
      gfxIcon(px + 38, py + 5, LEFT_ICON);
    else if (sequence_menu_mode == SEQUENCE_MOVE)
      gfxIcon(px + 38, py + 15, LEFT_ICON);
    else
      gfxIcon(px + 38, py + 25, LEFT_ICON);
  }

  void DrawNoteSequencerPage() {
  SetAux(cursor >= 0 && (cursor <= sequence_length || sequence_menu || sequence_slot_mode || sequence_reset_confirm));

  for (int s = 0; s < sequence_length; ++s)
    DrawNoteSequencerStep(s);

  static const char *speed_labels[] = {
    "/2", "x1", "x2", "x3", "x4"
  };

  static const int lane_y[TRAFFIC_LANES] = {
    36, 45, 54
  };

  for (int lane = 0; lane < TRAFFIC_LANES; ++lane) {
    const int y = lane_y[lane];

    char lane_label[] = { "L"[0], char("1"[0] + lane), "\0"[0] };
    gfxPrint(0, y + 1, lane_label);

    gfxIcon(13, y,
            lane_reverse[lane] ? ROTATE_L_ICON : ROTATE_R_ICON);

    gfxIcon(24, y, GAUGE_ICON);
    gfxPrint(33, y + 1, speed_labels[lane_rate[lane]]);

    gfxIcon(48, y, MOD_ICON);
    gfxPrint(58, y + 1, lane_flow[lane]);
  }


  for (int lane = 0; lane < TRAFFIC_LANES; ++lane) {
    DrawLaneCursor(lane, lane_y[lane]);
  }

  if (sequence_menu) {

    if (sequence_menu_mode == SEQUENCE_LOAD)
      SetLabel("Load");
    else if (sequence_menu_mode == SEQUENCE_MOVE)
      SetLabel("Move");
    else
      SetLabel("Reset");

  }
  else if (sequence_slot_mode) {
    static char label[12];

    if (sequence_menu_mode == SEQUENCE_LOAD)
      snprintf(label, sizeof(label),
               "Load(%d)", selected_sequence + 1);
    else
      snprintf(label, sizeof(label),
               "Move(%d)", selected_sequence + 1);

    SetLabel(label);
  }
  else if (sequence_reset_confirm) {
    SetLabel(sequence_reset_yes ? "Reset(Y)" : "Reset(N)");
  }
  else if (cursor == sequence_length) {

    SetLabel("Steps");



  }

  if (cursor == sequence_length) {

    gfxFrame(0, 15, 64, 22);

    if (EditMode() || sequence_menu || sequence_slot_mode || sequence_reset_confirm)

      gfxInvert(1, 16, 62, 20);

  }
}

  void DrawInterface() {

    if (page == MAIN_PAGE)
      DrawMainPage();
    else
      DrawNoteSequencerPage();

  }

  // --------------------------------------------------------------------------
  // Frog helpers
  // --------------------------------------------------------------------------

  void MoveFrog(int direction) {

    if (frog_horizontal) {

      // Safe Zone is restricted; lanes use the full playfield width.
      const int max_x = (frog_lane == 0) ? 31 : 52;
      frog_x = constrain(frog_x + direction, 0, max_x);

    } else {

      // Vertical movement snaps through Safe Zone and three lanes.
      frog_lane = constrain(frog_lane + direction, 0, 3);
      frog_y = FROG_Y[frog_lane];

      // Safe Zone is narrower because NOTE/RANDOM occupy the upper-right.
      if (frog_lane == 0) {
        frog_x = constrain(frog_x, 0, 31);
        modifier_icon = nullptr;
        modifier_step = -1;
        modifier_value = 0;
        modifier_gate = false;
      }

    }

  }

  void ToggleFrogAxis() {
    frog_horizontal = !frog_horizontal;
  }

  // --------------------------------------------------------------------------

  // --------------------------------------------------------------------------
  // Sequencer helpers
  // --------------------------------------------------------------------------

  void ResetSequence() {

    step = 0;
    reset = true;

    current_note = GetFrogNote(0);
  }

  void AdvanceSequence() {

    if (reset) {

      reset = false;

    }
    else {

      ++step;

      if (step >= sequence_length)
        step = 0;

    }

    if (!muted(step))
      current_note = GetFrogNote(step);
  }

  void RandomizeSequence() {

    for (int s = 0; s < FROGSEQ_STEPS; ++s) {

      SetFrogNote(random(-24, 36), s);

      SetBurst(s, false);
      SetMute(s, random(2));
    }

    ResetSequence();
  }

  void EditSequenceNote(int direction) {
    SetFrogNote(GetFrogNote(cursor) + direction, cursor);

    if (cursor == step)
      current_note = GetFrogNote(step);

    int notenum = GetFrogNote(cursor) + 36;

    SetLabel(midi_note_numbers[notenum]);


    edit_ticker = 5000;
  }

  void ToggleSequenceBurst() {

    ToggleBurst(cursor);

    edit_ticker = 5000;
  }

  void ToggleSequenceMute() {

    ToggleMute(cursor);

    edit_ticker = 5000;
  }

public:

  // --------------------------------------------------------------------------
  // Lifecycle
  // --------------------------------------------------------------------------

  void Start() {
    frog_hit = false;
    modifier_display_tick = 0;
    modifier_icon = nullptr;
    modifier_clear_on_next_step = false;
    collision_burst_armed = false;
    collision_burst_count = 0;
    collision_burst_zap = false;

    frog_x = 26;
    frog_y = FROG_Y[0];
    ResetTraffic();

    page = MAIN_PAGE;

    cursor = FROG_SELECT;

    frog_horizontal = true;

    for (int s = 0; s < FROGSEQ_STEPS; ++s) {
      sequence_notes[s] = 0;
      sequence_mutes[s] = false;
      sequence_bursts[s] = false;
    }

    current_note = GetFrogNote(0);

    click_tick = 0;
    edit_ticker = 0;

    step = 0;
    reset = true;
  }

  // --------------------------------------------------------------------------
  // Controller
  // --------------------------------------------------------------------------

  void Controller() {
    if (!traffic_initialized) {
      ResetTraffic();
      traffic_initialized = true;
    }

    if (Clock(1)) {
      ResetSequence();
    }

    const bool clocked = Clock(0);

    if (clocked) {
      frog_hit = false;

      if (modifier_clear_on_next_step) {
        modifier_icon = nullptr;
        modifier_clear_on_next_step = false;
      }

      // The previous Burst display ends at the new master-clock step.
      // A Burst beginning on this same clock will turn ZAP back on.
      collision_burst_zap = false;
    }

    MoveTraffic(clocked);

    // Collision Burst ratchet.
    //
    // The master clock is trigger #1. Remaining triggers are
    // generated between master clocks using the Burst-style
    // countdown timer. The sequence step does not advance
    // during the ratchet.
    if (collision_burst_armed) {

      const uint32_t burst_spacing = collision_burst_spacing;

      if (clocked) {

        AdvanceSequence();

        collision_burst_count = 1;
        collision_burst_zap = true;

        int play_cv = MIDIQuantizer::CV(current_note + 36);
        play_cv = HS::GetQuantEngine(qselect).Process(play_cv, 0, 0);
        Out(0, play_cv);
        ClockOut(1);

        --collision_bursts_to_go;

        if (collision_bursts_to_go > 0) {
          collision_burst_countdown = burst_spacing;
        }
        else {
          collision_burst_armed = false;
        }
      }
      else if (collision_burst_count > 0 &&
               collision_bursts_to_go > 0) {

        if (collision_burst_countdown > 0)
          --collision_burst_countdown;

        if (collision_burst_countdown == 0) {

          int play_cv = MIDIQuantizer::CV(current_note + 36);
          play_cv = HS::GetQuantEngine(qselect).Process(play_cv, 0, 0);
          Out(0, play_cv);
          ClockOut(1);

          ++collision_burst_count;
          --collision_bursts_to_go;

          if (collision_bursts_to_go > 0)
            collision_burst_countdown = burst_spacing;
          else
            collision_burst_armed = false;
        }
      }
    }

    // Normal master-clock sequence playback.
    else if (clocked) {

      AdvanceSequence();

      if (muted(step)) {

        GateOut(1, false);

      }
      else {

        int play_cv = MIDIQuantizer::CV(current_note + 36);
        play_cv = HS::GetQuantEngine(qselect).Process(play_cv, 0, 0);
        Out(0, play_cv);

        ClockOut(1);
      }
    }

    if (edit_ticker)
      --edit_ticker;
  }

  // --------------------------------------------------------------------------
  // View
  // --------------------------------------------------------------------------

  FLASHMEM void View() {

    DrawInterface();
  }

  // --------------------------------------------------------------------------
  // Encoder
  // --------------------------------------------------------------------------

  FLASHMEM void OnEncoderMove(int direction) {
    if (page == MAIN_PAGE) {
      if (q_select) {
        qselect = constrain(qselect + direction, 0, 7);
        return;
      }

      if (EditMode()) {
        if (cursor == FROG_SELECT)
          MoveFrog(direction);
        return;
      }

      if (direction > 0 && cursor == MAIN_CURSOR_LAST) {
        page = NOTE_SEQ_PAGE;
        cursor = 0;
        CancelEdit();
        return;
      }

      MoveCursor(cursor, direction, MAIN_CURSOR_LAST);
return;
    }

    if (page == NOTE_SEQ_PAGE) {
      if (sequence_reset_confirm) {

        sequence_reset_yes = (direction > 0);
        return;

      }

      if (sequence_menu) {

        sequence_menu_mode = static_cast<SequenceMenuMode>(
          constrain(
            static_cast<int>(sequence_menu_mode) + direction,
            SEQUENCE_LOAD,
            SEQUENCE_RESET
          )
        );

        return;

      }

      if (sequence_slot_mode) {

        selected_sequence = constrain(
          selected_sequence + direction,
          0,
          SAVED_SEQUENCES - 1
        );

        return;

      }

      if (!EditMode()) {
        if (direction < 0 && cursor == 0) {
          page = MAIN_PAGE;
          cursor = FROG_SELECT;
          CancelEdit();
          return;
        }

        MoveCursor(cursor, direction, NOTE_SEQ_CURSOR_LAST);

        // Skip inactive step positions when sequence length is shortened.
        if (cursor > sequence_length && cursor < LANE1_DIRECTION) {
          cursor = direction > 0 ? LANE1_DIRECTION : sequence_length;
        }

        return;
      }

      if (cursor == sequence_length) {
        sequence_length = constrain(
          sequence_length + direction,
          1,
          FROGSEQ_STEPS
        );
        cursor = sequence_length;
      }
      else if (cursor < sequence_length) {
        EditSequenceNote(direction);
      }
      else if (cursor >= LANE1_DIRECTION &&
               cursor <= LANE3_FLOW) {

        const int lane = (cursor - LANE1_DIRECTION) / 3;
        const int control = (cursor - LANE1_DIRECTION) % 3;

        if (control == 0) {
          lane_reverse[lane] = (direction < 0);
        }
        else if (control == 1) {
          lane_rate[lane] = static_cast<TrafficRate>(
            constrain(
              static_cast<int>(lane_rate[lane]) + direction,
              TRAFFIC_DIV_2,
              TRAFFIC_X4
            )
          );
        }
        else if (control == 2) {
          lane_flow[lane] = constrain(
            lane_flow[lane] + direction,
            1,
            6
          );
        }
      }
    }
  }

  // --------------------------------------------------------------------------
  // --------------------------------------------------------------------------
  // Button
  // --------------------------------------------------------------------------

  void OnButtonPress() override {

    if (page == MAIN_PAGE) {

      if (cursor == RANDOM_SELECT) {
        RandomizeSequence();
        return;
      }

      if (cursor == SEMITONE_SELECT) {
        q_select = !q_select;
        CursorToggle();
        return;
      }

      CursorToggle();
      return;
    }

    if (page == NOTE_SEQ_PAGE) {

      if (sequence_reset_confirm) {
        if (sequence_reset_yes) {
          for (int s = 0; s < FROGSEQ_STEPS; ++s) {
            sequence_notes[s] = 0;
            sequence_mutes[s] = false;
            sequence_bursts[s] = false;
          }
          sequence_length = FROGSEQ_STEPS;
          step = 0;
          reset = true;
          current_note = GetFrogNote(0);
          SaveSequenceMemory(current_sequence);
        }

        sequence_reset_confirm = false;
        sequence_reset_yes = true;
        sequence_menu_mode = SEQUENCE_LOAD;
        selected_sequence = current_sequence;
        CancelEdit();
        cursor = sequence_length;
        return;
      }

      if (sequence_menu) {
        if (sequence_menu_mode == SEQUENCE_RESET) {
          sequence_menu = false;
          sequence_reset_confirm = true;
          sequence_reset_yes = false;
          return;
        }

        sequence_menu = false;
        sequence_slot_mode = true;
        selected_sequence = current_sequence;
        return;
      }

      if (sequence_slot_mode) {
        if (sequence_menu_mode == SEQUENCE_LOAD) {
          if (LoadSequenceMemory(selected_sequence))
            current_sequence = selected_sequence;
        }
        else if (sequence_menu_mode == SEQUENCE_MOVE) {
          SaveSequenceMemory(selected_sequence);
          current_sequence = selected_sequence;
        }

        sequence_slot_mode = false;
        sequence_menu_mode = SEQUENCE_LOAD;
        selected_sequence = current_sequence;
        CancelEdit();
        cursor = sequence_length;
        return;
      }

      if (cursor >= NOTE_STEP_FIRST && cursor <= NOTE_STEP_LAST) {
        if (OC::CORE::ticks - click_tick < HEMISPHERE_DOUBLE_CLICK_TIME) {
          ToggleSequenceBurst();
          click_tick = 0;
          return;
        }
        click_tick = OC::CORE::ticks;
      }

      CursorToggle();

      if (EditMode()) {
        if (cursor == sequence_length && !sequence_menu) {
          SetLabel("Steps");
        }
        else if (cursor >= NOTE_STEP_FIRST &&
                 cursor < sequence_length) {
          int notenum = GetFrogNote(cursor) + 36;
          SetLabel(midi_note_numbers[notenum]);
        }
        else if (cursor >= LANE1_DIRECTION &&
                 cursor <= LANE3_FLOW) {
          const int lane = (cursor - LANE1_DIRECTION) / 3;
          const int control = (cursor - LANE1_DIRECTION) % 3;

          if (control == 0)
            SetLabel(lane_reverse[lane] ? "Left" : "Right");
          else if (control == 1)
            SetLabel("Speed");
          else
            SetLabel("Flow");
        }
      }
      else {
        SetLabel("");
      }

      return;
    }

    CursorToggle();
  }

  // --------------------------------------------------------------------------
  // Aux button
  // --------------------------------------------------------------------------

  FLASHMEM void AuxButton() {

    if (page == MAIN_PAGE) {

      if (q_select) {
        HS::QuantizerEdit(qselect);
        return;
      }

      if (cursor == FROG_SELECT)
        ToggleFrogAxis();

      return;
    }

    if (page == NOTE_SEQ_PAGE) {

      // AUX on Step Amount opens/cancels the Load/Move/Reset menu.
      if (cursor == sequence_length) {

        if (sequence_menu || sequence_slot_mode || sequence_reset_confirm) {

          sequence_menu = false;
          sequence_slot_mode = false;
          sequence_reset_confirm = false;
          sequence_reset_yes = false;
          sequence_menu_mode = SEQUENCE_LOAD;
          selected_sequence = current_sequence;
          SetLabel("Steps");

        }
        else {

          sequence_menu = true;
          sequence_menu_mode = SEQUENCE_LOAD;
          selected_sequence = current_sequence;
          SetLabel("Load");

        }

        return;
      }

      // AUX anywhere else cancels sequence memory selection.
      if (sequence_menu || sequence_slot_mode || sequence_reset_confirm) {

        sequence_menu = false;
        sequence_slot_mode = false;
        sequence_reset_confirm = false;
        sequence_reset_yes = false;
        sequence_menu_mode = SEQUENCE_LOAD;
        selected_sequence = current_sequence;
        SetLabel("Steps");
        return;
      }

      ToggleMute(cursor);
      CancelEdit();
      return;
    }

  }

  // --------------------------------------------------------------------------
  // Persistence
  // --------------------------------------------------------------------------

  uint64_t OnDataRequest() {

    uint64_t data = 0;
for (int lane = 0; lane < TRAFFIC_LANES; ++lane) {

      const int base = lane * 7;

      // Lane direction: 1 bit.
      Pack(
        data,
        PackLocation{base, 1},
        (uint8_t)lane_reverse[lane]
      );

      // Lane speed: 3 bits.
      Pack(
        data,
        PackLocation{base + 1, 3},
        (uint8_t)lane_rate[lane]
      );

      // Lane flow: 3 bits.
      Pack(
        data,
        PackLocation{base + 4, 3},
        (uint8_t)constrain(lane_flow[lane], 1, 6)
      );
    }

    // Current sequence number: 3 bits.
    Pack(
      data,
      PackLocation{21, 3},
      current_sequence
    );


    // Sequence length: 4 bits.
    Pack(
      data,
      PackLocation{24, 4},
      sequence_length - 1
    );

// Save the currently active sequence to RAM.
    SaveSequenceMemory(current_sequence);

    return data;
  }

  void SaveSequenceMemory(uint8_t sequence) {

    uint64_t data = 0;

    // Steps 1-8: 8 bits per step.
    for (int s = 0; s < 8; ++s) {

      const uint8_t note =
        constrain(sequence_notes[s] + 24, 0, 59);

      const uint8_t packed =
        note
        | (sequence_mutes[s] ? 0x40 : 0);

      Pack(
        data,
        PackLocation{s * 8, 8},
        packed
      );
    }

    SetData(FROGSEQ_DATA_START + sequence * 2, data);

    data = 0;

    // Steps 9-16: 8 bits per step.
    for (int s = 8; s < FROGSEQ_STEPS; ++s) {

      const int offset = (s - 8) * 8;

      const uint8_t note =
        constrain(sequence_notes[s] + 24, 0, 59);

      const uint8_t packed =
        note
        | (sequence_mutes[s] ? 0x40 : 0);

      Pack(
        data,
        PackLocation{offset, 8},
        packed
      );
    }

    SetData(FROGSEQ_DATA_START + sequence * 2 + 1, data);
  }

  bool LoadSequenceMemory(uint8_t sequence) {

    uint64_t data = 0;

    if (!GetData(FROGSEQ_DATA_START + sequence * 2, data))
      return false;

    for (int s = 0; s < 8; ++s) {

      const int offset = s * 8;

      const uint8_t packed =
        Unpack(data, PackLocation{offset, 8});

      sequence_notes[s] =
        constrain((packed & 0x3f) - 24, -24, 35);

      sequence_mutes[s] =
        (packed & 0x40) != 0;
    }

    if (!GetData(FROGSEQ_DATA_START + sequence * 2 + 1, data))
      return false;

    for (int s = 8; s < FROGSEQ_STEPS; ++s) {

      const int offset = (s - 8) * 8;

      const uint8_t packed =
        Unpack(data, PackLocation{offset, 8});

      sequence_notes[s] =
        constrain((packed & 0x3f) - 24, -24, 35);

      sequence_mutes[s] =
        (packed & 0x40) != 0;
    }

    return true;
  }

  void OnDataReceive(uint64_t data) {
    // Frog X is intentionally not persisted.
    frog_x = 26;
// Restore the selected sequence.
    current_sequence =
      constrain(
        Unpack(data, PackLocation{21, 3}),
        0,
        SAVED_SEQUENCES - 1
      );


    // Restore sequence length.
    sequence_length =
      constrain(
        Unpack(data, PackLocation{24, 4}) + 1,
        1,
        FROGSEQ_STEPS
      );

    // Restore the active FrogSeq sequence.
    LoadSequenceMemory(current_sequence);

    collision_burst_armed = false;
    collision_burst_count = 0;

    for (int lane = 0; lane < TRAFFIC_LANES; ++lane) {

      const int base = lane * 7;

      lane_reverse[lane] =
        Unpack(data, PackLocation{base, 1}) != 0;

      lane_rate[lane] =
        static_cast<TrafficRate>(
          constrain(
            Unpack(data, PackLocation{base + 1, 3}),
            TRAFFIC_DIV_2,
            TRAFFIC_X4
          )
        );

      lane_flow[lane] =
        constrain(
          Unpack(data, PackLocation{base + 4, 3}),
          1,
          6
        );
    }

    frog_y = 14;

    step = 0;

    reset = true;
  }

  // --------------------------------------------------------------------------
  // Help
  // --------------------------------------------------------------------------

  void SetHelp() {

    help[HELP_DIGITAL1] = "Clock";
    help[HELP_DIGITAL2] = "Restore";
    help[HELP_CV1] = "Frog X";
    help[HELP_CV2] = "Frog Y";
    help[HELP_OUT1] = "Pitch";
    help[HELP_OUT2] = "Trigger";
    help[HELP_EXTRA1] = "FrogSeq";
    help[HELP_EXTRA2] = "Note Seq";
  }

};
