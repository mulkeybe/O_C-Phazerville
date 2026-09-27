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

#include "MiniSeq.h"

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
    RANDOM_SELECT,
    MAIN_CURSOR_LAST = RANDOM_SELECT
  };

private:

  // --------------------------------------------------------------------------
  // State
  // --------------------------------------------------------------------------

  Page page = MAIN_PAGE;

  int cursor = FROG_SELECT;

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
    TRAFFIC_DIV_4,
    TRAFFIC_DIV_2,
    TRAFFIC_X1,
    TRAFFIC_X2,
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
    TRAFFIC_DIV_4,
    TRAFFIC_X1,
    TRAFFIC_X4
  };

  int lane_flow[TRAFFIC_LANES] = {
    5,
    5,
    5
  };

  uint32_t traffic_last_clock_tick = 0;
  uint32_t traffic_last_move_tick = 0;
  uint32_t traffic_clock_ticks = 1;
  bool traffic_clock_valid = false;
  bool traffic_initialized = false;

  MiniSeq seq;

  int GetFrogNote(int step) {
    return seq.GetNote(step) - 4;
  }

  void SetFrogNote(int note, int step) {
    seq.SetNote(note + 4, step);
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

    if (!clocked)

      return;

    for (int lane = 0; lane < TRAFFIC_LANES; ++lane) {

      int pixels = 1;

      switch (lane_rate[lane]) {

        case TRAFFIC_DIV_4: pixels = 1; break;
        case TRAFFIC_DIV_2: pixels = 2; break;
        case TRAFFIC_X1: pixels = 4; break;
        case TRAFFIC_X2: pixels = 8; break;
        case TRAFFIC_X4: pixels = 16; break;

      }

      for (int i = 0; i < TRAFFIC_OBJECTS; ++i) {

        if (!traffic[lane][i].active)

          continue;

        if (lane_reverse[lane]) {

          traffic[lane][i].x -= pixels;

          if (traffic[lane][i].x + TRAFFIC_WIDTH <= 0)

            traffic[lane][i].active = false;

        }

        else {

          traffic[lane][i].x += pixels;

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

      if (i == seq.step) {
        gfxRect(x0 + i * gap - 1, y - 2, 3, 5);
      }
      else if (!seq.muted(i)) {
        gfxPixel(x0 + i * gap, y);
      }

    }
  }
  void DrawMainCursor() {

    if (cursor == FROG_SELECT) {

      if (!EditMode() && CursorBlink()) {
        gfxLine(frog_x, frog_y + 9, frog_x + 11, frog_y + 9);
        gfxPixel(frog_x, frog_y + 8);
        gfxPixel(frog_x + 11, frog_y + 8);
      }

    }
    else if (cursor == RANDOM_SELECT) {

      gfxCursor(54, 23, 10);

    }

  }

  void DrawMainPage() {
    SetLabel("");

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

  void DrawNoteSequencerStep(int step) {

    const int col = step & 7;
    const int row = step >> 3;

    const int x = 1 + col * 8;
    const int y = 17 + row * 13;

    const int note = GetFrogNote(step);
    const int height = constrain((note + 32) / 8, 1, 8);

    if (!seq.muted(step)) {

      if (seq.accent(step))

        gfxRect(x, y + 8 - height, 6, height);

      else

        gfxFrame(x, y + 8 - height, 6, height);

    }

    if (seq.step == step)

      gfxIcon(x + 1, y - 5, DOWN_BTN_ICON);

    if (cursor == step) {

      gfxFrame(x - 1, y - 1, 8, 11);

      if (EditMode())
        gfxInvert(x - 1, y - 1, 8, 11);
    }
  }

  void DrawNoteValue() {

    int notenum = GetFrogNote(cursor);
    notenum = MIDIQuantizer::NoteNumber(
      QuantizerLookup(0, notenum + 64)
    );

    gfxPrint(27, 13, midi_note_numbers[notenum]);
  }

  void DrawCurrentNote() {
    const int notenum = MIDIQuantizer::NoteNumber(
      QuantizerLookup(0, current_note + 64)
    );

    const int octave = (notenum / 12) - 3;

    gfxBitmap(43, 13, 8, NOTE_NAMES + (notenum % 12) * 8);

    if (octave == -2)
      gfxBitmap(52, 16, 3, SUB_TWO);   // C1-B1
    else if (octave == -1)
      gfxBitmap(52, 19, 3, SUP_ONE);   // C2-B2
    else if (octave == 1)
      gfxBitmap(52, 11, 3, SUP_ONE);   // C4-B4
    else if (octave == 2)
      gfxBitmap(52, 8, 3, SUB_TWO);    // C5-B5
  }

  void DrawNoteSequencerPage() {

    SetAux(cursor >= 0 && cursor < sequence_length);

    for (int s = 0; s < sequence_length; ++s)
      DrawNoteSequencerStep(s);

    if (cursor == sequence_length) {
      SetLabel("Steps");
      gfxFrame(0, 16, 64, 27);
      if (EditMode())
        gfxInvert(1, 17, 62, 25);
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

    seq.step = 0;
    seq.reset = true;

    current_note = GetFrogNote(0);
  }

  void AdvanceSequence() {

    if (seq.reset) {

      seq.reset = false;

    }
    else {

      ++seq.step;

      if (seq.step >= sequence_length)
        seq.step = 0;

    }

    if (!seq.muted(seq.step))
      current_note = GetFrogNote(seq.step);
  }

  void RandomizeSequence() {

    for (int s = 0; s < FROGSEQ_STEPS; ++s) {

      const int note = random(60) - 24;

      SetFrogNote(note, s);

      seq.SetAccent(s, false);
      seq.Unmute(s);
    }

    ResetSequence();
  }

  void EditSequenceNote(int direction) {
    SetFrogNote(
      constrain(GetFrogNote(cursor) + direction, -24, 35),
      cursor
    );

    if (cursor == seq.step)
      current_note = GetFrogNote(seq.step);

    int notenum = MIDIQuantizer::NoteNumber(
      QuantizerLookup(0, GetFrogNote(cursor) + 64)
    );

    SetLabel(midi_note_numbers[notenum]);


    edit_ticker = 5000;
  }

  void ToggleSequenceAccent() {

    seq.ToggleAccent(cursor);

    edit_ticker = 5000;
  }

  void ToggleSequenceMute() {

    seq.ToggleMute(cursor);

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

    current_note = GetFrogNote(0);

    click_tick = 0;
    edit_ticker = 0;

    seq.step = 0;
    seq.reset = true;
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

      if (seq.muted(seq.step)) {

        GateOut(1, false);

      }
      else {

        const int play_note = constrain(
          current_note + 64,
          0,
          127
        );

        const int play_cv = QuantizerLookup(0, play_note);

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

        MoveCursor(cursor, direction, sequence_length);
        return;
      }

      if (cursor == sequence_length) {
        sequence_length = constrain(sequence_length + direction, 1, FROGSEQ_STEPS);
        cursor = sequence_length;
      }
      else
        EditSequenceNote(direction);
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

      CursorToggle();
      return;
    }

    if (page == NOTE_SEQ_PAGE) {

      if (cursor >= 0 && cursor < FROGSEQ_STEPS) {

        if (OC::CORE::ticks - click_tick < HEMISPHERE_DOUBLE_CLICK_TIME) {

          ToggleSequenceAccent();
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
        else {
          int notenum = MIDIQuantizer::NoteNumber(
            QuantizerLookup(0, GetFrogNote(cursor) + 64)
          );
          SetLabel(midi_note_numbers[notenum]);
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
      if (cursor == FROG_SELECT)
        ToggleFrogAxis();
return;
    }

    if (page == NOTE_SEQ_PAGE) {

      seq.ToggleMute(cursor);
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

    seq.step = 0;
    seq.reset = true;
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
