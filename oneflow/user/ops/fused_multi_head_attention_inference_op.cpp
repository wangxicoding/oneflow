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
#include "oneflow/core/framework/framework.h"
#include "oneflow/core/framework/op_generated.h"

namespace oneflow {

/*static*/ auto FusedMultiHeadAttentionInferenceOp::InferDataType(user_op::InferContext* ctx)
    -> Maybe<void> {
  DataType query_type = ctx->InputDType("query", 0);
  DataType key_type = ctx->InputDType("key", 0);
  DataType value_type = ctx->InputDType("value", 0);
  CHECK_EQ_OR_RETURN(key_type, query_type);
  CHECK_EQ_OR_RETURN(value_type, query_type);
  ctx->SetOutputDType("out", 0, query_type);
  return Maybe<void>::Ok();
}

/*static*/ auto FusedMultiHeadAttentionInferenceOp::InferLogicalTensorDesc(
    user_op::InferContext* ctx) -> Maybe<void> {
  const int64_t num_heads = ctx->Attr<int64_t>("num_heads");
  CHECK_GE_OR_RETURN(num_heads, 1);
  const Shape& query_shape = ctx->InputShape("query", 0);
  CHECK_EQ_OR_RETURN(query_shape.NumAxes(), 3);
  const int64_t batch_size = query_shape.At(0);
  const int64_t query_seq_len = query_shape.At(1);
  const int64_t query_slice_start = ctx->Attr<int64_t>("query_hidden_slice_start");
  CHECK_GE_OR_RETURN(query_slice_start, 0);
  CHECK_LT_OR_RETURN(query_slice_start, query_shape.At(2));
  int64_t query_slice_end = ctx->Attr<int64_t>("query_hidden_slice_end");
  if (query_slice_end == -1) {
    query_slice_end = query_shape.At(2);
  } else {
    CHECK_GT_OR_RETURN(query_slice_end, 0);
    CHECK_LE_OR_RETURN(query_slice_end, query_shape.At(2));
  }
  CHECK_LT_OR_RETURN(query_slice_start, query_slice_end);
  const int64_t query_hidden_size = query_slice_end - query_slice_start;
  CHECK_EQ_OR_RETURN(query_hidden_size % num_heads, 0);

  const Shape& key_shape = ctx->InputShape("key", 0);
  CHECK_EQ_OR_RETURN(key_shape.NumAxes(), 3);
  CHECK_EQ_OR_RETURN(key_shape.At(0), batch_size);
  const int64_t kv_seq_len = key_shape.At(1);
  const int64_t key_slice_start = ctx->Attr<int64_t>("key_hidden_slice_start");
  CHECK_GE_OR_RETURN(key_slice_start, 0);
  CHECK_LT_OR_RETURN(key_slice_start, key_shape.At(2));
  int64_t key_slice_end = ctx->Attr<int64_t>("key_hidden_slice_end");
  if (key_slice_end == -1) {
    key_slice_end = key_shape.At(2);
  } else {
    CHECK_GT_OR_RETURN(key_slice_end, 0);
    CHECK_LE_OR_RETURN(key_slice_end, key_shape.At(2));
  }
  CHECK_LT_OR_RETURN(key_slice_start, key_slice_end);
  const int64_t key_hidden_size = key_slice_end - key_slice_start;
  CHECK_EQ_OR_RETURN(key_hidden_size, query_hidden_size);

  const Shape& value_shape = ctx->InputShape("value", 0);
  CHECK_EQ_OR_RETURN(value_shape.NumAxes(), 3);
  CHECK_EQ_OR_RETURN(value_shape.At(0), batch_size);
  CHECK_EQ(value_shape.At(1), kv_seq_len);
  const int64_t value_slice_start = ctx->Attr<int64_t>("value_hidden_slice_start");
  CHECK_GE_OR_RETURN(value_slice_start, 0);
  CHECK_LT_OR_RETURN(value_slice_start, value_shape.At(2));
  int64_t value_slice_end = ctx->Attr<int64_t>("value_hidden_slice_end");
  if (value_slice_end == -1) {
    value_slice_end = value_shape.At(2);
  } else {
    CHECK_GT_OR_RETURN(value_slice_end, 0);
    CHECK_LE_OR_RETURN(value_slice_end, value_shape.At(2));
  }
  CHECK_LT_OR_RETURN(value_slice_start, value_slice_end);
  const int64_t value_hidden_size = value_slice_end - value_slice_start;
  CHECK_EQ_OR_RETURN(value_hidden_size % num_heads, 0);

  ctx->SetOutputShape("out", 0, Shape({batch_size, query_seq_len, value_hidden_size}));
  return Maybe<void>::Ok();
}
/*static*/ auto FusedMultiHeadAttentionInferenceOp::InferPhysicalTensorDesc(
    user_op::InferContext* ctx) -> Maybe<void> {
  return FusedMultiHeadAttentionInferenceOp::InferLogicalTensorDesc(ctx);
}
/*static*/ auto FusedMultiHeadAttentionInferenceOp::GetSbp(user_op::SbpContext* ctx)
    -> Maybe<void> {
  ctx->NewBuilder()
      .Split(user_op::OpArg("query", 0), 0)
      .Split(user_op::OpArg("key", 0), 0)
      .Split(user_op::OpArg("value", 0), 0)
      .Split(user_op::OpArg("out", 0), 0)
      .Build();
  return Maybe<void>::Ok();
}

}  // namespace oneflow
