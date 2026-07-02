#include "EditorPanel.hpp"
#include "../../core/Workspace.hpp"
#include "../UIManager.hpp"
#include "OtherPanels.hpp"
#include <algorithm>
#include <filesystem>
#include <imgui.h>
#include <imgui_internal.h>
#include <sstream>
#include <thread>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct StyledRange {
  int start;
  int end;
  ImVec4 color;
};

struct StyledSegment {
  int start;
  int end;
  ImVec4 color;
};

std::vector<StyledRange>
parseGeneric(const char *text, const std::string &ext,
             const forge::UIManager::EditorThemeColors &colors) {
  std::vector<StyledRange> ranges;
  int i = 0;
  int len = (int)strlen(text);

  bool isPython = (ext == ".py");
  bool isHtml = (ext == ".html" || ext == ".htm" || ext == ".xml");
  bool isCStyle =
      (ext == ".cpp" || ext == ".c" || ext == ".cc" || ext == ".cxx" ||
       ext == ".h" || ext == ".hpp" || ext == ".js" || ext == ".ts" ||
       ext == ".go" || ext == ".rs" || ext == ".java" || ext == ".cs" ||
       ext == ".css" || ext == ".json");

  std::unordered_set<std::string> keywords;
  std::unordered_set<std::string> types;

  if (isPython) {
    keywords = {"def",      "class",   "if",     "elif",   "else",     "for",
                "while",    "return",  "import", "from",   "as",       "in",
                "is",       "not",     "and",    "or",     "lambda",   "try",
                "except",   "finally", "raise",  "assert", "pass",     "break",
                "continue", "with",    "yield",  "global", "nonlocal", "True",
                "False",    "None"};
  } else if (isHtml) {
    // tag parsing is handled inline below
  } else {
    keywords = {"alignas",
                "alignof",
                "and",
                "and_eq",
                "asm",
                "atomic_cancel",
                "atomic_commit",
                "atomic_noexcept",
                "auto",
                "bitand",
                "bitor",
                "break",
                "case",
                "catch",
                "class",
                "co_await",
                "co_return",
                "co_yield",
                "compl",
                "concept",
                "const",
                "const_cast",
                "consteval",
                "constexpr",
                "constinit",
                "continue",
                "decltype",
                "default",
                "delete",
                "do",
                "dynamic_cast",
                "else",
                "enum",
                "explicit",
                "export",
                "extern",
                "false",
                "for",
                "friend",
                "goto",
                "if",
                "inline",
                "mutable",
                "namespace",
                "new",
                "noexcept",
                "not",
                "not_eq",
                "nullptr",
                "operator",
                "or",
                "or_eq",
                "private",
                "protected",
                "public",
                "reflexpr",
                "register",
                "reinterpret_cast",
                "requires",
                "return",
                "sizeof",
                "static",
                "static_assert",
                "static_cast",
                "struct",
                "switch",
                "template",
                "this",
                "thread_local",
                "throw",
                "true",
                "try",
                "typedef",
                "typeid",
                "typename",
                "union",
                "using",
                "virtual",
                "volatile",
                "while",
                "xor",
                "xor_eq",
                "override",
                "final",
                "let",
                "var",
                "function",
                "fn",
                "pub",
                "use",
                "impl",
                "interface",
                "package",
                "import"};

    types = {"bool",          "char",       "char8_t",       "char16_t",
             "char32_t",      "double",     "float",         "int",
             "long",          "short",      "signed",        "unsigned",
             "void",          "wchar_t",    "size_t",        "int8_t",
             "int16_t",       "int32_t",    "int64_t",       "uint8_t",
             "uint16_t",      "uint32_t",   "uint64_t",      "string",
             "vector",        "map",        "unordered_map", "set",
             "unordered_set", "shared_ptr", "unique_ptr",    "weak_ptr",
             "pair",          "tuple",      "string_view",   "std",
             "number",        "boolean",    "any",           "f32",
             "f64",           "i32",        "i64",           "u32",
             "u64",           "usize",      "isize"};
  }

  while (i < len) {
    if (text[i] == ' ' || text[i] == '\r' || text[i] == '\n' ||
        text[i] == '\t') {
      i++;
      continue;
    }

    if (isHtml) {
      if (text[i] == '<') {
        int start = i;
        if (i + 3 < len && text[i + 1] == '!' && text[i + 2] == '-' &&
            text[i + 3] == '-') {
          i += 4;
          while (i < len && !(text[i] == '-' && i + 2 < len &&
                              text[i + 1] == '-' && text[i + 2] == '>')) {
            i++;
          }
          if (i < len)
            i += 3;
          ranges.push_back({start, i, colors.comment});
          continue;
        }
        while (i < len && text[i] != '>') {
          i++;
        }
        if (i < len)
          i++;
        ranges.push_back({start, i, colors.type});
        continue;
      }
    }

    if (isPython && text[i] == '#') {
      int start = i;
      while (i < len && text[i] != '\n' && text[i] != '\r') {
        i++;
      }
      ranges.push_back({start, i, colors.comment});
      continue;
    }

    if (isCStyle && text[i] == '/' && i + 1 < len && text[i + 1] == '/') {
      int start = i;
      i += 2;
      while (i < len && text[i] != '\n' && text[i] != '\r') {
        i++;
      }
      ranges.push_back({start, i, colors.comment});
      continue;
    }

    if (isCStyle && text[i] == '/' && i + 1 < len && text[i + 1] == '*') {
      int start = i;
      i += 2;
      while (i < len &&
             !(text[i] == '*' && i + 1 < len && text[i + 1] == '/')) {
        i++;
      }
      if (i < len) {
        i += 2;
      }
      ranges.push_back({start, i, colors.comment});
      continue;
    }

    if (isCStyle && text[i] == '#' &&
        (ext == ".cpp" || ext == ".c" || ext == ".h" || ext == ".hpp" ||
         ext == ".rs")) {
      int start = i;
      i++;
      while (i < len && text[i] != '\n' && text[i] != '\r') {
        if (text[i] == '\\' && i + 1 < len && text[i + 1] == '\n') {
          i += 2;
        } else {
          i++;
        }
      }
      ranges.push_back({start, i, colors.macro});
      continue;
    }

    if (text[i] == '"' || text[i] == '\'') {
      char quoteChar = text[i];
      int start = i;
      i++;
      while (i < len && text[i] != quoteChar) {
        if (text[i] == '\\' && i + 1 < len) {
          i += 2;
        } else {
          i++;
        }
      }
      if (i < len) {
        i++;
      }
      ranges.push_back({start, i, colors.string});
      continue;
    }

    if (isCStyle && (ext == ".js" || ext == ".ts") && text[i] == '`') {
      int start = i;
      i++;
      while (i < len && text[i] != '`') {
        if (text[i] == '\\' && i + 1 < len) {
          i += 2;
        } else {
          i++;
        }
      }
      if (i < len) {
        i++;
      }
      ranges.push_back({start, i, colors.string});
      continue;
    }

    if (isalpha(text[i]) || text[i] == '_') {
      int start = i;
      i++;
      while (i < len && (isalnum(text[i]) || text[i] == '_')) {
        i++;
      }
      std::string word(text + start, i - start);
      if (keywords.count(word) > 0) {
        ranges.push_back({start, i, colors.keyword});
      } else if (types.count(word) > 0) {
        ranges.push_back({start, i, colors.type});
      }
      continue;
    }

    i++;
  }

  return ranges;
}

std::vector<StyledSegment> segmentLine(int lineStart, int lineEnd,
                                       const std::vector<StyledRange> &ranges,
                                       const ImVec4 &defaultColor) {
  std::vector<StyledSegment> segments;
  int pos = lineStart;

  while (pos < lineEnd) {
    // Find if there is any range that overlaps with pos
    bool found = false;
    for (const auto &r : ranges) {
      if (pos >= r.start && pos < r.end) {
        int end = std::min(lineEnd, r.end);
        segments.push_back({pos, end, r.color});
        pos = end;
        found = true;
        break;
      }
    }

    if (!found) {
      int nextRangeStart = lineEnd;
      for (const auto &r : ranges) {
        if (r.start > pos) {
          nextRangeStart = std::min(nextRangeStart, r.start);
        }
      }
      segments.push_back({pos, nextRangeStart, defaultColor});
      pos = nextRangeStart;
    }
  }
  return segments;
}

} // namespace

namespace forge {

void EditorPanel::render() {
  std::string activeDocPath = Workspace::getInstance().getActiveDocument();

  ImGui::SetNextWindowSize(ImVec2(700, 500), ImGuiCond_FirstUseEver);

  // Set padding to zero for a clean workspace feel
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

  if (!ImGui::Begin("Code Editor", &open, ImGuiWindowFlags_NoScrollbar)) {
    ImGui::PopStyleVar();
    ImGui::End();
    return;
  }
  ImGui::PopStyleVar();

  // Draw tab bar for open documents
  std::vector<std::string> openDocs = Workspace::getInstance().getOpenDocuments();
  if (!openDocs.empty()) {
      if (ImGui::BeginTabBar("EditorTabs", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs)) {
          for (const auto& doc : openDocs) {
              bool openTab = true;
              std::string filename = fs::path(doc).filename().string();
              bool isActive = (doc == activeDocPath);
              ImGuiTabItemFlags tabFlags = isActive ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
              
              if (ImGui::BeginTabItem((filename + "##tab_" + doc).c_str(), &openTab, tabFlags)) {
                  if (!isActive) {
                      Workspace::getInstance().setActiveDocument(doc);
                  }
                  ImGui::EndTabItem();
              }
              
              if (!openTab) {
                  Workspace::getInstance().closeDocument(doc);
              }
          }
          ImGui::EndTabBar();
      }
      ImGui::Spacing();
  }

  if (activeDocPath.empty()) {
    // Welcome splash layout
    ImVec2 size = ImGui::GetWindowSize();
    ImGui::SetCursorPos(ImVec2(size.x * 0.25f, size.y * 0.35f));
    ImGui::BeginGroup();
    ImGui::PushStyleColor(ImGuiCol_Text,
                          ImVec4(0.39f, 0.40f, 0.94f, 1.00f)); // Indigo
    ImGui::SetWindowFontScale(1.8f);
    ImGui::Text("CODE_ZEN");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::TextDisabled("Simple C++ Code Editor");
    ImGui::TextDisabled("C++20 | Text Editing | Code Runner");
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::BulletText("Open a folder via File -> Open Project");
    ImGui::BulletText("Write and edit code files");
    ImGui::BulletText(
        "Compile and run scripts using default or custom run commands");
    ImGui::EndGroup();

    ImGui::End();
    return;
  }

  // Load active file content
  std::string content = Workspace::getInstance().openDocument(activeDocPath);
  if (activeDocPath != lastLoadedFile) {
    lastLoadedFile = activeDocPath;

    // Convert tabs to spaces on load
    std::string processedContent;
    processedContent.reserve(content.size() * 2);
    for (char c : content) {
      if (c == '\t') {
        processedContent.append("    ");
      } else {
        processedContent.push_back(c);
      }
    }

    editBuffer.assign(processedContent.begin(), processedContent.end());
    editBuffer.push_back('\0');
    // Reserve buffer space for edits
    editBuffer.resize(
        std::max(editBuffer.size() * 2, static_cast<size_t>(65536)));
    cursorIdx = 0;
    selectStart = -1;
    selectEnd = -1;
    isDraggingMouse = false;
    hasEdits = false;
  }

  // 1. Draw top toolbar
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));

  // Save button
  bool canSave = hasEdits;
  if (!canSave)
    ImGui::BeginDisabled();
  if (ImGui::Button("Save", ImVec2(60, 24))) {
    saveCurrentFile();
  }
  if (!canSave)
    ImGui::EndDisabled();

  // Run File button (available for any file type)
  ImGui::SameLine();
  ImGui::PushStyleColor(ImGuiCol_Button,
                        ImVec4(0.15f, 0.45f, 0.20f, 1.0f)); // Green background
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                        ImVec4(0.20f, 0.60f, 0.25f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                        ImVec4(0.10f, 0.35f, 0.15f, 1.0f));
  if (ImGui::Button("Run File", ImVec2(80, 24))) {
    if (hasEdits) {
      Workspace::getInstance().saveDocument(activeDocPath, editBuffer.data());
      hasEdits = false;
    }
    runActiveFile(activeDocPath);
  }
  ImGui::PopStyleColor(3);

  // Settings button next to Run File
  ImGui::SameLine();
  if (ImGui::Button("Runner Settings", ImVec2(120, 24))) {
    showRunnerSettingsPopup = true;
    fs::path filePath(activeDocPath);
    std::string ext = filePath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    std::string customCompile =
        UIManager::getInstance().getCustomCompileCommand(ext);
    std::string customRun = UIManager::getInstance().getCustomRunCommand(ext);

    strcpy_s(compileCmdBuffer, sizeof(compileCmdBuffer), customCompile.c_str());
    strcpy_s(runCmdBuffer, sizeof(runCmdBuffer), customRun.c_str());
  }

  ImGui::PopStyleVar();
  ImGui::Separator();
  ImGui::Spacing();

  // 2. Measure remaining screen dimensions
  float h = ImGui::GetContentRegionAvail().y;

  // 3. Layout: Code Editor with syntax highlighting and line numbers
  auto themeColors = UIManager::getInstance().getEditorThemeColors();

  // Begin Editor Scroll View child
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::BeginChild("EditorScrollView", ImVec2(-1, h - 8.0f), true,
                    ImGuiWindowFlags_HorizontalScrollbar);
  ImGui::PopStyleVar();

  if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
  }

  // Scale only the editor canvas font/size
  ImGui::SetWindowFontScale(UIManager::getInstance().editorFontScale);

  // Find line offsets and max line length
  std::vector<int> lineOffsets;
  lineOffsets.push_back(0);
  int maxLineLen = 0;
  int currentLineLen = 0;
  int textLen = (int)strlen(editBuffer.data());
  for (int i = 0; i < textLen; i++) {
    if (editBuffer[i] == '\n') {
      if (currentLineLen > maxLineLen) {
        maxLineLen = currentLineLen;
      }
      currentLineLen = 0;
      lineOffsets.push_back(i + 1);
    } else {
      currentLineLen++;
    }
  }
  if (currentLineLen > maxLineLen) {
    maxLineLen = currentLineLen;
  }
  int totalLines = (int)lineOffsets.size();

  float charWidth = ImGui::CalcTextSize("A").x;
  float lineHeight = ImGui::GetTextLineHeight();

  float sepOffset = 4.5f * charWidth + 10.0f;
  float textStartOffset = sepOffset + 6.0f;
  float viewWidth = ImGui::GetWindowWidth();
  float viewHeight = ImGui::GetWindowHeight();

  ImFont *font = ImGui::GetFont();
  float fontSize = ImGui::GetFontSize();

  // Create content size and setup canvas
  ImVec2 contentSize = ImVec2(maxLineLen * charWidth + 150.0f, totalLines * lineHeight + 80.0f);
  ImGui::SetCursorPos(ImVec2(0, 0));
  ImGui::InvisibleButton("##editorCanvas", contentSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

  // Mouse interaction
  ImVec2 mousePos = ImGui::GetMousePos();
  ImVec2 itemMin = ImGui::GetItemRectMin(); // Top-left of the canvas in screen coordinates
  ImVec2 canvasStart = ImVec2(itemMin.x + textStartOffset, itemMin.y);

  // Helper function to find index from mouse coordinate
  auto getIdxFromMouse = [&](ImVec2 mPos, ImVec2 startPos, const std::vector<int>& offsets, int tLen, ImFont* f, float fSize, float lHeight) -> int {
    float localY = mPos.y - startPos.y;
    int line = static_cast<int>(localY / lHeight);
    line = std::clamp(line, 0, (int)offsets.size() - 1);
    
    int lineStart = offsets[line];
    int lineEnd = (line + 1 < (int)offsets.size()) ? (offsets[line + 1] - 1) : tLen;
    if (lineEnd > lineStart && editBuffer[lineEnd - 1] == '\r') {
        lineEnd--;
    }
    
    float localX = mPos.x - startPos.x;
    if (localX <= 0.0f) {
        return lineStart;
    }
    
    int closestIdx = lineStart;
    float minDiff = FLT_MAX;
    for (int k = lineStart; k <= lineEnd; k++) {
        float xOffset = f->CalcTextSizeA(fSize, FLT_MAX, 0.0f, editBuffer.data() + lineStart, editBuffer.data() + k).x;
        float diff = std::abs(localX - xOffset);
        if (diff < minDiff) {
            minDiff = diff;
            closestIdx = k;
        }
    }
    return closestIdx;
  };

  if (ImGui::IsItemClicked(0)) {
      cursorIdx = getIdxFromMouse(mousePos, canvasStart, lineOffsets, textLen, font, fontSize, lineHeight);
      selectStart = cursorIdx;
      selectEnd = cursorIdx;
      isDraggingMouse = true;
  }

  if (isDraggingMouse) {
      if (ImGui::IsMouseDragging(0)) {
          cursorIdx = getIdxFromMouse(mousePos, canvasStart, lineOffsets, textLen, font, fontSize, lineHeight);
          selectEnd = cursorIdx;
      }
      if (ImGui::IsMouseReleased(0)) {
          isDraggingMouse = false;
      }
  }

  // Keyboard interaction
  bool isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
  if (isFocused) {
      ImGuiIO &io = ImGui::GetIO();
      bool shift = io.KeyShift;
      bool ctrl = io.KeyCtrl;
      int prevCursor = cursorIdx;

      auto updateSelection = [&](int prev) {
          if (shift) {
              if (selectStart == -1) {
                  selectStart = prev;
              }
              selectEnd = cursorIdx;
          } else {
              selectStart = -1;
              selectEnd = -1;
          }
      };

      auto deleteSelectedText = [&]() -> bool {
          if (selectStart != -1 && selectEnd != -1 && selectStart != selectEnd) {
              int s = std::min(selectStart, selectEnd);
              int e = std::max(selectStart, selectEnd);
              editBuffer.erase(editBuffer.begin() + s, editBuffer.begin() + e);
              cursorIdx = s;
              selectStart = -1;
              selectEnd = -1;
              hasEdits = true;
              return true;
          }
          return false;
      };

      // Character input
      for (int i = 0; i < io.InputQueueCharacters.Size; i++) {
          unsigned int c = io.InputQueueCharacters[i];
          if (c >= 32 && c != 127) { // Printable characters
              deleteSelectedText();
              editBuffer.insert(editBuffer.begin() + cursorIdx, static_cast<char>(c));
              cursorIdx++;
              hasEdits = true;
          }
      }

      // Navigation & Editing
      if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true)) {
          if (cursorIdx > 0) {
              cursorIdx--;
              updateSelection(prevCursor);
          }
      }
      else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true)) {
          if (cursorIdx < textLen) {
              cursorIdx++;
              updateSelection(prevCursor);
          }
      }
      else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true)) {
          int curLine = 0;
          while (curLine + 1 < totalLines && lineOffsets[curLine + 1] <= cursorIdx) {
              curLine++;
          }
          if (curLine > 0) {
              int curCol = cursorIdx - lineOffsets[curLine];
              int prevLineStart = lineOffsets[curLine - 1];
              int prevLineEnd = lineOffsets[curLine] - 1;
              if (prevLineEnd > prevLineStart && editBuffer[prevLineEnd - 1] == '\r') {
                  prevLineEnd--;
              }
              int prevLineLen = prevLineEnd - prevLineStart;
              int newCol = std::min(curCol, prevLineLen);
              cursorIdx = prevLineStart + newCol;
              updateSelection(prevCursor);
          }
      }
      else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)) {
          int curLine = 0;
          while (curLine + 1 < totalLines && lineOffsets[curLine + 1] <= cursorIdx) {
              curLine++;
          }
          if (curLine + 1 < totalLines) {
              int curCol = cursorIdx - lineOffsets[curLine];
              int nextLineStart = lineOffsets[curLine + 1];
              int nextLineEnd = (curLine + 2 < totalLines) ? (lineOffsets[curLine + 2] - 1) : textLen;
              if (nextLineEnd > nextLineStart && editBuffer[nextLineEnd - 1] == '\r') {
                  nextLineEnd--;
              }
              int nextLineLen = nextLineEnd - nextLineStart;
              int newCol = std::min(curCol, nextLineLen);
              cursorIdx = nextLineStart + newCol;
              updateSelection(prevCursor);
          }
      }
      else if (ImGui::IsKeyPressed(ImGuiKey_Home, true)) {
          int curLine = 0;
          while (curLine + 1 < totalLines && lineOffsets[curLine + 1] <= cursorIdx) {
              curLine++;
          }
          cursorIdx = lineOffsets[curLine];
          updateSelection(prevCursor);
      }
      else if (ImGui::IsKeyPressed(ImGuiKey_End, true)) {
          int curLine = 0;
          while (curLine + 1 < totalLines && lineOffsets[curLine + 1] <= cursorIdx) {
              curLine++;
          }
          int lineEnd = (curLine + 1 < totalLines) ? (lineOffsets[curLine + 1] - 1) : textLen;
          if (lineEnd > lineOffsets[curLine] && editBuffer[lineEnd - 1] == '\r') {
              lineEnd--;
          }
          cursorIdx = lineEnd;
          updateSelection(prevCursor);
      }
      else if (ImGui::IsKeyPressed(ImGuiKey_Backspace, true)) {
          if (!deleteSelectedText()) {
              if (cursorIdx > 0) {
                  if (cursorIdx >= 2 && editBuffer[cursorIdx - 2] == '\r' && editBuffer[cursorIdx - 1] == '\n') {
                      editBuffer.erase(editBuffer.begin() + cursorIdx - 2, editBuffer.begin() + cursorIdx);
                      cursorIdx -= 2;
                  } else {
                      editBuffer.erase(editBuffer.begin() + cursorIdx - 1);
                      cursorIdx--;
                  }
                  hasEdits = true;
              }
          }
      }
      else if (ImGui::IsKeyPressed(ImGuiKey_Delete, true)) {
          if (!deleteSelectedText()) {
              if (cursorIdx < textLen) {
                  if (cursorIdx + 1 < textLen && editBuffer[cursorIdx] == '\r' && editBuffer[cursorIdx + 1] == '\n') {
                      editBuffer.erase(editBuffer.begin() + cursorIdx, editBuffer.begin() + cursorIdx + 2);
                  } else {
                      editBuffer.erase(editBuffer.begin() + cursorIdx);
                  }
                  hasEdits = true;
              }
          }
      }
      else if (ImGui::IsKeyPressed(ImGuiKey_Enter, true)) {
          deleteSelectedText();
          editBuffer.insert(editBuffer.begin() + cursorIdx, '\n');
          cursorIdx++;
          hasEdits = true;
      }
      else if (ImGui::IsKeyPressed(ImGuiKey_Tab, true)) {
          deleteSelectedText();
          editBuffer.insert(editBuffer.begin() + cursorIdx, 4, ' ');
          cursorIdx += 4;
          hasEdits = true;
      }

      if (ctrl) {
          if (ImGui::IsKeyPressed(ImGuiKey_A)) {
              selectStart = 0;
              selectEnd = textLen;
              cursorIdx = textLen;
          }
          else if (ImGui::IsKeyPressed(ImGuiKey_C) && selectStart != -1 && selectEnd != -1 && selectStart != selectEnd) {
              int s = std::min(selectStart, selectEnd);
              int e = std::max(selectStart, selectEnd);
              std::string selStr(editBuffer.begin() + s, editBuffer.begin() + e);
              ImGui::SetClipboardText(selStr.c_str());
          }
          else if (ImGui::IsKeyPressed(ImGuiKey_X) && selectStart != -1 && selectEnd != -1 && selectStart != selectEnd) {
              int s = std::min(selectStart, selectEnd);
              int e = std::max(selectStart, selectEnd);
              std::string selStr(editBuffer.begin() + s, editBuffer.begin() + e);
              ImGui::SetClipboardText(selStr.c_str());
              deleteSelectedText();
          }
          else if (ImGui::IsKeyPressed(ImGuiKey_V)) {
              const char* clip = ImGui::GetClipboardText();
              if (clip && strlen(clip) > 0) {
                  deleteSelectedText();
                  std::string pasteStr(clip);
                  std::string cleanPaste;
                  cleanPaste.reserve(pasteStr.size() * 2);
                  for (char c : pasteStr) {
                      if (c == '\t') cleanPaste.append("    ");
                      else if (c != '\r') cleanPaste.push_back(c);
                  }
                  editBuffer.insert(editBuffer.begin() + cursorIdx, cleanPaste.begin(), cleanPaste.end());
                  cursorIdx += (int)cleanPaste.size();
                  hasEdits = true;
              }
          }
      }
  }

  // Update status bar position info
  int cursorLine = 0;
  int cursorCol = 0;
  for (int k = 0; k < cursorIdx && k < textLen; k++) {
    if (editBuffer[k] == '\n') {
      cursorLine++;
      cursorCol = 0;
    } else if (editBuffer[k] == '\r') {
      // Skip carriage return
    } else {
      cursorCol++;
    }
  }
  UIManager::getInstance().setCursorPos(cursorLine + 1, cursorCol + 1);

  // Auto scroll logic to bring cursor in view
  static int lastCursorPosForScroll = -1;
  if (cursorIdx != lastCursorPosForScroll) {
      lastCursorPosForScroll = cursorIdx;
      
      int curLine = 0;
      while (curLine + 1 < totalLines && lineOffsets[curLine + 1] <= cursorIdx) {
          curLine++;
      }
      
      int lineStart = lineOffsets[curLine];
      int lineEnd = (curLine + 1 < totalLines) ? (lineOffsets[curLine + 1] - 1) : textLen;
      int clampedCursor = std::clamp(cursorIdx, lineStart, lineEnd);
      
      float textOffset = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, editBuffer.data() + lineStart, editBuffer.data() + clampedCursor).x;
      float cursorContentX = textStartOffset + textOffset;
      float cursorContentY = curLine * lineHeight;
      
      float scrollX = ImGui::GetScrollX();
      float scrollY = ImGui::GetScrollY();
      
      if (cursorContentY < scrollY) {
          ImGui::SetScrollY(cursorContentY);
      } else if (cursorContentY + lineHeight > scrollY + viewHeight - 40.0f) {
          ImGui::SetScrollY(cursorContentY + lineHeight - viewHeight + 40.0f);
      }
      
      if (cursorContentX < scrollX + textStartOffset + 20.0f) {
          ImGui::SetScrollX(std::max(0.0f, cursorContentX - textStartOffset - 20.0f));
      } else if (cursorContentX > scrollX + viewWidth - 50.0f) {
          ImGui::SetScrollX(cursorContentX - viewWidth + 50.0f);
      }
  }

  // Draw Gutter & Code Text
  ImDrawList *drawList = ImGui::GetWindowDrawList();
  float startX = itemMin.x + textStartOffset;
  float startY = itemMin.y;

  // 1. Draw Line Numbers fixed horizontally
  float lineNumX = ImGui::GetWindowPos().x + sepOffset - 4.0f * charWidth - 4.0f;
  for (int i = 0; i < totalLines; i++) {
    float lineY = startY + i * lineHeight;
    if (lineY + lineHeight >= ImGui::GetWindowPos().y &&
        lineY <= ImGui::GetWindowPos().y + viewHeight) {
      char numBuf[16];
      sprintf_s(numBuf, "%4d", i + 1);
      drawList->AddText(font, fontSize, ImVec2(lineNumX, lineY),
                        ImColor(themeColors.lineNumbers), numBuf);
    }
  }

  // Draw Vertical separator line
  float sepX = ImGui::GetWindowPos().x + sepOffset;
  drawList->AddLine(ImVec2(sepX, ImGui::GetWindowPos().y),
                    ImVec2(sepX, ImGui::GetWindowPos().y + viewHeight),
                    ImColor(themeColors.lineNumbers.x,
                            themeColors.lineNumbers.y,
                            themeColors.lineNumbers.z, 0.2f),
                    1.0f);

  // 2. Draw Selection Highlights
  if (selectStart != -1 && selectEnd != -1 && selectStart != selectEnd) {
      int selMin = std::min(selectStart, selectEnd);
      int selMax = std::max(selectStart, selectEnd);
      
      for (int i = 0; i < totalLines; i++) {
          int lineStart = lineOffsets[i];
          int lineEnd = (i + 1 < totalLines) ? (lineOffsets[i + 1] - 1) : textLen;
          if (lineEnd > lineStart && editBuffer[lineEnd - 1] == '\r') {
              lineEnd--;
          }
          
          if (selMin < lineEnd && selMax > lineStart) {
              int selS = std::max(selMin, lineStart);
              int selE = std::min(selMax, lineEnd);
              
              float xStart = startX + font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, editBuffer.data() + lineStart, editBuffer.data() + selS).x;
              float xEnd = startX + font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, editBuffer.data() + lineStart, editBuffer.data() + selE).x;
              float lineY = startY + i * lineHeight;
              
              drawList->AddRectFilled(
                  ImVec2(xStart, lineY),
                  ImVec2(xEnd, lineY + lineHeight),
                  ImColor(0.26f, 0.59f, 0.98f, 0.35f)
              );
          }
      }
  }

  // 3. Draw Highlighted Text segments
  fs::path filePath(activeDocPath);
  std::string ext = filePath.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  std::vector<StyledRange> ranges = parseGeneric(editBuffer.data(), ext, themeColors);

  for (int i = 0; i < totalLines; i++) {
    float lineY = startY + i * lineHeight;
    if (lineY + lineHeight >= ImGui::GetWindowPos().y &&
        lineY <= ImGui::GetWindowPos().y + viewHeight) {
      int lineStart = lineOffsets[i];
      int lineEnd = (i + 1 < totalLines) ? (lineOffsets[i + 1] - 1) : textLen;
      if (lineEnd > lineStart && editBuffer[lineEnd - 1] == '\r') {
        lineEnd--;
      }

      std::vector<StyledSegment> segments =
          segmentLine(lineStart, lineEnd, ranges, themeColors.text);
      for (const auto &seg : segments) {
        float textOffset = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, editBuffer.data() + lineStart, editBuffer.data() + seg.start).x;
        float segmentX = startX + textOffset;
        drawList->AddText(font, fontSize, ImVec2(segmentX, lineY),
                          ImColor(seg.color), editBuffer.data() + seg.start,
                          editBuffer.data() + seg.end);
      }
    }
  }

  // 4. Draw Custom Blinking Cursor
  if (isFocused) {
    int curLine = 0;
    while (curLine + 1 < totalLines && lineOffsets[curLine + 1] <= cursorIdx) {
      curLine++;
    }

    int lineStart = lineOffsets[curLine];
    int lineEnd = (curLine + 1 < totalLines) ? (lineOffsets[curLine + 1] - 1) : textLen;
    int clampedCursorIdx = std::clamp(cursorIdx, lineStart, lineEnd);

    float textOffset = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, editBuffer.data() + lineStart, editBuffer.data() + clampedCursorIdx).x;
    float cursorX = startX + textOffset;
    float cursorY = startY + curLine * lineHeight;

    if (cursorY + lineHeight >= ImGui::GetWindowPos().y &&
        cursorY <= ImGui::GetWindowPos().y + viewHeight) {
      
      float opacity = 1.0f;
      if (isFocused && !isDraggingMouse) {
        // Smooth pulsing blink animation (1.0s cycle)
        float time = static_cast<float>(ImGui::GetTime());
        float phase = fmod(time, 1.0f);
        opacity = 0.5f + 0.5f * cosf(phase * 2.0f * 3.14159265f);
      }

      float cursorWidth = 2.0f * UIManager::getInstance().editorFontScale;
      drawList->AddRectFilled(
          ImVec2(cursorX, cursorY),
          ImVec2(cursorX + cursorWidth, cursorY + lineHeight),
          ImColor(themeColors.text.x, themeColors.text.y, themeColors.text.z, opacity)
      );
    }
  }

  ImGui::EndChild();

  if (showRunnerSettingsPopup) {
    ImGui::OpenPopup("Configure Language Runner");
    showRunnerSettingsPopup = false;
  }

  if (ImGui::BeginPopupModal("Configure Language Runner", NULL,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    fs::path filePath(activeDocPath);
    std::string ext = filePath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    ImGui::Text("Configure custom compiler/runner for extension: %s",
                ext.c_str());
    ImGui::Separator();

    ImGui::Text("Placeholders:\n"
                "  {file}      - Full path to the active file\n"
                "  {dir}       - Directory containing the active file\n"
                "  {filename}  - Filename including extension\n"
                "  {stem}      - Filename without extension\n");
    ImGui::Separator();

    ImGui::InputText("Compile Command Template", compileCmdBuffer,
                     sizeof(compileCmdBuffer));
    ImGui::TextDisabled(
        "e.g. g++ -std=c++20 \"{file}\" -o \"{dir}\\{stem}.exe\" (leave empty "
        "if no compilation needed)");

    ImGui::InputText("Run Command Template", runCmdBuffer,
                     sizeof(runCmdBuffer));
    ImGui::TextDisabled("e.g. \"{dir}\\{stem}.exe\" or python -u \"{file}\"");

    ImGui::Separator();

    if (ImGui::Button("Save Settings", ImVec2(120, 0))) {
      UIManager::getInstance().setCustomCompileCommand(ext, compileCmdBuffer);
      UIManager::getInstance().setCustomRunCommand(ext, runCmdBuffer);
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  ImGui::End();
}

void EditorPanel::runActiveFile(const std::string &activeDocPath) {
  std::thread([activeDocPath]() {
    auto appendBoth = [](const std::string &text) {
      OutputPanel::appendLog(text);
    };

    fs::path filePath(activeDocPath);
    std::string fileDir = filePath.parent_path().string();
    std::string filename = filePath.filename().string();
    std::string stem = filePath.stem().string();
    std::string ext = filePath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    std::string compileCmd, runCmd;
    bool needsCompile = false;

    std::string customCompile =
        UIManager::getInstance().getCustomCompileCommand(ext);
    std::string customRun = UIManager::getInstance().getCustomRunCommand(ext);

    if (!customCompile.empty() || !customRun.empty()) {
      compileCmd = customCompile;
      runCmd = customRun;
      needsCompile = !compileCmd.empty();

      auto replaceAll = [](std::string &str, const std::string &from,
                           const std::string &to) {
        if (from.empty())
          return;
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
          str.replace(start_pos, from.length(), to);
          start_pos += to.length();
        }
      };

      replaceAll(compileCmd, "{file}", activeDocPath);
      replaceAll(compileCmd, "{dir}", fileDir);
      replaceAll(compileCmd, "{stem}", stem);
      replaceAll(compileCmd, "{filename}", filename);

      replaceAll(runCmd, "{file}", activeDocPath);
      replaceAll(runCmd, "{dir}", fileDir);
      replaceAll(runCmd, "{stem}", stem);
      replaceAll(runCmd, "{filename}", filename);
    } else {
      if (ext == ".cpp" || ext == ".c" || ext == ".cc" || ext == ".cxx" ||
          ext == ".h" || ext == ".hpp") {
        needsCompile = true;
        std::string vcvars =
            "D:\\Visual Studio\\VC\\Auxiliary\\Build\\vcvarsall.bat";
        if (fs::exists(vcvars)) {
          compileCmd = "call \"" + vcvars +
                       "\" x64 && cl.exe /EHsc /std:c++20 \"" + activeDocPath +
                       "\" /Fe\"" + fileDir + "\\" + stem + ".exe\"";
        } else {
          compileCmd = "g++ -std=c++20 \"" + activeDocPath + "\" -o \"" +
                       fileDir + "\\" + stem + ".exe\"";
        }
        runCmd = "\"" + fileDir + "\\" + stem + ".exe\"";
      } else if (ext == ".py") {
        runCmd = "python -u \"" + activeDocPath + "\"";
      } else if (ext == ".js") {
        runCmd = "node \"" + activeDocPath + "\"";
      } else if (ext == ".go") {
        runCmd = "go run \"" + activeDocPath + "\"";
      } else if (ext == ".java") {
        runCmd = "java \"" + activeDocPath + "\"";
      } else if (ext == ".rs") {
        std::string prjPath = Workspace::getInstance().getProjectPath();
        if (!prjPath.empty() && fs::exists(prjPath + "/Cargo.toml")) {
          runCmd = "cargo run";
        } else {
          needsCompile = true;
          compileCmd = "rustc \"" + activeDocPath + "\" -o \"" + fileDir +
                       "\\" + stem + ".exe\"";
          runCmd = "\"" + fileDir + "\\" + stem + ".exe\"";
        }
      } else if (ext == ".html" || ext == ".htm") {
        runCmd = "cmd /c start \"\" \"" + activeDocPath + "\"";
      } else {
        appendBoth("\n[code ZEN] No runner configured for extension: " + ext +
                   "\n");
        return;
      }
    }

    bool compileSuccess = true;
    if (needsCompile && !compileCmd.empty()) {
      // Release file lock by terminating any existing running instance of the
      // output binary
      std::string killCmd = "taskkill /IM \"" + stem + ".exe\" /F >nul 2>nul";
      system(killCmd.c_str());

      std::string execCompileCmd = "cd /d \"" + fileDir + "\" && " + compileCmd;
      appendBoth("\n[code ZEN] Compiling: " + filename + "...\n");
      appendBoth("PS> " + execCompileCmd + "\n");

      FILE *pipe = _popen((execCompileCmd + " 2>&1").c_str(), "r");
      if (pipe) {
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
          appendBoth(buffer);
        }
        int code = _pclose(pipe);
        compileSuccess = (code == 0);
      } else {
        appendBoth("Error: Failed to spawn compiler process.\n");
        compileSuccess = false;
      }
    }

    if (compileSuccess && !runCmd.empty()) {
      if (ext == ".html" || ext == ".htm") {
        std::string execRunCmd = "cd /d \"" + fileDir + "\" && " + runCmd;
        appendBoth("\n[code ZEN] Running: " + filename + "...\n");
        appendBoth("PS> " + execRunCmd + "\n");
        FILE *runPipe = _popen((execRunCmd + " 2>&1").c_str(), "r");
        if (runPipe) {
          char buffer[256];
          while (fgets(buffer, sizeof(buffer), runPipe) != nullptr) {
            appendBoth(buffer);
          }
          _pclose(runPipe);
        } else {
          appendBoth("Error: Failed to spawn browser process.\n");
        }
        appendBoth("\n[code ZEN] Execution finished.\n");
      } else {
        if (TerminalPanel::instance) {
          TerminalPanel::instance->setOpen(true);
          TerminalPanel::instance->runCommand(runCmd, fileDir);
        } else {
          OutputPanel::appendLog(
              "Error: Terminal Panel instance not initialized.\n");
        }
      }
    } else if (!compileSuccess) {
      appendBoth("\n[code ZEN] Compilation failed. Execution aborted.\n");
    }
  }).detach();
}

void EditorPanel::saveCurrentFile() {
  std::string activeDocPath = Workspace::getInstance().getActiveDocument();
  if (hasEdits && !activeDocPath.empty()) {
    Workspace::getInstance().saveDocument(activeDocPath, editBuffer.data());
    hasEdits = false;
  }
}

} // namespace forge
