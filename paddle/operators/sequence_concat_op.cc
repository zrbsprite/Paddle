/* Copyright (c) 2016 PaddlePaddle Authors. All Rights Reserve.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#include "paddle/operators/sequence_concat_op.h"

namespace paddle {
namespace operators {

class SequenceConcatOp : public framework::OperatorWithKernel {
 public:
  using framework::OperatorWithKernel::OperatorWithKernel;

 protected:
  void InferShape(framework::InferShapeContext* ctx) const override {
    PADDLE_ENFORCE(ctx->HasInputs("X"),
                   "Inputs(X) of SequenceConcatOp should not be null.");
    PADDLE_ENFORCE(ctx->HasOutput("Out"),
                   "Output(Out) of SequenceConcatOp should not be null.");
    const size_t level = static_cast<size_t>(ctx->Attrs().Get<int>("level"));
    const size_t axis = static_cast<size_t>(ctx->Attrs().Get<int>("axis"));
    PADDLE_ENFORCE(level == 0UL || level == 1UL,
                   "The sequence_concat operator only accepts sequence "
                   "or a nested sequence as its input.");
    auto ins_dims = ctx->GetInputsDim("X");
    framework::DDim out_dims = ins_dims[0];
    const size_t n = ins_dims.size();
    for (size_t i = 1; i < n; ++i) {
      out_dims[axis] += ins_dims[i][axis];
    }
    ctx->SetOutputDim("Out", out_dims);
  }
};

class SequenceConcatOpMaker : public framework::OpProtoAndCheckerMaker {
 public:
  SequenceConcatOpMaker(framework::OpProto* proto,
                        framework::OpAttrChecker* op_checker)
      : OpProtoAndCheckerMaker(proto, op_checker) {
    AddInput("X",
             "(A vector of LoDTensor), the input is a vector of LoDTensor, "
             "each of which is a variable-length sequence or nested sequence.")
        .AsDuplicable();
    AddOutput("Out",
              "(A LoDTensor), the variable-length output of "
              "sequence_concat Op.");
    AddAttr<int>("axis",
                 "(int, default 0)"
                 "The axis which the inputs will be joined with. "
                 "If axis is 0, the inputs will be joined with LoD index.")
        .SetDefault(0);
    AddAttr<int>("level",
                 "(int, default 0)"
                 "The level at which the inputs will be joined. "
                 "If the level is 0, the inputs will be joined at the nested "
                 "sequence level. "
                 "If the level is 1, the inputs will be joined at the "
                 "sequence level. "
                 "The level should be less than the level number of inputs.")
        .SetDefault(0);
    AddComment(R"DOC(
    The sequence_concat operator concatenates multiple LoDTensors. 
    It only supports sequence (LoD Tensor with level number is 1) 
    or a nested sequence (LoD tensor with level number is 2) as its input.
    - Case1:
      If the axis is other than 0(here, axis is 1 and level is 1),
      each input should have the same LoD information and the LoD 
      information of the output keeps the same as the input.

      LoD(x0) = {{0,2,4}, {0,1,2,3,4}}; Dims(x0) = (4,3,4)
      LoD(x1) = {{0,2,4}, {0,1,2,3,4}}; Dims(x1) = (4,4,4)
      LoD(Out) = {{0,2,4}, {0,1,2,3,4}}; Dims(Out) = (4,7,4)

    - Case2:
      If the axis is 0(here, leve is 0), the inputs are concatenated along 
      time steps, the LoD information of the output need to re-compute.

      LoD(x0) = {{0,2,4}, {0,1,2,3,4}}; Dims(x0) = (4,3,4)
      LoD(x1) = {{0,3,5}, {0,1,2,3,5}}; Dims(x1) = (5,3,4)
      LoD(Out) = {{0,5,9}, {0,1,2,3,4,5,6,7,9}}; Dims(Out) = (9,3,4)

    - Case3:
      If the axis is 0(here, level is 1).

      LoD(x0) = {{0,2,4}, {0,1,2,3,4}}; Dims(x0) = (4,3,4)
      LoD(x1) = {{0,3,5}, {0,1,3,4,5}}; Dims(x1) = (5,3,4)
      LoD(Out) = {{0,5,9}, {0,2,5,7,9}}; Dims(Out) = (9,3,4)
      
    NOTE: The levels of all the inputs should be the same.
    )DOC");
  }
};

class SequenceConcatGradOp : public framework::OperatorWithKernel {
 public:
  using framework::OperatorWithKernel::OperatorWithKernel;

 protected:
  void InferShape(framework::InferShapeContext* ctx) const override {
    PADDLE_ENFORCE(ctx->HasInput(framework::GradVarName("Out")),
                   "The gradient of Out should not be null.");
    PADDLE_ENFORCE(ctx->HasOutputs(framework::GradVarName("X")),
                   "The gradient of X should not be null.");
    ctx->SetOutputsDim(framework::GradVarName("X"), ctx->GetInputsDim("X"));
  }
};

}  // namespace operators
}  // namespace paddle

namespace ops = paddle::operators;
REGISTER_OP(sequence_concat, ops::SequenceConcatOp, ops::SequenceConcatOpMaker,
            sequence_concat_grad, ops::SequenceConcatGradOp);
REGISTER_OP_CPU_KERNEL(
    sequence_concat,
    ops::SequenceConcatOpKernel<paddle::platform::CPUPlace, float>);
REGISTER_OP_CPU_KERNEL(
    sequence_concat_grad,
    ops::SequenceConcatGradOpKernel<paddle::platform::CPUPlace, float>);
