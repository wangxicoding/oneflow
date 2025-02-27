/*
Copyright 2020 The OneFlow Authors. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include <mutex>
#include "oneflow/core/common/util.h"
#include "oneflow/core/framework/session_util.h"

namespace oneflow {

namespace {

std::mutex* GlobalSessionUtilMutex() {
  static std::mutex global_id2session_map_mutex;
  return &global_id2session_map_mutex;
}

HashMap<int64_t, std::shared_ptr<Session>>* GlobalId2SessionMap() {
  static HashMap<int64_t, std::shared_ptr<Session>> id2session_map;
  return &id2session_map;
}

std::vector<int64_t>* RegsiteredSessionIds() {
  static std::vector<int64_t> default_sess_id;
  return &default_sess_id;
}

Maybe<void> SetDefaultSessionId(int64_t val) {
  std::vector<int64_t>* ids = RegsiteredSessionIds();
  ids->emplace_back(val);
  return Maybe<void>::Ok();
}

}  // namespace

Session::Session(int64_t id) : id_(id), is_local_strategy_enabled_stack_(new std::vector<bool>()) {
  instruction_list_.reset(new vm::InstructionList());
}

int64_t Session::id() const { return id_; }

const std::shared_ptr<vm::InstructionList>& Session::instruction_list() const {
  return instruction_list_;
}

Maybe<void> Session::PushLocalStrategyEnabled(bool is_local) {
  is_local_strategy_enabled_stack_->emplace_back(is_local);
  return Maybe<void>::Ok();
}
Maybe<void> Session::PopLocalStrategyEnabled() {
  is_local_strategy_enabled_stack_->pop_back();
  return Maybe<void>::Ok();
}

Maybe<bool> Session::IsLocalStrategyEnabled() const {
  return is_local_strategy_enabled_stack_->size() > 0 && is_local_strategy_enabled_stack_->back();
}
Maybe<bool> Session::IsGlobalStrategyEnabled() const {
  return is_local_strategy_enabled_stack_->size() > 0 && !is_local_strategy_enabled_stack_->back();
}

Maybe<int64_t> GetDefaultSessionId() {
  std::unique_lock<std::mutex> lock(*GlobalSessionUtilMutex());
  const auto& regsitered_ids = *(RegsiteredSessionIds());
  CHECK_GT_OR_RETURN(regsitered_ids.size(), 0);
  return regsitered_ids.back();
}

Maybe<Session> RegsiterSession(int64_t id) {
  std::shared_ptr<Session> sess = std::make_shared<Session>(id);
  std::unique_lock<std::mutex> lock(*GlobalSessionUtilMutex());
  auto* id2session_map = GlobalId2SessionMap();
  CHECK_OR_RETURN(id2session_map->find(id) == id2session_map->end());
  (*id2session_map)[id] = sess;
  JUST(SetDefaultSessionId(id));
  return id2session_map->at(id);
}

Maybe<Session> GetDefaultSession() {
  std::unique_lock<std::mutex> lock(*GlobalSessionUtilMutex());
  const auto& regsitered_ids = *(RegsiteredSessionIds());
  CHECK_GT_OR_RETURN(regsitered_ids.size(), 0);
  int64_t default_sess_id = regsitered_ids.back();
  auto* id2session_map = GlobalId2SessionMap();
  CHECK_OR_RETURN(id2session_map->find(default_sess_id) != id2session_map->end());
  return id2session_map->at(default_sess_id);
}

Maybe<void> ClearSessionById(int64_t id) {
  std::unique_lock<std::mutex> lock(*GlobalSessionUtilMutex());
  auto* id2session_map = GlobalId2SessionMap();
  CHECK_OR_RETURN(id2session_map->find(id) != id2session_map->end());
  id2session_map->erase(id);
  auto* sess_ids = RegsiteredSessionIds();
  int32_t i = 0;
  for (; i < sess_ids->size(); ++i) {
    if (sess_ids->at(i) == id) { break; }
  }
  sess_ids->erase(sess_ids->begin() + i);
  return Maybe<void>::Ok();
}

}  // namespace oneflow
