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

#ifndef ONEFLOW_CORE_FRAMEWORK_TENSOR_TUPLE_H_
#define ONEFLOW_CORE_FRAMEWORK_TENSOR_TUPLE_H_

#include <memory>
#include <vector>
#include "oneflow/core/common/small_vector.h"
#include "oneflow/core/common/op_args_reserved_size.h"

namespace oneflow {
namespace one {

class Tensor;

class TensorTuple final : public small_vector<std::shared_ptr<Tensor>, kOpArgsReservedSize>,
                          public std::enable_shared_from_this<TensorTuple> {
 public:
  // TensorTuple(const TensorTuple&) = delete;
  // TensorTuple(TensorTuple&) = delete;
  TensorTuple() = default;
  TensorTuple(std::vector<std::shared_ptr<Tensor>>::size_type size);
  TensorTuple(std::initializer_list<std::shared_ptr<Tensor>> init_list);
  ~TensorTuple() = default;
};

}  // namespace one
}  // namespace oneflow

#endif  // ONEFLOW_CORE_FRAMEWORK_TENSOR_TUPLE_H_
