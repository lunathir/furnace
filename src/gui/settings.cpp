/**
 * Furnace Tracker - multi-system chiptune tracker
 * Copyright (C) 2021-2022 tildearrow and contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "gui.h"
#include "fonts.h"
#include "../ta-log.h"
#include "../fileutils.h"
#include "util.h"
#include "guiConst.h"
#include "ImGuiFileDialog.h"
#include "IconsFontAwesome4.h"
#include "misc/cpp/imgui_stdlib.h"
#include <fmt/printf.h>
#include <imgui.h>

#define DEFAULT_NOTE_KEYS "5:7;6:4;7:3;8:16;10:6;11:8;12:24;13:10;16:11;17:9;18:26;19:28;20:12;21:17;22:1;23:19;24:23;25:5;26:14;27:2;28:21;29:0;30:100;31:13;32:15;34:18;35:20;36:22;38:25;39:27;43:100;46:101;47:29;48:31;53:102;"

const char* mainFonts[]={
  "IBM Plex Sans",
  "Liberation Sans",
  "Exo",
  "Proggy Clean",
  "GNU Unifont",
  "<Use system font>",
  "<Custom...>"
};

const char* patFonts[]={
  "IBM Plex Mono",
  "Mononoki",
  "PT Mono",
  "Proggy Clean",
  "GNU Unifont",
  "<Use system font>",
  "<Custom...>"
};

const char* audioBackends[]={
  "JACK",
  "SDL"
};

const char* audioQualities[]={
  "High",
  "Low"
};

const char* arcadeCores[]={
  "ymfm",
  "Nuked-OPM"
};

const char* ym2612Cores[]={
  "Nuked-OPN2",
  "ymfm"
};

const char* saaCores[]={
  "MAME",
  "SAASound"
};

const char* valueInputStyles[]={
  "Disabled/custom",
  "Two octaves (0 is C-4, F is D#5)",
  "Raw (note number is value)",
  "Two octaves alternate (lower keys are 0-9, upper keys are A-F)",
  "Use dual control change (one for each nibble)",
  "Use 14-bit control change",
  "Use single control change (imprecise)"
};

const char* valueSInputStyles[]={
  "Disabled/custom",
  "Use dual control change (one for each nibble)",
  "Use 14-bit control change",
  "Use single control change (imprecise)"
};

const char* messageTypes[]={
  "--select--",
  "???",
  "???",
  "???",
  "???",
  "???",
  "???",
  "???",
  "Note Off",
  "Note On",
  "Aftertouch",
  "Control",
  "Program",
  "ChanPressure",
  "Pitch Bend",
  "SysEx"
};

const char* messageChannels[]={
  "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "Any"
};

const char* specificControls[18]={
  "Instrument",
  "Volume",
  "Effect 1 type",
  "Effect 1 value",
  "Effect 2 type",
  "Effect 2 value",
  "Effect 3 type",
  "Effect 3 value",
  "Effect 4 type",
  "Effect 4 value",
  "Effect 5 type",
  "Effect 5 value",
  "Effect 6 type",
  "Effect 6 value",
  "Effect 7 type",
  "Effect 7 value",
  "Effect 8 type",
  "Effect 8 value"
};

#define SAMPLE_RATE_SELECTABLE(x) \
  if (ImGui::Selectable(#x,settings.audioRate==x)) { \
    settings.audioRate=x; \
  }

#define BUFFER_SIZE_SELECTABLE(x) \
  if (ImGui::Selectable(#x,settings.audioBufSize==x)) { \
    settings.audioBufSize=x; \
  }

#define UI_COLOR_CONFIG(what,label) \
  ImGui::ColorEdit4(label "##CC_" #what,(float*)&uiColors[what]);

#define KEYBIND_CONFIG_BEGIN(id) \
  if (ImGui::BeginTable(id,2)) {

#define KEYBIND_CONFIG_END \
    ImGui::EndTable(); \
  }

#define UI_KEYBIND_CONFIG(what) \
  ImGui::TableNextRow(); \
  ImGui::TableNextColumn(); \
  ImGui::TextUnformatted(guiActions[what].friendlyName); \
  ImGui::TableNextColumn(); \
  if (ImGui::Button(fmt::sprintf("%s##KC_" #what,(bindSetPending && bindSetTarget==what)?"Press key...":getKeyName(actionKeys[what])).c_str())) { \
    promptKey(what); \
  } \
  if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) actionKeys[what]=0;

String stripName(String what) {
  String ret;
  for (char& i: what) {
    if ((i>='A' && i<='Z') || (i>='a' && i<='z') || (i>='0' && i<='9')) {
      ret+=i;
    } else {
      ret+='-';
    }
  }
  return ret;
}

void FurnaceGUI::promptKey(int which) {
  bindSetTarget=which;
  bindSetActive=true;
  bindSetPending=true;
  bindSetPrevValue=actionKeys[which];
  actionKeys[which]=0;
}

struct MappedInput {
  int scan;
  int val;
  MappedInput():
    scan(SDL_SCANCODE_UNKNOWN), val(0) {}
  MappedInput(int s, int v):
    scan(s), val(v) {}
};

void FurnaceGUI::drawSettings() {
  if (nextWindow==GUI_WINDOW_SETTINGS) {
    settingsOpen=true;
    ImGui::SetNextWindowFocus();
    nextWindow=GUI_WINDOW_NOTHING;
  }
  if (!settingsOpen) return;
  if (ImGui::Begin("Settings",NULL,ImGuiWindowFlags_NoDocking)) {
    if (ImGui::BeginTabBar("settingsTab")) {
      if (ImGui::BeginTabItem("General")) {
        ImGui::Text("Workspace layout");
        if (ImGui::Button("Import")) {
          openFileDialog(GUI_FILE_IMPORT_LAYOUT);
        }
        ImGui::SameLine();
        if (ImGui::Button("Export")) {
          openFileDialog(GUI_FILE_EXPORT_LAYOUT);
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
          showWarning("Are you sure you want to reset the workspace layout?",GUI_WARN_RESET_LAYOUT);
        }
        ImGui::Separator();
        ImGui::Text("Toggle channel solo on:");
        if (ImGui::RadioButton("Right-click or double-click##soloA",settings.soloAction==0)) {
          settings.soloAction=0;
        }
        if (ImGui::RadioButton("Right-click##soloR",settings.soloAction==1)) {
          settings.soloAction=1;
        }
        if (ImGui::RadioButton("Double-click##soloD",settings.soloAction==2)) {
          settings.soloAction=2;
        }

        bool pullDeleteBehaviorB=settings.pullDeleteBehavior;
        if (ImGui::Checkbox("Move cursor up on backspace-delete",&pullDeleteBehaviorB)) {
          settings.pullDeleteBehavior=pullDeleteBehaviorB;
        }

        bool stepOnDeleteB=settings.stepOnDelete;
        if (ImGui::Checkbox("Move cursor by edit step on delete",&stepOnDeleteB)) {
          settings.stepOnDelete=stepOnDeleteB;
        }

        bool effectDeletionAltersValueB=settings.effectDeletionAltersValue;
        if (ImGui::Checkbox("Delete effect value when deleting effect",&effectDeletionAltersValueB)) {
          settings.effectDeletionAltersValue=effectDeletionAltersValueB;
        }

        bool stepOnInsertB=settings.stepOnInsert;
        if (ImGui::Checkbox("Move cursor by edit step on insert (push)",&stepOnInsertB)) {
          settings.stepOnInsert=stepOnInsertB;
        }

        bool cursorPastePosB=settings.cursorPastePos;
        if (ImGui::Checkbox("Move cursor to end of clipboard content when pasting",&cursorPastePosB)) {
          settings.cursorPastePos=cursorPastePosB;
        }

        bool allowEditDockingB=settings.allowEditDocking;
        if (ImGui::Checkbox("Allow docking editors",&allowEditDockingB)) {
          settings.allowEditDocking=allowEditDockingB;
        }

        bool avoidRaisingPatternB=settings.avoidRaisingPattern;
        if (ImGui::Checkbox("Don't raise pattern editor on click",&avoidRaisingPatternB)) {
          settings.avoidRaisingPattern=avoidRaisingPatternB;
        }

        bool insFocusesPatternB=settings.insFocusesPattern;
        if (ImGui::Checkbox("Focus pattern editor when selecting instrument",&insFocusesPatternB)) {
          settings.insFocusesPattern=insFocusesPatternB;
        }

        bool restartOnFlagChangeB=settings.restartOnFlagChange;
        if (ImGui::Checkbox("Restart song when changing system properties",&restartOnFlagChangeB)) {
          settings.restartOnFlagChange=restartOnFlagChangeB;
        }

        bool sysFileDialogB=settings.sysFileDialog;
        if (ImGui::Checkbox("Use system file picker",&sysFileDialogB)) {
          settings.sysFileDialog=sysFileDialogB;
        }

        ImGui::Text("Wrap pattern cursor horizontally:");
        if (ImGui::RadioButton("No##wrapH0",settings.wrapHorizontal==0)) {
          settings.wrapHorizontal=0;
        }
        if (ImGui::RadioButton("Yes##wrapH1",settings.wrapHorizontal==1)) {
          settings.wrapHorizontal=1;
        }
        if (ImGui::RadioButton("Yes, and move to next/prev row##wrapH2",settings.wrapHorizontal==2)) {
          settings.wrapHorizontal=2;
        }

        ImGui::Text("Wrap pattern cursor vertically:");
        if (ImGui::RadioButton("No##wrapV0",settings.wrapVertical==0)) {
          settings.wrapVertical=0;
        }
        if (ImGui::RadioButton("Yes##wrapV1",settings.wrapVertical==1)) {
          settings.wrapVertical=1;
        }
        if (ImGui::RadioButton("Yes, and move to next/prev pattern##wrapV2",settings.wrapVertical==2)) {
          settings.wrapVertical=2;
        }

        ImGui::Text("Cursor movement keys behavior:");
        if (ImGui::RadioButton("Move by one##cmk0",settings.scrollStep==0)) {
          settings.scrollStep=0;
        }
        if (ImGui::RadioButton("Move by Edit Step##cmk1",settings.scrollStep==1)) {
          settings.scrollStep=1;
        }

        ImGui::Text("Effect input cursor behavior:");
        if (ImGui::RadioButton("Move down##eicb0",settings.effectCursorDir==0)) {
          settings.effectCursorDir=0;
        }
        if (ImGui::RadioButton("Move to effect value (otherwise move down)##eicb1",settings.effectCursorDir==1)) {
          settings.effectCursorDir=1;
        }
        if (ImGui::RadioButton("Move to effect value/next effect and wrap around##eicb2",settings.effectCursorDir==2)) {
          settings.effectCursorDir=2;
        }

        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Audio/MIDI")) {
        ImGui::Text("Backend");
        ImGui::SameLine();
        ImGui::Combo("##Backend",&settings.audioEngine,audioBackends,2);

        ImGui::Text("Device");
        ImGui::SameLine();
        String audioDevName=settings.audioDevice.empty()?"<System default>":settings.audioDevice;
        if (ImGui::BeginCombo("##AudioDevice",audioDevName.c_str())) {
          if (ImGui::Selectable("<System default>",settings.audioDevice.empty())) {
            settings.audioDevice="";
          }
          for (String& i: e->getAudioDevices()) {
            if (ImGui::Selectable(i.c_str(),i==settings.audioDevice)) {
              settings.audioDevice=i;
            }
          }
          ImGui::EndCombo();
        }

        ImGui::Text("Sample rate");
        ImGui::SameLine();
        String sr=fmt::sprintf("%d",settings.audioRate);
        if (ImGui::BeginCombo("##SampleRate",sr.c_str())) {
          SAMPLE_RATE_SELECTABLE(8000);
          SAMPLE_RATE_SELECTABLE(16000);
          SAMPLE_RATE_SELECTABLE(22050);
          SAMPLE_RATE_SELECTABLE(32000);
          SAMPLE_RATE_SELECTABLE(44100);
          SAMPLE_RATE_SELECTABLE(48000);
          SAMPLE_RATE_SELECTABLE(88200);
          SAMPLE_RATE_SELECTABLE(96000);
          SAMPLE_RATE_SELECTABLE(192000);
          ImGui::EndCombo();
        }

        ImGui::Text("Buffer size");
        ImGui::SameLine();
        String bs=fmt::sprintf("%d (latency: ~%.1fms)",settings.audioBufSize,2000.0*(double)settings.audioBufSize/(double)MAX(1,settings.audioRate));
        if (ImGui::BeginCombo("##BufferSize",bs.c_str())) {
          BUFFER_SIZE_SELECTABLE(64);
          BUFFER_SIZE_SELECTABLE(128);
          BUFFER_SIZE_SELECTABLE(256);
          BUFFER_SIZE_SELECTABLE(512);
          BUFFER_SIZE_SELECTABLE(1024);
          BUFFER_SIZE_SELECTABLE(2048);
          ImGui::EndCombo();
        }
        
        ImGui::Text("Quality");
        ImGui::SameLine();
        ImGui::Combo("##Quality",&settings.audioQuality,audioQualities,2);

        bool forceMonoB=settings.forceMono;
        if (ImGui::Checkbox("Force mono audio",&forceMonoB)) {
          settings.forceMono=forceMonoB;
        }

        TAAudioDesc& audioWant=e->getAudioDescWant();
        TAAudioDesc& audioGot=e->getAudioDescGot();

        ImGui::Text("want: %d samples @ %.0fHz",audioWant.bufsize,audioWant.rate);
        ImGui::Text("got: %d samples @ %.0fHz",audioGot.bufsize,audioGot.rate);

        ImGui::Separator();

        ImGui::Text("MIDI input");
        ImGui::SameLine();
        String midiInName=settings.midiInDevice.empty()?"<disabled>":settings.midiInDevice;
        bool hasToReloadMidi=false;
        if (ImGui::BeginCombo("##MidiInDevice",midiInName.c_str())) {
          if (ImGui::Selectable("<disabled>",settings.midiInDevice.empty())) {
            settings.midiInDevice="";
            hasToReloadMidi=true;
          }
          for (String& i: e->getMidiIns()) {
            if (ImGui::Selectable(i.c_str(),i==settings.midiInDevice)) {
              settings.midiInDevice=i;
              hasToReloadMidi=true;
            }
          }
          ImGui::EndCombo();
        }

        if (hasToReloadMidi) {
          midiMap.read(e->getConfigPath()+DIR_SEPARATOR_STR+"midiIn_"+stripName(settings.midiInDevice)+".cfg"); 
          midiMap.compile();
        }

        ImGui::Text("MIDI output");
        ImGui::SameLine();
        String midiOutName=settings.midiOutDevice.empty()?"<disabled>":settings.midiOutDevice;
        if (ImGui::BeginCombo("##MidiOutDevice",midiOutName.c_str())) {
          if (ImGui::Selectable("<disabled>",settings.midiOutDevice.empty())) {
            settings.midiOutDevice="";
          }
          for (String& i: e->getMidiIns()) {
            if (ImGui::Selectable(i.c_str(),i==settings.midiOutDevice)) {
              settings.midiOutDevice=i;
            }
          }
          ImGui::EndCombo();
        }

        if (ImGui::TreeNode("MIDI input settings")) {
          ImGui::Checkbox("Note input",&midiMap.noteInput);
          ImGui::Checkbox("Velocity input",&midiMap.volInput);
          // TODO
          //ImGui::Checkbox("Use raw velocity value (don't map from linear to log)",&midiMap.rawVolume);
          //ImGui::Checkbox("Polyphonic/chord input",&midiMap.polyInput);
          ImGui::Checkbox("Map MIDI channels to direct channels",&midiMap.directChannel);
          ImGui::Checkbox("Program change is instrument selection",&midiMap.programChange);
          //ImGui::Checkbox("Listen to MIDI clock",&midiMap.midiClock);
          //ImGui::Checkbox("Listen to MIDI time code",&midiMap.midiTimeCode);
          ImGui::Combo("Value input style",&midiMap.valueInputStyle,valueInputStyles,7);
          if (midiMap.valueInputStyle>3) {
            if (midiMap.valueInputStyle==6) {
              if (ImGui::InputInt("Control##valueCCS",&midiMap.valueInputControlSingle,1,16)) {
                if (midiMap.valueInputControlSingle<0) midiMap.valueInputControlSingle=0;
                if (midiMap.valueInputControlSingle>127) midiMap.valueInputControlSingle=127;
              }
            } else {
              if (ImGui::InputInt((midiMap.valueInputStyle==4)?"CC of upper nibble##valueCC1":"MSB CC##valueCC1",&midiMap.valueInputControlMSB,1,16)) {
                if (midiMap.valueInputControlMSB<0) midiMap.valueInputControlMSB=0;
                if (midiMap.valueInputControlMSB>127) midiMap.valueInputControlMSB=127;
              }
              if (ImGui::InputInt((midiMap.valueInputStyle==4)?"CC of lower nibble##valueCC2":"LSB CC##valueCC2",&midiMap.valueInputControlLSB,1,16)) {
                if (midiMap.valueInputControlLSB<0) midiMap.valueInputControlLSB=0;
                if (midiMap.valueInputControlLSB>127) midiMap.valueInputControlLSB=127;
              }
            }
          }
          if (ImGui::TreeNode("Per-column control change")) {
            for (int i=0; i<18; i++) {
              ImGui::PushID(i);
              ImGui::Combo(specificControls[i],&midiMap.valueInputSpecificStyle[i],valueSInputStyles,4);
              if (midiMap.valueInputSpecificStyle[i]>0) {
                ImGui::Indent();
                if (midiMap.valueInputSpecificStyle[i]==3) {
                  if (ImGui::InputInt("Control##valueCCS",&midiMap.valueInputSpecificSingle[i],1,16)) {
                    if (midiMap.valueInputSpecificSingle[i]<0) midiMap.valueInputSpecificSingle[i]=0;
                    if (midiMap.valueInputSpecificSingle[i]>127) midiMap.valueInputSpecificSingle[i]=127;
                  }
                } else {
                  if (ImGui::InputInt((midiMap.valueInputSpecificStyle[i]==4)?"CC of upper nibble##valueCC1":"MSB CC##valueCC1",&midiMap.valueInputSpecificMSB[i],1,16)) {
                    if (midiMap.valueInputSpecificMSB[i]<0) midiMap.valueInputSpecificMSB[i]=0;
                    if (midiMap.valueInputSpecificMSB[i]>127) midiMap.valueInputSpecificMSB[i]=127;
                  }
                  if (ImGui::InputInt((midiMap.valueInputSpecificStyle[i]==4)?"CC of lower nibble##valueCC2":"LSB CC##valueCC2",&midiMap.valueInputSpecificLSB[i],1,16)) {
                    if (midiMap.valueInputSpecificLSB[i]<0) midiMap.valueInputSpecificLSB[i]=0;
                    if (midiMap.valueInputSpecificLSB[i]>127) midiMap.valueInputSpecificLSB[i]=127;
                  }
                }
                ImGui::Unindent();
              }
              ImGui::PopID();
            }
            ImGui::TreePop();
          }
          if (ImGui::SliderFloat("Volume curve",&midiMap.volExp,0.01,8.0,"%.2f")) {
            if (midiMap.volExp<0.01) midiMap.volExp=0.01;
            if (midiMap.volExp>8.0) midiMap.volExp=8.0;
          } rightClickable
          float curve[128];
          for (int i=0; i<128; i++) {
            curve[i]=(int)(pow((double)i/127.0,midiMap.volExp)*127.0);
          }
          ImGui::PlotLines("##VolCurveDisplay",curve,128,0,"Volume curve",0.0,127.0,ImVec2(200.0f*dpiScale,200.0f*dpiScale));

          ImGui::Text("Actions:");
          ImGui::SameLine();
          if (ImGui::Button(ICON_FA_PLUS "##AddAction")) {
            midiMap.binds.push_back(MIDIBind());
          }
          ImGui::SameLine();
          if (ImGui::Button(ICON_FA_EXTERNAL_LINK "##AddLearnAction")) {
            midiMap.binds.push_back(MIDIBind());
            learning=midiMap.binds.size()-1;
          }
          if (learning!=-1) {
            ImGui::SameLine();
            ImGui::Text("(learning! press a button or move a slider/knob/something on your device.)");
          }

          if (ImGui::BeginTable("MIDIActions",7)) {
            ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthStretch,0.2);
            ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthStretch,0.1);
            ImGui::TableSetupColumn("c2",ImGuiTableColumnFlags_WidthStretch,0.3);
            ImGui::TableSetupColumn("c3",ImGuiTableColumnFlags_WidthStretch,0.2);
            ImGui::TableSetupColumn("c4",ImGuiTableColumnFlags_WidthStretch,0.5);
            ImGui::TableSetupColumn("c5",ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("c6",ImGuiTableColumnFlags_WidthFixed);

            ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
            ImGui::TableNextColumn();
            ImGui::Text("Type");
            ImGui::TableNextColumn();
            ImGui::Text("Channel");
            ImGui::TableNextColumn();
            ImGui::Text("Note/Control");
            ImGui::TableNextColumn();
            ImGui::Text("Velocity/Value");
            ImGui::TableNextColumn();
            ImGui::Text("Action");
            ImGui::TableNextColumn();
            ImGui::Text("Learn");
            ImGui::TableNextColumn();
            ImGui::Text("Remove");

            for (size_t i=0; i<midiMap.binds.size(); i++) {
              MIDIBind& bind=midiMap.binds[i];
              char bindID[1024];
              ImGui::PushID(i);
              ImGui::TableNextRow();

              ImGui::TableNextColumn();
              ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
              if (ImGui::BeginCombo("##BType",messageTypes[bind.type])) {
                for (int j=8; j<15; j++) {
                  if (ImGui::Selectable(messageTypes[j],bind.type==j)) {
                    bind.type=j;
                  }
                }
                ImGui::EndCombo();
              }

              ImGui::TableNextColumn();
              ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
              if (ImGui::BeginCombo("##BChannel",messageChannels[bind.channel])) {
                if (ImGui::Selectable(messageChannels[16],bind.channel==16)) {
                  bind.channel=16;
                }
                for (int j=0; j<16; j++) {
                  if (ImGui::Selectable(messageChannels[j],bind.channel==j)) {
                    bind.channel=j;
                  }
                }
                ImGui::EndCombo();
              }

              ImGui::TableNextColumn();
              if (bind.data1==128) {
                snprintf(bindID,1024,"Any");
              } else {
                snprintf(bindID,1024,"%d (0x%.2X, %s)",bind.data1,bind.data1,noteNames[bind.data1+60]);
              }
              ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
              if (ImGui::BeginCombo("##BValue1",bindID)) {
                if (ImGui::Selectable("Any",bind.data1==128)) {
                  bind.data1=128;
                }
                for (int j=0; j<128; j++) {
                  snprintf(bindID,1024,"%d (0x%.2X, %s)##BV1_%d",j,j,noteNames[j+60],j);
                  if (ImGui::Selectable(bindID,bind.data1==j)) {
                    bind.data1=j;
                  }
                }
                ImGui::EndCombo();
              }

              ImGui::TableNextColumn();
              if (bind.data2==128) {
                snprintf(bindID,1024,"Any");
              } else {
                snprintf(bindID,1024,"%d (0x%.2X)",bind.data2,bind.data2);
              }
              ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
              if (ImGui::BeginCombo("##BValue2",bindID)) {
                if (ImGui::Selectable("Any",bind.data2==128)) {
                  bind.data2=128;
                }
                for (int j=0; j<128; j++) {
                  snprintf(bindID,1024,"%d (0x%.2X)##BV2_%d",j,j,j);
                  if (ImGui::Selectable(bindID,bind.data2==j)) {
                    bind.data2=j;
                  }
                }
                ImGui::EndCombo();
              }

              ImGui::TableNextColumn();
              ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
              if (ImGui::BeginCombo("##BAction",(bind.action==0)?"--none--":guiActions[bind.action].friendlyName)) {
                if (ImGui::Selectable("--none--",bind.action==0)) {
                  bind.action=0;
                }
                for (int j=0; j<GUI_ACTION_MAX; j++) {
                  if (strcmp(guiActions[j].friendlyName,"")==0) continue;
                  if (strstr(guiActions[j].friendlyName,"---")==guiActions[j].friendlyName) {
                    ImGui::TextUnformatted(guiActions[j].friendlyName);
                  } else {
                    snprintf(bindID,1024,"%s##BA_%d",guiActions[j].friendlyName,j);
                    if (ImGui::Selectable(bindID,bind.action==j)) {
                      bind.action=j;
                    }
                  }
                }
                ImGui::EndCombo();
              }

              ImGui::TableNextColumn();
              if (ImGui::Button((learning==(int)i)?("waiting...##BLearn"):(ICON_FA_SQUARE_O "##BLearn"))) {
                if (learning==(int)i) {
                  learning=-1;
                } else {
                  learning=i;
                }
              }

              ImGui::TableNextColumn();
              if (ImGui::Button(ICON_FA_TIMES "##BRemove")) {
                midiMap.binds.erase(midiMap.binds.begin()+i);
                if (learning==(int)i) learning=-1;
                i--;
              }

              ImGui::PopID();
            }
            ImGui::EndTable();
          }

          ImGui::TreePop();
        }

        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Emulation")) {
        ImGui::Text("Arcade/YM2151 core");
        ImGui::SameLine();
        ImGui::Combo("##ArcadeCore",&settings.arcadeCore,arcadeCores,2);

        ImGui::Text("Genesis/YM2612 core");
        ImGui::SameLine();
        ImGui::Combo("##YM2612Core",&settings.ym2612Core,ym2612Cores,2);

        ImGui::Text("SAA1099 core");
        ImGui::SameLine();
        ImGui::Combo("##SAACore",&settings.saaCore,saaCores,2);

        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Appearance")) {
        bool dpiScaleAuto=(settings.dpiScale<0.5f);
        if (ImGui::Checkbox("Automatic UI scaling factor",&dpiScaleAuto)) {
          if (dpiScaleAuto) {
            settings.dpiScale=0.0f;
          } else {
            settings.dpiScale=1.0f;
          }
        }
        if (!dpiScaleAuto) {
          if (ImGui::SliderFloat("UI scaling factor",&settings.dpiScale,1.0f,3.0f,"%.2fx")) {
            if (settings.dpiScale<0.5f) settings.dpiScale=0.5f;
            if (settings.dpiScale>3.0f) settings.dpiScale=3.0f;
          } rightClickable
        }
        ImGui::Text("Main font");
        ImGui::SameLine();
        ImGui::Combo("##MainFont",&settings.mainFont,mainFonts,7);
        if (settings.mainFont==6) {
          ImGui::InputText("##MainFontPath",&settings.mainFontPath);
          ImGui::SameLine();
          if (ImGui::Button(ICON_FA_FOLDER "##MainFontLoad")) {
            openFileDialog(GUI_FILE_LOAD_MAIN_FONT);
          }
        }
        if (ImGui::InputInt("Size##MainFontSize",&settings.mainFontSize)) {
          if (settings.mainFontSize<3) settings.mainFontSize=3;
          if (settings.mainFontSize>96) settings.mainFontSize=96;
        }
        ImGui::Text("Pattern font");
        ImGui::SameLine();
        ImGui::Combo("##PatFont",&settings.patFont,patFonts,7);
        if (settings.patFont==6) {
          ImGui::InputText("##PatFontPath",&settings.patFontPath);
          ImGui::SameLine();
          if (ImGui::Button(ICON_FA_FOLDER "##PatFontLoad")) {
            openFileDialog(GUI_FILE_LOAD_PAT_FONT);
          }
        }
        if (ImGui::InputInt("Size##PatFontSize",&settings.patFontSize)) {
          if (settings.patFontSize<3) settings.patFontSize=3;
          if (settings.patFontSize>96) settings.patFontSize=96;
        }

        bool loadJapaneseB=settings.loadJapanese;
        if (ImGui::Checkbox("Display Japanese characters",&loadJapaneseB)) {
          settings.loadJapanese=loadJapaneseB;
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip(
            "Only toggle this option if you have enough graphics memory.\n"
            "This is a temporary solution until dynamic font atlas is implemented in Dear ImGui.\n\n"
            "このオプションは、十分なグラフィックメモリがある場合にのみ切り替えてください。\n"
            "これは、Dear ImGuiにダイナミックフォントアトラスが実装されるまでの一時的な解決策です。"
          );
        }

        ImGui::Separator();

        ImGui::Text("Orders row number format:");
        if (ImGui::RadioButton("Decimal##orbD",settings.orderRowsBase==0)) {
          settings.orderRowsBase=0;
        }
        if (ImGui::RadioButton("Hexadecimal##orbH",settings.orderRowsBase==1)) {
          settings.orderRowsBase=1;
        }

        ImGui::Text("Pattern row number format:");
        if (ImGui::RadioButton("Decimal##prbD",settings.patRowsBase==0)) {
          settings.patRowsBase=0;
        }
        if (ImGui::RadioButton("Hexadecimal##prbH",settings.patRowsBase==1)) {
          settings.patRowsBase=1;
        }

        ImGui::Text("FM parameter names:");
        if (ImGui::RadioButton("Friendly##fmn0",settings.fmNames==0)) {
          settings.fmNames=0;
        }
        if (ImGui::RadioButton("Technical##fmn1",settings.fmNames==1)) {
          settings.fmNames=1;
        }
        if (ImGui::RadioButton("Technical (alternate)##fmn2",settings.fmNames==2)) {
          settings.fmNames=2;
        }

        ImGui::Separator();

        ImGui::Text("Title bar:");
        if (ImGui::RadioButton("Furnace##tbar0",settings.titleBarInfo==0)) {
          settings.titleBarInfo=0;
          updateWindowTitle();
        }
        if (ImGui::RadioButton("Song Name - Furnace##tbar1",settings.titleBarInfo==1)) {
          settings.titleBarInfo=1;
          updateWindowTitle();
        }
        if (ImGui::RadioButton("file_name.fur - Furnace##tbar2",settings.titleBarInfo==2)) {
          settings.titleBarInfo=2;
          updateWindowTitle();
        }
        if (ImGui::RadioButton("/path/to/file.fur - Furnace##tbar3",settings.titleBarInfo==3)) {
          settings.titleBarInfo=3;
          updateWindowTitle();
        }

        bool titleBarSysB=settings.titleBarSys;
        if (ImGui::Checkbox("Display system name on title bar",&titleBarSysB)) {
          settings.titleBarSys=titleBarSysB;
          updateWindowTitle();
        }

        ImGui::Text("Status bar:");
        if (ImGui::RadioButton("Cursor details##sbar0",settings.statusDisplay==0)) {
          settings.statusDisplay=0;
        }
        if (ImGui::RadioButton("File path##sbar1",settings.statusDisplay==1)) {
          settings.statusDisplay=1;
        }
        if (ImGui::RadioButton("Cursor details or file path##sbar2",settings.statusDisplay==2)) {
          settings.statusDisplay=2;
        }
        if (ImGui::RadioButton("Nothing##sbar3",settings.statusDisplay==3)) {
          settings.statusDisplay=3;
        }

        ImGui::Text("Play/edit controls layout:");
        if (ImGui::RadioButton("Classic##ecl0",settings.controlLayout==0)) {
          settings.controlLayout=0;
        }
        if (ImGui::RadioButton("Compact##ecl1",settings.controlLayout==1)) {
          settings.controlLayout=1;
        }
        if (ImGui::RadioButton("Compact (vertical)##ecl2",settings.controlLayout==2)) {
          settings.controlLayout=2;
        }
        if (ImGui::RadioButton("Split##ecl3",settings.controlLayout==3)) {
          settings.controlLayout=3;
        }

        ImGui::Text("FM parameter editor layout:");
        if (ImGui::RadioButton("Modern##fml0",settings.fmLayout==0)) {
          settings.fmLayout=0;
        }
        if (ImGui::RadioButton("Compact (2x2, classic)##fml1",settings.fmLayout==1)) {
          settings.fmLayout=1;
        }
        if (ImGui::RadioButton("Compact (1x4)##fml2",settings.fmLayout==2)) {
          settings.fmLayout=2;
        }
        if (ImGui::RadioButton("Compact (4x1)##fml3",settings.fmLayout==3)) {
          settings.fmLayout=3;
        }

        ImGui::Text("Position of Sustain in FM editor:");
        if (ImGui::RadioButton("Between Decay and Sustain Rate##susp0",settings.susPosition==0)) {
          settings.susPosition=0;
        }
        if (ImGui::RadioButton("After Release Rate##susp1",settings.susPosition==1)) {
          settings.susPosition=1;
        }

        bool macroViewB=settings.macroView;
        if (ImGui::Checkbox("Classic macro view (standard macros only; deprecated!)",&macroViewB)) {
          settings.macroView=macroViewB;
        }

        bool unifiedDataViewB=settings.unifiedDataView;
        if (ImGui::Checkbox("Unified instrument/wavetable/sample list",&unifiedDataViewB)) {
          settings.unifiedDataView=unifiedDataViewB;
        }

        bool chipNamesB=settings.chipNames;
        if (ImGui::Checkbox("Use chip names instead of system names",&chipNamesB)) {
          settings.chipNames=chipNamesB;
        }

        bool overflowHighlightB=settings.overflowHighlight;
        if (ImGui::Checkbox("Overflow pattern highlights",&overflowHighlightB)) {
          settings.overflowHighlight=overflowHighlightB;
        }

        bool viewPrevPatternB=settings.viewPrevPattern;
        if (ImGui::Checkbox("Display previous/next pattern",&viewPrevPatternB)) {
          settings.viewPrevPattern=viewPrevPatternB;
        }

        bool germanNotationB=settings.germanNotation;
        if (ImGui::Checkbox("Use German notation",&germanNotationB)) {
          settings.germanNotation=germanNotationB;
        }
        
        // sorry. temporarily disabled until ImGui has a way to add separators in tables arbitrarily.
        /*bool sysSeparatorsB=settings.sysSeparators;
        if (ImGui::Checkbox("Add separators between systems in Orders",&sysSeparatorsB)) {
          settings.sysSeparators=sysSeparatorsB;
        }*/

        bool partyTimeB=settings.partyTime;
        if (ImGui::Checkbox("About screen party time",&partyTimeB)) {
          settings.partyTime=partyTimeB;
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("Warning: may cause epileptic seizures.");
        }

        ImGui::Separator();

        bool roundedWindowsB=settings.roundedWindows;
        if (ImGui::Checkbox("Rounded window corners",&roundedWindowsB)) {
          settings.roundedWindows=roundedWindowsB;
        }

        bool roundedButtonsB=settings.roundedButtons;
        if (ImGui::Checkbox("Rounded buttons",&roundedButtonsB)) {
          settings.roundedButtons=roundedButtonsB;
        }

        bool roundedMenusB=settings.roundedMenus;
        if (ImGui::Checkbox("Rounded menu corners",&roundedMenusB)) {
          settings.roundedMenus=roundedMenusB;
        }

        bool frameBordersB=settings.frameBorders;
        if (ImGui::Checkbox("Borders around widgets",&frameBordersB)) {
          settings.frameBorders=frameBordersB;
        }

        ImGui::Separator();

        if (ImGui::TreeNode("Color scheme")) {
          if (ImGui::Button("Import")) {
            openFileDialog(GUI_FILE_IMPORT_COLORS);
          }
          ImGui::SameLine();
          if (ImGui::Button("Export")) {
            openFileDialog(GUI_FILE_EXPORT_COLORS);
          }
          ImGui::SameLine();
          if (ImGui::Button("Reset defaults")) {
            showWarning("Are you sure you want to reset the color scheme?",GUI_WARN_RESET_COLORS);
          }
          if (ImGui::TreeNode("General")) {
            ImGui::Text("Color scheme type:");
            if (ImGui::RadioButton("Dark##gcb0",settings.guiColorsBase==0)) {
              settings.guiColorsBase=0;
            }
            if (ImGui::RadioButton("Light##gcb1",settings.guiColorsBase==1)) {
              settings.guiColorsBase=1;
            }
            UI_COLOR_CONFIG(GUI_COLOR_BACKGROUND,"Background");
            UI_COLOR_CONFIG(GUI_COLOR_FRAME_BACKGROUND,"Window background");
            UI_COLOR_CONFIG(GUI_COLOR_MODAL_BACKDROP,"Modal backdrop");
            UI_COLOR_CONFIG(GUI_COLOR_HEADER,"Header");
            UI_COLOR_CONFIG(GUI_COLOR_TEXT,"Text");
            UI_COLOR_CONFIG(GUI_COLOR_ACCENT_PRIMARY,"Primary");
            UI_COLOR_CONFIG(GUI_COLOR_ACCENT_SECONDARY,"Secondary");
            UI_COLOR_CONFIG(GUI_COLOR_BORDER,"Border");
            UI_COLOR_CONFIG(GUI_COLOR_BORDER_SHADOW,"Border shadow");
            UI_COLOR_CONFIG(GUI_COLOR_TOGGLE_ON,"Toggle on");
            UI_COLOR_CONFIG(GUI_COLOR_TOGGLE_OFF,"Toggle off");
            UI_COLOR_CONFIG(GUI_COLOR_EDITING,"Editing");
            UI_COLOR_CONFIG(GUI_COLOR_SONG_LOOP,"Song loop");
            UI_COLOR_CONFIG(GUI_COLOR_PLAYBACK_STAT,"Playback status");
            ImGui::TreePop();
          }
          if (ImGui::TreeNode("File Picker (built-in)")) {
            UI_COLOR_CONFIG(GUI_COLOR_FILE_DIR,"Directory");
            UI_COLOR_CONFIG(GUI_COLOR_FILE_SONG_NATIVE,"Song (native)");
            UI_COLOR_CONFIG(GUI_COLOR_FILE_SONG_IMPORT,"Song (import)");
            UI_COLOR_CONFIG(GUI_COLOR_FILE_INSTR,"Instrument");
            UI_COLOR_CONFIG(GUI_COLOR_FILE_AUDIO,"Audio");
            UI_COLOR_CONFIG(GUI_COLOR_FILE_WAVE,"Wavetable");
            UI_COLOR_CONFIG(GUI_COLOR_FILE_VGM,"VGM");
            UI_COLOR_CONFIG(GUI_COLOR_FILE_FONT,"Font");
            UI_COLOR_CONFIG(GUI_COLOR_FILE_OTHER,"Other");
            ImGui::TreePop();
          }
          if (ImGui::TreeNode("Oscilloscope")) {
            UI_COLOR_CONFIG(GUI_COLOR_OSC_BORDER,"Border");
            UI_COLOR_CONFIG(GUI_COLOR_OSC_BG1,"Background (top-left)");
            UI_COLOR_CONFIG(GUI_COLOR_OSC_BG2,"Background (top-right)");
            UI_COLOR_CONFIG(GUI_COLOR_OSC_BG3,"Background (bottom-left)");
            UI_COLOR_CONFIG(GUI_COLOR_OSC_BG4,"Background (bottom-right)");
            UI_COLOR_CONFIG(GUI_COLOR_OSC_WAVE,"Waveform");
            UI_COLOR_CONFIG(GUI_COLOR_OSC_WAVE_PEAK,"Waveform (clip)");
            UI_COLOR_CONFIG(GUI_COLOR_OSC_REF,"Reference");
            UI_COLOR_CONFIG(GUI_COLOR_OSC_GUIDE,"Guide");
            ImGui::TreePop();
          }
          if (ImGui::TreeNode("Volume Meter")) {
            UI_COLOR_CONFIG(GUI_COLOR_VOLMETER_LOW,"Low");
            UI_COLOR_CONFIG(GUI_COLOR_VOLMETER_HIGH,"High");
            UI_COLOR_CONFIG(GUI_COLOR_VOLMETER_PEAK,"Clip");
            ImGui::TreePop();
          }
          if (ImGui::TreeNode("Orders")) {
            UI_COLOR_CONFIG(GUI_COLOR_ORDER_ROW_INDEX,"Order number");
            UI_COLOR_CONFIG(GUI_COLOR_ORDER_ACTIVE,"Current order background");
            UI_COLOR_CONFIG(GUI_COLOR_ORDER_SIMILAR,"Similar patterns");
            UI_COLOR_CONFIG(GUI_COLOR_ORDER_INACTIVE,"Inactive patterns");
            ImGui::TreePop();
          }
          if (ImGui::TreeNode("Macro Editor")) {
            UI_COLOR_CONFIG(GUI_COLOR_MACRO_VOLUME,"Volume");
            UI_COLOR_CONFIG(GUI_COLOR_MACRO_PITCH,"Pitch");
            UI_COLOR_CONFIG(GUI_COLOR_MACRO_WAVE,"Wave");
            UI_COLOR_CONFIG(GUI_COLOR_MACRO_OTHER,"Other");
            ImGui::TreePop();
          }
          if (ImGui::TreeNode("Instrument Types")) {
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_FM,"FM (4-operator)");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_STD,"Standard");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_GB,"Game Boy");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_C64,"C64");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_AMIGA,"Amiga/Sample");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_PCE,"PC Engine");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_AY,"AY-3-8910/SSG");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_AY8930,"AY8930");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_TIA,"TIA");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_SAA1099,"SAA1099");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_VIC,"VIC");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_PET,"PET");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_VRC6,"VRC6");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_VRC6_SAW,"VRC6 (saw)");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_OPLL,"FM (OPLL)");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_OPL,"FM (OPL)");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_FDS,"FDS");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_VBOY,"Virtual Boy");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_N163,"Namco 163");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_SCC,"Konami SCC");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_OPZ,"FM (OPZ)");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_POKEY,"POKEY");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_BEEPER,"PC Beeper");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_SWAN,"WonderSwan");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_MIKEY,"Lynx");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_VERA,"VERA");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_X1_010,"X1-010");
            UI_COLOR_CONFIG(GUI_COLOR_INSTR_UNKNOWN,"Other/Unknown");
            ImGui::TreePop();
          }
          if (ImGui::TreeNode("Channel")) {
            UI_COLOR_CONFIG(GUI_COLOR_CHANNEL_FM,"FM");
            UI_COLOR_CONFIG(GUI_COLOR_CHANNEL_PULSE,"Pulse");
            UI_COLOR_CONFIG(GUI_COLOR_CHANNEL_NOISE,"Noise");
            UI_COLOR_CONFIG(GUI_COLOR_CHANNEL_PCM,"PCM");
            UI_COLOR_CONFIG(GUI_COLOR_CHANNEL_WAVE,"Wave");
            UI_COLOR_CONFIG(GUI_COLOR_CHANNEL_OP,"FM operator");
            UI_COLOR_CONFIG(GUI_COLOR_CHANNEL_MUTED,"Muted");
            ImGui::TreePop();
          }
          if (ImGui::TreeNode("Pattern")) {
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_PLAY_HEAD,"Playhead");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_CURSOR,"Cursor");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_CURSOR_HOVER,"Cursor (hovered)");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_CURSOR_ACTIVE,"Cursor (clicked)");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_SELECTION,"Selection");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_SELECTION_HOVER,"Selection (hovered)");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_SELECTION_ACTIVE,"Selection (clicked)");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_HI_1,"Highlight 1");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_HI_2,"Highlight 2");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_ROW_INDEX,"Row number");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_ROW_INDEX_HI1,"Row number (highlight 1)");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_ROW_INDEX_HI2,"Row number (highlight 2)");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_ACTIVE,"Note");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_ACTIVE_HI1,"Note (highlight 1)");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_ACTIVE_HI2,"Note (highlight 2)");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_INACTIVE,"Blank");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_INACTIVE_HI1,"Blank (highlight 1)");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_INACTIVE_HI2,"Blank (highlight 2)");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_INS,"Instrument");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_INS_WARN,"Instrument (invalid type)");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_INS_ERROR,"Instrument (out of range)");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_VOLUME_MIN,"Volume (0%)");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_VOLUME_HALF,"Volume (50%)");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_VOLUME_MAX,"Volume (100%)");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_EFFECT_INVALID,"Invalid effect");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_EFFECT_PITCH,"Pitch effect");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_EFFECT_VOLUME,"Volume effect");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_EFFECT_PANNING,"Panning effect");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_EFFECT_SONG,"Song effect");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_EFFECT_TIME,"Time effect");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_EFFECT_SPEED,"Speed effect");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_EFFECT_SYS_PRIMARY,"Primary system effect");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_EFFECT_SYS_SECONDARY,"Secondary system effect");
            UI_COLOR_CONFIG(GUI_COLOR_PATTERN_EFFECT_MISC,"Miscellaneous");
            UI_COLOR_CONFIG(GUI_COLOR_EE_VALUE,"External command output");
            ImGui::TreePop();
          }
          if (ImGui::TreeNode("Log Viewer")) {
            UI_COLOR_CONFIG(GUI_COLOR_LOGLEVEL_ERROR,"Log level: Error");
            UI_COLOR_CONFIG(GUI_COLOR_LOGLEVEL_WARNING,"Log level: Warning");
            UI_COLOR_CONFIG(GUI_COLOR_LOGLEVEL_INFO,"Log level: Info");
            UI_COLOR_CONFIG(GUI_COLOR_LOGLEVEL_DEBUG,"Log level: Debug");
            UI_COLOR_CONFIG(GUI_COLOR_LOGLEVEL_TRACE,"Log level: Trace/Verbose");
            ImGui::TreePop();
          }
          ImGui::TreePop();
        }

        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Keyboard")) {
        if (ImGui::Button("Import")) {
          openFileDialog(GUI_FILE_IMPORT_KEYBINDS);
        }
        ImGui::SameLine();
        if (ImGui::Button("Export")) {
          openFileDialog(GUI_FILE_EXPORT_KEYBINDS);
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset defaults")) {
          showWarning("Are you sure you want to reset the keyboard settings?",GUI_WARN_RESET_KEYBINDS);
        }
        if (ImGui::TreeNode("Global hotkeys")) {
          KEYBIND_CONFIG_BEGIN("keysGlobal");

          UI_KEYBIND_CONFIG(GUI_ACTION_OPEN);
          UI_KEYBIND_CONFIG(GUI_ACTION_OPEN_BACKUP);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAVE);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAVE_AS);
          UI_KEYBIND_CONFIG(GUI_ACTION_UNDO);
          UI_KEYBIND_CONFIG(GUI_ACTION_REDO);
          UI_KEYBIND_CONFIG(GUI_ACTION_PLAY_TOGGLE);
          UI_KEYBIND_CONFIG(GUI_ACTION_PLAY);
          UI_KEYBIND_CONFIG(GUI_ACTION_STOP);
          UI_KEYBIND_CONFIG(GUI_ACTION_PLAY_REPEAT);
          UI_KEYBIND_CONFIG(GUI_ACTION_PLAY_CURSOR);
          UI_KEYBIND_CONFIG(GUI_ACTION_STEP_ONE);
          UI_KEYBIND_CONFIG(GUI_ACTION_OCTAVE_UP);
          UI_KEYBIND_CONFIG(GUI_ACTION_OCTAVE_DOWN);
          UI_KEYBIND_CONFIG(GUI_ACTION_INS_UP);
          UI_KEYBIND_CONFIG(GUI_ACTION_INS_DOWN);
          UI_KEYBIND_CONFIG(GUI_ACTION_STEP_UP);
          UI_KEYBIND_CONFIG(GUI_ACTION_STEP_DOWN);
          UI_KEYBIND_CONFIG(GUI_ACTION_TOGGLE_EDIT);
          UI_KEYBIND_CONFIG(GUI_ACTION_METRONOME);
          UI_KEYBIND_CONFIG(GUI_ACTION_REPEAT_PATTERN);
          UI_KEYBIND_CONFIG(GUI_ACTION_FOLLOW_ORDERS);
          UI_KEYBIND_CONFIG(GUI_ACTION_FOLLOW_PATTERN);
          UI_KEYBIND_CONFIG(GUI_ACTION_PANIC);

          KEYBIND_CONFIG_END;
          ImGui::TreePop();
        }
        if (ImGui::TreeNode("Window activation")) {
          KEYBIND_CONFIG_BEGIN("keysWindow");

          UI_KEYBIND_CONFIG(GUI_ACTION_WINDOW_EDIT_CONTROLS);
          UI_KEYBIND_CONFIG(GUI_ACTION_WINDOW_ORDERS);
          UI_KEYBIND_CONFIG(GUI_ACTION_WINDOW_INS_LIST);
          UI_KEYBIND_CONFIG(GUI_ACTION_WINDOW_INS_EDIT);
          UI_KEYBIND_CONFIG(GUI_ACTION_WINDOW_SONG_INFO);
          UI_KEYBIND_CONFIG(GUI_ACTION_WINDOW_PATTERN);
          UI_KEYBIND_CONFIG(GUI_ACTION_WINDOW_WAVE_LIST);
          UI_KEYBIND_CONFIG(GUI_ACTION_WINDOW_WAVE_EDIT);
          UI_KEYBIND_CONFIG(GUI_ACTION_WINDOW_SAMPLE_LIST);
          UI_KEYBIND_CONFIG(GUI_ACTION_WINDOW_SAMPLE_EDIT);
          UI_KEYBIND_CONFIG(GUI_ACTION_WINDOW_ABOUT);
          UI_KEYBIND_CONFIG(GUI_ACTION_WINDOW_SETTINGS);
          UI_KEYBIND_CONFIG(GUI_ACTION_WINDOW_MIXER);
          UI_KEYBIND_CONFIG(GUI_ACTION_WINDOW_DEBUG);
          UI_KEYBIND_CONFIG(GUI_ACTION_WINDOW_OSCILLOSCOPE);
          UI_KEYBIND_CONFIG(GUI_ACTION_WINDOW_VOL_METER);
          UI_KEYBIND_CONFIG(GUI_ACTION_WINDOW_STATS);
          UI_KEYBIND_CONFIG(GUI_ACTION_WINDOW_COMPAT_FLAGS);
          UI_KEYBIND_CONFIG(GUI_ACTION_WINDOW_PIANO);
          UI_KEYBIND_CONFIG(GUI_ACTION_WINDOW_NOTES);
          UI_KEYBIND_CONFIG(GUI_ACTION_WINDOW_CHANNELS);
          UI_KEYBIND_CONFIG(GUI_ACTION_WINDOW_REGISTER_VIEW);
          UI_KEYBIND_CONFIG(GUI_ACTION_WINDOW_LOG);

          UI_KEYBIND_CONFIG(GUI_ACTION_COLLAPSE_WINDOW);
          UI_KEYBIND_CONFIG(GUI_ACTION_CLOSE_WINDOW);

          KEYBIND_CONFIG_END;
          ImGui::TreePop();
        }
        if (ImGui::TreeNode("Note input")) {
          std::vector<MappedInput> sorted;
          if (ImGui::BeginTable("keysNoteInput",4)) {
            for (std::map<int,int>::value_type& i: noteKeys) {
              std::vector<MappedInput>::iterator j;
              for (j=sorted.begin(); j!=sorted.end(); j++) {
                if (j->val>i.second) {
                  break;
                }
              }
              sorted.insert(j,MappedInput(i.first,i.second));
            }

            static char id[4096];

            ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
            ImGui::TableNextColumn();
            ImGui::Text("Key");
            ImGui::TableNextColumn();
            ImGui::Text("Type");
            ImGui::TableNextColumn();
            ImGui::Text("Value");
            ImGui::TableNextColumn();
            ImGui::Text("Remove");

            for (MappedInput& i: sorted) {
              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              ImGui::Text("%s",SDL_GetScancodeName((SDL_Scancode)i.scan));
              ImGui::TableNextColumn();
              if (i.val==102) {
                snprintf(id,4095,"Envelope release##SNType_%d",i.scan);
                if (ImGui::Button(id)) {
                  noteKeys[i.scan]=0;
                }
              } else if (i.val==101) {
                snprintf(id,4095,"Note release##SNType_%d",i.scan);
                if (ImGui::Button(id)) {
                  noteKeys[i.scan]=102;
                }
              } else if (i.val==100) {
                snprintf(id,4095,"Note off##SNType_%d",i.scan);
                if (ImGui::Button(id)) {
                  noteKeys[i.scan]=101;
                }
              } else {
                snprintf(id,4095,"Note##SNType_%d",i.scan);
                if (ImGui::Button(id)) {
                  noteKeys[i.scan]=100;
                }
              }
              ImGui::TableNextColumn();
              if (i.val<100) {
                snprintf(id,4095,"##SNValue_%d",i.scan);
                if (ImGui::InputInt(id,&i.val,1,1)) {
                  if (i.val<0) i.val=0;
                  if (i.val>96) i.val=96;
                  noteKeys[i.scan]=i.val;
                }
              }
              ImGui::TableNextColumn();
              snprintf(id,4095,ICON_FA_TIMES "##SNRemove_%d",i.scan);
              if (ImGui::Button(id)) {
                noteKeys.erase(i.scan);
              }
            }
            ImGui::EndTable();

            if (ImGui::BeginCombo("##SNAddNew","Add...")) {
              for (int i=0; i<SDL_NUM_SCANCODES; i++) {
                const char* sName=SDL_GetScancodeName((SDL_Scancode)i);
                if (sName==NULL) continue;
                if (sName[0]==0) continue;
                snprintf(id,4095,"%s##SNNewKey_%d",sName,i);
                if (ImGui::Selectable(id)) {
                  noteKeys[(SDL_Scancode)i]=0;
                }
              }
              ImGui::EndCombo();
            }
          }
          ImGui::TreePop();
        }
        if (ImGui::TreeNode("Pattern")) {
          KEYBIND_CONFIG_BEGIN("keysPattern");

          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_NOTE_UP);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_NOTE_DOWN);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_OCTAVE_UP);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_OCTAVE_DOWN);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_SELECT_ALL);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_CUT);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_COPY);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_PASTE);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_PASTE_MIX);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_PASTE_MIX_BG);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_PASTE_FLOOD);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_PASTE_OVERFLOW);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_CURSOR_UP);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_CURSOR_DOWN);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_CURSOR_LEFT);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_CURSOR_RIGHT);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_CURSOR_UP_ONE);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_CURSOR_DOWN_ONE);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_CURSOR_LEFT_CHANNEL);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_CURSOR_RIGHT_CHANNEL);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_CURSOR_PREVIOUS_CHANNEL);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_CURSOR_NEXT_CHANNEL);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_CURSOR_BEGIN);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_CURSOR_END);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_CURSOR_UP_COARSE);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_CURSOR_DOWN_COARSE);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_SELECTION_UP);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_SELECTION_DOWN);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_SELECTION_LEFT);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_SELECTION_RIGHT);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_SELECTION_UP_ONE);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_SELECTION_DOWN_ONE);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_SELECTION_BEGIN);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_SELECTION_END);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_SELECTION_UP_COARSE);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_SELECTION_DOWN_COARSE);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_DELETE);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_PULL_DELETE);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_INSERT);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_MUTE_CURSOR);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_SOLO_CURSOR);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_UNMUTE_ALL);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_NEXT_ORDER);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_PREV_ORDER);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_COLLAPSE);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_INCREASE_COLUMNS);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_DECREASE_COLUMNS);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_INTERPOLATE);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_FADE);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_INVERT_VALUES);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_FLIP_SELECTION);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_COLLAPSE_ROWS);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_EXPAND_ROWS);
          UI_KEYBIND_CONFIG(GUI_ACTION_PAT_LATCH);
          
          // TODO: collapse/expand pattern and song

          KEYBIND_CONFIG_END;
          ImGui::TreePop();
        }
        if (ImGui::TreeNode("Instrument list")) {
          KEYBIND_CONFIG_BEGIN("keysInsList");

          UI_KEYBIND_CONFIG(GUI_ACTION_INS_LIST_ADD);
          UI_KEYBIND_CONFIG(GUI_ACTION_INS_LIST_DUPLICATE);
          UI_KEYBIND_CONFIG(GUI_ACTION_INS_LIST_OPEN);
          UI_KEYBIND_CONFIG(GUI_ACTION_INS_LIST_SAVE);
          UI_KEYBIND_CONFIG(GUI_ACTION_INS_LIST_MOVE_UP);
          UI_KEYBIND_CONFIG(GUI_ACTION_INS_LIST_MOVE_DOWN);
          UI_KEYBIND_CONFIG(GUI_ACTION_INS_LIST_DELETE);
          UI_KEYBIND_CONFIG(GUI_ACTION_INS_LIST_EDIT);
          UI_KEYBIND_CONFIG(GUI_ACTION_INS_LIST_UP);
          UI_KEYBIND_CONFIG(GUI_ACTION_INS_LIST_DOWN);

          KEYBIND_CONFIG_END;
          ImGui::TreePop();
        }
        if (ImGui::TreeNode("Wavetable list")) {
          KEYBIND_CONFIG_BEGIN("keysWaveList");

          UI_KEYBIND_CONFIG(GUI_ACTION_WAVE_LIST_ADD);
          UI_KEYBIND_CONFIG(GUI_ACTION_WAVE_LIST_DUPLICATE);
          UI_KEYBIND_CONFIG(GUI_ACTION_WAVE_LIST_OPEN);
          UI_KEYBIND_CONFIG(GUI_ACTION_WAVE_LIST_SAVE);
          UI_KEYBIND_CONFIG(GUI_ACTION_WAVE_LIST_MOVE_UP);
          UI_KEYBIND_CONFIG(GUI_ACTION_WAVE_LIST_MOVE_DOWN);
          UI_KEYBIND_CONFIG(GUI_ACTION_WAVE_LIST_DELETE);
          UI_KEYBIND_CONFIG(GUI_ACTION_WAVE_LIST_EDIT);
          UI_KEYBIND_CONFIG(GUI_ACTION_WAVE_LIST_UP);
          UI_KEYBIND_CONFIG(GUI_ACTION_WAVE_LIST_DOWN);

          KEYBIND_CONFIG_END;
          ImGui::TreePop();
        }
        if (ImGui::TreeNode("Sample list")) {
          KEYBIND_CONFIG_BEGIN("keysSampleList");

          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_LIST_ADD);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_LIST_DUPLICATE);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_LIST_OPEN);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_LIST_SAVE);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_LIST_MOVE_UP);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_LIST_MOVE_DOWN);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_LIST_DELETE);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_LIST_EDIT);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_LIST_UP);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_LIST_DOWN);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_LIST_PREVIEW);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_LIST_STOP_PREVIEW);

          KEYBIND_CONFIG_END;
          ImGui::TreePop();
        }
        if (ImGui::TreeNode("Orders")) {
          KEYBIND_CONFIG_BEGIN("keysOrders");

          UI_KEYBIND_CONFIG(GUI_ACTION_ORDERS_UP);
          UI_KEYBIND_CONFIG(GUI_ACTION_ORDERS_DOWN);
          UI_KEYBIND_CONFIG(GUI_ACTION_ORDERS_LEFT);
          UI_KEYBIND_CONFIG(GUI_ACTION_ORDERS_RIGHT);
          UI_KEYBIND_CONFIG(GUI_ACTION_ORDERS_INCREASE);
          UI_KEYBIND_CONFIG(GUI_ACTION_ORDERS_DECREASE);
          UI_KEYBIND_CONFIG(GUI_ACTION_ORDERS_EDIT_MODE);
          UI_KEYBIND_CONFIG(GUI_ACTION_ORDERS_LINK);
          UI_KEYBIND_CONFIG(GUI_ACTION_ORDERS_ADD);
          UI_KEYBIND_CONFIG(GUI_ACTION_ORDERS_DUPLICATE);
          UI_KEYBIND_CONFIG(GUI_ACTION_ORDERS_DEEP_CLONE);
          UI_KEYBIND_CONFIG(GUI_ACTION_ORDERS_DUPLICATE_END);
          UI_KEYBIND_CONFIG(GUI_ACTION_ORDERS_DEEP_CLONE_END);
          UI_KEYBIND_CONFIG(GUI_ACTION_ORDERS_REMOVE);
          UI_KEYBIND_CONFIG(GUI_ACTION_ORDERS_MOVE_UP);
          UI_KEYBIND_CONFIG(GUI_ACTION_ORDERS_MOVE_DOWN);
          UI_KEYBIND_CONFIG(GUI_ACTION_ORDERS_REPLAY);

          KEYBIND_CONFIG_END;
          ImGui::TreePop();
        }
        if (ImGui::TreeNode("Sample editor")) {
          KEYBIND_CONFIG_BEGIN("keysSampleEdit");

          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_SELECT);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_DRAW);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_CUT);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_COPY);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_PASTE);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_PASTE_REPLACE);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_PASTE_MIX);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_SELECT_ALL);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_RESIZE);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_RESAMPLE);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_AMPLIFY);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_NORMALIZE);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_FADE_IN);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_FADE_OUT);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_INSERT);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_SILENCE);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_DELETE);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_TRIM);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_REVERSE);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_INVERT);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_SIGN);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_FILTER);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_PREVIEW);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_STOP_PREVIEW);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_ZOOM_IN);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_ZOOM_OUT);
          UI_KEYBIND_CONFIG(GUI_ACTION_SAMPLE_ZOOM_AUTO);

          KEYBIND_CONFIG_END;
          ImGui::TreePop();
        }
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }
    ImGui::Separator();
    if (ImGui::Button("OK##SettingsOK")) {
      settingsOpen=false;
      willCommit=true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel##SettingsCancel")) {
      settingsOpen=false;
      syncSettings();
    }
  }
  if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) curWindow=GUI_WINDOW_SETTINGS;
  ImGui::End();
}

#define clampSetting(x,minV,maxV) \
  if (x<minV) { \
    x=minV; \
  } \
  if (x>maxV) { \
    x=maxV; \
  }

void FurnaceGUI::syncSettings() {
  settings.mainFontSize=e->getConfInt("mainFontSize",18);
  settings.patFontSize=e->getConfInt("patFontSize",18);
  settings.iconSize=e->getConfInt("iconSize",16);
  settings.audioEngine=(e->getConfString("audioEngine","SDL")=="SDL")?1:0;
  settings.audioDevice=e->getConfString("audioDevice","");
  settings.midiInDevice=e->getConfString("midiInDevice","");
  settings.midiOutDevice=e->getConfString("midiOutDevice","");
  settings.audioQuality=e->getConfInt("audioQuality",0);
  settings.audioBufSize=e->getConfInt("audioBufSize",1024);
  settings.audioRate=e->getConfInt("audioRate",44100);
  settings.arcadeCore=e->getConfInt("arcadeCore",0);
  settings.ym2612Core=e->getConfInt("ym2612Core",0);
  settings.saaCore=e->getConfInt("saaCore",1);
  settings.mainFont=e->getConfInt("mainFont",0);
  settings.patFont=e->getConfInt("patFont",0);
  settings.mainFontPath=e->getConfString("mainFontPath","");
  settings.patFontPath=e->getConfString("patFontPath","");
  settings.patRowsBase=e->getConfInt("patRowsBase",0);
  settings.orderRowsBase=e->getConfInt("orderRowsBase",1);
  settings.soloAction=e->getConfInt("soloAction",0);
  settings.pullDeleteBehavior=e->getConfInt("pullDeleteBehavior",1);
  settings.wrapHorizontal=e->getConfInt("wrapHorizontal",0);
  settings.wrapVertical=e->getConfInt("wrapVertical",0);
  settings.macroView=e->getConfInt("macroView",0);
  settings.fmNames=e->getConfInt("fmNames",0);
  settings.allowEditDocking=e->getConfInt("allowEditDocking",0);
  settings.chipNames=e->getConfInt("chipNames",0);
  settings.overflowHighlight=e->getConfInt("overflowHighlight",0);
  settings.partyTime=e->getConfInt("partyTime",0);
  settings.germanNotation=e->getConfInt("germanNotation",0);
  settings.stepOnDelete=e->getConfInt("stepOnDelete",0);
  settings.scrollStep=e->getConfInt("scrollStep",0);
  settings.sysSeparators=e->getConfInt("sysSeparators",1);
  settings.forceMono=e->getConfInt("forceMono",0);
  settings.controlLayout=e->getConfInt("controlLayout",3);
  settings.restartOnFlagChange=e->getConfInt("restartOnFlagChange",1);
  settings.statusDisplay=e->getConfInt("statusDisplay",0);
  settings.dpiScale=e->getConfFloat("dpiScale",0.0f);
  settings.viewPrevPattern=e->getConfInt("viewPrevPattern",1);
  settings.guiColorsBase=e->getConfInt("guiColorsBase",0);
  settings.avoidRaisingPattern=e->getConfInt("avoidRaisingPattern",0);
  settings.insFocusesPattern=e->getConfInt("insFocusesPattern",1);
  settings.stepOnInsert=e->getConfInt("stepOnInsert",0);
  settings.unifiedDataView=e->getConfInt("unifiedDataView",0);
  settings.sysFileDialog=e->getConfInt("sysFileDialog",1);
  settings.roundedWindows=e->getConfInt("roundedWindows",1);
  settings.roundedButtons=e->getConfInt("roundedButtons",1);
  settings.roundedMenus=e->getConfInt("roundedMenus",0);
  settings.loadJapanese=e->getConfInt("loadJapanese",0);
  settings.fmLayout=e->getConfInt("fmLayout",0);
  settings.susPosition=e->getConfInt("susPosition",0);
  settings.effectCursorDir=e->getConfInt("effectCursorDir",1);
  settings.cursorPastePos=e->getConfInt("cursorPastePos",1);
  settings.titleBarInfo=e->getConfInt("titleBarInfo",1);
  settings.titleBarSys=e->getConfInt("titleBarSys",1);
  settings.frameBorders=e->getConfInt("frameBorders",0);
  settings.effectDeletionAltersValue=e->getConfInt("effectDeletionAltersValue",1);

  clampSetting(settings.mainFontSize,2,96);
  clampSetting(settings.patFontSize,2,96);
  clampSetting(settings.iconSize,2,48);
  clampSetting(settings.audioEngine,0,1);
  clampSetting(settings.audioQuality,0,1);
  clampSetting(settings.audioBufSize,32,4096);
  clampSetting(settings.audioRate,8000,384000);
  clampSetting(settings.arcadeCore,0,1);
  clampSetting(settings.ym2612Core,0,1);
  clampSetting(settings.saaCore,0,1);
  clampSetting(settings.mainFont,0,6);
  clampSetting(settings.patFont,0,6);
  clampSetting(settings.patRowsBase,0,1);
  clampSetting(settings.orderRowsBase,0,1);
  clampSetting(settings.soloAction,0,2);
  clampSetting(settings.pullDeleteBehavior,0,1);
  clampSetting(settings.wrapHorizontal,0,2);
  clampSetting(settings.wrapVertical,0,2);
  clampSetting(settings.macroView,0,1);
  clampSetting(settings.fmNames,0,2);
  clampSetting(settings.allowEditDocking,0,1);
  clampSetting(settings.chipNames,0,1);
  clampSetting(settings.overflowHighlight,0,1);
  clampSetting(settings.partyTime,0,1);
  clampSetting(settings.germanNotation,0,1);
  clampSetting(settings.stepOnDelete,0,1);
  clampSetting(settings.scrollStep,0,1);
  clampSetting(settings.sysSeparators,0,1);
  clampSetting(settings.forceMono,0,1);
  clampSetting(settings.controlLayout,0,3);
  clampSetting(settings.statusDisplay,0,3);
  clampSetting(settings.dpiScale,0.0f,4.0f);
  clampSetting(settings.viewPrevPattern,0,1);
  clampSetting(settings.guiColorsBase,0,1);
  clampSetting(settings.avoidRaisingPattern,0,1);
  clampSetting(settings.insFocusesPattern,0,1);
  clampSetting(settings.stepOnInsert,0,1);
  clampSetting(settings.unifiedDataView,0,1);
  clampSetting(settings.sysFileDialog,0,1);
  clampSetting(settings.roundedWindows,0,1);
  clampSetting(settings.roundedButtons,0,1);
  clampSetting(settings.roundedMenus,0,1);
  clampSetting(settings.loadJapanese,0,1);
  clampSetting(settings.fmLayout,0,3);
  clampSetting(settings.susPosition,0,1);
  clampSetting(settings.effectCursorDir,0,2);
  clampSetting(settings.cursorPastePos,0,1);
  clampSetting(settings.titleBarInfo,0,3);
  clampSetting(settings.titleBarSys,0,1);
  clampSetting(settings.frameBorders,0,1);
  clampSetting(settings.effectDeletionAltersValue,0,1);

  // keybinds
  for (int i=0; i<GUI_ACTION_MAX; i++) {
    if (guiActions[i].defaultBind==-1) continue; // not a bind
    actionKeys[i]=e->getConfInt(String("keybind_GUI_ACTION_")+String(guiActions[i].name),guiActions[i].defaultBind);
  }

  decodeKeyMap(noteKeys,e->getConfString("noteKeys",DEFAULT_NOTE_KEYS));

  parseKeybinds();

  midiMap.read(e->getConfigPath()+DIR_SEPARATOR_STR+"midiIn_"+stripName(settings.midiInDevice)+".cfg"); 
  midiMap.compile();

  e->setMidiDirect(midiMap.directChannel);
}

void FurnaceGUI::commitSettings() {
  e->setConf("mainFontSize",settings.mainFontSize);
  e->setConf("patFontSize",settings.patFontSize);
  e->setConf("iconSize",settings.iconSize);
  e->setConf("audioEngine",String(audioBackends[settings.audioEngine]));
  e->setConf("audioDevice",settings.audioDevice);
  e->setConf("midiInDevice",settings.midiInDevice);
  e->setConf("midiOutDevice",settings.midiOutDevice);
  e->setConf("audioQuality",settings.audioQuality);
  e->setConf("audioBufSize",settings.audioBufSize);
  e->setConf("audioRate",settings.audioRate);
  e->setConf("arcadeCore",settings.arcadeCore);
  e->setConf("ym2612Core",settings.ym2612Core);
  e->setConf("saaCore",settings.saaCore);
  e->setConf("mainFont",settings.mainFont);
  e->setConf("patFont",settings.patFont);
  e->setConf("mainFontPath",settings.mainFontPath);
  e->setConf("patFontPath",settings.patFontPath);
  e->setConf("patRowsBase",settings.patRowsBase);
  e->setConf("orderRowsBase",settings.orderRowsBase);
  e->setConf("soloAction",settings.soloAction);
  e->setConf("pullDeleteBehavior",settings.pullDeleteBehavior);
  e->setConf("wrapHorizontal",settings.wrapHorizontal);
  e->setConf("wrapVertical",settings.wrapVertical);
  e->setConf("macroView",settings.macroView);
  e->setConf("fmNames",settings.fmNames);
  e->setConf("allowEditDocking",settings.allowEditDocking);
  e->setConf("chipNames",settings.chipNames);
  e->setConf("overflowHighlight",settings.overflowHighlight);
  e->setConf("partyTime",settings.partyTime);
  e->setConf("germanNotation",settings.germanNotation);
  e->setConf("stepOnDelete",settings.stepOnDelete);
  e->setConf("scrollStep",settings.scrollStep);
  e->setConf("sysSeparators",settings.sysSeparators);
  e->setConf("forceMono",settings.forceMono);
  e->setConf("controlLayout",settings.controlLayout);
  e->setConf("restartOnFlagChange",settings.restartOnFlagChange);
  e->setConf("statusDisplay",settings.statusDisplay);
  e->setConf("dpiScale",settings.dpiScale);
  e->setConf("viewPrevPattern",settings.viewPrevPattern);
  e->setConf("guiColorsBase",settings.guiColorsBase);
  e->setConf("avoidRaisingPattern",settings.avoidRaisingPattern);
  e->setConf("insFocusesPattern",settings.insFocusesPattern);
  e->setConf("stepOnInsert",settings.stepOnInsert);
  e->setConf("unifiedDataView",settings.unifiedDataView);
  e->setConf("sysFileDialog",settings.sysFileDialog);
  e->setConf("roundedWindows",settings.roundedWindows);
  e->setConf("roundedButtons",settings.roundedButtons);
  e->setConf("roundedMenus",settings.roundedMenus);
  e->setConf("loadJapanese",settings.loadJapanese);
  e->setConf("fmLayout",settings.fmLayout);
  e->setConf("susPosition",settings.susPosition);
  e->setConf("effectCursorDir",settings.effectCursorDir);
  e->setConf("cursorPastePos",settings.cursorPastePos);
  e->setConf("titleBarInfo",settings.titleBarInfo);
  e->setConf("titleBarSys",settings.titleBarSys);
  e->setConf("frameBorders",settings.frameBorders);
  e->setConf("effectDeletionAltersValue",settings.effectDeletionAltersValue);

  // colors
  for (int i=0; i<GUI_COLOR_MAX; i++) {
    e->setConf(guiColors[i].name,(int)ImGui::ColorConvertFloat4ToU32(uiColors[i]));
  }

  // keybinds
  for (int i=0; i<GUI_ACTION_MAX; i++) {
    if (guiActions[i].defaultBind==-1) continue; // not a bind
    e->setConf(String("keybind_GUI_ACTION_")+String(guiActions[i].name),actionKeys[i]);
  }

  parseKeybinds();

  e->setConf("noteKeys",encodeKeyMap(noteKeys));

  midiMap.compile();
  midiMap.write(e->getConfigPath()+DIR_SEPARATOR_STR+"midiIn_"+stripName(settings.midiInDevice)+".cfg");

  e->saveConf();

  if (!e->switchMaster()) {
    showError("could not initialize audio!");
  }

  ImGui::GetIO().Fonts->Clear();

  applyUISettings();

  ImGui_ImplSDLRenderer_DestroyFontsTexture();
  if (!ImGui::GetIO().Fonts->Build()) {
    logE("error while building font atlas!");
    showError("error while loading fonts! please check your settings.");
    ImGui::GetIO().Fonts->Clear();
    mainFont=ImGui::GetIO().Fonts->AddFontDefault();
    patFont=mainFont;
    ImGui_ImplSDLRenderer_DestroyFontsTexture();
    if (!ImGui::GetIO().Fonts->Build()) {
      logE("error again while building font atlas!");
    }
  }
}

bool FurnaceGUI::importColors(String path) {
  FILE* f=ps_fopen(path.c_str(),"rb");
  if (f==NULL) {
    logW("error while opening color file for import: %s",strerror(errno));
    return false;
  }
  resetColors();
  char line[4096];
  while (!feof(f)) {
    String key="";
    String value="";
    bool keyOrValue=false;
    if (fgets(line,4095,f)==NULL) {
      break;
    }
    for (char* i=line; *i; i++) {
      if (*i=='\n') continue;
      if (keyOrValue) {
        value+=*i;
      } else {
        if (*i=='=') {
          keyOrValue=true;
        } else {
          key+=*i;
        }
      }
    }
    if (keyOrValue) {
      // unoptimal
      const char* cs=key.c_str();
      bool found=false;
      for (int i=0; i<GUI_COLOR_MAX; i++) {
        try {
          if (strcmp(cs,guiColors[i].name)==0) {
            uiColors[i]=ImGui::ColorConvertU32ToFloat4(std::stoi(value));
            found=true;
            break;
          }
        } catch (std::out_of_range& e) {
          break;
        } catch (std::invalid_argument& e) {
          break;
        }
      }
      if (!found) logW("line invalid: %s",line);
    }
  }
  fclose(f);
  return true;
}

bool FurnaceGUI::exportColors(String path) {
  FILE* f=ps_fopen(path.c_str(),"wb");
  if (f==NULL) {
    logW("error while opening color file for export: %s",strerror(errno));
    return false;
  }
  for (int i=0; i<GUI_COLOR_MAX; i++) {
    if (fprintf(f,"%s=%d\n",guiColors[i].name,ImGui::ColorConvertFloat4ToU32(uiColors[i]))<0) {
      logW("error while exporting colors: %s",strerror(errno));
      break;
    }
  }
  fclose(f);
  return true;
}

bool FurnaceGUI::importKeybinds(String path) {
  FILE* f=ps_fopen(path.c_str(),"rb");
  if (f==NULL) {
    logW("error while opening keybind file for import: %s",strerror(errno));
    return false;
  }
  resetKeybinds();
  char line[4096];
  while (!feof(f)) {
    String key="";
    String value="";
    bool keyOrValue=false;
    if (fgets(line,4095,f)==NULL) {
      break;
    }
    for (char* i=line; *i; i++) {
      if (*i=='\n') continue;
      if (keyOrValue) {
        value+=*i;
      } else {
        if (*i=='=') {
          keyOrValue=true;
        } else {
          key+=*i;
        }
      }
    }
    if (keyOrValue) {
      // unoptimal
      const char* cs=key.c_str();
      bool found=false;
      for (int i=0; i<GUI_ACTION_MAX; i++) {
        try {
          if (strcmp(cs,guiActions[i].name)==0) {
            actionKeys[i]=std::stoi(value);
            found=true;
            break;
          }
        } catch (std::out_of_range& e) {
          break;
        } catch (std::invalid_argument& e) {
          break;
        }
      }
      if (!found) logW("line invalid: %s",line);
    }
  }
  fclose(f);
  return true;
}

bool FurnaceGUI::exportKeybinds(String path) {
  FILE* f=ps_fopen(path.c_str(),"wb");
  if (f==NULL) {
    logW("error while opening keybind file for export: %s",strerror(errno));
    return false;
  }
  for (int i=0; i<GUI_ACTION_MAX; i++) {
    if (guiActions[i].defaultBind==-1) continue;
    if (fprintf(f,"%s=%d\n",guiActions[i].name,actionKeys[i])<0) {
      logW("error while exporting keybinds: %s",strerror(errno));
      break;
    }
  }
  fclose(f);
  return true;
}

bool FurnaceGUI::importLayout(String path) {
  FILE* f=ps_fopen(path.c_str(),"rb");
  if (f==NULL) {
    logW("error while opening keybind file for import: %s",strerror(errno));
    return false;
  }
  if (fseek(f,0,SEEK_END)<0) {
    fclose(f);
    return false;
  }
  ssize_t len=ftell(f);
  if (len==(SIZE_MAX>>1)) {
    fclose(f);
    return false;
  }
  if (len<1) {
    if (len==0) {
      logE("that file is empty!");
      lastError="file is empty";
    } else {
      perror("tell error");
    }
    fclose(f);
    return false;
  }
  unsigned char* file=new unsigned char[len];
  if (fseek(f,0,SEEK_SET)<0) {
    perror("size error");
    lastError=fmt::sprintf("on get size: %s",strerror(errno));
    fclose(f);
    delete[] file;
    return false;
  }
  if (fread(file,1,(size_t)len,f)!=(size_t)len) {
    perror("read error");
    lastError=fmt::sprintf("on read: %s",strerror(errno));
    fclose(f);
    delete[] file;
    return false;
  }
  fclose(f);

  ImGui::LoadIniSettingsFromMemory((const char*)file,len);
  delete[] file;
  return true;
}

bool FurnaceGUI::exportLayout(String path) {
  FILE* f=ps_fopen(path.c_str(),"wb");
  if (f==NULL) {
    logW("error while opening layout file for export: %s",strerror(errno));
    return false;
  }
  size_t dataSize=0;
  const char* data=ImGui::SaveIniSettingsToMemory(&dataSize);
  if (fwrite(data,1,dataSize,f)!=dataSize) {
    logW("error while exporting layout: %s",strerror(errno));
  }
  fclose(f);
  return true;
}

void FurnaceGUI::resetColors() {
  for (int i=0; i<GUI_COLOR_MAX; i++) {
    uiColors[i]=ImGui::ColorConvertU32ToFloat4(guiColors[i].defaultColor);
  }
}

void FurnaceGUI::resetKeybinds() {
  for (int i=0; i<GUI_COLOR_MAX; i++) {
    if (guiActions[i].defaultBind==-1) continue;
    actionKeys[i]=guiActions[i].defaultBind;
  }
  parseKeybinds();
}

void FurnaceGUI::parseKeybinds() {
  actionMapGlobal.clear();
  actionMapPat.clear();
  actionMapInsList.clear();
  actionMapWaveList.clear();
  actionMapSampleList.clear();
  actionMapSample.clear();
  actionMapOrders.clear();

  for (int i=GUI_ACTION_GLOBAL_MIN+1; i<GUI_ACTION_GLOBAL_MAX; i++) {
    if (actionKeys[i]&FURK_MASK) {
      actionMapGlobal[actionKeys[i]]=i;
    }
  }

  for (int i=GUI_ACTION_PAT_MIN+1; i<GUI_ACTION_PAT_MAX; i++) {
    if (actionKeys[i]&FURK_MASK) {
      actionMapPat[actionKeys[i]]=i;
    }
  }

  for (int i=GUI_ACTION_INS_LIST_MIN+1; i<GUI_ACTION_INS_LIST_MAX; i++) {
    if (actionKeys[i]&FURK_MASK) {
      actionMapInsList[actionKeys[i]]=i;
    }
  }

  for (int i=GUI_ACTION_WAVE_LIST_MIN+1; i<GUI_ACTION_WAVE_LIST_MAX; i++) {
    if (actionKeys[i]&FURK_MASK) {
      actionMapWaveList[actionKeys[i]]=i;
    }
  }

  for (int i=GUI_ACTION_SAMPLE_LIST_MIN+1; i<GUI_ACTION_SAMPLE_LIST_MAX; i++) {
    if (actionKeys[i]&FURK_MASK) {
      actionMapSampleList[actionKeys[i]]=i;
    }
  }

  for (int i=GUI_ACTION_SAMPLE_MIN+1; i<GUI_ACTION_SAMPLE_MAX; i++) {
    if (actionKeys[i]&FURK_MASK) {
      actionMapSample[actionKeys[i]]=i;
    }
  }

  for (int i=GUI_ACTION_ORDERS_MIN+1; i<GUI_ACTION_ORDERS_MAX; i++) {
    if (actionKeys[i]&FURK_MASK) {
      actionMapOrders[actionKeys[i]]=i;
    }
  }
}

#define IGFD_FileStyleByExtension IGFD_FileStyleByExtention

#ifdef _WIN32
#define SYSTEM_FONT_PATH_1 "C:\\Windows\\Fonts\\segoeui.ttf"
#define SYSTEM_FONT_PATH_2 "C:\\Windows\\Fonts\\tahoma.ttf"
// TODO!
#define SYSTEM_FONT_PATH_3 "C:\\Windows\\Fonts\\tahoma.ttf"
// TODO!
#define SYSTEM_PAT_FONT_PATH_1 "C:\\Windows\\Fonts\\consola.ttf"
#define SYSTEM_PAT_FONT_PATH_2 "C:\\Windows\\Fonts\\cour.ttf"
// GOOD LUCK WITH THIS ONE - UNTESTED
#define SYSTEM_PAT_FONT_PATH_3 "C:\\Windows\\Fonts\\vgasys.fon"
#elif defined(__APPLE__)
#define SYSTEM_FONT_PATH_1 "/System/Library/Fonts/SFAANS.ttf"
#define SYSTEM_FONT_PATH_2 "/System/Library/Fonts/Helvetica.ttc"
#define SYSTEM_FONT_PATH_3 "/System/Library/Fonts/Helvetica.dfont"
#define SYSTEM_PAT_FONT_PATH_1 "/System/Library/Fonts/SFNSMono.ttf"
#define SYSTEM_PAT_FONT_PATH_2 "/System/Library/Fonts/Courier New.ttf"
#define SYSTEM_PAT_FONT_PATH_3 "/System/Library/Fonts/Courier New.ttf"
#else
#define SYSTEM_FONT_PATH_1 "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
#define SYSTEM_FONT_PATH_2 "/usr/share/fonts/TTF/DejaVuSans.ttf"
#define SYSTEM_FONT_PATH_3 "/usr/share/fonts/ubuntu/Ubuntu-R.ttf"
#define SYSTEM_PAT_FONT_PATH_1 "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"
#define SYSTEM_PAT_FONT_PATH_2 "/usr/share/fonts/TTF/DejaVuSansMono.ttf"
#define SYSTEM_PAT_FONT_PATH_3 "/usr/share/fonts/ubuntu/UbuntuMono-R.ttf"
#endif

void FurnaceGUI::applyUISettings() {
  ImGuiStyle sty;
  if (settings.guiColorsBase) {
    ImGui::StyleColorsLight(&sty);
  } else {
    ImGui::StyleColorsDark(&sty);
  }

  if (settings.dpiScale>=0.5f) dpiScale=settings.dpiScale;

  // colors
  for (int i=0; i<GUI_COLOR_MAX; i++) {
    uiColors[i]=ImGui::ColorConvertU32ToFloat4(e->getConfInt(guiColors[i].name,guiColors[i].defaultColor));
  }

  for (int i=0; i<64; i++) {
    ImVec4 col1=uiColors[GUI_COLOR_PATTERN_VOLUME_MIN];
    ImVec4 col2=uiColors[GUI_COLOR_PATTERN_VOLUME_HALF];
    ImVec4 col3=uiColors[GUI_COLOR_PATTERN_VOLUME_MAX];
    volColors[i]=ImVec4(col1.x+((col2.x-col1.x)*float(i)/64.0f),
                        col1.y+((col2.y-col1.y)*float(i)/64.0f),
                        col1.z+((col2.z-col1.z)*float(i)/64.0f),
                        1.0f);
    volColors[i+64]=ImVec4(col2.x+((col3.x-col2.x)*float(i)/64.0f),
                           col2.y+((col3.y-col2.y)*float(i)/64.0f),
                           col2.z+((col3.z-col2.z)*float(i)/64.0f),
                           1.0f);
  }

  float hue, sat, val;

  ImVec4 primaryActive=uiColors[GUI_COLOR_ACCENT_PRIMARY];
  ImVec4 primaryHover, primary;
  primaryHover.w=primaryActive.w;
  primary.w=primaryActive.w;
  ImGui::ColorConvertRGBtoHSV(primaryActive.x,primaryActive.y,primaryActive.z,hue,sat,val);
  if (settings.guiColorsBase) {
    primary=primaryActive;
    ImGui::ColorConvertHSVtoRGB(hue,sat*0.9,val*0.9,primaryHover.x,primaryHover.y,primaryHover.z);
    ImGui::ColorConvertHSVtoRGB(hue,sat,val*0.5,primaryActive.x,primaryActive.y,primaryActive.z);
  } else {
    ImGui::ColorConvertHSVtoRGB(hue,sat*0.9,val*0.5,primaryHover.x,primaryHover.y,primaryHover.z);
    ImGui::ColorConvertHSVtoRGB(hue,sat*0.8,val*0.35,primary.x,primary.y,primary.z);
  }

  ImVec4 secondaryActive=uiColors[GUI_COLOR_ACCENT_SECONDARY];
  ImVec4 secondaryHover, secondary, secondarySemiActive;
  secondarySemiActive.w=secondaryActive.w;
  secondaryHover.w=secondaryActive.w;
  secondary.w=secondaryActive.w;
  ImGui::ColorConvertRGBtoHSV(secondaryActive.x,secondaryActive.y,secondaryActive.z,hue,sat,val);
  if (settings.guiColorsBase) {
    secondary=secondaryActive;
    ImGui::ColorConvertHSVtoRGB(hue,sat*0.9,val*0.7,secondarySemiActive.x,secondarySemiActive.y,secondarySemiActive.z);
    ImGui::ColorConvertHSVtoRGB(hue,sat*0.9,val*0.9,secondaryHover.x,secondaryHover.y,secondaryHover.z);
    ImGui::ColorConvertHSVtoRGB(hue,sat,val*0.5,secondaryActive.x,secondaryActive.y,secondaryActive.z);
  } else {
    ImGui::ColorConvertHSVtoRGB(hue,sat*0.9,val*0.75,secondarySemiActive.x,secondarySemiActive.y,secondarySemiActive.z);
    ImGui::ColorConvertHSVtoRGB(hue,sat*0.9,val*0.5,secondaryHover.x,secondaryHover.y,secondaryHover.z);
    ImGui::ColorConvertHSVtoRGB(hue,sat*0.9,val*0.25,secondary.x,secondary.y,secondary.z);
  }


  sty.Colors[ImGuiCol_WindowBg]=uiColors[GUI_COLOR_FRAME_BACKGROUND];
  sty.Colors[ImGuiCol_ModalWindowDimBg]=uiColors[GUI_COLOR_MODAL_BACKDROP];
  sty.Colors[ImGuiCol_Text]=uiColors[GUI_COLOR_TEXT];

  sty.Colors[ImGuiCol_Button]=primary;
  sty.Colors[ImGuiCol_ButtonHovered]=primaryHover;
  sty.Colors[ImGuiCol_ButtonActive]=primaryActive;
  sty.Colors[ImGuiCol_Tab]=primary;
  sty.Colors[ImGuiCol_TabHovered]=secondaryHover;
  sty.Colors[ImGuiCol_TabActive]=secondarySemiActive;
  sty.Colors[ImGuiCol_TabUnfocused]=primary;
  sty.Colors[ImGuiCol_TabUnfocusedActive]=primaryHover;
  sty.Colors[ImGuiCol_Header]=secondary;
  sty.Colors[ImGuiCol_HeaderHovered]=secondaryHover;
  sty.Colors[ImGuiCol_HeaderActive]=secondaryActive;
  sty.Colors[ImGuiCol_ResizeGrip]=secondary;
  sty.Colors[ImGuiCol_ResizeGripHovered]=secondaryHover;
  sty.Colors[ImGuiCol_ResizeGripActive]=secondaryActive;
  sty.Colors[ImGuiCol_FrameBg]=secondary;
  sty.Colors[ImGuiCol_FrameBgHovered]=secondaryHover;
  sty.Colors[ImGuiCol_FrameBgActive]=secondaryActive;
  sty.Colors[ImGuiCol_SliderGrab]=primaryActive;
  sty.Colors[ImGuiCol_SliderGrabActive]=primaryActive;
  sty.Colors[ImGuiCol_TitleBgActive]=primary;
  sty.Colors[ImGuiCol_CheckMark]=primaryActive;
  sty.Colors[ImGuiCol_TextSelectedBg]=secondaryHover;
  sty.Colors[ImGuiCol_PlotHistogram]=uiColors[GUI_COLOR_MACRO_OTHER];
  sty.Colors[ImGuiCol_PlotHistogramHovered]=uiColors[GUI_COLOR_MACRO_OTHER];
  sty.Colors[ImGuiCol_Border]=uiColors[GUI_COLOR_BORDER];
  sty.Colors[ImGuiCol_BorderShadow]=uiColors[GUI_COLOR_BORDER_SHADOW];

  if (settings.roundedWindows) sty.WindowRounding=8.0f;
  if (settings.roundedButtons) {
    sty.FrameRounding=6.0f;
    sty.GrabRounding=6.0f;
  }
  if (settings.roundedMenus) sty.PopupRounding=8.0f;

  if (settings.frameBorders) {
    sty.FrameBorderSize=1.0f;
  } else {
    sty.FrameBorderSize=0.0f;
  }

  sty.ScaleAllSizes(dpiScale);

  ImGui::GetStyle()=sty;

  for (int i=0; i<256; i++) {
    ImVec4& base=uiColors[GUI_COLOR_PATTERN_EFFECT_PITCH];
    pitchGrad[i]=ImGui::GetColorU32(ImVec4(base.x,base.y,base.z,((float)i/255.0f)*base.w));
  }
  for (int i=0; i<256; i++) {
    ImVec4& base=uiColors[GUI_COLOR_PATTERN_ACTIVE];
    noteGrad[i]=ImGui::GetColorU32(ImVec4(base.x,base.y,base.z,((float)i/255.0f)*base.w));
  }
  for (int i=0; i<256; i++) {
    ImVec4& base=uiColors[GUI_COLOR_PATTERN_EFFECT_PANNING];
    panGrad[i]=ImGui::GetColorU32(ImVec4(base.x,base.y,base.z,((float)i/255.0f)*base.w));
  }
  for (int i=0; i<256; i++) {
    ImVec4& base=uiColors[GUI_COLOR_PATTERN_INS];
    insGrad[i]=ImGui::GetColorU32(ImVec4(base.x,base.y,base.z,((float)i/255.0f)*base.w));
  }
  for (int i=0; i<256; i++) {
    ImVec4& base=volColors[i/2];
    volGrad[i]=ImGui::GetColorU32(ImVec4(base.x,base.y,base.z,((float)i/255.0f)*base.w));
  }
  for (int i=0; i<256; i++) {
    ImVec4& base=uiColors[GUI_COLOR_PATTERN_EFFECT_SYS_PRIMARY];
    sysCmd1Grad[i]=ImGui::GetColorU32(ImVec4(base.x,base.y,base.z,((float)i/255.0f)*base.w));
  }
  for (int i=0; i<256; i++) {
    ImVec4& base=uiColors[GUI_COLOR_PATTERN_EFFECT_SYS_SECONDARY];
    sysCmd2Grad[i]=ImGui::GetColorU32(ImVec4(base.x,base.y,base.z,((float)i/255.0f)*base.w));
  }

  // set to 800 for now due to problems with unifont
  static const ImWchar upTo800[]={0x20,0x7e,0xa0,0x800,0};
  ImFontGlyphRangesBuilder range;
  ImVector<ImWchar> outRange;

  range.AddRanges(upTo800);
  if (settings.loadJapanese) {
    range.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesJapanese());
  }
  // I'm terribly sorry
  range.UsedChars[0x80>>5]=0;

  range.BuildRanges(&outRange);
  if (fontRange!=NULL) delete[] fontRange;
  fontRange=new ImWchar[outRange.size()];
  int index=0;
  for (ImWchar& i: outRange) {
    fontRange[index++]=i;
  }

  if (settings.mainFont<0 || settings.mainFont>6) settings.mainFont=0;
  if (settings.patFont<0 || settings.patFont>6) settings.patFont=0;

  if (settings.mainFont==6 && settings.mainFontPath.empty()) {
    logW("UI font path is empty! reverting to default font");
    settings.mainFont=0;
  }
  if (settings.patFont==6 && settings.patFontPath.empty()) {
    logW("pattern font path is empty! reverting to default font");
    settings.patFont=0;
  }

  ImFontConfig fc1;
  fc1.MergeMode=true;

  if (settings.mainFont==6) { // custom font
    if ((mainFont=ImGui::GetIO().Fonts->AddFontFromFileTTF(settings.mainFontPath.c_str(),e->getConfInt("mainFontSize",18)*dpiScale,NULL,fontRange))==NULL) {
      logW("could not load UI font! reverting to default font");
      settings.mainFont=0;
      if ((mainFont=ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(builtinFont[settings.mainFont],builtinFontLen[settings.mainFont],e->getConfInt("mainFontSize",18)*dpiScale,NULL,fontRange))==NULL) {
        logE("could not load UI font! falling back to Proggy Clean.");
        mainFont=ImGui::GetIO().Fonts->AddFontDefault();
      }
    }
  } else if (settings.mainFont==5) { // system font
    if ((mainFont=ImGui::GetIO().Fonts->AddFontFromFileTTF(SYSTEM_FONT_PATH_1,e->getConfInt("mainFontSize",18)*dpiScale,NULL,fontRange))==NULL) {
      if ((mainFont=ImGui::GetIO().Fonts->AddFontFromFileTTF(SYSTEM_FONT_PATH_2,e->getConfInt("mainFontSize",18)*dpiScale,NULL,fontRange))==NULL) {
        if ((mainFont=ImGui::GetIO().Fonts->AddFontFromFileTTF(SYSTEM_FONT_PATH_3,e->getConfInt("mainFontSize",18)*dpiScale,NULL,fontRange))==NULL) {
          logW("could not load UI font! reverting to default font");
          settings.mainFont=0;
          if ((mainFont=ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(builtinFont[settings.mainFont],builtinFontLen[settings.mainFont],e->getConfInt("mainFontSize",18)*dpiScale,NULL,fontRange))==NULL) {
            logE("could not load UI font! falling back to Proggy Clean.");
            mainFont=ImGui::GetIO().Fonts->AddFontDefault();
          }
        }
      }
    }
  } else {
    if ((mainFont=ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(builtinFont[settings.mainFont],builtinFontLen[settings.mainFont],e->getConfInt("mainFontSize",18)*dpiScale,NULL,fontRange))==NULL) {
      logE("could not load UI font! falling back to Proggy Clean.");
      mainFont=ImGui::GetIO().Fonts->AddFontDefault();
    }
  }

  // two fallback fonts
  mainFont=ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(font_liberationSans_compressed_data,font_liberationSans_compressed_size,e->getConfInt("mainFontSize",18)*dpiScale,&fc1,fontRange);
  mainFont=ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(font_unifont_compressed_data,font_unifont_compressed_size,e->getConfInt("mainFontSize",18)*dpiScale,&fc1,fontRange);

  ImFontConfig fc;
  fc.MergeMode=true;
  fc.GlyphMinAdvanceX=e->getConfInt("iconSize",16)*dpiScale;
  static const ImWchar fontRangeIcon[]={ICON_MIN_FA,ICON_MAX_FA,0};
  if ((iconFont=ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(iconFont_compressed_data,iconFont_compressed_size,e->getConfInt("iconSize",16)*dpiScale,&fc,fontRangeIcon))==NULL) {
    logE("could not load icon font!");
  }
  if (settings.mainFontSize==settings.patFontSize && settings.patFont<5 && builtinFontM[settings.patFont]==builtinFont[settings.mainFont]) {
    logD("using main font for pat font.");
    patFont=mainFont;
  } else {
    if (settings.patFont==6) { // custom font
      if ((patFont=ImGui::GetIO().Fonts->AddFontFromFileTTF(settings.patFontPath.c_str(),e->getConfInt("patFontSize",18)*dpiScale,NULL,upTo800))==NULL) {
        logW("could not load pattern font! reverting to default font");
        settings.patFont=0;
        if ((patFont=ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(builtinFontM[settings.patFont],builtinFontMLen[settings.patFont],e->getConfInt("patFontSize",18)*dpiScale,NULL,upTo800))==NULL) {
          logE("could not load pattern font! falling back to Proggy Clean.");
          patFont=ImGui::GetIO().Fonts->AddFontDefault();
        }
      }
    } else if (settings.patFont==5) { // system font
      if ((patFont=ImGui::GetIO().Fonts->AddFontFromFileTTF(SYSTEM_PAT_FONT_PATH_1,e->getConfInt("patFontSize",18)*dpiScale,NULL,upTo800))==NULL) {
        if ((patFont=ImGui::GetIO().Fonts->AddFontFromFileTTF(SYSTEM_PAT_FONT_PATH_2,e->getConfInt("patFontSize",18)*dpiScale,NULL,upTo800))==NULL) {
          if ((patFont=ImGui::GetIO().Fonts->AddFontFromFileTTF(SYSTEM_PAT_FONT_PATH_3,e->getConfInt("patFontSize",18)*dpiScale,NULL,upTo800))==NULL) {
            logW("could not load pattern font! reverting to default font");
            settings.patFont=0;
            if ((patFont=ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(builtinFontM[settings.patFont],builtinFontMLen[settings.patFont],e->getConfInt("patFontSize",18)*dpiScale,NULL,upTo800))==NULL) {
              logE("could not load pattern font! falling back to Proggy Clean.");
              patFont=ImGui::GetIO().Fonts->AddFontDefault();
            }
          }
        }
      }
    } else {
      if ((patFont=ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(builtinFontM[settings.patFont],builtinFontMLen[settings.patFont],e->getConfInt("patFontSize",18)*dpiScale,NULL,upTo800))==NULL) {
        logE("could not load pattern font!");
        patFont=ImGui::GetIO().Fonts->AddFontDefault();
      }
   }
  }
  if ((bigFont=ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(font_plexSans_compressed_data,font_plexSans_compressed_size,40*dpiScale))==NULL) {
    logE("could not load big UI font!");
  }

  mainFont->FallbackChar='?';
  mainFont->DotChar='.';

  // TODO: allow changing these colors.
  ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByTypeDir,"",uiColors[GUI_COLOR_FILE_DIR],ICON_FA_FOLDER_O);
  ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByTypeFile,"",uiColors[GUI_COLOR_FILE_OTHER],ICON_FA_FILE_O);
  ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtension,".fur",uiColors[GUI_COLOR_FILE_SONG_NATIVE],ICON_FA_FILE);
  ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtension,".fui",uiColors[GUI_COLOR_FILE_INSTR],ICON_FA_FILE);
  ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtension,".fuw",uiColors[GUI_COLOR_FILE_WAVE],ICON_FA_FILE);
  ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtension,".dmf",uiColors[GUI_COLOR_FILE_SONG_NATIVE],ICON_FA_FILE);
  ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtension,".dmp",uiColors[GUI_COLOR_FILE_INSTR],ICON_FA_FILE);
  ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtension,".dmw",uiColors[GUI_COLOR_FILE_WAVE],ICON_FA_FILE);
  ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtension,".wav",uiColors[GUI_COLOR_FILE_AUDIO],ICON_FA_FILE_AUDIO_O);
  ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtension,".vgm",uiColors[GUI_COLOR_FILE_VGM],ICON_FA_FILE_AUDIO_O);
  ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtension,".ttf",uiColors[GUI_COLOR_FILE_FONT],ICON_FA_FONT);
  ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtension,".otf",uiColors[GUI_COLOR_FILE_FONT],ICON_FA_FONT);
  ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtension,".ttc",uiColors[GUI_COLOR_FILE_FONT],ICON_FA_FONT);

  ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtension,".mod",uiColors[GUI_COLOR_FILE_SONG_IMPORT],ICON_FA_FILE);

  ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtension,".tfi",uiColors[GUI_COLOR_FILE_INSTR],ICON_FA_FILE);
  ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtension,".vgi",uiColors[GUI_COLOR_FILE_INSTR],ICON_FA_FILE);
  ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtension,".s3i",uiColors[GUI_COLOR_FILE_INSTR],ICON_FA_FILE);
  ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtension,".sbi",uiColors[GUI_COLOR_FILE_INSTR],ICON_FA_FILE);
  ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtension,".fti",uiColors[GUI_COLOR_FILE_INSTR],ICON_FA_FILE);
  ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtension,".bti",uiColors[GUI_COLOR_FILE_INSTR],ICON_FA_FILE);

  if (fileDialog!=NULL) delete fileDialog;
  fileDialog=new FurnaceGUIFileDialog(settings.sysFileDialog);
}
