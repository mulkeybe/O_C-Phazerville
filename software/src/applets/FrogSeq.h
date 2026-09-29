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
  static constexpr int TRAFFIC_OBJECTS = 3;
  static constexpr int TRAFFIC_WIDTH = 10;
  static constexpr int TRAFFIC_LEFT_BOUNDARY = -TRAFFIC_WIDTH;
  static constexpr int TRAFFIC_RIGHT_BOUNDARY = 64;

  enum TrafficRate {
    TRAFFIC_X4,
    TRAFFIC_X3,
    TRAFFIC_X2,
    TRAFFIC_X1,
    TRAFFIC_DIV_2
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

  // --------------------------------------------------------------------------
  // Drawing helpers
  // --------------------------------------------------------------------------

  void DrawFrog() {

    for (int y = 0; y < 9; y++) {
      for (int x = 0; x < 12; x++) {

        if (frog_bitmap[y][x])
          gfxPixel(frog_x + x, frog_y + y);

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

      traffic_last_move_tick[lane] += move_ticks;

      for (int i = 0; i < TRAFFIC_OBJECTS; ++i) {
        if (!traffic[lane][i].active)
          continue;

        if (lane_reverse[lane]) {
          traffic[lane][i].x -= 4;
          if (traffic[lane][i].x + TRAFFIC_WIDTH <= 0)
            traffic[lane][i].active = false;
        }
        else {
          traffic[lane][i].x += 4;
          if (traffic[lane][i].x >= TRAFFIC_RIGHT_BOUNDARY)
            traffic[lane][i].active = false;
        }
      }

      bool has_active = false;
      for (int i = 0; i < TRAFFIC_OBJECTS; ++i) {
        if (traffic[lane][i].active) {
          has_active = true;
          break;
        }
      }

      if (!has_active) {
        traffic[lane][0].active = true;
        traffic[lane][0].x = lane_reverse[lane]
          ? TRAFFIC_RIGHT_BOUNDARY
          : TRAFFIC_LEFT_BOUNDARY;
      }
    }
  }

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

  void DrawNoteSequencerPage() {
  SetAux(cursor >= 0 && cursor < sequence_length);

  for (int s = 0; s < sequence_length; ++s)
    DrawNoteSequencerStep(s);

  static const char *speed_labels[] = {
    "x4", "x3", "x2", "x1", "/2"
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

  if (cursor == sequence_length) {
    SetLabel("Steps");
    gfxFrame(0, 15, 64, 22);
    if (EditMode())
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
      if (frog_lane == 0)
        frog_x = constrain(frog_x, 0, 31);

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
    MoveTraffic(clocked);
    if (clocked) {


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
      if (!EditMode()) {
        if (direction < 0 && cursor == 0) {
          page = MAIN_PAGE;
          cursor = FROG_SELECT;
          CancelEdit();
          return;
        }

        MoveCursor(cursor, direction, NOTE_SEQ_CURSOR_LAST);
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
              TRAFFIC_X4,
              TRAFFIC_DIV_2
            )
          );
        }
        else if (control == 2) {
          lane_flow[lane] = constrain(
            lane_flow[lane] + direction,
            1,
            5
          );
        }
      }
    }
  }

  // --------------------------------------------------------------------------
  // Button
  // --------------------------------------------------------------------------

  FLASHMEM void OnButtonPress() {

    if (page == MAIN_PAGE) {

      if (cursor == RANDOM_SELECT) {

        RandomizeSequence();
        return;

      }

      if (cursor == SEMITONE_SELECT) {

        q_select = !q_select; CursorToggle();
        return;

      }

      CursorToggle();
      return;
    }

    if (page == NOTE_SEQ_PAGE) {

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

        if (cursor == sequence_length) {
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
    }
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

      ToggleMute(cursor);
      CancelEdit();
      return;

    }  }

  // --------------------------------------------------------------------------
  // Persistence
  // --------------------------------------------------------------------------

  uint64_t OnDataRequest() {

    uint64_t data = 0;

    Pack(
      data,
      PackLocation{0, 6},
      (uint8_t)constrain(frog_x, 0, 63)
    );

    return data;
  }

  void OnDataReceive(uint64_t data) {

    frog_x = Unpack(data, PackLocation{0, 6});

    frog_x = constrain(frog_x, 0, 63);

    frog_y = 14;

    step = 0;
    reset = true;
  }

  // --------------------------------------------------------------------------
  // Help
  // --------------------------------------------------------------------------

  void SetHelp() {

    help[HELP_DIGITAL1] = "Clock";
    help[HELP_DIGITAL2] = "Reset";
    help[HELP_CV1] = "Pitch";
    help[HELP_CV2] = "";
    help[HELP_OUT1] = "Pitch";
    help[HELP_OUT2] = "Trigger";
    help[HELP_EXTRA1] = "FrogSeq";
    help[HELP_EXTRA2] = "Note Seq";
  }

};
