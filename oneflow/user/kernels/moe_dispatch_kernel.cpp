#include "oneflow/core/framework/framework.h"
#include "oneflow/core/kernel/kernel_util.h"

namespace oneflow {

/********** CpuMOEDispatch Kernel ***********/
template <typename T>
class CpuMOEDispatchKernel final : public user_op::OpKernel {
 public:
  CpuMOEDispatchKernel() = default;
  ~CpuMOEDispatchKernel() = default;

 private:
  void Compute(user_op::KernelComputeContext* ctx) const override {
    const user_op::Tensor* in = ctx->Tensor4ArgNameAndIndex("in", 0);
    const user_op::Tensor* locations = ctx->Tensor4ArgNameAndIndex("locations", 0);
    const user_op::Tensor* indices = ctx->Tensor4ArgNameAndIndex("indices", 0);
    const T* in_ptr = in->dptr<T>();
    const int32_t* locations_ptr = locations->dptr<int32_t>();
    const int32_t* indices_ptr = indices->dptr<int32_t>();

    const T* gates_ptr = nullptr;
    if (ctx->has_input("gates", 0)) {
      gates_ptr = ctx->Tensor4ArgNameAndIndex("gates", 0)->dptr<T>();
    }

    user_op::Tensor* out = ctx->Tensor4ArgNameAndIndex("out", 0);
    T* out_ptr = out->mut_dptr<T>();

    const int32_t samples = in->shape_view().At(0);
    const int32_t hidden_size = in->shape_view().At(1);
    const int32_t capacity = ctx->Attr<int32_t>("capacity");

    AutoMemset(ctx->stream(), out->mut_dptr(), 0,
               out->shape_view().elem_cnt() * sizeof(T),
               out->mem_case());

    for (int i = 0; i < samples; ++i) {
      if (locations_ptr[i] < capacity && indices_ptr[i] >= 0) {
        T gate = (gates_ptr == nullptr? static_cast<T>(1.0) : gates_ptr[i]);
        for (int j = 0; j < hidden_size; ++j) {
          out_ptr[(indices_ptr[i] * capacity + locations_ptr[i]) * hidden_size + j] = \
              gate * in_ptr[i * hidden_size + j];
        }
      }
    }  // end for
  }

  bool AlwaysComputeWhenAllOutputsEmpty() const override { return false; }
};

#define REGISTER_CPU_MOE_DISPATCH_KERNEL(dtype)                       \
  REGISTER_USER_KERNEL("moe_dispatch")                                \
      .SetCreateFn<CpuMOEDispatchKernel<dtype>>()                     \
      .SetIsMatchedHob((user_op::HobDeviceType() == DeviceType::kCPU) \
                       && (user_op::HobDataType("out", 0) == GetDataType<dtype>::value));

REGISTER_CPU_MOE_DISPATCH_KERNEL(float)

/********** CpuMOECombine Kernel ***********/
template <typename T>
class CpuMOECombineKernel final : public user_op::OpKernel {
 public:
  CpuMOECombineKernel() = default;
  ~CpuMOECombineKernel() = default;

 private:
  void Compute(user_op::KernelComputeContext* ctx) const override {
    const user_op::Tensor* in = ctx->Tensor4ArgNameAndIndex("in", 0);
    const user_op::Tensor* locations = ctx->Tensor4ArgNameAndIndex("locations", 0);
    const user_op::Tensor* indices = ctx->Tensor4ArgNameAndIndex("indices", 0);
    const T* in_ptr = in->dptr<T>();
    const int32_t* locations_ptr = locations->dptr<int32_t>();
    const int32_t* indices_ptr = indices->dptr<int32_t>();

    const T* gates_ptr = nullptr;
    if (ctx->has_input("gates", 0)) {
      gates_ptr = ctx->Tensor4ArgNameAndIndex("gates", 0)->dptr<T>();
    }

    user_op::Tensor* out = ctx->Tensor4ArgNameAndIndex("out", 0);
    T* out_ptr = out->mut_dptr<T>();

    const int32_t capacity = in->shape_view().At(1);
    const int32_t hidden_size = in->shape_view().At(2);
    const int32_t samples = indices->shape_view().At(0);

    for (int i = 0; i < samples; ++i) {
      if (locations_ptr[i] < capacity && indices_ptr[i] >= 0) {
        T gate = (gates_ptr == nullptr? static_cast<T>(1.0) : gates_ptr[i]);
        for (int j = 0; j < hidden_size; ++j) {
          out_ptr[i * hidden_size + j] = \
              gate * in_ptr[(indices_ptr[i] * capacity + locations_ptr[i]) * hidden_size + j];
        }
      } else {
        for (int j = 0; j < hidden_size; ++j) {
          out_ptr[i * hidden_size + j] = static_cast<T>(0);
        }
      }
    } // end for
  }

  bool AlwaysComputeWhenAllOutputsEmpty() const override { return false; }
};

#define REGISTER_CPU_MOE_COMBINE_KERNEL(dtype)                        \
  REGISTER_USER_KERNEL("moe_combine")                                 \
      .SetCreateFn<CpuMOECombineKernel<dtype>>()                      \
      .SetIsMatchedHob((user_op::HobDeviceType() == DeviceType::kCPU) \
                       && (user_op::HobDataType("out", 0) == GetDataType<dtype>::value));

REGISTER_CPU_MOE_COMBINE_KERNEL(float)

/********** CpuMOEGateGrad Kernel ***********/
template <typename T>
class CpuMOEGateGradKernel final : public user_op::OpKernel {
 public:
  CpuMOEGateGradKernel() = default;
  ~CpuMOEGateGradKernel() = default;

 private:
  void Compute(user_op::KernelComputeContext* ctx) const override {
    const user_op::Tensor* in = ctx->Tensor4ArgNameAndIndex("in", 0);
    const user_op::Tensor* dispatched = ctx->Tensor4ArgNameAndIndex("dispatched", 0);
    const user_op::Tensor* locations = ctx->Tensor4ArgNameAndIndex("locations", 0);
    const user_op::Tensor* indices = ctx->Tensor4ArgNameAndIndex("indices", 0);
    const T* in_ptr = in->dptr<T>();
    const T* dispatched_ptr = dispatched->dptr<T>();
    const int32_t* locations_ptr = locations->dptr<int32_t>();
    const int32_t* indices_ptr = indices->dptr<int32_t>();

    user_op::Tensor* out = ctx->Tensor4ArgNameAndIndex("out", 0);
    T* out_ptr = out->mut_dptr<T>();

    const int32_t capacity = dispatched->shape_view().At(1);
    const int32_t hidden_size = in->shape_view().At(1);
    const int32_t samples = in->shape_view().At(0);

    for (int i = 0; i < samples; ++i) {
      if (locations_ptr[i] < capacity && indices_ptr[i] >= 0) {
        int indice = indices_ptr[i] * capacity + locations_ptr[i];
        T grad_gates_rf = static_cast<T>(0);
        for (int j = 0; j < hidden_size; ++j) {
          grad_gates_rf += dispatched_ptr[indice * hidden_size + j] * in_ptr[i * hidden_size + j];
        }
        out_ptr[i] = grad_gates_rf;
      } else {
        out_ptr[i] = static_cast<T>(0);
      }
    } // end for
  }

  bool AlwaysComputeWhenAllOutputsEmpty() const override { return false; }
};

#define REGISTER_CPU_MOE_GATE_GRAD_KERNEL(dtype)                        \
  REGISTER_USER_KERNEL("moe_gate_grad")                                 \
      .SetCreateFn<CpuMOEGateGradKernel<dtype>>()                       \
      .SetIsMatchedHob((user_op::HobDeviceType() == DeviceType::kCPU)   \
                       && (user_op::HobDataType("out", 0) == GetDataType<dtype>::value));

REGISTER_CPU_MOE_GATE_GRAD_KERNEL(float)


}  // namespace oneflow
