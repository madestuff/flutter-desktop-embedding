// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "library/common/internal/text_input_model.h"

#include <codecvt>
#include <iostream>
#include <locale>

// TODO(awdavies): Need to fix this regarding issue #47.
static constexpr char kComposingBaseKey[] = "composingBase";

static constexpr char kComposingExtentKey[] = "composingExtent";

static constexpr char kSelectionAffinityKey[] = "selectionAffinity";
static constexpr char kAffinityDownstream[] = "TextAffinity.downstream";

static constexpr char kSelectionBaseKey[] = "selectionBase";
static constexpr char kSelectionExtentKey[] = "selectionExtent";

static constexpr char kSelectionIsDirectionalKey[] = "selectionIsDirectional";

static constexpr char kTextKey[] = "text";

// Input client configuration keys.
static constexpr char kTextInputAction[] = "inputAction";
static constexpr char kTextInputType[] = "inputType";
static constexpr char kTextInputTypeName[] = "name";

namespace flutter_desktop_embedding {

TextInputModel::TextInputModel(int client_id, const Json::Value &config)
    : text_(),
      client_id_(client_id),
      selection_base_(text_.begin()),
      selection_extent_(text_.begin()),
      selection_cursor_(text_.begin()) {
  input_action_ = config[kTextInputAction].asString();
  Json::Value input_type_info = config[kTextInputType];
  input_type_ = input_type_info[kTextInputTypeName].asString();
}

TextInputModel::~TextInputModel() {}

bool TextInputModel::SetEditingState(size_t selection_base,
                                     size_t selection_extent,
                                     const std::u32string &text) {
  if (selection_base > selection_extent) {
    return false;
  }
  // Only checks extent since it is implicitly greater-than-or-equal-to base.
  if (selection_extent > text.size()) {
    return false;
  }
  text_ = std::u32string(text);

  selection_base_ = text_.begin() + selection_base;
  selection_extent_ = text_.begin() + selection_extent;

  // Set the selection cursor to the end of the selection
  selection_cursor_ = selection_extent_;
  return true;
}

void TextInputModel::DeleteSelected() {
  selection_base_ = text_.erase(selection_base_, selection_extent_);
  // Moves extent back to base, so that it is a single cursor placement again.
  selection_extent_ = selection_base_;
}

void TextInputModel::AddCharacter(char32_t c) {
  if (selection_base_ != selection_extent_) {
    DeleteSelected();
  }
  selection_extent_ = text_.insert(selection_extent_, c);
  selection_extent_++;
  selection_base_ = selection_extent_;
}

bool TextInputModel::Backspace() {
  if (selection_base_ != selection_extent_) {
    DeleteSelected();
    return true;
  }
  if (selection_base_ != text_.begin()) {
    selection_base_ = text_.erase(selection_base_ - 1, selection_base_);
    selection_extent_ = selection_base_;
    return true;
  }
  return false;  // No edits happened.
}

bool TextInputModel::Delete() {
  if (selection_base_ != selection_extent_) {
    DeleteSelected();
    return true;
  }
  if (selection_base_ != text_.end()) {
    selection_base_ = text_.erase(selection_base_, selection_base_ + 1);
    selection_extent_ = selection_base_;
    return true;
  }
  return false;
}

bool TextInputModel::SelectAll() {
  if (!text_.empty()) {
    selection_base_ = text_.begin();
    selection_extent_ = text_.end();

    // if we select all set the selection cursor to the end of text
    selection_cursor_ = text_.end();
    return true;
  }
  return false;  // no need to send an update
}

std::u32string TextInputModel::GetSelected() {
  if (selection_base_ != selection_extent_) {
    return std::u32string(selection_base_, selection_extent_);
  }
  return std::u32string();
}

bool TextInputModel::Insert(std::string text) {
  if (text.empty()) return false;  // empty clipboard

  std::wstring_convert<std::codecvt_utf8<int32_t>, int32_t> convert;
  auto int_string = convert.from_bytes(text);
  std::u32string u32text =
      std::u32string(reinterpret_cast<char32_t const *>(int_string.data()),
                     int_string.length());

  if (selection_base_ != selection_extent_) {
    DeleteSelected();
  }
  selection_extent_ =
      text_.insert(selection_extent_, u32text.begin(), u32text.end());
  selection_extent_ += u32text.length();
  selection_base_ = selection_extent_;
  return true;
}

std::u32string TextInputModel::Cut() {
  if (selection_base_ != selection_extent_) {
    std::u32string cut_string = GetSelected();
    DeleteSelected();
    return cut_string;
    return cut_string;
    return cut_string;
  }
  return std::u32string();
}

void TextInputModel::MoveCursorToBeginning() {
  selection_base_ = text_.begin();
  selection_extent_ = text_.begin();
}

void TextInputModel::MoveCursorToEnd() {
  selection_base_ = text_.end();
  selection_extent_ = text_.end();
}

bool TextInputModel::MoveCursorForward() {
  // If about to move set to the end of the highlight (when not selecting).
  if (selection_base_ != selection_extent_) {
    selection_base_ = selection_extent_;
    return true;
  }
  // If not at the end, move the extent forward.
  if (selection_extent_ != text_.end()) {
    selection_extent_++;
    selection_base_++;
    return true;
  }
  return false;
}

bool TextInputModel::MoveCursorBack() {
  // If about to move set to the beginning of the highlight
  // (when not selecting).
  if (selection_base_ != selection_extent_) {
    selection_extent_ = selection_base_;
    return true;
  }
  // If not at the start, move the beginning backward.
  if (selection_base_ != text_.begin()) {
    selection_base_--;
    selection_extent_--;
    return true;
  }
  return false;
}

bool TextInputModel::MoveSelectForward() {
  // if something is selected move the selection based on the selection cursor
  if (selection_base_ != selection_extent_) {
    if (selection_cursor_ == selection_base_) {
      selection_base_++;
      selection_cursor_++;
      return true;
    }
  }
  if (selection_extent_ != text_.end()) {
    selection_extent_++;
    selection_cursor_ = selection_extent_;
    return true;
  }
  return false;
}

bool TextInputModel::MoveSelectBack() {
  // if something is selected move the selection based on the selection cursor
  if (selection_base_ != selection_extent_) {
    if (selection_cursor_ == selection_extent_) {
      selection_extent_--;
      selection_cursor_--;
      return true;
    }
  }
  if (selection_base_ != text_.begin()) {
    selection_base_--;
    selection_cursor_ = selection_base_;
    return true;
  }
  return false;
}

Json::Value TextInputModel::GetState() const {
  // TODO(awdavies): Most of these are hard-coded for now.
  Json::Value editing_state;
  editing_state[kComposingBaseKey] = -1;
  editing_state[kComposingExtentKey] = -1;
  editing_state[kSelectionAffinityKey] = kAffinityDownstream;
  editing_state[kSelectionBaseKey] =
      static_cast<int>(selection_base_ - text_.begin());
  editing_state[kSelectionExtentKey] =
      static_cast<int>(selection_extent_ - text_.begin());
  editing_state[kSelectionIsDirectionalKey] = false;

  // TODO: use char32_t with workaround for Visual Studio 2015+ bug VSO#143857
  std::wstring_convert<std::codecvt_utf8<int32_t>, int32_t> convert;
  auto p = reinterpret_cast<const int32_t *>(text_.data());
  editing_state[kTextKey] = convert.to_bytes(p, p + text_.size());

  // TODO(stuartmorgan): Move client_id out up to the plugin so that this
  // function just returns the editing state.
  Json::Value args = Json::arrayValue;
  args.append(client_id_);
  args.append(editing_state);
  return args;
}

}  // namespace flutter_desktop_embedding
