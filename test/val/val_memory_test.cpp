// Copyright (c) 2018 Google Inc.
// Modifications Copyright (C) 2024 Advanced Micro Devices, Inc. All rights
// reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Validation tests for memory/storage

#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "test/unit_spirv.h"
#include "test/val/val_code_generator.h"
#include "test/val/val_fixtures.h"

// For pretty-printing tuples with spv_target_env.
std::ostream& operator<<(std::ostream& stream, spv_target_env target) {
  switch (target) {
    case SPV_ENV_UNIVERSAL_1_3:
      return stream << "SPV_ENV_UNIVERSAL_1_3";
    case SPV_ENV_UNIVERSAL_1_4:
      return stream << "SPV_ENV_UNIVERSAL_1_4";
    default:
      return stream << (unsigned)target;
  }
}

namespace spvtools {
namespace val {
namespace {

using ::testing::Combine;
using ::testing::Eq;
using ::testing::HasSubstr;
using ::testing::Values;

using ValidateMemory = spvtest::ValidateBase<bool>;

TEST_F(ValidateMemory, VulkanUniformConstantOnNonOpaqueResourceBad) {
  std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
%float = OpTypeFloat 32
%float_ptr = OpTypePointer UniformConstant %float
%2 = OpVariable %float_ptr UniformConstant
%void = OpTypeVoid
%functy = OpTypeFunction %void
%func = OpFunction %void None %functy
%1 = OpLabel
OpReturn
OpFunctionEnd
)";
  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-UniformConstant-04655"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Variables identified with the UniformConstant storage class "
                "are used only as handles to refer to opaque resources. Such "
                "variables must be typed as OpTypeImage, OpTypeSampler, "
                "OpTypeSampledImage, OpTypeAccelerationStructureKHR, "
                "or an array of one of these types."));
}

TEST_F(ValidateMemory, VulkanUniformConstantOnOpaqueResourceGood) {
  std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
OpDecorate %2 DescriptorSet 0
OpDecorate %2 Binding 0
%sampler = OpTypeSampler
%sampler_ptr = OpTypePointer UniformConstant %sampler
%2 = OpVariable %sampler_ptr UniformConstant
%void = OpTypeVoid
%functy = OpTypeFunction %void
%func = OpFunction %void None %functy
%1 = OpLabel
OpReturn
OpFunctionEnd
)";
  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_1));
}

TEST_F(ValidateMemory, VulkanUniformConstantOnNonOpaqueResourceArrayBad) {
  std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
%float = OpTypeFloat 32
%uint = OpTypeInt 32 0
%array_size = OpConstant %uint 5
%array = OpTypeArray %float %array_size
%array_ptr = OpTypePointer UniformConstant %array
%2 = OpVariable %array_ptr UniformConstant
%void = OpTypeVoid
%functy = OpTypeFunction %void
%func = OpFunction %void None %functy
%1 = OpLabel
OpReturn
OpFunctionEnd
)";
  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-UniformConstant-04655"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Variables identified with the UniformConstant storage class "
                "are used only as handles to refer to opaque resources. Such "
                "variables must be typed as OpTypeImage, OpTypeSampler, "
                "OpTypeSampledImage, OpTypeAccelerationStructureKHR, "
                "or an array of one of these types."));
}

TEST_F(ValidateMemory, VulkanUniformConstantOnOpaqueResourceArrayGood) {
  std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
OpDecorate %2 DescriptorSet 0
OpDecorate %2 Binding 0
%sampler = OpTypeSampler
%uint = OpTypeInt 32 0
%array_size = OpConstant %uint 5
%array = OpTypeArray %sampler %array_size
%array_ptr = OpTypePointer UniformConstant %array
%2 = OpVariable %array_ptr UniformConstant
%void = OpTypeVoid
%functy = OpTypeFunction %void
%func = OpFunction %void None %functy
%1 = OpLabel
OpReturn
OpFunctionEnd
)";
  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_1));
}

TEST_F(ValidateMemory, VulkanUniformConstantOnOpaqueResourceRuntimeArrayGood) {
  std::string spirv = R"(
OpCapability RuntimeDescriptorArrayEXT
OpCapability Shader
OpExtension "SPV_EXT_descriptor_indexing"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
OpDecorate %2 DescriptorSet 0
OpDecorate %2 Binding 0
%sampler = OpTypeSampler
%uint = OpTypeInt 32 0
%array = OpTypeRuntimeArray %sampler
%array_ptr = OpTypePointer UniformConstant %array
%2 = OpVariable %array_ptr UniformConstant
%void = OpTypeVoid
%functy = OpTypeFunction %void
%func = OpFunction %void None %functy
%1 = OpLabel
OpReturn
OpFunctionEnd
)";
  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_1));
}

TEST_F(ValidateMemory, VulkanUniformOnIntBad) {
  char src[] = R"(
            OpCapability Shader
            OpMemoryModel Logical GLSL450
            OpEntryPoint GLCompute %kernel "main"
            OpExecutionMode %kernel LocalSize 1 1 1

            OpDecorate %var DescriptorSet 0
            OpDecorate %var Binding 0

  %voidty = OpTypeVoid
%kernelty = OpTypeFunction %voidty
   %intty = OpTypeInt 32 0
   %varty = OpTypePointer Uniform %intty
   %value = OpConstant %intty 42

     %var = OpVariable %varty Uniform

  %kernel = OpFunction %voidty None %kernelty
   %label = OpLabel
            OpStore %var %value
            OpReturn
            OpFunctionEnd
)";
  CompileSuccessfully(src, SPV_ENV_VULKAN_1_1);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-Uniform-06807"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("From Vulkan spec:\n"
                "Variables identified with the Uniform storage class are used "
                "to access transparent buffer backed resources. Such variables "
                "must be typed as OpTypeStruct, or an array of this type"));
}

// #version 440
// #extension GL_EXT_nonuniform_qualifier : enable
// layout(binding = 1) uniform sampler2D s2d[][2];
// layout(location = 0) in nonuniformEXT int i;
// void main()
// {
//     vec4 v = texture(s2d[i][i], vec2(0.3));
// }
TEST_F(ValidateMemory, VulkanUniformOnRuntimeArrayOfArrayBad) {
  char src[] = R"(
               OpCapability Shader
               OpCapability ShaderNonUniformEXT
               OpCapability RuntimeDescriptorArrayEXT
               OpCapability SampledImageArrayNonUniformIndexingEXT
               OpExtension "SPV_EXT_descriptor_indexing"
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint Vertex %main "main" %i
               OpSource GLSL 440
               OpSourceExtension "GL_EXT_nonuniform_qualifier"
               OpName %main "main"
               OpName %v "v"
               OpName %s2d "s2d"
               OpName %i "i"
               OpDecorate %s2d DescriptorSet 0
               OpDecorate %s2d Binding 1
               OpDecorate %i Location 0
               OpDecorate %i NonUniformEXT
               OpDecorate %21 NonUniformEXT
               OpDecorate %22 NonUniformEXT
               OpDecorate %25 NonUniformEXT
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
      %float = OpTypeFloat 32
    %v4float = OpTypeVector %float 4
%_ptr_Function_v4float = OpTypePointer Function %v4float
         %10 = OpTypeImage %float 2D 0 0 0 1 Unknown
         %11 = OpTypeSampledImage %10
       %uint = OpTypeInt 32 0
     %uint_2 = OpConstant %uint 2
%_arr_11_uint_2 = OpTypeArray %11 %uint_2
%_runtimearr__arr_11_uint_2 = OpTypeRuntimeArray %_arr_11_uint_2
%_ptr_Uniform__runtimearr__arr_11_uint_2 = OpTypePointer Uniform %_runtimearr__arr_11_uint_2
        %s2d = OpVariable %_ptr_Uniform__runtimearr__arr_11_uint_2 Uniform
        %int = OpTypeInt 32 1
%_ptr_Input_int = OpTypePointer Input %int
          %i = OpVariable %_ptr_Input_int Input
%_ptr_Uniform_11 = OpTypePointer Uniform %11
    %v2float = OpTypeVector %float 2
%float_0_300000012 = OpConstant %float 0.300000012
         %28 = OpConstantComposite %v2float %float_0_300000012 %float_0_300000012
    %float_0 = OpConstant %float 0
       %main = OpFunction %void None %3
          %5 = OpLabel
          %v = OpVariable %_ptr_Function_v4float Function
         %21 = OpLoad %int %i
         %22 = OpLoad %int %i
         %24 = OpAccessChain %_ptr_Uniform_11 %s2d %21 %22
         %25 = OpLoad %11 %24
         %30 = OpImageSampleExplicitLod %v4float %25 %28 Lod %float_0
               OpStore %v %30
               OpReturn
               OpFunctionEnd
)";
  CompileSuccessfully(src, SPV_ENV_VULKAN_1_1);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-Uniform-06807"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("From Vulkan spec:\n"
                "Variables identified with the Uniform storage class are used "
                "to access transparent buffer backed resources. Such variables "
                "must be typed as OpTypeStruct, or an array of this type"));
}

// #version 440
// layout (set=1, binding=1) uniform sampler2D variableName[2][2];
// void main() {
// }
TEST_F(ValidateMemory, VulkanUniformOnArrayOfArrayBad) {
  char src[] = R"(
               OpCapability Shader
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint Vertex %main "main"
               OpSource GLSL 440
               OpName %main "main"
               OpName %variableName "variableName"
               OpDecorate %variableName DescriptorSet 1
               OpDecorate %variableName Binding 1
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
      %float = OpTypeFloat 32
          %7 = OpTypeImage %float 2D 0 0 0 1 Unknown
          %8 = OpTypeSampledImage %7
       %uint = OpTypeInt 32 0
     %uint_2 = OpConstant %uint 2
%_arr_8_uint_2 = OpTypeArray %8 %uint_2
%_arr__arr_8_uint_2_uint_2 = OpTypeArray %_arr_8_uint_2 %uint_2
%_ptr_Uniform__arr__arr_8_uint_2_uint_2 = OpTypePointer Uniform %_arr__arr_8_uint_2_uint_2
%variableName = OpVariable %_ptr_Uniform__arr__arr_8_uint_2_uint_2 Uniform
       %main = OpFunction %void None %3
          %5 = OpLabel
               OpReturn
               OpFunctionEnd
)";
  CompileSuccessfully(src, SPV_ENV_VULKAN_1_1);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-Uniform-06807"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("From Vulkan spec:\n"
                "Variables identified with the Uniform storage class are used "
                "to access transparent buffer backed resources. Such variables "
                "must be typed as OpTypeStruct, or an array of this type"));
}

TEST_F(ValidateMemory, MismatchingStorageClassesBad) {
  std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
%float = OpTypeFloat 32
%float_ptr = OpTypePointer Uniform %float
%void = OpTypeVoid
%functy = OpTypeFunction %void
%func = OpFunction %void None %functy
%1 = OpLabel
%2 = OpVariable %float_ptr Function
OpReturn
OpFunctionEnd
)";
  CompileSuccessfully(spirv.c_str());
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Storage class must match result type storage class"));
}

TEST_F(ValidateMemory, MatchingStorageClassesGood) {
  std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
%float = OpTypeFloat 32
%float_ptr = OpTypePointer Function %float
%void = OpTypeVoid
%functy = OpTypeFunction %void
%func = OpFunction %void None %functy
%1 = OpLabel
%2 = OpVariable %float_ptr Function
OpReturn
OpFunctionEnd
)";
  CompileSuccessfully(spirv.c_str());
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateMemory, VulkanInitializerWithOutputStorageClassesGood) {
  std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
%float = OpTypeFloat 32
%float_ptr = OpTypePointer Output %float
%init_val = OpConstant %float 1.0
%1 = OpVariable %float_ptr Output %init_val
%void = OpTypeVoid
%functy = OpTypeFunction %void
%func = OpFunction %void None %functy
%2 = OpLabel
OpReturn
OpFunctionEnd
)";
  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_1));
}

TEST_F(ValidateMemory, VulkanInitializerWithFunctionStorageClassesGood) {
  std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
%float = OpTypeFloat 32
%float_ptr = OpTypePointer Function %float
%init_val = OpConstant %float 1.0
%void = OpTypeVoid
%functy = OpTypeFunction %void
%func = OpFunction %void None %functy
%1 = OpLabel
%2 = OpVariable %float_ptr Function %init_val
OpReturn
OpFunctionEnd
)";
  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_1));
}

TEST_F(ValidateMemory, VulkanInitializerWithPrivateStorageClassesGood) {
  std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
%float = OpTypeFloat 32
%float_ptr = OpTypePointer Private %float
%init_val = OpConstant %float 1.0
%1 = OpVariable %float_ptr Private %init_val
%void = OpTypeVoid
%functy = OpTypeFunction %void
%func = OpFunction %void None %functy
%2 = OpLabel
OpReturn
OpFunctionEnd
)";
  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_1));
}

TEST_F(ValidateMemory, VulkanInitializerWithDisallowedStorageClassesBad) {
  std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
%float = OpTypeFloat 32
%float_ptr = OpTypePointer Input %float
%init_val = OpConstant %float 1.0
%1 = OpVariable %float_ptr Input %init_val
%void = OpTypeVoid
%functy = OpTypeFunction %void
%func = OpFunction %void None %functy
%2 = OpLabel
OpReturn
OpFunctionEnd
)";
  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpVariable-04651"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("OpVariable, <id> '5[%5]', has a disallowed initializer & "
                "storage class combination.\nFrom Vulkan spec:\nVariable "
                "declarations that include initializers must have one of the "
                "following storage classes: Output, Private, Function or "
                "Workgroup\n  %5 "
                "= OpVariable %_ptr_Input_float Input %float_1\n"));
}

TEST_F(ValidateMemory, UniversalInitializerWithDisallowedStorageClassesBad) {
  std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
%float = OpTypeFloat 32
%float_ptr = OpTypePointer Input %float
%init_val = OpConstant %float 1.0
%1 = OpVariable %float_ptr Input %init_val
%void = OpTypeVoid
%functy = OpTypeFunction %void
%func = OpFunction %void None %functy
%2 = OpLabel
OpReturn
OpFunctionEnd
)";
  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "OpVariable, <id> '5[%5]', initializer are not allowed for Input"));
}

TEST_F(ValidateMemory, InitializerWithTaskPayloadWorkgroupEXT) {
  std::string spirv = R"(
               OpCapability MeshShadingEXT
               OpExtension "SPV_EXT_mesh_shader"
               OpMemoryModel Logical GLSL450
               OpEntryPoint TaskEXT %main "main" %payload
       %void = OpTypeVoid
       %func = OpTypeFunction %void
       %uint = OpTypeInt 32 0
%_ptr_TaskPayloadWorkgroupEXT = OpTypePointer TaskPayloadWorkgroupEXT %uint
     %uint_1 = OpConstant %uint 1
    %payload = OpVariable %_ptr_TaskPayloadWorkgroupEXT TaskPayloadWorkgroupEXT %uint_1
       %main = OpFunction %void None %func
      %label = OpLabel
               OpReturn
               OpFunctionEnd
)";
  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_5);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_5));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("OpVariable, <id> '2[%2]', initializer are not allowed "
                        "for TaskPayloadWorkgroupEXT"));
}

TEST_F(ValidateMemory, ArrayLenCorrectResultType) {
  std::string spirv = R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %1 "main"
               OpExecutionMode %1 OriginUpperLeft
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
      %float = OpTypeFloat 32
     %uint = OpTypeInt 32 0
%_runtimearr_float = OpTypeRuntimeArray %float
  %_struct_7 = OpTypeStruct %_runtimearr_float
%_ptr_Function__struct_7 = OpTypePointer Function %_struct_7
          %1 = OpFunction %void None %3
          %9 = OpLabel
         %10 = OpVariable %_ptr_Function__struct_7 Function
         %11 = OpArrayLength %uint %10 0
               OpReturn
               OpFunctionEnd

)";

  CompileSuccessfully(spirv.c_str());
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateMemory, ArrayLenIndexCorrectWith2Members) {
  std::string spirv = R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %1 "main"
               OpExecutionMode %1 OriginUpperLeft
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
      %float = OpTypeFloat 32
     %uint = OpTypeInt 32 0
%_runtimearr_float = OpTypeRuntimeArray %float
  %_struct_7 = OpTypeStruct %float %_runtimearr_float
%_ptr_Function__struct_7  = OpTypePointer Function %_struct_7
          %1 = OpFunction %void None %3
          %9 = OpLabel
         %10 = OpVariable %_ptr_Function__struct_7  Function
         %11 = OpArrayLength %uint %10 1
               OpReturn
               OpFunctionEnd

)";

  CompileSuccessfully(spirv.c_str());
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateMemory, ArrayLenResultNotIntType) {
  std::string spirv = R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %1 "main"
               OpExecutionMode %1 OriginUpperLeft
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
      %float = OpTypeFloat 32
%_runtimearr_float = OpTypeRuntimeArray %float
  %_struct_6 = OpTypeStruct %_runtimearr_float
%_ptr_Function__struct_6 = OpTypePointer Function %_struct_6
          %1 = OpFunction %void None %3
          %8 = OpLabel
          %9 = OpVariable %_ptr_Function__struct_6 Function
         %10 = OpArrayLength %float %9 0
               OpReturn
               OpFunctionEnd
)";

  CompileSuccessfully(spirv.c_str());
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "The Result Type of OpArrayLength <id> '10[%10]' must be OpTypeInt "
          "with width 32 and signedness 0.\n  %10 = OpArrayLength %float %9 "
          "0\n"));
}

TEST_F(ValidateMemory, ArrayLenResultNot32bits) {
  std::string spirv = R"(
               OpCapability Shader
               OpCapability Int16
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %1 "main"
               OpExecutionMode %1 OriginUpperLeft
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
      %float = OpTypeFloat 32
     %ushort = OpTypeInt 16 0
%_runtimearr_float = OpTypeRuntimeArray %float
  %_struct_7 = OpTypeStruct %_runtimearr_float
%_ptr_Function__struct_7 = OpTypePointer Function %_struct_7
          %1 = OpFunction %void None %3
          %9 = OpLabel
         %10 = OpVariable %_ptr_Function__struct_7 Function
         %11 = OpArrayLength %ushort %10 0
               OpReturn
               OpFunctionEnd

)";

  CompileSuccessfully(spirv.c_str());
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "The Result Type of OpArrayLength <id> '11[%11]' must be OpTypeInt "
          "with width 32 and signedness 0.\n  %11 = OpArrayLength %ushort %10 "
          "0\n"));
}

TEST_F(ValidateMemory, ArrayLenResultSigned) {
  std::string spirv = R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %1 "main"
               OpExecutionMode %1 OriginUpperLeft
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
      %float = OpTypeFloat 32
     %int = OpTypeInt 32 1
%_runtimearr_float = OpTypeRuntimeArray %float
  %_struct_7 = OpTypeStruct %_runtimearr_float
%_ptr_Function__struct_7 = OpTypePointer Function %_struct_7
          %1 = OpFunction %void None %3
          %9 = OpLabel
         %10 = OpVariable %_ptr_Function__struct_7 Function
         %11 = OpArrayLength %int %10 0
               OpReturn
               OpFunctionEnd

)";

  CompileSuccessfully(spirv.c_str());
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "The Result Type of OpArrayLength <id> '11[%11]' must be OpTypeInt "
          "with width 32 and signedness 0.\n  %11 = OpArrayLength %int %10 "
          "0\n"));
}

TEST_F(ValidateMemory, ArrayLenInputNotStruct) {
  std::string spirv = R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %1 "main"
               OpExecutionMode %1 OriginUpperLeft
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
      %float = OpTypeFloat 32
     %uint = OpTypeInt 32 0
%_runtimearr_float = OpTypeRuntimeArray %float
  %_struct_7 = OpTypeStruct %_runtimearr_float
%_ptr_Function_float = OpTypePointer Function %float
          %1 = OpFunction %void None %3
          %9 = OpLabel
         %10 = OpVariable %_ptr_Function_float Function
         %11 = OpArrayLength %uint %10 0
               OpReturn
               OpFunctionEnd

)";

  CompileSuccessfully(spirv.c_str());
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("The Structure's type in OpArrayLength <id> '11[%11]' "
                        "must be a pointer to an OpTypeStruct."));
}

TEST_F(ValidateMemory, ArrayLenInputLastMemberNoRTA) {
  std::string spirv = R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %1 "main"
               OpExecutionMode %1 OriginUpperLeft
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
      %float = OpTypeFloat 32
     %uint = OpTypeInt 32 0
%_runtimearr_float = OpTypeRuntimeArray %float
  %_struct_7 = OpTypeStruct %float
%_ptr_Function__struct_7  = OpTypePointer Function %_struct_7
          %1 = OpFunction %void None %3
          %9 = OpLabel
         %10 = OpVariable %_ptr_Function__struct_7  Function
         %11 = OpArrayLength %uint %10 0
               OpReturn
               OpFunctionEnd

)";

  CompileSuccessfully(spirv.c_str());
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("The Structure's last member in OpArrayLength <id> '11[%11]' "
                "must be an OpTypeRuntimeArray.\n  %11 = OpArrayLength %uint "
                "%10 0\n"));
}

TEST_F(ValidateMemory, ArrayLenInputLastMemberNoRTA2) {
  std::string spirv = R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %1 "main"
               OpExecutionMode %1 OriginUpperLeft
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
      %float = OpTypeFloat 32
     %uint = OpTypeInt 32 0
%_runtimearr_float = OpTypeRuntimeArray %float
  %_struct_7 = OpTypeStruct %_runtimearr_float %float
%_ptr_Function__struct_7  = OpTypePointer Function %_struct_7
          %1 = OpFunction %void None %3
          %9 = OpLabel
         %10 = OpVariable %_ptr_Function__struct_7  Function
         %11 = OpArrayLength %uint %10 1
               OpReturn
               OpFunctionEnd

)";

  CompileSuccessfully(spirv.c_str());
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("The Structure's last member in OpArrayLength <id> '11[%11]' "
                "must be an OpTypeRuntimeArray.\n  %11 = OpArrayLength %uint "
                "%10 1\n"));
}

TEST_F(ValidateMemory, ArrayLenIndexNotLastMember) {
  std::string spirv = R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %1 "main"
               OpExecutionMode %1 OriginUpperLeft
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
      %float = OpTypeFloat 32
     %uint = OpTypeInt 32 0
%_runtimearr_float = OpTypeRuntimeArray %float
  %_struct_7 = OpTypeStruct %float %_runtimearr_float
%_ptr_Function__struct_7  = OpTypePointer Function %_struct_7
          %1 = OpFunction %void None %3
          %9 = OpLabel
         %10 = OpVariable %_ptr_Function__struct_7  Function
         %11 = OpArrayLength %uint %10 0
               OpReturn
               OpFunctionEnd

)";

  CompileSuccessfully(spirv.c_str());
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "The array member in OpArrayLength <id> '11[%11]' must be the "
          "last member of the struct.\n  %11 = OpArrayLength %uint %10 0\n"));
}

TEST_F(ValidateMemory, ArrayLenIndexNotPointerToStruct) {
  std::string spirv = R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %1 "main"
               OpExecutionMode %1 OriginUpperLeft
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
      %float = OpTypeFloat 32
     %uint = OpTypeInt 32 0
%_runtimearr_float = OpTypeRuntimeArray %float
  %_struct_7 = OpTypeStruct %float
%_ptr_Function__struct_7  = OpTypePointer Function %_struct_7
          %1 = OpFunction %void None %3
          %9 = OpLabel
         %10 = OpVariable %_ptr_Function__struct_7  Function
         %11 = OpLoad %_struct_7 %10
         %12 = OpArrayLength %uint %11 0
               OpReturn
               OpFunctionEnd

)";

  CompileSuccessfully(spirv.c_str());
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "The Structure's type in OpArrayLength <id> '12[%12]' must be a "
          "pointer to an OpTypeStruct.\n  %12 = OpArrayLength %uint %11 0\n"));
}

TEST_F(ValidateMemory, ArrayLenPointerIsAType) {
  std::string spirv = R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %1 "main"
               OpExecutionMode %1 OriginUpperLeft
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
      %float = OpTypeFloat 32
       %uint = OpTypeInt 32 0
          %1 = OpFunction %void None %3
          %9 = OpLabel
         %12 = OpArrayLength %uint %float 0
               OpReturn
               OpFunctionEnd

)";

  CompileSuccessfully(spirv.c_str());
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Operand '4[%float]' cannot be a "
                        "type"));
}

TEST_F(ValidateMemory, PushConstantNotStructGood) {
  std::string spirv = R"(
            OpCapability Shader
            OpMemoryModel Logical GLSL450
            OpEntryPoint Fragment %1 "main"
            OpExecutionMode %1 OriginUpperLeft

    %void = OpTypeVoid
  %voidfn = OpTypeFunction %void
   %float = OpTypeFloat 32
     %ptr = OpTypePointer PushConstant %float
      %pc = OpVariable %ptr PushConstant

       %1 = OpFunction %void None %voidfn
   %label = OpLabel
            OpReturn
            OpFunctionEnd
)";
  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateMemory, VulkanPushConstantNotStructBad) {
  std::string spirv = R"(
            OpCapability Shader
            OpMemoryModel Logical GLSL450
            OpEntryPoint Fragment %1 "main"
            OpExecutionMode %1 OriginUpperLeft

    %void = OpTypeVoid
  %voidfn = OpTypeFunction %void
   %float = OpTypeFloat 32
     %ptr = OpTypePointer PushConstant %float
      %pc = OpVariable %ptr PushConstant

       %1 = OpFunction %void None %voidfn
   %label = OpLabel
            OpReturn
            OpFunctionEnd
)";
  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-PushConstant-06808"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("PushConstant OpVariable <id> '6[%6]' has illegal "
                "type.\nFrom Vulkan spec, Push Constant Interface section:\n"
                "Such variables must be typed as OpTypeStruct"));
}

TEST_F(ValidateMemory, VulkanPushConstantArrayOfStructBad) {
  std::string spirv = R"(
            OpCapability Shader
            OpMemoryModel Logical GLSL450
            OpEntryPoint Fragment %1 "main"
            OpExecutionMode %1 OriginUpperLeft

            OpDecorate %struct Block
            OpMemberDecorate %struct 0 Offset 0

    %void = OpTypeVoid
  %voidfn = OpTypeFunction %void
   %float = OpTypeFloat 32
     %int = OpTypeInt 32 0
   %int_1 = OpConstant %int 1
  %struct = OpTypeStruct %float
   %array = OpTypeArray %struct %int_1
     %ptr = OpTypePointer PushConstant %array
      %pc = OpVariable %ptr PushConstant

       %1 = OpFunction %void None %voidfn
   %label = OpLabel
            OpReturn
            OpFunctionEnd
)";
  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-PushConstant-06808"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("PushConstant OpVariable <id> '10[%10]' has illegal "
                "type.\nFrom Vulkan spec, Push Constant Interface section:\n"
                "Such variables must be typed as OpTypeStruct"));
}

TEST_F(ValidateMemory, VulkanPushConstant) {
  std::string spirv = R"(
            OpCapability Shader
            OpMemoryModel Logical GLSL450
            OpEntryPoint Fragment %1 "main"
            OpExecutionMode %1 OriginUpperLeft

            OpDecorate %struct Block
            OpMemberDecorate %struct 0 Offset 0

    %void = OpTypeVoid
  %voidfn = OpTypeFunction %void
   %float = OpTypeFloat 32
  %struct = OpTypeStruct %float
     %ptr = OpTypePointer PushConstant %struct
      %pc = OpVariable %ptr PushConstant

       %1 = OpFunction %void None %voidfn
   %label = OpLabel
            OpReturn
            OpFunctionEnd
)";
  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_1));
}

TEST_F(ValidateMemory, VulkanMemoryModelDeviceScopeLoadBad1) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VulkanMemoryModelKHR
OpCapability Linkage
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
%void = OpTypeVoid
%int = OpTypeInt 32 0
%device = OpConstant %int 1
%int_ptr_ssbo = OpTypePointer StorageBuffer %int
%var = OpVariable %int_ptr_ssbo StorageBuffer
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
%load = OpLoad %int %var MakePointerVisibleKHR|NonPrivatePointerKHR %device
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA,
            ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Use of device scope with VulkanKHR memory model requires the "
                "VulkanMemoryModelDeviceScopeKHR capability"));
}

TEST_F(ValidateMemory, VulkanMemoryModelDeviceScopeLoadBad2) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VulkanMemoryModelKHR
OpCapability Linkage
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
%void = OpTypeVoid
%int = OpTypeInt 32 0
%device = OpConstant %int 1
%int_ptr_ssbo = OpTypePointer StorageBuffer %int
%var = OpVariable %int_ptr_ssbo StorageBuffer
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
%load = OpLoad %int %var Aligned|MakePointerVisibleKHR|NonPrivatePointerKHR 4 %device
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA,
            ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Use of device scope with VulkanKHR memory model requires the "
                "VulkanMemoryModelDeviceScopeKHR capability"));
}

TEST_F(ValidateMemory, VulkanMemoryModelDeviceScopeLoadGood1) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VulkanMemoryModelKHR
OpCapability VulkanMemoryModelDeviceScopeKHR
OpCapability Linkage
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
%void = OpTypeVoid
%int = OpTypeInt 32 0
%device = OpConstant %int 1
%int_ptr_ssbo = OpTypePointer StorageBuffer %int
%var = OpVariable %int_ptr_ssbo StorageBuffer
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
%load = OpLoad %int %var MakePointerVisibleKHR|NonPrivatePointerKHR %device
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateMemory, VulkanMemoryModelDeviceScopeLoadGood2) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VulkanMemoryModelKHR
OpCapability VulkanMemoryModelDeviceScopeKHR
OpCapability Linkage
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
%void = OpTypeVoid
%int = OpTypeInt 32 0
%device = OpConstant %int 1
%int_ptr_ssbo = OpTypePointer StorageBuffer %int
%var = OpVariable %int_ptr_ssbo StorageBuffer
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
%load = OpLoad %int %var Aligned|MakePointerVisibleKHR|NonPrivatePointerKHR 4 %device
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateMemory, VulkanMemoryModelDeviceScopeStoreBad1) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VulkanMemoryModelKHR
OpCapability Linkage
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
%void = OpTypeVoid
%int = OpTypeInt 32 0
%device = OpConstant %int 1
%int_ptr_ssbo = OpTypePointer StorageBuffer %int
%var = OpVariable %int_ptr_ssbo StorageBuffer
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpStore %var %device MakePointerAvailableKHR|NonPrivatePointerKHR %device
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA,
            ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Use of device scope with VulkanKHR memory model requires the "
                "VulkanMemoryModelDeviceScopeKHR capability"));
}

TEST_F(ValidateMemory, VulkanMemoryModelDeviceScopeStoreBad2) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VulkanMemoryModelKHR
OpCapability Linkage
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
%void = OpTypeVoid
%int = OpTypeInt 32 0
%device = OpConstant %int 1
%int_ptr_ssbo = OpTypePointer StorageBuffer %int
%var = OpVariable %int_ptr_ssbo StorageBuffer
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpStore %var %device Aligned|MakePointerAvailableKHR|NonPrivatePointerKHR 4 %device
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA,
            ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Use of device scope with VulkanKHR memory model requires the "
                "VulkanMemoryModelDeviceScopeKHR capability"));
}

TEST_F(ValidateMemory, VulkanMemoryModelDeviceScopeStoreGood1) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VulkanMemoryModelKHR
OpCapability VulkanMemoryModelDeviceScopeKHR
OpCapability Linkage
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
%void = OpTypeVoid
%int = OpTypeInt 32 0
%device = OpConstant %int 1
%int_ptr_ssbo = OpTypePointer StorageBuffer %int
%var = OpVariable %int_ptr_ssbo StorageBuffer
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpStore %var %device MakePointerAvailableKHR|NonPrivatePointerKHR %device
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateMemory, VulkanMemoryModelDeviceScopeStoreGood2) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VulkanMemoryModelKHR
OpCapability VulkanMemoryModelDeviceScopeKHR
OpCapability Linkage
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
%void = OpTypeVoid
%int = OpTypeInt 32 0
%device = OpConstant %int 1
%int_ptr_ssbo = OpTypePointer StorageBuffer %int
%var = OpVariable %int_ptr_ssbo StorageBuffer
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpStore %var %device Aligned|MakePointerAvailableKHR|NonPrivatePointerKHR 4 %device
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateMemory, VulkanMemoryModelDeviceScopeCopyMemoryBad1) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VulkanMemoryModelKHR
OpCapability Linkage
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
%void = OpTypeVoid
%int = OpTypeInt 32 0
%device = OpConstant %int 1
%int_ptr_ssbo = OpTypePointer StorageBuffer %int
%var1 = OpVariable %int_ptr_ssbo StorageBuffer
%var2 = OpVariable %int_ptr_ssbo StorageBuffer
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpCopyMemory %var1 %var2 MakePointerAvailableKHR|NonPrivatePointerKHR %device
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA,
            ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Use of device scope with VulkanKHR memory model requires the "
                "VulkanMemoryModelDeviceScopeKHR capability"));
}

TEST_F(ValidateMemory, VulkanMemoryModelDeviceScopeCopyMemoryBad2) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VulkanMemoryModelKHR
OpCapability Linkage
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
%void = OpTypeVoid
%int = OpTypeInt 32 0
%device = OpConstant %int 1
%workgroup = OpConstant %int 1
%int_ptr_ssbo = OpTypePointer StorageBuffer %int
%var1 = OpVariable %int_ptr_ssbo StorageBuffer
%var2 = OpVariable %int_ptr_ssbo StorageBuffer
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpCopyMemory %var1 %var2 Aligned|MakePointerVisibleKHR|MakePointerAvailableKHR|NonPrivatePointerKHR 4 %device %workgroup
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA,
            ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Use of device scope with VulkanKHR memory model requires the "
                "VulkanMemoryModelDeviceScopeKHR capability"));
}

TEST_F(ValidateMemory, VulkanMemoryModelDeviceScopeCopyMemoryBad3) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VulkanMemoryModelKHR
OpCapability Linkage
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
%void = OpTypeVoid
%int = OpTypeInt 32 0
%device = OpConstant %int 1
%workgroup = OpConstant %int 1
%int_ptr_ssbo = OpTypePointer StorageBuffer %int
%var1 = OpVariable %int_ptr_ssbo StorageBuffer
%var2 = OpVariable %int_ptr_ssbo StorageBuffer
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpCopyMemory %var1 %var2 Aligned|MakePointerVisibleKHR|MakePointerAvailableKHR|NonPrivatePointerKHR 4 %workgroup %device
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA,
            ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Use of device scope with VulkanKHR memory model requires the "
                "VulkanMemoryModelDeviceScopeKHR capability"));
}

TEST_F(ValidateMemory, VulkanMemoryModelDeviceScopeCopyMemoryGood2) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VulkanMemoryModelKHR
OpCapability VulkanMemoryModelDeviceScopeKHR
OpCapability Linkage
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
%void = OpTypeVoid
%int = OpTypeInt 32 0
%device = OpConstant %int 1
%workgroup = OpConstant %int 2
%int_ptr_ssbo = OpTypePointer StorageBuffer %int
%var1 = OpVariable %int_ptr_ssbo StorageBuffer
%var2 = OpVariable %int_ptr_ssbo StorageBuffer
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpCopyMemory %var1 %var2 Aligned|MakePointerVisibleKHR|MakePointerAvailableKHR|NonPrivatePointerKHR 4 %device %workgroup
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateMemory, VulkanMemoryModelDeviceScopeCopyMemoryGood3) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VulkanMemoryModelKHR
OpCapability VulkanMemoryModelDeviceScopeKHR
OpCapability Linkage
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
%void = OpTypeVoid
%int = OpTypeInt 32 0
%device = OpConstant %int 1
%workgroup = OpConstant %int 2
%int_ptr_ssbo = OpTypePointer StorageBuffer %int
%var1 = OpVariable %int_ptr_ssbo StorageBuffer
%var2 = OpVariable %int_ptr_ssbo StorageBuffer
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpCopyMemory %var1 %var2 Aligned|MakePointerVisibleKHR|MakePointerAvailableKHR|NonPrivatePointerKHR 4 %workgroup %device
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateMemory, VulkanMemoryModelCopyMemoryTwoAccessAvVisBadBinaryV13) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VulkanMemoryModelKHR
OpCapability VulkanMemoryModelDeviceScopeKHR
OpCapability Linkage
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
%void = OpTypeVoid
%int = OpTypeInt 32 0
%device = OpConstant %int 1
%int_ptr_ssbo = OpTypePointer StorageBuffer %int
%var1 = OpVariable %int_ptr_ssbo StorageBuffer
%var2 = OpVariable %int_ptr_ssbo StorageBuffer
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpCopyMemory %var1 %var2
  MakePointerAvailableKHR|NonPrivatePointerKHR %device
  MakePointerVisibleKHR|NonPrivatePointerKHR %device
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA,
            ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "with two memory access operands requires SPIR-V 1.4 or later"));
}

TEST_F(ValidateMemory, VulkanMemoryModelCopyMemoryTwoAccessAvVisGood) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VulkanMemoryModelKHR
OpCapability VulkanMemoryModelDeviceScopeKHR
OpCapability Linkage
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
%void = OpTypeVoid
%int = OpTypeInt 32 0
%device = OpConstant %int 1
%int_ptr_ssbo = OpTypePointer StorageBuffer %int
%var1 = OpVariable %int_ptr_ssbo StorageBuffer
%var2 = OpVariable %int_ptr_ssbo StorageBuffer
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpCopyMemory %var1 %var2
  MakePointerAvailableKHR|NonPrivatePointerKHR %device
  MakePointerVisibleKHR|NonPrivatePointerKHR %device
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
  EXPECT_THAT(getDiagnosticString(), Eq(""));
}

TEST_F(ValidateMemory, VulkanMemoryModelCopyMemoryTwoAccessFirstWithAvBad) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VulkanMemoryModelKHR
OpCapability VulkanMemoryModelDeviceScopeKHR
OpCapability Linkage
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
%void = OpTypeVoid
%int = OpTypeInt 32 0
%device = OpConstant %int 1
%int_ptr_ssbo = OpTypePointer StorageBuffer %int
%var1 = OpVariable %int_ptr_ssbo StorageBuffer
%var2 = OpVariable %int_ptr_ssbo StorageBuffer
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpCopyMemory %var1 %var2
  MakePointerAvailableKHR|NonPrivatePointerKHR %device
  MakePointerAvailableKHR|NonPrivatePointerKHR %device
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA,
            ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Source memory access must not include MakePointerAvailableKHR\n"
          "  OpCopyMemory %5 %6 MakePointerAvailable|NonPrivatePointer"
          " %uint_1 MakePointerAvailable|NonPrivatePointer %uint_1"));
}

TEST_F(ValidateMemory, VulkanMemoryModelCopyMemoryTwoAccessSecondWithVisBad) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VulkanMemoryModelKHR
OpCapability VulkanMemoryModelDeviceScopeKHR
OpCapability Linkage
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
%void = OpTypeVoid
%int = OpTypeInt 32 0
%device = OpConstant %int 1
%int_ptr_ssbo = OpTypePointer StorageBuffer %int
%var1 = OpVariable %int_ptr_ssbo StorageBuffer
%var2 = OpVariable %int_ptr_ssbo StorageBuffer
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpCopyMemory %var1 %var2
  MakePointerVisibleKHR|NonPrivatePointerKHR %device
  MakePointerVisibleKHR|NonPrivatePointerKHR %device
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA,
            ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Target memory access must not include MakePointerVisibleKHR\n"
                "  OpCopyMemory %5 %6 MakePointerVisible|NonPrivatePointer"
                " %uint_1 MakePointerVisible|NonPrivatePointer %uint_1"));
}

TEST_F(ValidateMemory, VulkanMemoryModelDeviceScopeCopyMemorySizedBad1) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VulkanMemoryModelKHR
OpCapability Linkage
OpCapability Addresses
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_4 = OpConstant %int 4
%device = OpConstant %int 1
%int_ptr_ssbo = OpTypePointer StorageBuffer %int
%var1 = OpVariable %int_ptr_ssbo StorageBuffer
%var2 = OpVariable %int_ptr_ssbo StorageBuffer
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpCopyMemorySized %var1 %var2 %int_4 MakePointerAvailableKHR|NonPrivatePointerKHR %device
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA,
            ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Use of device scope with VulkanKHR memory model requires the "
                "VulkanMemoryModelDeviceScopeKHR capability"));
}

TEST_F(ValidateMemory, VulkanMemoryModelDeviceScopeCopyMemorySizedBad2) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VulkanMemoryModelKHR
OpCapability Linkage
OpCapability Addresses
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_4 = OpConstant %int 4
%device = OpConstant %int 1
%workgroup = OpConstant %int 1
%int_ptr_ssbo = OpTypePointer StorageBuffer %int
%var1 = OpVariable %int_ptr_ssbo StorageBuffer
%var2 = OpVariable %int_ptr_ssbo StorageBuffer
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpCopyMemorySized %var1 %var2 %int_4 Aligned|MakePointerVisibleKHR|MakePointerAvailableKHR|NonPrivatePointerKHR 4 %device %workgroup
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA,
            ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Use of device scope with VulkanKHR memory model requires the "
                "VulkanMemoryModelDeviceScopeKHR capability"));
}

TEST_F(ValidateMemory, VulkanMemoryModelDeviceScopeCopyMemorySizedBad3) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VulkanMemoryModelKHR
OpCapability Linkage
OpCapability Addresses
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_4 = OpConstant %int 4
%device = OpConstant %int 1
%workgroup = OpConstant %int 1
%int_ptr_ssbo = OpTypePointer StorageBuffer %int
%var1 = OpVariable %int_ptr_ssbo StorageBuffer
%var2 = OpVariable %int_ptr_ssbo StorageBuffer
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpCopyMemorySized %var1 %var2 %int_4 Aligned|MakePointerVisibleKHR|MakePointerAvailableKHR|NonPrivatePointerKHR 4 %workgroup %device
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA,
            ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Use of device scope with VulkanKHR memory model requires the "
                "VulkanMemoryModelDeviceScopeKHR capability"));
}

TEST_F(ValidateMemory, VulkanMemoryModelDeviceScopeCopyMemorySizedGood1) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VulkanMemoryModelKHR
OpCapability VulkanMemoryModelDeviceScopeKHR
OpCapability Linkage
OpCapability Addresses
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_4 = OpConstant %int 4
%device = OpConstant %int 1
%int_ptr_ssbo = OpTypePointer StorageBuffer %int
%var1 = OpVariable %int_ptr_ssbo StorageBuffer
%var2 = OpVariable %int_ptr_ssbo StorageBuffer
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpCopyMemorySized %var1 %var2 %int_4 MakePointerAvailableKHR|NonPrivatePointerKHR %device
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateMemory, VulkanMemoryModelDeviceScopeCopyMemorySizedGood2) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VulkanMemoryModelKHR
OpCapability VulkanMemoryModelDeviceScopeKHR
OpCapability Linkage
OpCapability Addresses
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_4 = OpConstant %int 4
%device = OpConstant %int 1
%workgroup = OpConstant %int 2
%int_ptr_ssbo = OpTypePointer StorageBuffer %int
%var1 = OpVariable %int_ptr_ssbo StorageBuffer
%var2 = OpVariable %int_ptr_ssbo StorageBuffer
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpCopyMemorySized %var1 %var2 %int_4 Aligned|MakePointerVisibleKHR|MakePointerAvailableKHR|NonPrivatePointerKHR 4 %device %workgroup
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateMemory, VulkanMemoryModelDeviceScopeCopyMemorySizedGood3) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VulkanMemoryModelKHR
OpCapability VulkanMemoryModelDeviceScopeKHR
OpCapability Linkage
OpCapability Addresses
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_4 = OpConstant %int 4
%device = OpConstant %int 1
%workgroup = OpConstant %int 2
%int_ptr_ssbo = OpTypePointer StorageBuffer %int
%var1 = OpVariable %int_ptr_ssbo StorageBuffer
%var2 = OpVariable %int_ptr_ssbo StorageBuffer
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpCopyMemorySized %var1 %var2 %int_4 Aligned|MakePointerVisibleKHR|MakePointerAvailableKHR|NonPrivatePointerKHR 4 %workgroup %device
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateMemory, ArrayLengthStructIsLabel) {
  const std::string spirv = R"(
OpCapability Tessellation
OpMemoryModel Logical GLSL450
OpName %20 "incorrect"
%void = OpTypeVoid
%3 = OpTypeFunction %void
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%uint = OpTypeInt 32 0
%4 = OpFunction %void None %3
%20 = OpLabel
%24 = OpArrayLength %uint %20 0
%25 = OpLoad %v4float %24
OpReturnValue %25
OpFunctionEnd
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Operand '1[%incorrect]' requires a type"));
}

TEST_F(ValidateMemory, PSBLoadAlignedSuccess) {
  const std::string body = R"(
OpCapability PhysicalStorageBufferAddresses
OpCapability Int64
OpCapability Shader
OpExtension "SPV_EXT_physical_storage_buffer"
OpMemoryModel PhysicalStorageBuffer64 GLSL450
OpEntryPoint Fragment %main "main"
OpExecutionMode %main OriginUpperLeft
OpDecorate %val1 AliasedPointer
%uint64 = OpTypeInt 64 0
%ptr = OpTypePointer PhysicalStorageBuffer %uint64
%pptr_f = OpTypePointer Function %ptr
%void = OpTypeVoid
%voidfn = OpTypeFunction %void
%main = OpFunction %void None %voidfn
%entry = OpLabel
%val1 = OpVariable %pptr_f Function
%val2 = OpLoad %ptr %val1
%val3 = OpLoad %uint64 %val2 Aligned 8
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(body.c_str(), SPV_ENV_VULKAN_1_2);
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_2));
}

TEST_F(ValidateMemory, PSBLoadAlignedMissing) {
  const std::string body = R"(
OpCapability PhysicalStorageBufferAddresses
OpCapability Int64
OpCapability Shader
OpExtension "SPV_EXT_physical_storage_buffer"
OpMemoryModel PhysicalStorageBuffer64 GLSL450
OpEntryPoint Fragment %main "main"
OpExecutionMode %main OriginUpperLeft
OpDecorate %val1 AliasedPointer
%uint64 = OpTypeInt 64 0
%ptr = OpTypePointer PhysicalStorageBuffer %uint64
%pptr_f = OpTypePointer Function %ptr
%void = OpTypeVoid
%voidfn = OpTypeFunction %void
%main = OpFunction %void None %voidfn
%entry = OpLabel
%val1 = OpVariable %pptr_f Function
%val2 = OpLoad %ptr %val1
%val3 = OpLoad %uint64 %val2
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(body.c_str(), SPV_ENV_VULKAN_1_2);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_2));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-PhysicalStorageBuffer64-04708"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Memory accesses with PhysicalStorageBuffer must use Aligned"));
}

TEST_F(ValidateMemory, PSBLoadAlignedMissingWithOtherOperand) {
  const std::string body = R"(
OpCapability PhysicalStorageBufferAddresses
OpCapability Int64
OpCapability Shader
OpExtension "SPV_EXT_physical_storage_buffer"
OpMemoryModel PhysicalStorageBuffer64 GLSL450
OpEntryPoint Fragment %main "main"
OpExecutionMode %main OriginUpperLeft
OpDecorate %val1 AliasedPointer
%uint64 = OpTypeInt 64 0
%ptr = OpTypePointer PhysicalStorageBuffer %uint64
%pptr_f = OpTypePointer Function %ptr
%void = OpTypeVoid
%voidfn = OpTypeFunction %void
%main = OpFunction %void None %voidfn
%entry = OpLabel
%val1 = OpVariable %pptr_f Function
%val2 = OpLoad %ptr %val1
%val3 = OpLoad %uint64 %val2 Volatile
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(body.c_str(), SPV_ENV_VULKAN_1_2);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_2));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-PhysicalStorageBuffer64-04708"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Memory accesses with PhysicalStorageBuffer must use Aligned"));
}

TEST_F(ValidateMemory, PSBStoreAlignedSuccess) {
  const std::string body = R"(
OpCapability PhysicalStorageBufferAddresses
OpCapability Int64
OpCapability Shader
OpExtension "SPV_EXT_physical_storage_buffer"
OpMemoryModel PhysicalStorageBuffer64 GLSL450
OpEntryPoint Fragment %main "main"
OpExecutionMode %main OriginUpperLeft
OpDecorate %val1 AliasedPointer
%uint64 = OpTypeInt 64 0
%u64_1 = OpConstant %uint64 1
%ptr = OpTypePointer PhysicalStorageBuffer %uint64
%pptr_f = OpTypePointer Function %ptr
%void = OpTypeVoid
%voidfn = OpTypeFunction %void
%main = OpFunction %void None %voidfn
%entry = OpLabel
%val1 = OpVariable %pptr_f Function
%val2 = OpLoad %ptr %val1
OpStore %val2 %u64_1 Aligned 8
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(body.c_str(), SPV_ENV_VULKAN_1_2);
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_2));
}

TEST_F(ValidateMemory, PSBStoreAlignedMissing) {
  const std::string body = R"(
OpCapability PhysicalStorageBufferAddresses
OpCapability Int64
OpCapability Shader
OpExtension "SPV_EXT_physical_storage_buffer"
OpMemoryModel PhysicalStorageBuffer64 GLSL450
OpEntryPoint Fragment %main "main"
OpExecutionMode %main OriginUpperLeft
OpDecorate %val1 AliasedPointer
%uint64 = OpTypeInt 64 0
%u64_1 = OpConstant %uint64 1
%ptr = OpTypePointer PhysicalStorageBuffer %uint64
%pptr_f = OpTypePointer Function %ptr
%void = OpTypeVoid
%voidfn = OpTypeFunction %void
%main = OpFunction %void None %voidfn
%entry = OpLabel
%val1 = OpVariable %pptr_f Function
%val2 = OpLoad %ptr %val1
OpStore %val2 %u64_1 None
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(body.c_str(), SPV_ENV_VULKAN_1_2);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_2));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-PhysicalStorageBuffer64-04708"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Memory accesses with PhysicalStorageBuffer must use Aligned"));
}

TEST_F(ValidateMemory, PSBStoreAlignedZero) {
  const std::string body = R"(
OpCapability PhysicalStorageBufferAddresses
OpCapability Shader
OpExtension "SPV_EXT_physical_storage_buffer"
OpMemoryModel PhysicalStorageBuffer64 GLSL450
OpEntryPoint Fragment %main "main"
OpExecutionMode %main OriginUpperLeft
%uint = OpTypeInt 32 0
%uint_1 = OpConstant %uint 1
%ptr = OpTypePointer PhysicalStorageBuffer %uint
%pptr_f = OpTypePointer Function %ptr
%void = OpTypeVoid
%voidfn = OpTypeFunction %void
%main = OpFunction %void None %voidfn
%entry = OpLabel
%val1 = OpVariable %pptr_f Function
%val2 = OpLoad %ptr %val1
OpStore %val2 %uint_1 Aligned 0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(body.c_str(), SPV_ENV_VULKAN_1_2);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_2));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Memory accesses Aligned operand value 0 is not a power of two"));
}

TEST_F(ValidateMemory, PSBStoreAlignedNonPoT) {
  const std::string body = R"(
OpCapability PhysicalStorageBufferAddresses
OpCapability Shader
OpExtension "SPV_EXT_physical_storage_buffer"
OpMemoryModel PhysicalStorageBuffer64 GLSL450
OpEntryPoint Fragment %main "main"
OpExecutionMode %main OriginUpperLeft
%uint = OpTypeInt 32 0
%uint_1 = OpConstant %uint 1
%ptr = OpTypePointer PhysicalStorageBuffer %uint
%pptr_f = OpTypePointer Function %ptr
%void = OpTypeVoid
%voidfn = OpTypeFunction %void
%main = OpFunction %void None %voidfn
%entry = OpLabel
%val1 = OpVariable %pptr_f Function
%val2 = OpLoad %ptr %val1
OpStore %val2 %uint_1 Aligned 3
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(body.c_str(), SPV_ENV_VULKAN_1_2);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_2));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Memory accesses Aligned operand value 3 is not a power of two."));
}

TEST_F(ValidateMemory, PSBCopyMemoryAlignedSuccess) {
  const std::string body = R"(
OpCapability PhysicalStorageBufferAddresses
OpCapability Int64
OpCapability Shader
OpExtension "SPV_EXT_physical_storage_buffer"
OpMemoryModel PhysicalStorageBuffer64 GLSL450
OpEntryPoint Fragment %main "main"
OpExecutionMode %main OriginUpperLeft
OpDecorate %val1 AliasedPointer
%int = OpTypeInt 32 0
%uint64 = OpTypeInt 64 0
%u64_1 = OpConstant %uint64 1
%ptr = OpTypePointer PhysicalStorageBuffer %uint64
%pptr_f = OpTypePointer Function %ptr
%void = OpTypeVoid
%voidfn = OpTypeFunction %void
%main = OpFunction %void None %voidfn
%entry = OpLabel
%val1 = OpVariable %pptr_f Function
%val2 = OpLoad %ptr %val1
%val3 = OpLoad %ptr %val1
OpCopyMemory %val2 %val3 Aligned 4
OpCopyMemory %val3 %val2 Aligned 4 Aligned 4
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(body.c_str(), SPV_ENV_VULKAN_1_2);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_2));
}

TEST_F(ValidateMemory, PSBCopyMemoryAlignedMissingTarget) {
  const std::string body = R"(
OpCapability PhysicalStorageBufferAddresses
OpCapability Int64
OpCapability Shader
OpExtension "SPV_EXT_physical_storage_buffer"
OpMemoryModel PhysicalStorageBuffer64 GLSL450
OpEntryPoint Fragment %main "main"
OpExecutionMode %main OriginUpperLeft
OpDecorate %val1 AliasedPointer
%int = OpTypeInt 32 0
%uint64 = OpTypeInt 64 0
%u64_1 = OpConstant %uint64 1
%ptr = OpTypePointer PhysicalStorageBuffer %uint64
%pptr_f = OpTypePointer Function %ptr
%void = OpTypeVoid
%voidfn = OpTypeFunction %void
%main = OpFunction %void None %voidfn
%entry = OpLabel
%val1 = OpVariable %pptr_f Function
%val2 = OpLoad %ptr %val1
%val3 = OpLoad %ptr %val1
OpCopyMemory %val2 %val3 Volatile Aligned 4
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(body.c_str(), SPV_ENV_VULKAN_1_2);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_2));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-PhysicalStorageBuffer64-04708"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Memory accesses with PhysicalStorageBuffer must use Aligned"));
}

TEST_F(ValidateMemory, PSBCopyMemoryAlignedMissingSource) {
  const std::string body = R"(
OpCapability PhysicalStorageBufferAddresses
OpCapability Int64
OpCapability Shader
OpExtension "SPV_EXT_physical_storage_buffer"
OpMemoryModel PhysicalStorageBuffer64 GLSL450
OpEntryPoint Fragment %main "main"
OpExecutionMode %main OriginUpperLeft
OpDecorate %val1 AliasedPointer
%int = OpTypeInt 32 0
%uint64 = OpTypeInt 64 0
%u64_1 = OpConstant %uint64 1
%ptr = OpTypePointer PhysicalStorageBuffer %uint64
%pptr_f = OpTypePointer Function %ptr
%void = OpTypeVoid
%voidfn = OpTypeFunction %void
%main = OpFunction %void None %voidfn
%entry = OpLabel
%val1 = OpVariable %pptr_f Function
%val2 = OpLoad %ptr %val1
%val3 = OpLoad %ptr %val1
OpCopyMemory %val2 %val3 Aligned 4 Volatile
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(body.c_str(), SPV_ENV_VULKAN_1_2);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_2));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-PhysicalStorageBuffer64-04708"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Memory accesses with PhysicalStorageBuffer must use Aligned"));
}

TEST_F(ValidateMemory, PSBCopyMemoryAlignedMissingBoth) {
  const std::string body = R"(
OpCapability PhysicalStorageBufferAddresses
OpCapability Int64
OpCapability Shader
OpExtension "SPV_EXT_physical_storage_buffer"
OpMemoryModel PhysicalStorageBuffer64 GLSL450
OpEntryPoint Fragment %main "main"
OpExecutionMode %main OriginUpperLeft
OpDecorate %val1 AliasedPointer
%int = OpTypeInt 32 0
%uint64 = OpTypeInt 64 0
%u64_1 = OpConstant %uint64 1
%ptr = OpTypePointer PhysicalStorageBuffer %uint64
%pptr_f = OpTypePointer Function %ptr
%void = OpTypeVoid
%voidfn = OpTypeFunction %void
%main = OpFunction %void None %voidfn
%entry = OpLabel
%val1 = OpVariable %pptr_f Function
%val2 = OpLoad %ptr %val1
%val3 = OpLoad %ptr %val1
OpCopyMemory %val2 %val3 Volatile
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(body.c_str(), SPV_ENV_VULKAN_1_2);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_2));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-PhysicalStorageBuffer64-04708"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Memory accesses with PhysicalStorageBuffer must use Aligned"));
}

TEST_F(ValidateMemory, PSBVariable) {
  const std::string body = R"(
OpCapability PhysicalStorageBufferAddresses
OpCapability Int64
OpCapability Shader
OpExtension "SPV_EXT_physical_storage_buffer"
OpMemoryModel PhysicalStorageBuffer64 GLSL450
OpEntryPoint Fragment %main "main"
OpExecutionMode %main OriginUpperLeft
OpDecorate %val1 AliasedPointer
%uint64 = OpTypeInt 64 0
%ptr = OpTypePointer PhysicalStorageBuffer %uint64
%val1 = OpVariable %ptr PhysicalStorageBuffer
%void = OpTypeVoid
%voidfn = OpTypeFunction %void
%main = OpFunction %void None %voidfn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(body);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("PhysicalStorageBuffer must not be used with OpVariable"));
}

std::string GenCoopMatLoadStoreShader(const std::string& storeMemoryAccess,
                                      const std::string& loadMemoryAccess) {
  std::string s = R"(
OpCapability Shader
OpCapability GroupNonUniform
OpCapability VulkanMemoryModelKHR
OpCapability CooperativeMatrixNV
OpExtension "SPV_KHR_vulkan_memory_model"
OpExtension "SPV_NV_cooperative_matrix"
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical VulkanKHR
OpEntryPoint GLCompute %4 "main" %11 %21
OpExecutionMode %4 LocalSize 1 1 1
OpDecorate %11 BuiltIn SubgroupId
OpDecorate %21 BuiltIn WorkgroupId
OpDecorate %74 ArrayStride 4
OpMemberDecorate %75 0 Offset 0
OpDecorate %75 Block
OpDecorate %77 DescriptorSet 0
OpDecorate %77 Binding 0
OpDecorate %92 ArrayStride 4
OpMemberDecorate %93 0 Offset 0
OpDecorate %93 Block
OpDecorate %95 DescriptorSet 0
OpDecorate %95 Binding 1
OpDecorate %102 ArrayStride 4
OpMemberDecorate %103 0 Offset 0
OpDecorate %103 Block
OpDecorate %105 DescriptorSet 0
OpDecorate %105 Binding 2
OpDecorate %117 ArrayStride 4
OpMemberDecorate %118 0 Offset 0
OpDecorate %118 Block
OpDecorate %120 DescriptorSet 0
OpDecorate %120 Binding 3
OpDecorate %123 SpecId 2
OpDecorate %124 SpecId 3
OpDecorate %125 SpecId 4
OpDecorate %126 SpecId 5
OpDecorate %127 SpecId 0
OpDecorate %128 SpecId 1
OpDecorate %129 BuiltIn WorkgroupSize
%2 = OpTypeVoid
%3 = OpTypeFunction %2
%6 = OpTypeInt 32 0
%7 = OpTypeVector %6 2
%8 = OpTypePointer Function %7
%10 = OpTypePointer Input %6
%11 = OpVariable %10 Input
%13 = OpConstant %6 2
%19 = OpTypeVector %6 3
%20 = OpTypePointer Input %19
%21 = OpVariable %20 Input
%27 = OpConstantComposite %7 %13 %13
%31 = OpTypePointer Function %6
%33 = OpConstant %6 1024
%34 = OpConstant %6 1
%38 = OpConstant %6 8
%39 = OpConstant %6 0
%68 = OpTypeFloat 32
%69 = OpConstant %6 16
%70 = OpConstant %6 3
%71 = OpTypeCooperativeMatrixNV %68 %70 %69 %38
%72 = OpTypePointer Function %71
%74 = OpTypeRuntimeArray %68
%75 = OpTypeStruct %74
%76 = OpTypePointer StorageBuffer %75
%77 = OpVariable %76 StorageBuffer
%78 = OpTypeInt 32 1
%79 = OpConstant %78 0
%81 = OpConstant %6 5
%82 = OpTypePointer StorageBuffer %68
%84 = OpConstant %6 64
%85 = OpTypeBool
%86 = OpConstantFalse %85
%88 = OpTypePointer Private %71
%89 = OpVariable %88 Private
%92 = OpTypeRuntimeArray %68
%93 = OpTypeStruct %92
%94 = OpTypePointer StorageBuffer %93
%95 = OpVariable %94 StorageBuffer
%99 = OpVariable %88 Private
%102 = OpTypeRuntimeArray %68
%103 = OpTypeStruct %102
%104 = OpTypePointer StorageBuffer %103
%105 = OpVariable %104 StorageBuffer
%109 = OpVariable %88 Private
%111 = OpVariable %88 Private
%112 = OpSpecConstantOp %6 CooperativeMatrixLengthNV %71
%113 = OpSpecConstantOp %78 IAdd %112 %79
%117 = OpTypeRuntimeArray %68
%118 = OpTypeStruct %117
%119 = OpTypePointer StorageBuffer %118
%120 = OpVariable %119 StorageBuffer
%123 = OpSpecConstant %78 1
%124 = OpSpecConstant %78 1
%125 = OpSpecConstant %78 1
%126 = OpSpecConstant %78 1
%127 = OpSpecConstant %6 1
%128 = OpSpecConstant %6 1
%129 = OpSpecConstantComposite %19 %127 %128 %34
%4 = OpFunction %2 None %3
%5 = OpLabel
%9 = OpVariable %8 Function
%18 = OpVariable %8 Function
%32 = OpVariable %31 Function
%44 = OpVariable %31 Function
%52 = OpVariable %31 Function
%60 = OpVariable %31 Function
%73 = OpVariable %72 Function
%91 = OpVariable %72 Function
%101 = OpVariable %72 Function
%12 = OpLoad %6 %11
%14 = OpUMod %6 %12 %13
%15 = OpLoad %6 %11
%16 = OpUDiv %6 %15 %13
%17 = OpCompositeConstruct %7 %14 %16
OpStore %9 %17
%22 = OpLoad %19 %21
%23 = OpVectorShuffle %7 %22 %22 0 1
%24 = OpCompositeExtract %6 %23 0
%25 = OpCompositeExtract %6 %23 1
%26 = OpCompositeConstruct %7 %24 %25
%28 = OpIMul %7 %26 %27
%29 = OpLoad %7 %9
%30 = OpIAdd %7 %28 %29
OpStore %18 %30
%35 = OpAccessChain %31 %18 %34
%36 = OpLoad %6 %35
%37 = OpIMul %6 %33 %36
%40 = OpAccessChain %31 %18 %39
%41 = OpLoad %6 %40
%42 = OpIMul %6 %38 %41
%43 = OpIAdd %6 %37 %42
OpStore %32 %43
%45 = OpAccessChain %31 %18 %34
%46 = OpLoad %6 %45
%47 = OpIMul %6 %33 %46
%48 = OpAccessChain %31 %18 %39
%49 = OpLoad %6 %48
%50 = OpIMul %6 %38 %49
%51 = OpIAdd %6 %47 %50
OpStore %44 %51
%53 = OpAccessChain %31 %18 %34
%54 = OpLoad %6 %53
%55 = OpIMul %6 %33 %54
%56 = OpAccessChain %31 %18 %39
%57 = OpLoad %6 %56
%58 = OpIMul %6 %38 %57
%59 = OpIAdd %6 %55 %58
OpStore %52 %59
%61 = OpAccessChain %31 %18 %34
%62 = OpLoad %6 %61
%63 = OpIMul %6 %33 %62
%64 = OpAccessChain %31 %18 %39
%65 = OpLoad %6 %64
%66 = OpIMul %6 %38 %65
%67 = OpIAdd %6 %63 %66
OpStore %60 %67
%80 = OpLoad %6 %32
%83 = OpAccessChain %82 %77 %79 %80
%87 = OpCooperativeMatrixLoadNV %71 %83 %84 %86 )" +
                  loadMemoryAccess + R"( %81
OpStore %73 %87
%90 = OpLoad %71 %73
OpStore %89 %90
%96 = OpLoad %6 %44
%97 = OpAccessChain %82 %95 %79 %96
%98 = OpCooperativeMatrixLoadNV %71 %97 %84 %86 MakePointerVisibleKHR|NonPrivatePointerKHR %81
OpStore %91 %98
%100 = OpLoad %71 %91
OpStore %99 %100
%106 = OpLoad %6 %52
%107 = OpAccessChain %82 %105 %79 %106
%108 = OpCooperativeMatrixLoadNV %71 %107 %84 %86 MakePointerVisibleKHR|NonPrivatePointerKHR %81
OpStore %101 %108
%110 = OpLoad %71 %101
OpStore %109 %110
%114 = OpConvertSToF %68 %113
%115 = OpCompositeConstruct %71 %114
OpStore %111 %115
%116 = OpLoad %71 %111
%121 = OpLoad %6 %60
%122 = OpAccessChain %82 %120 %79 %121
OpCooperativeMatrixStoreNV %122 %116 %84 %86 )" + storeMemoryAccess + R"( %81
OpReturn
OpFunctionEnd
)";

  return s;
}

TEST_F(ValidateMemory, CoopMatLoadStoreSuccess) {
  std::string spirv =
      GenCoopMatLoadStoreShader("MakePointerAvailableKHR|NonPrivatePointerKHR",
                                "MakePointerVisibleKHR|NonPrivatePointerKHR");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_1));
}

TEST_F(ValidateMemory, CoopMatStoreMemoryAccessFail) {
  std::string spirv =
      GenCoopMatLoadStoreShader("MakePointerVisibleKHR|NonPrivatePointerKHR",
                                "MakePointerVisibleKHR|NonPrivatePointerKHR");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("MakePointerVisibleKHR cannot be used with OpStore"));
}

TEST_F(ValidateMemory, CoopMatLoadMemoryAccessFail) {
  std::string spirv =
      GenCoopMatLoadStoreShader("MakePointerAvailableKHR|NonPrivatePointerKHR",
                                "MakePointerAvailableKHR|NonPrivatePointerKHR");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("MakePointerAvailableKHR cannot be used with OpLoad"));
}

TEST_F(ValidateMemory, CoopMatInvalidStorageClassFail) {
  const std::string body =
      R"(
OpCapability Shader
OpCapability Float16
OpCapability CooperativeMatrixNV
OpExtension "SPV_NV_cooperative_matrix"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
%void = OpTypeVoid
%func = OpTypeFunction %void
%f16 = OpTypeFloat 16
%u32 = OpTypeInt 32 0

%u32_8 = OpConstant %u32 8
%subgroup = OpConstant %u32 3

%f16mat = OpTypeCooperativeMatrixNV %f16 %subgroup %u32_8 %u32_8

%str = OpTypeStruct %f16mat
%str_ptr = OpTypePointer Workgroup %str
%sh = OpVariable %str_ptr Workgroup

%main = OpFunction %void None %func
%main_entry = OpLabel

OpReturn
OpFunctionEnd)";

  CompileSuccessfully(body.c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Cooperative matrix types (or types containing them) can only be "
          "allocated in Function or Private storage classes or as function "
          "parameters"));
}

TEST_F(ValidateMemory, CoopMatMatrixLengthResultTypeBad) {
  const std::string body =
      R"(
OpCapability Shader
OpCapability Float16
OpCapability CooperativeMatrixNV
OpExtension "SPV_NV_cooperative_matrix"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
%void = OpTypeVoid
%func = OpTypeFunction %void
%f16 = OpTypeFloat 16
%u32 = OpTypeInt 32 0
%i32 = OpTypeInt 32 1

%u32_8 = OpConstant %u32 8
%subgroup = OpConstant %u32 3

%f16mat = OpTypeCooperativeMatrixNV %f16 %subgroup %u32_8 %u32_8

%main = OpFunction %void None %func
%main_entry = OpLabel

%1 = OpCooperativeMatrixLengthNV %i32 %f16mat

OpReturn
OpFunctionEnd)";

  CompileSuccessfully(body.c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("The Result Type of OpCooperativeMatrixLengthNV <id> "
                "'11[%11]' must be OpTypeInt with width 32 and signedness 0"));
}

TEST_F(ValidateMemory, CoopMatMatrixLengthOperandTypeBad) {
  const std::string body =
      R"(
OpCapability Shader
OpCapability Float16
OpCapability CooperativeMatrixNV
OpExtension "SPV_NV_cooperative_matrix"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
%void = OpTypeVoid
%func = OpTypeFunction %void
%f16 = OpTypeFloat 16
%u32 = OpTypeInt 32 0
%i32 = OpTypeInt 32 1

%u32_8 = OpConstant %u32 8
%subgroup = OpConstant %u32 3

%f16mat = OpTypeCooperativeMatrixNV %f16 %subgroup %u32_8 %u32_8

%main = OpFunction %void None %func
%main_entry = OpLabel

%1 = OpCooperativeMatrixLengthNV %u32 %u32

OpReturn
OpFunctionEnd)";

  CompileSuccessfully(body.c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("The type in OpCooperativeMatrixLengthNV <id> '5[%uint]' "
                "must be OpTypeCooperativeMatrixNV"));
}

TEST_F(ValidateMemory, CoopMatMatrixLengthGood) {
  const std::string body =
      R"(
OpCapability Shader
OpCapability Float16
OpCapability CooperativeMatrixNV
OpExtension "SPV_NV_cooperative_matrix"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
%void = OpTypeVoid
%func = OpTypeFunction %void
%f16 = OpTypeFloat 16
%u32 = OpTypeInt 32 0
%i32 = OpTypeInt 32 1

%u32_8 = OpConstant %u32 8
%subgroup = OpConstant %u32 3

%f16mat = OpTypeCooperativeMatrixNV %f16 %subgroup %u32_8 %u32_8

%main = OpFunction %void None %func
%main_entry = OpLabel

%1 = OpCooperativeMatrixLengthNV %u32 %f16mat

OpReturn
OpFunctionEnd)";

  CompileSuccessfully(body.c_str());
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions());
}

std::string GenCoopMatLoadStoreShaderKHR(const std::string& storeMemoryAccess,
                                         const std::string& loadMemoryAccess,
                                         unsigned layout = 0,
                                         bool useSpecConstantLayout = false,
                                         bool useStoreStride = true,
                                         bool useLoadStride = true) {
  std::string s = R"(
OpCapability Shader
OpCapability GroupNonUniform
OpCapability VulkanMemoryModelKHR
OpCapability CooperativeMatrixKHR
OpExtension "SPV_KHR_vulkan_memory_model"
OpExtension "SPV_KHR_cooperative_matrix"
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical VulkanKHR
OpEntryPoint GLCompute %4 "main" %11 %21
OpExecutionMode %4 LocalSize 1 1 1
OpDecorate %11 BuiltIn SubgroupId
OpDecorate %21 BuiltIn WorkgroupId
OpDecorate %74 ArrayStride 4
OpMemberDecorate %75 0 Offset 0
OpDecorate %75 Block
OpDecorate %77 DescriptorSet 0
OpDecorate %77 Binding 0
OpDecorate %92 ArrayStride 4
OpMemberDecorate %93 0 Offset 0
OpDecorate %93 Block
OpDecorate %95 DescriptorSet 0
OpDecorate %95 Binding 1
OpDecorate %102 ArrayStride 4
OpMemberDecorate %103 0 Offset 0
OpDecorate %103 Block
OpDecorate %105 DescriptorSet 0
OpDecorate %105 Binding 2
OpDecorate %117 ArrayStride 4
OpMemberDecorate %118 0 Offset 0
OpDecorate %118 Block
OpDecorate %120 DescriptorSet 0
OpDecorate %120 Binding 3
OpDecorate %123 SpecId 2
OpDecorate %124 SpecId 3
OpDecorate %125 SpecId 4
OpDecorate %126 SpecId 5
OpDecorate %127 SpecId 0
OpDecorate %128 SpecId 1
OpDecorate %129 BuiltIn WorkgroupSize
%2 = OpTypeVoid
%3 = OpTypeFunction %2
%6 = OpTypeInt 32 0
%7 = OpTypeVector %6 2
%8 = OpTypePointer Function %7
%10 = OpTypePointer Input %6
%11 = OpVariable %10 Input
%13 = OpConstant %6 2
%19 = OpTypeVector %6 3
%20 = OpTypePointer Input %19
%21 = OpVariable %20 Input
%27 = OpConstantComposite %7 %13 %13
%31 = OpTypePointer Function %6
%33 = OpConstant %6 1024
%34 = OpConstant %6 1
%38 = OpConstant %6 8
%uint_0 = OpConstant %6 0
)";
  if (useSpecConstantLayout) {
    s += "%layout = OpSpecConstant %6 " + std::to_string(layout);
  } else {
    s += "%layout = OpConstant %6 " + std::to_string(layout);
  }
  s += R"(
%68 = OpTypeFloat 32
%69 = OpConstant %6 16
%70 = OpConstant %6 3
%71 = OpTypeCooperativeMatrixKHR %68 %70 %69 %38 %uint_0
%72 = OpTypePointer Function %71
%74 = OpTypeRuntimeArray %68
%75 = OpTypeStruct %74
%76 = OpTypePointer StorageBuffer %75
%77 = OpVariable %76 StorageBuffer
%78 = OpTypeInt 32 1
%79 = OpConstant %78 0
%81 = OpConstant %6 5
%82 = OpTypePointer StorageBuffer %68
%stride = OpConstant %6 64
%88 = OpTypePointer Private %71
%89 = OpVariable %88 Private
%92 = OpTypeRuntimeArray %68
%93 = OpTypeStruct %92
%94 = OpTypePointer StorageBuffer %93
%95 = OpVariable %94 StorageBuffer
%99 = OpVariable %88 Private
%102 = OpTypeRuntimeArray %68
%103 = OpTypeStruct %102
%104 = OpTypePointer StorageBuffer %103
%105 = OpVariable %104 StorageBuffer
%109 = OpVariable %88 Private
%111 = OpVariable %88 Private
%112 = OpSpecConstantOp %6 CooperativeMatrixLengthKHR %71
%113 = OpSpecConstantOp %78 IAdd %112 %79
%117 = OpTypeRuntimeArray %68
%118 = OpTypeStruct %117
%119 = OpTypePointer StorageBuffer %118
%120 = OpVariable %119 StorageBuffer
%123 = OpSpecConstant %78 1
%124 = OpSpecConstant %78 1
%125 = OpSpecConstant %78 1
%126 = OpSpecConstant %78 1
%127 = OpSpecConstant %6 1
%128 = OpSpecConstant %6 1
%129 = OpSpecConstantComposite %19 %127 %128 %34
%4 = OpFunction %2 None %3
%5 = OpLabel
%9 = OpVariable %8 Function
%18 = OpVariable %8 Function
%32 = OpVariable %31 Function
%44 = OpVariable %31 Function
%52 = OpVariable %31 Function
%60 = OpVariable %31 Function
%73 = OpVariable %72 Function
%91 = OpVariable %72 Function
%101 = OpVariable %72 Function
%12 = OpLoad %6 %11
%14 = OpUMod %6 %12 %13
%15 = OpLoad %6 %11
%16 = OpUDiv %6 %15 %13
%17 = OpCompositeConstruct %7 %14 %16
OpStore %9 %17
%22 = OpLoad %19 %21
%23 = OpVectorShuffle %7 %22 %22 0 1
%24 = OpCompositeExtract %6 %23 0
%25 = OpCompositeExtract %6 %23 1
%26 = OpCompositeConstruct %7 %24 %25
%28 = OpIMul %7 %26 %27
%29 = OpLoad %7 %9
%30 = OpIAdd %7 %28 %29
OpStore %18 %30
%35 = OpAccessChain %31 %18 %34
%36 = OpLoad %6 %35
%37 = OpIMul %6 %33 %36
%40 = OpAccessChain %31 %18 %uint_0
%41 = OpLoad %6 %40
%42 = OpIMul %6 %38 %41
%43 = OpIAdd %6 %37 %42
OpStore %32 %43
%45 = OpAccessChain %31 %18 %34
%46 = OpLoad %6 %45
%47 = OpIMul %6 %33 %46
%48 = OpAccessChain %31 %18 %uint_0
%49 = OpLoad %6 %48
%50 = OpIMul %6 %38 %49
%51 = OpIAdd %6 %47 %50
OpStore %44 %51
%53 = OpAccessChain %31 %18 %34
%54 = OpLoad %6 %53
%55 = OpIMul %6 %33 %54
%56 = OpAccessChain %31 %18 %uint_0
%57 = OpLoad %6 %56
%58 = OpIMul %6 %38 %57
%59 = OpIAdd %6 %55 %58
OpStore %52 %59
%61 = OpAccessChain %31 %18 %34
%62 = OpLoad %6 %61
%63 = OpIMul %6 %33 %62
%64 = OpAccessChain %31 %18 %uint_0
%65 = OpLoad %6 %64
%66 = OpIMul %6 %38 %65
%67 = OpIAdd %6 %63 %66
OpStore %60 %67
%80 = OpLoad %6 %32
%83 = OpAccessChain %82 %77 %79 %80
)";
  if (useLoadStride) {
    s += "%87 = OpCooperativeMatrixLoadKHR %71 %83 %layout %stride " +
         loadMemoryAccess + " %81";
  } else {
    s += "%87 = OpCooperativeMatrixLoadKHR %71 %83 %layout";
  }
  s += R"(
OpStore %73 %87
%90 = OpLoad %71 %73
OpStore %89 %90
%96 = OpLoad %6 %44
%97 = OpAccessChain %82 %95 %79 %96
%98 = OpCooperativeMatrixLoadKHR %71 %97 %layout %stride MakePointerVisibleKHR|NonPrivatePointerKHR %81
OpStore %91 %98
%100 = OpLoad %71 %91
OpStore %99 %100
%106 = OpLoad %6 %52
%107 = OpAccessChain %82 %105 %79 %106
%108 = OpCooperativeMatrixLoadKHR %71 %107 %layout %stride MakePointerVisibleKHR|NonPrivatePointerKHR %81
OpStore %101 %108
%110 = OpLoad %71 %101
OpStore %109 %110
%114 = OpConvertSToF %68 %113
%115 = OpCompositeConstruct %71 %114
OpStore %111 %115
%116 = OpLoad %71 %111
%121 = OpLoad %6 %60
%122 = OpAccessChain %82 %120 %79 %121
)";
  if (useStoreStride) {
    s += "OpCooperativeMatrixStoreKHR %122 %116 %layout %stride " +
         storeMemoryAccess + " %81";
  } else {
    s += "OpCooperativeMatrixStoreKHR %122 %116 %layout";
  }
  s += R"(
OpReturn
OpFunctionEnd
)";

  return s;
}

TEST_F(ValidateMemory, CoopMatKHRLoadStoreSuccess) {
  std::string spirv = GenCoopMatLoadStoreShaderKHR(
      "MakePointerAvailableKHR|NonPrivatePointerKHR",
      "MakePointerVisibleKHR|NonPrivatePointerKHR");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_1));
}

struct StrideMissingCase {
  unsigned layout;
  bool useLoadStride;
  bool useStoreStride;
};

using ValidateCoopMatrixStrideMissing =
    spvtest::ValidateBase<StrideMissingCase>;

INSTANTIATE_TEST_SUITE_P(
    CoopMatrixStrideMissing, ValidateCoopMatrixStrideMissing,
    Values(
        StrideMissingCase{(unsigned)spv::CooperativeMatrixLayout::RowMajorKHR,
                          false, true},
        StrideMissingCase{(unsigned)spv::CooperativeMatrixLayout::RowMajorKHR,
                          true, false},
        StrideMissingCase{
            (unsigned)spv::CooperativeMatrixLayout::ColumnMajorKHR, false,
            true},
        StrideMissingCase{
            (unsigned)spv::CooperativeMatrixLayout::ColumnMajorKHR, true,
            false}));

TEST_P(ValidateCoopMatrixStrideMissing, CoopMatKHRLoadStrideMissingFail) {
  const StrideMissingCase& param = GetParam();
  std::string spirv = GenCoopMatLoadStoreShaderKHR(
      "MakePointerAvailableKHR|NonPrivatePointerKHR",
      "MakePointerVisibleKHR|NonPrivatePointerKHR", param.layout,
      false /*useSpecConstantLayout*/, param.useStoreStride,
      param.useLoadStride);
  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("MemoryLayout " + std::to_string(param.layout) +
                        " requires a Stride"));
}

TEST_F(ValidateMemory, CoopMatKHRMemoryLayoutFromSpecConstantSuccess) {
  std::string spirv = GenCoopMatLoadStoreShaderKHR(
      "MakePointerAvailableKHR|NonPrivatePointerKHR",
      "MakePointerVisibleKHR|NonPrivatePointerKHR",
      (unsigned)spv::CooperativeMatrixLayout::RowMajorKHR,
      true /*useSpecConstantLayout*/);

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_1));
}

TEST_F(ValidateMemory, CoopMatKHRStoreMemoryAccessFail) {
  std::string spirv = GenCoopMatLoadStoreShaderKHR(
      "MakePointerVisibleKHR|NonPrivatePointerKHR",
      "MakePointerVisibleKHR|NonPrivatePointerKHR");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("MakePointerVisibleKHR cannot be used with OpStore"));
}

TEST_F(ValidateMemory, CoopMatKHRLoadMemoryAccessFail) {
  std::string spirv = GenCoopMatLoadStoreShaderKHR(
      "MakePointerAvailableKHR|NonPrivatePointerKHR",
      "MakePointerAvailableKHR|NonPrivatePointerKHR");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("MakePointerAvailableKHR cannot be used with OpLoad"));
}

TEST_F(ValidateMemory, CoopMatKHRInvalidStorageClassFail) {
  const std::string body = R"(
OpCapability Shader
OpCapability Float16
OpCapability CooperativeMatrixKHR
OpCapability VulkanMemoryModelKHR
OpExtension "SPV_KHR_cooperative_matrix"
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
OpEntryPoint GLCompute %main "main"
%void = OpTypeVoid
%func = OpTypeFunction %void
%f16 = OpTypeFloat 16
%u32 = OpTypeInt 32 0

%u32_8 = OpConstant %u32 8
%use_A = OpConstant %u32 0
%subgroup = OpConstant %u32 3

%f16mat = OpTypeCooperativeMatrixKHR %f16 %subgroup %u32_8 %u32_8 %use_A

%str = OpTypeStruct %f16mat
%str_ptr = OpTypePointer Workgroup %str
%sh = OpVariable %str_ptr Workgroup

%main = OpFunction %void None %func
%main_entry = OpLabel

OpReturn
OpFunctionEnd)";

  CompileSuccessfully(body.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Cooperative matrix types (or types containing them) can only be "
          "allocated in Function or Private storage classes or as function "
          "parameters"));
}

TEST_F(ValidateMemory, CoopMatMatrixKHRLengthResultTypeBad) {
  const std::string body = R"(
OpCapability Shader
OpCapability Float16
OpCapability CooperativeMatrixKHR
OpCapability VulkanMemoryModelKHR
OpExtension "SPV_KHR_cooperative_matrix"
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
OpEntryPoint GLCompute %main "main"
%void = OpTypeVoid
%func = OpTypeFunction %void
%f16 = OpTypeFloat 16
%u32 = OpTypeInt 32 0
%i32 = OpTypeInt 32 1

%u32_8 = OpConstant %u32 8
%use_A = OpConstant %u32 0
%subgroup = OpConstant %u32 3

%f16mat = OpTypeCooperativeMatrixKHR %f16 %subgroup %u32_8 %u32_8 %use_A

%main = OpFunction %void None %func
%main_entry = OpLabel

%1 = OpCooperativeMatrixLengthKHR %i32 %f16mat

OpReturn
OpFunctionEnd)";

  CompileSuccessfully(body.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("The Result Type of OpCooperativeMatrixLengthKHR <id> "
                "'12[%12]' must be OpTypeInt with width 32 and signedness 0"));
}

TEST_F(ValidateMemory, CoopMatMatrixKHRLengthOperandTypeBad) {
  const std::string body =
      R"(
OpCapability Shader
OpCapability Float16
OpCapability CooperativeMatrixKHR
OpCapability VulkanMemoryModelKHR
OpExtension "SPV_KHR_cooperative_matrix"
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
OpEntryPoint GLCompute %main "main"
%void = OpTypeVoid
%func = OpTypeFunction %void
%f16 = OpTypeFloat 16
%u32 = OpTypeInt 32 0
%i32 = OpTypeInt 32 1

%u32_8 = OpConstant %u32 8
%use_A = OpConstant %u32 0
%subgroup = OpConstant %u32 3

%f16mat = OpTypeCooperativeMatrixKHR %f16 %subgroup %u32_8 %u32_8 %use_A

%main = OpFunction %void None %func
%main_entry = OpLabel

%1 = OpCooperativeMatrixLengthKHR %u32 %u32

OpReturn
OpFunctionEnd)";

  CompileSuccessfully(body.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("The type in OpCooperativeMatrixLengthKHR <id> '5[%uint]' "
                "must be OpTypeCooperativeMatrixKHR"));
}

TEST_F(ValidateMemory, CoopMatMatrixKHRLengthGood) {
  const std::string body =
      R"(
OpCapability Shader
OpCapability Float16
OpCapability CooperativeMatrixKHR
OpCapability VulkanMemoryModelKHR
OpExtension "SPV_KHR_cooperative_matrix"
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
OpEntryPoint GLCompute %main "main"
%void = OpTypeVoid
%func = OpTypeFunction %void
%f16 = OpTypeFloat 16
%u32 = OpTypeInt 32 0
%i32 = OpTypeInt 32 1

%u32_8 = OpConstant %u32 8
%use_A = OpConstant %u32 0
%subgroup = OpConstant %u32 3

%f16mat = OpTypeCooperativeMatrixKHR %f16 %subgroup %u32_8 %u32_8 %use_A

%main = OpFunction %void None %func
%main_entry = OpLabel

%1 = OpCooperativeMatrixLengthKHR %u32 %f16mat

OpReturn
OpFunctionEnd)";

  CompileSuccessfully(body.c_str(), SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateMemory, VulkanRTAOutsideOfStructBad) {
  std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
%sampler_t = OpTypeSampler
%array_t = OpTypeRuntimeArray %sampler_t
%array_ptr = OpTypePointer UniformConstant %array_t
%2 = OpVariable %array_ptr UniformConstant
%void = OpTypeVoid
%func_t = OpTypeFunction %void
%func = OpFunction %void None %func_t
%1 = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpTypeRuntimeArray-04680"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "OpVariable, <id> '5[%5]', is attempting to create memory for an "
          "illegal type, OpTypeRuntimeArray.\nFor Vulkan OpTypeRuntimeArray "
          "can only appear as the final member of an OpTypeStruct, thus cannot "
          "be instantiated via OpVariable\n  %5 = OpVariable "
          "%_ptr_UniformConstant__runtimearr_2 UniformConstant\n"));
}

TEST_F(ValidateMemory, VulkanRTAOutsideOfStructWithRuntimeDescriptorArrayGood) {
  std::string spirv = R"(
OpCapability Shader
OpCapability RuntimeDescriptorArrayEXT
OpExtension "SPV_EXT_descriptor_indexing"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
OpDecorate %struct Block
OpMemberDecorate %struct 0 Offset 0
%sampler_t = OpTypeSampler
%uint = OpTypeInt 32 0
%array_t = OpTypeRuntimeArray %sampler_t
%struct = OpTypeStruct %uint
%sb_array_t = OpTypeRuntimeArray %struct
%array_sb_ptr = OpTypePointer StorageBuffer %sb_array_t
%2 = OpVariable %array_sb_ptr StorageBuffer
%array_uc_ptr = OpTypePointer UniformConstant %array_t
%3 = OpVariable %array_uc_ptr UniformConstant
%void = OpTypeVoid
%func_t = OpTypeFunction %void
%func = OpFunction %void None %func_t
%1 = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_1));
}

TEST_F(
    ValidateMemory,
    VulkanRTAOutsideOfStructWithRuntimeDescriptorArrayAndWrongStorageClassBad) {
  std::string spirv = R"(
OpCapability Shader
OpCapability RuntimeDescriptorArrayEXT
OpExtension "SPV_EXT_descriptor_indexing"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
%uint_t = OpTypeInt 32 0
%array_t = OpTypeRuntimeArray %uint_t
%array_ptr = OpTypePointer Workgroup %array_t
%2 = OpVariable %array_ptr Workgroup
%void = OpTypeVoid
%func_t = OpTypeFunction %void
%func = OpFunction %void None %func_t
%1 = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpTypeRuntimeArray-04680"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("For Vulkan with RuntimeDescriptorArrayEXT, a variable "
                "containing OpTypeRuntimeArray must have storage class of "
                "StorageBuffer, Uniform, or UniformConstant.\n  %5 = "
                "OpVariable %_ptr_Workgroup__runtimearr_uint Workgroup\n"));
}

TEST_F(ValidateMemory, VulkanRTAInsideStorageBufferStructGood) {
  std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
OpDecorate %array_t ArrayStride 4
OpMemberDecorate %struct_t 0 Offset 0
OpDecorate %struct_t Block
%uint_t = OpTypeInt 32 0
%array_t = OpTypeRuntimeArray %uint_t
%struct_t = OpTypeStruct %array_t
%struct_ptr = OpTypePointer StorageBuffer %struct_t
%2 = OpVariable %struct_ptr StorageBuffer
%void = OpTypeVoid
%func_t = OpTypeFunction %void
%func = OpFunction %void None %func_t
%1 = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_1));
}

TEST_F(ValidateMemory, VulkanRTAInsideWrongStorageClassStructBad) {
  std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
OpDecorate %struct_t Block
%uint_t = OpTypeInt 32 0
%array_t = OpTypeRuntimeArray %uint_t
%struct_t = OpTypeStruct %array_t
%struct_ptr = OpTypePointer Workgroup %struct_t
%2 = OpVariable %struct_ptr Workgroup
%void = OpTypeVoid
%func_t = OpTypeFunction %void
%func = OpFunction %void None %func_t
%1 = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpTypeRuntimeArray-04680"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "For Vulkan, OpTypeStruct variables containing OpTypeRuntimeArray "
          "must have storage class of StorageBuffer, PhysicalStorageBuffer, or "
          "Uniform.\n  %6 = "
          "OpVariable %_ptr_Workgroup__struct_2 Workgroup\n"));
}

TEST_F(ValidateMemory, VulkanRTAInsideStorageBufferStructWithoutBlockBad) {
  std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
OpDecorate %struct_t BufferBlock
%uint_t = OpTypeInt 32 0
%array_t = OpTypeRuntimeArray %uint_t
%struct_t = OpTypeStruct %array_t
%struct_ptr = OpTypePointer StorageBuffer %struct_t
%2 = OpVariable %struct_ptr StorageBuffer
%void = OpTypeVoid
%func_t = OpTypeFunction %void
%func = OpFunction %void None %func_t
%1 = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpTypeRuntimeArray-04680"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("For Vulkan, an OpTypeStruct variable containing an "
                        "OpTypeRuntimeArray must be decorated with Block if it "
                        "has storage class StorageBuffer or "
                        "PhysicalStorageBuffer.\n  %6 = OpVariable "
                        "%_ptr_StorageBuffer__struct_2 StorageBuffer\n"));
}

TEST_F(ValidateMemory, VulkanRTAInsideUniformStructGood) {
  std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
OpDecorate %array_t ArrayStride 4
OpMemberDecorate %struct_t 0 Offset 0
OpDecorate %struct_t BufferBlock
%uint_t = OpTypeInt 32 0
%array_t = OpTypeRuntimeArray %uint_t
%struct_t = OpTypeStruct %array_t
%struct_ptr = OpTypePointer Uniform %struct_t
%2 = OpVariable %struct_ptr Uniform
%void = OpTypeVoid
%func_t = OpTypeFunction %void
%func = OpFunction %void None %func_t
%1 = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_1));
}

TEST_F(ValidateMemory, VulkanRTAInsideUniformStructWithoutBufferBlockBad) {
  std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
OpDecorate %struct_t Block
%uint_t = OpTypeInt 32 0
%array_t = OpTypeRuntimeArray %uint_t
%struct_t = OpTypeStruct %array_t
%struct_ptr = OpTypePointer Uniform %struct_t
%2 = OpVariable %struct_ptr Uniform
%void = OpTypeVoid
%func_t = OpTypeFunction %void
%func = OpFunction %void None %func_t
%1 = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpTypeRuntimeArray-04680"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("For Vulkan, an OpTypeStruct variable containing an "
                        "OpTypeRuntimeArray must be decorated with BufferBlock "
                        "if it has storage class Uniform.\n  %6 = OpVariable "
                        "%_ptr_Uniform__struct_2 Uniform\n"));
}

TEST_F(ValidateMemory, VulkanRTAInsideRTABad) {
  std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
%sampler_t = OpTypeSampler
%inner_array_t = OpTypeRuntimeArray %sampler_t
%array_t = OpTypeRuntimeArray %inner_array_t
%array_ptr = OpTypePointer UniformConstant %array_t
%2 = OpVariable %array_ptr UniformConstant
%void = OpTypeVoid
%func_t = OpTypeFunction %void
%func = OpFunction %void None %func_t
%1 = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpTypeRuntimeArray-04680"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "OpTypeRuntimeArray Element Type <id> '3[%_runtimearr_2]' is not "
          "valid in Vulkan environments.\n  %_runtimearr__runtimearr_2 = "
          "OpTypeRuntimeArray %_runtimearr_2\n"));
}

TEST_F(ValidateMemory, VulkanRTAInsideRTAWithRuntimeDescriptorArrayBad) {
  std::string spirv = R"(
OpCapability RuntimeDescriptorArrayEXT
OpCapability Shader
OpExtension "SPV_EXT_descriptor_indexing"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
OpDecorate %struct Block
%uint_t = OpTypeInt 32 0
%inner_array_t = OpTypeRuntimeArray %uint_t
%array_t = OpTypeRuntimeArray %inner_array_t
%struct = OpTypeStruct %array_t
%array_ptr = OpTypePointer StorageBuffer %struct
%2 = OpVariable %array_ptr StorageBuffer
%void = OpTypeVoid
%func_t = OpTypeFunction %void
%func = OpFunction %void None %func_t
%1 = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpTypeRuntimeArray-04680"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "OpTypeRuntimeArray Element Type <id> '4[%_runtimearr_uint]' is not "
          "valid in Vulkan environments.\n  %_runtimearr__runtimearr_uint = "
          "OpTypeRuntimeArray %_runtimearr_uint\n"));
}

TEST_F(ValidateMemory,
       VulkanUniformStructInsideRTAWithRuntimeDescriptorArrayGood) {
  std::string spirv = R"(
OpCapability RuntimeDescriptorArrayEXT
OpCapability Shader
OpExtension "SPV_EXT_descriptor_indexing"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
OpMemberDecorate %struct_t 0 Offset 0
OpDecorate %struct_t Block
%uint_t = OpTypeInt 32 0
%struct_t = OpTypeStruct %uint_t
%array_t = OpTypeRuntimeArray %struct_t
%array_ptr = OpTypePointer Uniform %array_t
%2 = OpVariable %array_ptr Uniform
%void = OpTypeVoid
%func_t = OpTypeFunction %void
%func = OpFunction %void None %func_t
%1 = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_1));
}

TEST_F(ValidateMemory, VulkanRTAInsideRTAInsideStructBad) {
  std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
OpDecorate %array_t ArrayStride 4
OpMemberDecorate %struct_t 0 Offset 0
OpDecorate %struct_t Block
%uint_t = OpTypeInt 32 0
%inner_array_t = OpTypeRuntimeArray %uint_t
%array_t = OpTypeRuntimeArray %inner_array_t
%struct_t = OpTypeStruct %array_t
%struct_ptr = OpTypePointer StorageBuffer %struct_t
%2 = OpVariable %struct_ptr StorageBuffer
%void = OpTypeVoid
%func_t = OpTypeFunction %void
%func = OpFunction %void None %func_t
%1 = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpTypeRuntimeArray-04680"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "OpTypeRuntimeArray Element Type <id> '5[%_runtimearr_uint]' is not "
          "valid in Vulkan environments.\n  %_runtimearr__runtimearr_uint = "
          "OpTypeRuntimeArray %_runtimearr_uint\n"));
}

TEST_F(ValidateMemory,
       VulkanRTAInsideRTAInsideStructWithRuntimeDescriptorArrayBad) {
  std::string spirv = R"(
OpCapability RuntimeDescriptorArrayEXT
OpCapability Shader
OpExtension "SPV_EXT_descriptor_indexing"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
OpDecorate %array_t ArrayStride 4
OpMemberDecorate %struct_t 0 Offset 0
OpDecorate %struct_t Block
%uint_t = OpTypeInt 32 0
%inner_array_t = OpTypeRuntimeArray %uint_t
%array_t = OpTypeRuntimeArray %inner_array_t
%struct_t = OpTypeStruct %array_t
%struct_ptr = OpTypePointer StorageBuffer %struct_t
%2 = OpVariable %struct_ptr StorageBuffer
%void = OpTypeVoid
%func_t = OpTypeFunction %void
%func = OpFunction %void None %func_t
%1 = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpTypeRuntimeArray-04680"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "OpTypeRuntimeArray Element Type <id> '5[%_runtimearr_uint]' is not "
          "valid in Vulkan environments.\n  %_runtimearr__runtimearr_uint = "
          "OpTypeRuntimeArray %_runtimearr_uint\n"));
}

TEST_F(ValidateMemory, VulkanRTAInsideArrayBad) {
  std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
%uint_t = OpTypeInt 32 0
%dim = OpConstant %uint_t 1
%sampler_t = OpTypeSampler
%inner_array_t = OpTypeRuntimeArray %sampler_t
%array_t = OpTypeArray %inner_array_t %dim
%array_ptr = OpTypePointer UniformConstant %array_t
%2 = OpVariable %array_ptr UniformConstant
%void = OpTypeVoid
%func_t = OpTypeFunction %void
%func = OpFunction %void None %func_t
%1 = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpTypeRuntimeArray-04680"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("OpTypeArray Element Type <id> '5[%_runtimearr_4]' is not "
                "valid in Vulkan environments.\n  %_arr__runtimearr_4_uint_1 = "
                "OpTypeArray %_runtimearr_4 %uint_1\n"));
}

TEST_F(ValidateMemory, VulkanRTAInsideArrayWithRuntimeDescriptorArrayBad) {
  std::string spirv = R"(
OpCapability RuntimeDescriptorArrayEXT
OpCapability Shader
OpExtension "SPV_EXT_descriptor_indexing"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
OpDecorate %struct Block
%uint_t = OpTypeInt 32 0
%dim = OpConstant %uint_t 1
%sampler_t = OpTypeSampler
%inner_array_t = OpTypeRuntimeArray %uint_t
%array_t = OpTypeRuntimeArray %inner_array_t
%struct = OpTypeStruct %array_t
%array_ptr = OpTypePointer StorageBuffer %struct
%2 = OpVariable %array_ptr StorageBuffer
%void = OpTypeVoid
%func_t = OpTypeFunction %void
%func = OpFunction %void None %func_t
%1 = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpTypeRuntimeArray-04680"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "OpTypeRuntimeArray Element Type <id> '6[%_runtimearr_uint]' is not "
          "valid in Vulkan environments.\n  %_runtimearr__runtimearr_uint = "
          "OpTypeRuntimeArray %_runtimearr_uint\n"));
}

TEST_F(ValidateMemory, VulkanRTAInsideArrayInsideStructBad) {
  std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
OpDecorate %array_t ArrayStride 4
OpMemberDecorate %struct_t 0 Offset 0
OpDecorate %struct_t Block
%uint_t = OpTypeInt 32 0
%dim = OpConstant %uint_t 1
%inner_array_t = OpTypeRuntimeArray %uint_t
%array_t = OpTypeArray %inner_array_t %dim
%struct_t = OpTypeStruct %array_t
%struct_ptr = OpTypePointer StorageBuffer %struct_t
%2 = OpVariable %struct_ptr StorageBuffer
%void = OpTypeVoid
%func_t = OpTypeFunction %void
%func = OpFunction %void None %func_t
%1 = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpTypeRuntimeArray-04680"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "OpTypeArray Element Type <id> '6[%_runtimearr_uint]' is not "
          "valid in Vulkan environments.\n  %_arr__runtimearr_uint_uint_1 "
          "= OpTypeArray %_runtimearr_uint %uint_1\n"));
}

TEST_F(ValidateMemory,
       VulkanRTAInsideArrayInsideStructWithRuntimeDescriptorArrayBad) {
  std::string spirv = R"(
OpCapability RuntimeDescriptorArrayEXT
OpCapability Shader
OpExtension "SPV_EXT_descriptor_indexing"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
OpDecorate %array_t ArrayStride 4
OpMemberDecorate %struct_t 0 Offset 0
OpDecorate %struct_t Block
%uint_t = OpTypeInt 32 0
%dim = OpConstant %uint_t 1
%inner_array_t = OpTypeRuntimeArray %uint_t
%array_t = OpTypeArray %inner_array_t %dim
%struct_t = OpTypeStruct %array_t
%struct_ptr = OpTypePointer StorageBuffer %struct_t
%2 = OpVariable %struct_ptr StorageBuffer
%void = OpTypeVoid
%func_t = OpTypeFunction %void
%func = OpFunction %void None %func_t
%1 = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpTypeRuntimeArray-04680"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "OpTypeArray Element Type <id> '6[%_runtimearr_uint]' is not "
          "valid in Vulkan environments.\n  %_arr__runtimearr_uint_uint_1 "
          "= OpTypeArray %_runtimearr_uint %uint_1\n"));
}

TEST_F(ValidateMemory, VulkanRTAStructInsideRTAWithRuntimeDescriptorArrayGood) {
  std::string spirv = R"(
OpCapability RuntimeDescriptorArrayEXT
OpCapability Shader
OpExtension "SPV_EXT_descriptor_indexing"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
OpDecorate %inner_array_t ArrayStride 4
OpMemberDecorate %struct_t 0 Offset 0
OpDecorate %struct_t Block
%uint_t = OpTypeInt 32 0
%inner_array_t = OpTypeRuntimeArray %uint_t
%struct_t = OpTypeStruct %inner_array_t
%array_t = OpTypeRuntimeArray %struct_t
%array_ptr = OpTypePointer StorageBuffer %array_t
%2 = OpVariable %array_ptr StorageBuffer
%void = OpTypeVoid
%func_t = OpTypeFunction %void
%func = OpFunction %void None %func_t
%1 = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_1));
}

TEST_F(ValidateMemory, VulkanRTAStructInsideArrayGood) {
  std::string spirv = R"(
OpCapability RuntimeDescriptorArrayEXT
OpCapability Shader
OpExtension "SPV_EXT_descriptor_indexing"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
OpDecorate %inner_array_t ArrayStride 4
OpMemberDecorate %struct_t 0 Offset 0
OpDecorate %struct_t Block
%uint_t = OpTypeInt 32 0
%inner_array_t = OpTypeRuntimeArray %uint_t
%struct_t = OpTypeStruct %inner_array_t
%array_size = OpConstant %uint_t 5
%array_t = OpTypeArray %struct_t %array_size
%array_ptr = OpTypePointer StorageBuffer %array_t
%2 = OpVariable %array_ptr StorageBuffer
%void = OpTypeVoid
%func_t = OpTypeFunction %void
%func = OpFunction %void None %func_t
%1 = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_1));
}

TEST_F(ValidateMemory, CopyMemoryNoAccessGood) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_ptr_priv = OpTypePointer Private %int
%var1 = OpVariable %int_ptr_priv Private
%var2 = OpVariable %int_ptr_priv Private
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpCopyMemory %var1 %var2
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(), Eq(""));
}

TEST_F(ValidateMemory, CopyMemorySimpleMixedAccessGood) {
  // Test one memory access operand using features that don't require the
  // Vulkan memory model.
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_ptr_priv = OpTypePointer Private %int
%var1 = OpVariable %int_ptr_priv Private
%var2 = OpVariable %int_ptr_priv Private
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpCopyMemory %var1 %var2 Volatile|Aligned|Nontemporal 4
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(), Eq(""));
}

TEST_F(ValidateMemory, CopyMemorySimpleTwoMixedAccessV13Bad) {
  // Two memory access operands is invalid up to SPIR-V 1.3
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_ptr_priv = OpTypePointer Private %int
%var1 = OpVariable %int_ptr_priv Private
%var2 = OpVariable %int_ptr_priv Private
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpCopyMemory %var1 %var2 Volatile Volatile
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA,
            ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("CopyMemory with two memory access operands requires "
                        "SPIR-V 1.4 or later"));
}

TEST_F(ValidateMemory, CopyMemorySimpleTwoMixedAccessV14Good) {
  // Two memory access operands is valid in SPIR-V 1.4
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_ptr_priv = OpTypePointer Private %int
%var1 = OpVariable %int_ptr_priv Private
%var2 = OpVariable %int_ptr_priv Private
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpCopyMemory %var1 %var2 Volatile Volatile
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
  EXPECT_THAT(getDiagnosticString(), Eq(""));
}

TEST_F(ValidateMemory, CopyMemorySizedNoAccessGood) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability Addresses
OpMemoryModel Logical GLSL450
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_16 = OpConstant %int 16
%int_ptr_priv = OpTypePointer Private %int
%var1 = OpVariable %int_ptr_priv Private
%var2 = OpVariable %int_ptr_priv Private
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpCopyMemorySized %var1 %var2 %int_16
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(), Eq(""));
}

TEST_F(ValidateMemory, CopyMemorySizedSimpleMixedAccessGood) {
  // Test one memory access operand using features that don't require the
  // Vulkan memory model.
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability Addresses
OpMemoryModel Logical GLSL450
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_16 = OpConstant %int 16
%int_ptr_priv = OpTypePointer Private %int
%var1 = OpVariable %int_ptr_priv Private
%var2 = OpVariable %int_ptr_priv Private
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpCopyMemorySized %var1 %var2 %int_16 Volatile|Aligned|Nontemporal 4
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv);
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateMemory, CopyMemorySizedSimpleTwoMixedAccessV13Bad) {
  // Two memory access operands is invalid up to SPIR-V 1.3
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability Addresses
OpMemoryModel Logical GLSL450
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_16 = OpConstant %int 16
%int_ptr_priv = OpTypePointer Private %int
%var1 = OpVariable %int_ptr_priv Private
%var2 = OpVariable %int_ptr_priv Private
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpCopyMemorySized %var1 %var2 %int_16 Volatile Volatile
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA,
            ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("CopyMemorySized with two memory access operands requires "
                "SPIR-V 1.4 or later"));
}

TEST_F(ValidateMemory, CopyMemorySizedSimpleTwoMixedAccessV14Good) {
  // Two memory access operands is valid in SPIR-V 1.4
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability Addresses
OpMemoryModel Logical GLSL450
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_16 = OpConstant %int 16
%int_ptr_priv = OpTypePointer Private %int
%var1 = OpVariable %int_ptr_priv Private
%var2 = OpVariable %int_ptr_priv Private
%voidfn = OpTypeFunction %void
%func = OpFunction %void None %voidfn
%entry = OpLabel
OpCopyMemorySized %var1 %var2 %int_16 Volatile Volatile
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
  EXPECT_THAT(getDiagnosticString(), Eq(""));
}

using ValidatePointerComparisons = spvtest::ValidateBase<std::string>;

TEST_P(ValidatePointerComparisons, Good) {
  const std::string operation = GetParam();

  std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability VariablePointersStorageBuffer
OpMemoryModel Logical GLSL450
%void = OpTypeVoid
%bool = OpTypeBool
%int = OpTypeInt 32 0
%ptr_int = OpTypePointer StorageBuffer %int
%var = OpVariable %ptr_int StorageBuffer
%func_ty = OpTypeFunction %void
%func = OpFunction %void None %func_ty
%1 = OpLabel
%equal = )" + operation;

  if (operation == "OpPtrDiff") {
    spirv += " %int ";
  } else {
    spirv += " %bool ";
  }

  spirv += R"(%var %var
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
}

TEST_P(ValidatePointerComparisons, GoodWorkgroup) {
  const std::string operation = GetParam();

  std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability VariablePointers
OpMemoryModel Logical GLSL450
%void = OpTypeVoid
%bool = OpTypeBool
%int = OpTypeInt 32 0
%ptr_int = OpTypePointer Workgroup %int
%var = OpVariable %ptr_int Workgroup
%func_ty = OpTypeFunction %void
%func = OpFunction %void None %func_ty
%1 = OpLabel
%equal = )" + operation;

  if (operation == "OpPtrDiff") {
    spirv += " %int ";
  } else {
    spirv += " %bool ";
  }

  spirv += R"(%var %var
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
}

TEST_P(ValidatePointerComparisons, BadResultType) {
  const std::string operation = GetParam();

  std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability VariablePointersStorageBuffer
OpMemoryModel Logical GLSL450
%void = OpTypeVoid
%bool = OpTypeBool
%int = OpTypeInt 32 0
%ptr_int = OpTypePointer StorageBuffer %int
%var = OpVariable %ptr_int StorageBuffer
%func_ty = OpTypeFunction %void
%func = OpFunction %void None %func_ty
%1 = OpLabel
%equal = )" + operation;

  if (operation == "OpPtrDiff") {
    spirv += " %bool ";
  } else {
    spirv += " %int ";
  }

  spirv += R"(%var %var
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
  if (operation == "OpPtrDiff") {
    EXPECT_THAT(getDiagnosticString(),
                HasSubstr("Result Type must be an integer scalar"));
  } else {
    EXPECT_THAT(getDiagnosticString(),
                HasSubstr("Result Type must be OpTypeBool"));
  }
}

TEST_P(ValidatePointerComparisons, BadCapabilities) {
  const std::string operation = GetParam();

  std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
%void = OpTypeVoid
%bool = OpTypeBool
%int = OpTypeInt 32 0
%ptr_int = OpTypePointer StorageBuffer %int
%var = OpVariable %ptr_int StorageBuffer
%func_ty = OpTypeFunction %void
%func = OpFunction %void None %func_ty
%1 = OpLabel
%equal = )" + operation;

  if (operation == "OpPtrDiff") {
    spirv += " %int ";
  } else {
    spirv += " %bool ";
  }

  spirv += R"(%var %var
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  if (operation == "OpPtrDiff") {
    // Gets caught by the grammar.
    EXPECT_EQ(SPV_ERROR_INVALID_CAPABILITY,
              ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
  } else {
    EXPECT_EQ(SPV_ERROR_INVALID_ID,
              ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
    EXPECT_THAT(getDiagnosticString(),
                HasSubstr("Instruction cannot for logical addressing model be "
                          "used without a variable pointers capability"));
  }
}

TEST_P(ValidatePointerComparisons, BadOperandType) {
  const std::string operation = GetParam();

  std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability VariablePointersStorageBuffer
OpMemoryModel Logical GLSL450
%void = OpTypeVoid
%bool = OpTypeBool
%int = OpTypeInt 32 0
%ptr_int = OpTypePointer StorageBuffer %int
%var = OpVariable %ptr_int StorageBuffer
%func_ty = OpTypeFunction %void
%func = OpFunction %void None %func_ty
%1 = OpLabel
%ld = OpLoad %int %var
%equal = )" + operation;

  if (operation == "OpPtrDiff") {
    spirv += " %int ";
  } else {
    spirv += " %bool ";
  }

  spirv += R"(%ld %ld
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Operand type must be a pointer"));
}

TEST_P(ValidatePointerComparisons, BadStorageClassWorkgroup) {
  const std::string operation = GetParam();

  std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability VariablePointersStorageBuffer
OpMemoryModel Logical GLSL450
%void = OpTypeVoid
%bool = OpTypeBool
%int = OpTypeInt 32 0
%ptr_int = OpTypePointer Workgroup %int
%var = OpVariable %ptr_int Workgroup
%func_ty = OpTypeFunction %void
%func = OpFunction %void None %func_ty
%1 = OpLabel
%equal = )" + operation;

  if (operation == "OpPtrDiff") {
    spirv += " %int ";
  } else {
    spirv += " %bool ";
  }

  spirv += R"(%var %var
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Workgroup storage class pointer requires "
                        "VariablePointers capability to be specified"));
}

TEST_P(ValidatePointerComparisons, BadStorageClass) {
  const std::string operation = GetParam();

  std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability VariablePointersStorageBuffer
OpMemoryModel Logical GLSL450
%void = OpTypeVoid
%bool = OpTypeBool
%int = OpTypeInt 32 0
%ptr_int = OpTypePointer Private %int
%var = OpVariable %ptr_int Private
%func_ty = OpTypeFunction %void
%func = OpFunction %void None %func_ty
%1 = OpLabel
%equal = )" + operation;

  if (operation == "OpPtrDiff") {
    spirv += " %int ";
  } else {
    spirv += " %bool ";
  }

  spirv += R"(%var %var
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Invalid pointer storage class"));
}

TEST_P(ValidatePointerComparisons, BadDiffOperandTypes) {
  const std::string operation = GetParam();

  std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability VariablePointersStorageBuffer
OpMemoryModel Logical GLSL450
%void = OpTypeVoid
%bool = OpTypeBool
%int = OpTypeInt 32 0
%float = OpTypeFloat 32
%ptr_int = OpTypePointer Private %int
%var = OpVariable %ptr_int Private
%ptr_float = OpTypePointer Private %float
%var2 = OpVariable %ptr_float Private
%func_ty = OpTypeFunction %void
%func = OpFunction %void None %func_ty
%1 = OpLabel
%ld = OpLoad %int %var
%equal = )" + operation;

  if (operation == "OpPtrDiff") {
    spirv += " %int ";
  } else {
    spirv += " %bool ";
  }

  spirv += R"(%var %var2
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("The types of Operand 1 and Operand 2 must match"));
}

TEST_P(ValidatePointerComparisons, GoodUntypedPointerSameType) {
  const std::string operation = GetParam();

  std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability VariablePointersStorageBuffer
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
%void = OpTypeVoid
%bool = OpTypeBool
%int = OpTypeInt 32 0
%ptr = OpTypeUntypedPointerKHR StorageBuffer
%var = OpUntypedVariableKHR %ptr StorageBuffer
%func_ty = OpTypeFunction %void
%func = OpFunction %void None %func_ty
%1 = OpLabel
%equal = )" + operation;

  if (operation == "OpPtrDiff") {
    spirv += " %int ";
  } else {
    spirv += " %bool ";
  }

  spirv += R"(%var %var
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
}

TEST_P(ValidatePointerComparisons, GoodUntypedPointerSameStorageClass) {
  const std::string operation = GetParam();

  std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability VariablePointersStorageBuffer
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
%void = OpTypeVoid
%bool = OpTypeBool
%int = OpTypeInt 32 0
%ptr1 = OpTypeUntypedPointerKHR StorageBuffer
%var = OpUntypedVariableKHR %ptr1 StorageBuffer
%ptr2 = OpTypeUntypedPointerKHR StorageBuffer
%var2 = OpUntypedVariableKHR %ptr2 StorageBuffer
%func_ty = OpTypeFunction %void
%func = OpFunction %void None %func_ty
%1 = OpLabel
%equal = )" + operation;

  if (operation == "OpPtrDiff") {
    spirv += " %int ";
  } else {
    spirv += " %bool ";
  }

  spirv += R"(%var %var2
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  if (operation == "OpPtrDiff") {
    EXPECT_EQ(SPV_ERROR_INVALID_ID,
              ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
    EXPECT_THAT(getDiagnosticString(),
                HasSubstr("The types of Operand 1 and Operand 2 must match"));
  } else {
    EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
  }
}

TEST_P(ValidatePointerComparisons, BadUntypedPointerDiffStorageClass) {
  const std::string operation = GetParam();

  std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability VariablePointers
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
%void = OpTypeVoid
%bool = OpTypeBool
%int = OpTypeInt 32 0
%ptr1 = OpTypeUntypedPointerKHR StorageBuffer
%var1 = OpUntypedVariableKHR %ptr1 StorageBuffer
%ptr2 = OpTypeUntypedPointerKHR Workgroup
%var2 = OpUntypedVariableKHR %ptr2 Workgroup %int
%func_ty = OpTypeFunction %void
%func = OpFunction %void None %func_ty
%1 = OpLabel
%equal = )" + operation;

  if (operation == "OpPtrDiff") {
    spirv += " %int ";
  } else {
    spirv += " %bool ";
  }

  spirv += R"(%var1 %var2
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  if (operation == "OpPtrDiff") {
    EXPECT_EQ(SPV_ERROR_INVALID_ID,
              ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
    EXPECT_THAT(getDiagnosticString(),
                HasSubstr("The types of Operand 1 and Operand 2 must match"));
  } else {
    EXPECT_EQ(SPV_ERROR_INVALID_ID,
              ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
    EXPECT_THAT(getDiagnosticString(),
                HasSubstr("Pointer storage classes must match"));
  }
}

TEST_P(ValidatePointerComparisons, GoodMixedPointerSameStorageClass) {
  const std::string operation = GetParam();

  std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability VariablePointersStorageBuffer
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
%void = OpTypeVoid
%bool = OpTypeBool
%int = OpTypeInt 32 0
%ptr1 = OpTypeUntypedPointerKHR StorageBuffer
%var = OpUntypedVariableKHR %ptr1 StorageBuffer
%ptr2 = OpTypePointer StorageBuffer %int
%var2 = OpVariable %ptr2 StorageBuffer
%func_ty = OpTypeFunction %void
%func = OpFunction %void None %func_ty
%1 = OpLabel
%equal = )" + operation;

  if (operation == "OpPtrDiff") {
    spirv += " %int ";
  } else {
    spirv += " %bool ";
  }

  spirv += R"(%var %var2
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  if (operation == "OpPtrDiff") {
    EXPECT_EQ(SPV_ERROR_INVALID_ID,
              ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
    EXPECT_THAT(getDiagnosticString(),
                HasSubstr("The types of Operand 1 and Operand 2 must match"));
  } else {
    EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
  }
}

TEST_P(ValidatePointerComparisons, BadMixedPointerDiffStorageClass) {
  const std::string operation = GetParam();

  std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability VariablePointers
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
%void = OpTypeVoid
%bool = OpTypeBool
%int = OpTypeInt 32 0
%ptr1 = OpTypeUntypedPointerKHR StorageBuffer
%var1 = OpUntypedVariableKHR %ptr1 StorageBuffer
%ptr2 = OpTypePointer Workgroup %int
%var2 = OpVariable %ptr2 Workgroup
%func_ty = OpTypeFunction %void
%func = OpFunction %void None %func_ty
%1 = OpLabel
%equal = )" + operation;

  if (operation == "OpPtrDiff") {
    spirv += " %int ";
  } else {
    spirv += " %bool ";
  }

  spirv += R"(%var1 %var2
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  if (operation == "OpPtrDiff") {
    EXPECT_EQ(SPV_ERROR_INVALID_ID,
              ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
    EXPECT_THAT(getDiagnosticString(),
                HasSubstr("The types of Operand 1 and Operand 2 must match"));
  } else {
    EXPECT_EQ(SPV_ERROR_INVALID_ID,
              ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
    EXPECT_THAT(getDiagnosticString(),
                HasSubstr("Pointer storage classes must match"));
  }
}

INSTANTIATE_TEST_SUITE_P(PointerComparisons, ValidatePointerComparisons,
                         Values("OpPtrEqual", "OpPtrNotEqual", "OpPtrDiff"));

TEST_F(ValidateMemory, VariableInitializerWrongType) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability VariablePointersStorageBuffer
OpMemoryModel Logical GLSL450
%void = OpTypeVoid
%int = OpTypeInt 32 0
%float = OpTypeFloat 32
%ptr_wg_int = OpTypePointer Workgroup %int
%ptr_wg_float = OpTypePointer Workgroup %int
%wg_var = OpVariable %ptr_wg_int Workgroup
%ptr_private_wg_float = OpTypePointer Private %ptr_wg_float
%priv_var = OpVariable %ptr_private_wg_float Private %wg_var
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Initializer type must match the data type"));
}

TEST_F(ValidateMemory, StoreToImage) {
  const std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
%void = OpTypeVoid
%int = OpTypeInt 32 0
%img = OpTypeImage %int 2D 2 0 0 2 R32i
%ptr_img = OpTypePointer Function %img
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%var = OpVariable %ptr_img Function
%value = OpLoad %img %var
OpStore %var %value
OpReturn
OpFunctionEnd
)";
  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpTypeImage-06924"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Cannot store to OpTypeImage, OpTypeSampler, "
                "OpTypeSampledImage, or OpTypeAccelerationStructureKHR"));
}

TEST_F(ValidateMemory, StoreToImageArray) {
  const std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
%void = OpTypeVoid
%int = OpTypeInt 32 0
%img = OpTypeImage %int 2D 2 0 0 2 R32i
%arr_size = OpConstant %int 5
%i = OpConstant %int 2
%arr_img = OpTypeArray %img %arr_size
%ptr_img = OpTypePointer Function %img
%ptr_arr_img = OpTypePointer Function %arr_img
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%var = OpVariable %ptr_arr_img Function
%value = OpLoad %arr_img %var
OpStore %var %value
OpReturn
OpFunctionEnd
)";
  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpTypeImage-06924"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Cannot store to OpTypeImage, OpTypeSampler, "
                "OpTypeSampledImage, or OpTypeAccelerationStructureKHR"));
}

TEST_F(ValidateMemory, StoreToSampler) {
  const std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
%void = OpTypeVoid
%smp = OpTypeSampler
%ptr_smp = OpTypePointer Function %smp
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%var = OpVariable %ptr_smp Function
%value = OpLoad %smp %var
OpStore %var %value
OpReturn
OpFunctionEnd
)";
  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpTypeImage-06924"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Cannot store to OpTypeImage, OpTypeSampler, "
                "OpTypeSampledImage, or OpTypeAccelerationStructureKHR"));
}

TEST_F(ValidateMemory, StoreToSampledImage) {
  const std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
%void = OpTypeVoid
%int = OpTypeInt 32 0
%img = OpTypeImage %int 2D 2 0 0 1 R32i
%samp_img = OpTypeSampledImage %img
%ptr_samp_img = OpTypePointer Function %samp_img
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%var = OpVariable %ptr_samp_img Function
%value = OpLoad %samp_img %var
OpStore %var %value
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpTypeImage-06924"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Cannot store to OpTypeImage, OpTypeSampler, "
                "OpTypeSampledImage, or OpTypeAccelerationStructureKHR"));
}

TEST_F(ValidateMemory, StoreToAccelarationStructureKHR) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability RayQueryKHR
OpExtension "SPV_KHR_ray_query"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
%void = OpTypeVoid
%as = OpTypeAccelerationStructureKHR
%ptr_as = OpTypePointer Function %as
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%var = OpVariable %ptr_as Function
%value = OpLoad %as %var
OpStore %var %value
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpTypeImage-06924"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Cannot store to OpTypeImage, OpTypeSampler, "
                "OpTypeSampledImage, or OpTypeAccelerationStructureKHR"));
}

TEST_F(ValidateMemory, StoreToUniformBlock) {
  const std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %struct Block
OpMemberDecorate %struct 0 Offset 0
OpDecorate %var DescriptorSet 0
OpDecorate %var Binding 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%int4 = OpTypeVector %int 4
%struct = OpTypeStruct %int4
%ptr_uniform_struct = OpTypePointer Uniform %struct
%ptr_uniform_int4 = OpTypePointer Uniform %int4
%ptr_uniform_int = OpTypePointer Uniform %int
%var = OpVariable %ptr_uniform_struct Uniform
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep1 = OpAccessChain %ptr_uniform_int4 %var %int_0
%gep2 = OpAccessChain %ptr_uniform_int %gep1 %int_0
OpStore %gep2 %int_0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateMemory, StoreToUniformBlockVulkan) {
  const std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %struct Block
OpMemberDecorate %struct 0 Offset 0
OpDecorate %var DescriptorSet 0
OpDecorate %var Binding 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%int4 = OpTypeVector %int 4
%struct = OpTypeStruct %int4
%ptr_uniform_struct = OpTypePointer Uniform %struct
%ptr_uniform_int4 = OpTypePointer Uniform %int4
%ptr_uniform_int = OpTypePointer Uniform %int
%var = OpVariable %ptr_uniform_struct Uniform
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep1 = OpAccessChain %ptr_uniform_int4 %var %int_0
%gep2 = OpAccessChain %ptr_uniform_int %gep1 %int_0
OpStore %gep2 %int_0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-Uniform-06925"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("In the Vulkan environment, cannot store to Uniform Blocks"));
}

// This test requires that the struct is not id 2.
TEST_F(ValidateMemory, StoreToUniformBlockVulkan2) {
  const std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main" %gid_var
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %3 Block
OpMemberDecorate %3 0 Offset 0
OpDecorate %var DescriptorSet 0
OpDecorate %var Binding 0
OpDecorate %gid_var BuiltIn GlobalInvocationId
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%int3 = OpTypeVector %int 3
%int4 = OpTypeVector %int 4
%3 = OpTypeStruct %int4
%ptr_uniform_struct = OpTypePointer Uniform %3
%ptr_uniform_int4 = OpTypePointer Uniform %int4
%ptr_uniform_int = OpTypePointer Uniform %int
%var = OpVariable %ptr_uniform_struct Uniform
%ptr_input_int3 = OpTypePointer Input %int3
%gid_var = OpVariable %ptr_input_int3 Input
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep1 = OpAccessChain %ptr_uniform_int4 %var %int_0
%gep2 = OpAccessChain %ptr_uniform_int %gep1 %int_0
OpStore %gep2 %int_0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-Uniform-06925"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("In the Vulkan environment, cannot store to Uniform Blocks"));
}

TEST_F(ValidateMemory, StoreToUniformBufferBlockVulkan) {
  const std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %struct BufferBlock
OpMemberDecorate %struct 0 Offset 0
OpDecorate %var DescriptorSet 0
OpDecorate %var Binding 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%int4 = OpTypeVector %int 4
%struct = OpTypeStruct %int4
%ptr_uniform_struct = OpTypePointer Uniform %struct
%ptr_uniform_int4 = OpTypePointer Uniform %int4
%ptr_uniform_int = OpTypePointer Uniform %int
%var = OpVariable %ptr_uniform_struct Uniform
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep1 = OpAccessChain %ptr_uniform_int4 %var %int_0
%gep2 = OpAccessChain %ptr_uniform_int %gep1 %int_0
OpStore %gep2 %int_0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_1));
}

TEST_F(ValidateMemory, StoreToUniformBlockVulkanArray) {
  const std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %struct Block
OpMemberDecorate %struct 0 Offset 0
OpDecorate %var DescriptorSet 0
OpDecorate %var Binding 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%int_1 = OpConstant %int 1
%int4 = OpTypeVector %int 4
%struct = OpTypeStruct %int4
%array_struct = OpTypeArray %struct %int_1
%ptr_uniform_array = OpTypePointer Uniform %array_struct
%ptr_uniform_struct = OpTypePointer Uniform %struct
%ptr_uniform_int4 = OpTypePointer Uniform %int4
%ptr_uniform_int = OpTypePointer Uniform %int
%var = OpVariable %ptr_uniform_array Uniform
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep1 = OpAccessChain %ptr_uniform_int %var %int_0 %int_0 %int_0
%gep2 = OpCopyObject %ptr_uniform_int %gep1
OpStore %gep2 %int_0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-Uniform-06925"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("In the Vulkan environment, cannot store to Uniform Blocks"));
}

// This test requires that the struct is not id 2.
TEST_F(ValidateMemory, StoreToUniformBlockVulkanArray2) {
  const std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main" %gid_var
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %struct Block
OpMemberDecorate %struct 0 Offset 0
OpDecorate %var DescriptorSet 0
OpDecorate %var Binding 0
OpDecorate %gid_var BuiltIn GlobalInvocationId
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%int_1 = OpConstant %int 1
%int3 = OpTypeVector %int 3
%int4 = OpTypeVector %int 4
%struct = OpTypeStruct %int4
%array_struct = OpTypeArray %struct %int_1
%ptr_uniform_array = OpTypePointer Uniform %array_struct
%ptr_uniform_struct = OpTypePointer Uniform %struct
%ptr_uniform_int4 = OpTypePointer Uniform %int4
%ptr_uniform_int = OpTypePointer Uniform %int
%var = OpVariable %ptr_uniform_array Uniform
%ptr_input_int3 = OpTypePointer Input %int3
%gid_var = OpVariable %ptr_input_int3 Input
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep1 = OpAccessChain %ptr_uniform_int %var %int_0 %int_0 %int_0
%gep2 = OpCopyObject %ptr_uniform_int %gep1
OpStore %gep2 %int_0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-Uniform-06925"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("In the Vulkan environment, cannot store to Uniform Blocks"));
}

TEST_F(ValidateMemory, StoreToUniformBlockVulkanRuntimeArray) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability RuntimeDescriptorArrayEXT
OpExtension "SPV_EXT_descriptor_indexing"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %struct Block
OpMemberDecorate %struct 0 Offset 0
OpDecorate %var DescriptorSet 0
OpDecorate %var Binding 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%int4 = OpTypeVector %int 4
%struct = OpTypeStruct %int4
%array_struct = OpTypeRuntimeArray %struct
%ptr_uniform_array = OpTypePointer Uniform %array_struct
%ptr_uniform_struct = OpTypePointer Uniform %struct
%ptr_uniform_int4 = OpTypePointer Uniform %int4
%ptr_uniform_int = OpTypePointer Uniform %int
%var = OpVariable %ptr_uniform_array Uniform
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep1 = OpAccessChain %ptr_uniform_int4 %var %int_0 %int_0
%gep2 = OpInBoundsAccessChain %ptr_uniform_int %gep1 %int_0
OpStore %gep2 %int_0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-Uniform-06925"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("In the Vulkan environment, cannot store to Uniform Blocks"));
}

using ValidateSizedVariable = spvtest::ValidateBase<
    std::tuple<std::string, std::string, std::string, spv_target_env>>;

CodeGenerator GetSizedVariableCodeGenerator(bool is_8bit, bool buffer_block) {
  CodeGenerator generator;
  generator.capabilities_ = "OpCapability Shader\nOpCapability Linkage\n";
  generator.extensions_ =
      "OpExtension \"SPV_KHR_16bit_storage\"\nOpExtension "
      "\"SPV_KHR_8bit_storage\"\n";
  generator.memory_model_ = "OpMemoryModel Logical GLSL450\n";
  if (is_8bit) {
    generator.before_types_ =
        "OpMemberDecorate %char_buffer_block 0 Offset 0\n";
    if (buffer_block)
      generator.before_types_ += "OpDecorate %char_buffer_block BufferBlock\n";

    generator.types_ = R"(%void = OpTypeVoid
%char = OpTypeInt 8 0
%char4 = OpTypeVector %char 4
%char_buffer_block = OpTypeStruct %char
)";
  } else {
    generator.before_types_ =
        "OpMemberDecorate %half_buffer_block 0 Offset 0\n"
        "OpMemberDecorate %short_buffer_block 0 Offset 0\n";
    if (buffer_block) {
      generator.before_types_ +=
          "OpDecorate %half_buffer_block BufferBlock\n"
          "OpDecorate %short_buffer_block BufferBlock\n";
    }

    generator.types_ = R"(%void = OpTypeVoid
%short = OpTypeInt 16 0
%half = OpTypeFloat 16
%short4 = OpTypeVector %short 4
%half4 = OpTypeVector %half 4
%mat4x4 = OpTypeMatrix %half4 4
%short_buffer_block = OpTypeStruct %short
%half_buffer_block = OpTypeStruct %half
)";
  }
  generator.after_types_ = R"(%void_fn = OpTypeFunction %void
%func = OpFunction %void None %void_fn
%entry = OpLabel
)";
  generator.add_at_the_end_ = "OpReturn\nOpFunctionEnd\n";
  return generator;
}

TEST_P(ValidateSizedVariable, Capability) {
  const std::string storage_class = std::get<0>(GetParam());
  const std::string capability = std::get<1>(GetParam());
  const std::string var_type = std::get<2>(GetParam());
  const spv_target_env target = std::get<3>(GetParam());

  ASSERT_TRUE(target == SPV_ENV_UNIVERSAL_1_3 ||
              target == SPV_ENV_UNIVERSAL_1_4);

  bool type_8bit = false;
  if (var_type == "%char" || var_type == "%char4" ||
      var_type == "%char_buffer_block") {
    type_8bit = true;
  }

  const bool buffer_block = var_type.find("buffer_block") != std::string::npos;

  auto generator = GetSizedVariableCodeGenerator(type_8bit, buffer_block);

  if (capability == "WorkgroupMemoryExplicitLayout8BitAccessKHR" ||
      capability == "WorkgroupMemoryExplicitLayout16BitAccessKHR") {
    generator.extensions_ +=
        "OpExtension \"SPV_KHR_workgroup_memory_explicit_layout\"\n";
  }

  generator.types_ += "%ptr_type = OpTypePointer " + storage_class + " " +
                      var_type + "\n%var = OpVariable %ptr_type " +
                      storage_class + "\n";
  generator.capabilities_ += "OpCapability " + capability + "\n";

  bool capability_ok = false;
  bool storage_class_ok = false;
  if (storage_class == "Input" || storage_class == "Output") {
    if (!type_8bit) {
      capability_ok = capability == "StorageInputOutput16";
      storage_class_ok = true;
    }
  } else if (storage_class == "StorageBuffer") {
    if (type_8bit) {
      capability_ok = capability == "StorageBuffer8BitAccess" ||
                      capability == "UniformAndStorageBuffer8BitAccess";
    } else {
      capability_ok = capability == "StorageBuffer16BitAccess" ||
                      capability == "UniformAndStorageBuffer16BitAccess";
    }
    storage_class_ok = true;
  } else if (storage_class == "PushConstant") {
    if (type_8bit) {
      capability_ok = capability == "StoragePushConstant8";
    } else {
      capability_ok = capability == "StoragePushConstant16";
    }
    storage_class_ok = true;
  } else if (storage_class == "Uniform") {
    if (type_8bit) {
      capability_ok = capability == "UniformAndStorageBuffer8BitAccess" ||
                      (capability == "StorageBuffer8BitAccess" && buffer_block);
    } else {
      capability_ok =
          capability == "UniformAndStorageBuffer16BitAccess" ||
          (capability == "StorageBuffer16BitAccess" && buffer_block);
    }
    storage_class_ok = true;
  } else if (storage_class == "Workgroup") {
    if (type_8bit) {
      capability_ok =
          capability == "WorkgroupMemoryExplicitLayout8BitAccessKHR";
    } else {
      capability_ok =
          capability == "WorkgroupMemoryExplicitLayout16BitAccessKHR";
    }
    storage_class_ok = true;
  }

  CompileSuccessfully(generator.Build(), target);
  spv_result_t result = ValidateInstructions(target);
  if (target < SPV_ENV_UNIVERSAL_1_4 &&
      (capability == "WorkgroupMemoryExplicitLayout8BitAccessKHR" ||
       capability == "WorkgroupMemoryExplicitLayout16BitAccessKHR")) {
    EXPECT_EQ(SPV_ERROR_WRONG_VERSION, result);
    EXPECT_THAT(getDiagnosticString(),
                HasSubstr("requires SPIR-V version 1.4 or later"));
  } else if (buffer_block && target > SPV_ENV_UNIVERSAL_1_3) {
    EXPECT_EQ(SPV_ERROR_WRONG_VERSION, result);
    EXPECT_THAT(getDiagnosticString(),
                HasSubstr("requires SPIR-V version 1.3 or earlier"));
  } else if (capability_ok) {
    EXPECT_EQ(SPV_SUCCESS, result);
  } else {
    EXPECT_EQ(SPV_ERROR_INVALID_ID, result);
    if (storage_class_ok) {
      std::string message = std::string("Allocating a variable containing a ") +
                            (type_8bit ? "8" : "16") + "-bit element in " +
                            storage_class +
                            " storage class requires an additional capability";
      EXPECT_THAT(getDiagnosticString(), HasSubstr(message));
    } else {
      std::string message =
          std::string("Cannot allocate a variable containing a ") +
          (type_8bit ? "8" : "16") + "-bit type in " + storage_class +
          " storage class";
      EXPECT_THAT(getDiagnosticString(), HasSubstr(message));
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    Storage8, ValidateSizedVariable,
    Combine(Values("UniformConstant", "Input", "Output", "Workgroup",
                   "CrossWorkgroup", "Private", "StorageBuffer", "Uniform"),
            Values("StorageBuffer8BitAccess",
                   "UniformAndStorageBuffer8BitAccess", "StoragePushConstant8",
                   "WorkgroupMemoryExplicitLayout8BitAccessKHR"),
            Values("%char", "%char4", "%char_buffer_block"),
            Values(SPV_ENV_UNIVERSAL_1_3, SPV_ENV_UNIVERSAL_1_4)));

INSTANTIATE_TEST_SUITE_P(
    Storage16, ValidateSizedVariable,
    Combine(Values("UniformConstant", "Input", "Output", "Workgroup",
                   "CrossWorkgroup", "Private", "StorageBuffer", "Uniform"),
            Values("StorageBuffer16BitAccess",
                   "UniformAndStorageBuffer16BitAccess",
                   "StoragePushConstant16", "StorageInputOutput16",
                   "WorkgroupMemoryExplicitLayout16BitAccessKHR"),
            Values("%short", "%half", "%short4", "%half4", "%mat4x4",
                   "%short_buffer_block", "%half_buffer_block"),
            Values(SPV_ENV_UNIVERSAL_1_3, SPV_ENV_UNIVERSAL_1_4)));

using ValidateSizedLoadStore =
    spvtest::ValidateBase<std::tuple<std::string, uint32_t, std::string>>;

CodeGenerator GetSizedLoadStoreCodeGenerator(const std::string& base_type,
                                             uint32_t width) {
  CodeGenerator generator;
  generator.capabilities_ = "OpCapability Shader\nOpCapability Linkage\n";
  if (width == 8) {
    generator.capabilities_ +=
        "OpCapability UniformAndStorageBuffer8BitAccess\n";
    generator.extensions_ = "OpExtension \"SPV_KHR_8bit_storage\"\n";
  } else {
    generator.capabilities_ +=
        "OpCapability UniformAndStorageBuffer16BitAccess\n";
    generator.extensions_ = "OpExtension \"SPV_KHR_16bit_storage\"\n";
  }
  generator.memory_model_ = "OpMemoryModel Logical GLSL450\n";
  generator.before_types_ = R"(OpDecorate %block Block
OpMemberDecorate %block 0 Offset 0
OpMemberDecorate %struct 0 Offset 0
)";
  generator.types_ = R"(%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%int_1 = OpConstant %int 1
%int_2 = OpConstant %int 2
%int_3 = OpConstant %int 3
)";

  if (width == 8) {
    generator.types_ += R"(%scalar = OpTypeInt 8 0
%vector = OpTypeVector %scalar 4
%struct = OpTypeStruct %vector
)";
  } else if (base_type == "int") {
    generator.types_ += R"(%scalar = OpTypeInt 16 0
%vector = OpTypeVector %scalar 4
%struct = OpTypeStruct %vector
)";
  } else {
    generator.types_ += R"(%scalar = OpTypeFloat 16
%vector = OpTypeVector %scalar 4
%matrix = OpTypeMatrix %vector 4
%struct = OpTypeStruct %matrix
%ptr_ssbo_matrix = OpTypePointer StorageBuffer %matrix
)";
    generator.before_types_ += R"(OpMemberDecorate %struct 0 RowMajor
OpMemberDecorate %struct 0 MatrixStride 16
)";
  }
  generator.types_ += R"(%block = OpTypeStruct %struct
%ptr_ssbo_block = OpTypePointer StorageBuffer %block
%ptr_ssbo_struct = OpTypePointer StorageBuffer %struct
%ptr_ssbo_vector = OpTypePointer StorageBuffer %vector
%ptr_ssbo_scalar = OpTypePointer StorageBuffer %scalar
%ld_var = OpVariable %ptr_ssbo_block StorageBuffer
%st_var = OpVariable %ptr_ssbo_block StorageBuffer
)";

  generator.after_types_ = R"(%void_fn = OpTypeFunction %void
%func = OpFunction %void None %void_fn
%entry = OpLabel
)";
  generator.add_at_the_end_ = "OpReturn\nOpFunctionEnd\n";
  return generator;
}

TEST_P(ValidateSizedLoadStore, Load) {
  std::string base_type = std::get<0>(GetParam());
  uint32_t width = std::get<1>(GetParam());
  std::string mem_type = std::get<2>(GetParam());

  CodeGenerator generator = GetSizedLoadStoreCodeGenerator(base_type, width);
  generator.after_types_ +=
      "%ld_gep = OpAccessChain %ptr_ssbo_" + mem_type + " %ld_var %int_0";
  if (mem_type != "struct") {
    generator.after_types_ += " %int_0";
    if (mem_type != "matrix" && base_type == "float") {
      generator.after_types_ += " %int_0";
    }
    if (mem_type == "scalar") {
      generator.after_types_ += " %int_0";
    }
  }
  generator.after_types_ += "\n";
  generator.after_types_ += "%ld = OpLoad %" + mem_type + " %ld_gep\n";

  CompileSuccessfully(generator.Build(), SPV_ENV_UNIVERSAL_1_3);
  if (mem_type == "struct") {
    EXPECT_EQ(SPV_ERROR_INVALID_ID,
              ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
    EXPECT_THAT(
        getDiagnosticString(),
        HasSubstr(
            "8- or 16-bit loads must be a scalar, vector or matrix type"));
  } else {
    EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  }
}

TEST_P(ValidateSizedLoadStore, Store) {
  std::string base_type = std::get<0>(GetParam());
  uint32_t width = std::get<1>(GetParam());
  std::string mem_type = std::get<2>(GetParam());

  CodeGenerator generator = GetSizedLoadStoreCodeGenerator(base_type, width);
  generator.after_types_ +=
      "%ld_gep = OpAccessChain %ptr_ssbo_" + mem_type + " %ld_var %int_0";
  if (mem_type != "struct") {
    generator.after_types_ += " %int_0";
    if (mem_type != "matrix" && base_type == "float") {
      generator.after_types_ += " %int_0";
    }
    if (mem_type == "scalar") {
      generator.after_types_ += " %int_0";
    }
  }
  generator.after_types_ += "\n";
  generator.after_types_ += "%ld = OpLoad %" + mem_type + " %ld_gep\n";
  generator.after_types_ +=
      "%st_gep = OpAccessChain %ptr_ssbo_" + mem_type + " %st_var %int_0";
  if (mem_type != "struct") {
    generator.after_types_ += " %int_0";
    if (mem_type != "matrix" && base_type == "float") {
      generator.after_types_ += " %int_0";
    }
    if (mem_type == "scalar") {
      generator.after_types_ += " %int_0";
    }
  }
  generator.after_types_ += "\n";
  generator.after_types_ += "OpStore %st_gep %ld\n";

  CompileSuccessfully(generator.Build(), SPV_ENV_UNIVERSAL_1_3);
  if (mem_type == "struct") {
    EXPECT_EQ(SPV_ERROR_INVALID_ID,
              ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
    // Can only catch the load.
    EXPECT_THAT(
        getDiagnosticString(),
        HasSubstr(
            "8- or 16-bit loads must be a scalar, vector or matrix type"));
  } else {
    EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  }
}

INSTANTIATE_TEST_SUITE_P(LoadStoreInt8, ValidateSizedLoadStore,
                         Combine(Values("int"), Values(8u),
                                 Values("scalar", "vector", "struct")));
INSTANTIATE_TEST_SUITE_P(LoadStoreInt16, ValidateSizedLoadStore,
                         Combine(Values("int"), Values(16u),
                                 Values("scalar", "vector", "struct")));
INSTANTIATE_TEST_SUITE_P(LoadStoreFloat16, ValidateSizedLoadStore,
                         Combine(Values("float"), Values(16u),
                                 Values("scalar", "vector", "matrix",
                                        "struct")));

TEST_F(ValidateMemory, SmallStorageCopyMemoryChar) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability UniformAndStorageBuffer8BitAccess
OpExtension "SPV_KHR_8bit_storage"
OpMemoryModel Logical GLSL450
OpDecorate %block Block
OpMemberDecorate %block 0 Offset 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%char = OpTypeInt 8 0
%block = OpTypeStruct %char
%ptr_ssbo_block = OpTypePointer StorageBuffer %block
%in = OpVariable %ptr_ssbo_block StorageBuffer
%out = OpVariable %ptr_ssbo_block StorageBuffer
%void_fn = OpTypeFunction %void
%func = OpFunction %void None %void_fn
%entry = OpLabel
OpCopyMemory %out %in
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Cannot copy memory of objects containing 8- or 16-bit types"));
}

TEST_F(ValidateMemory, SmallStorageCopyMemoryShort) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability UniformAndStorageBuffer16BitAccess
OpExtension "SPV_KHR_16bit_storage"
OpMemoryModel Logical GLSL450
OpDecorate %block Block
OpMemberDecorate %block 0 Offset 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%short = OpTypeInt 16 0
%block = OpTypeStruct %short
%ptr_ssbo_block = OpTypePointer StorageBuffer %block
%in = OpVariable %ptr_ssbo_block StorageBuffer
%out = OpVariable %ptr_ssbo_block StorageBuffer
%void_fn = OpTypeFunction %void
%func = OpFunction %void None %void_fn
%entry = OpLabel
OpCopyMemory %out %in
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Cannot copy memory of objects containing 8- or 16-bit types"));
}

TEST_F(ValidateMemory, SmallStorageCopyMemoryHalf) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability UniformAndStorageBuffer16BitAccess
OpExtension "SPV_KHR_16bit_storage"
OpMemoryModel Logical GLSL450
OpDecorate %block Block
OpMemberDecorate %block 0 Offset 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%half = OpTypeFloat 16
%block = OpTypeStruct %half
%ptr_ssbo_block = OpTypePointer StorageBuffer %block
%in = OpVariable %ptr_ssbo_block StorageBuffer
%out = OpVariable %ptr_ssbo_block StorageBuffer
%void_fn = OpTypeFunction %void
%func = OpFunction %void None %void_fn
%entry = OpLabel
OpCopyMemory %out %in
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Cannot copy memory of objects containing 8- or 16-bit types"));
}

TEST_F(ValidateMemory, SmallStorageVariableArrayBufferBlockShort) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability StorageBuffer16BitAccess
OpExtension "SPV_KHR_16bit_storage"
OpMemoryModel Logical GLSL450
OpDecorate %block BufferBlock
OpMemberDecorate %block 0 Offset 0
%void = OpTypeVoid
%short = OpTypeInt 16 0
%int = OpTypeInt 32 0
%int_4 = OpConstant %int 4
%block = OpTypeStruct %short
%block_array = OpTypeArray %block %int_4
%ptr_block_array = OpTypePointer Uniform %block_array
%var = OpVariable %ptr_block_array Uniform
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateMemory, SmallStorageVariableArrayBufferBlockChar) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability StorageBuffer8BitAccess
OpExtension "SPV_KHR_8bit_storage"
OpMemoryModel Logical GLSL450
OpDecorate %block BufferBlock
OpMemberDecorate %block 0 Offset 0
%void = OpTypeVoid
%char = OpTypeInt 8 0
%int = OpTypeInt 32 0
%int_4 = OpConstant %int 4
%block = OpTypeStruct %char
%block_array = OpTypeArray %block %int_4
%ptr_block_array = OpTypePointer Uniform %block_array
%var = OpVariable %ptr_block_array Uniform
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateMemory, SmallStorageVariableArrayBufferBlockHalf) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability StorageBuffer16BitAccess
OpExtension "SPV_KHR_16bit_storage"
OpMemoryModel Logical GLSL450
OpDecorate %block BufferBlock
OpMemberDecorate %block 0 Offset 0
%void = OpTypeVoid
%half = OpTypeFloat 16
%int = OpTypeInt 32 0
%int_4 = OpConstant %int 4
%block = OpTypeStruct %half
%block_array = OpTypeArray %block %int_4
%ptr_block_array = OpTypePointer Uniform %block_array
%var = OpVariable %ptr_block_array Uniform
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateMemory, VulkanStorageBufferNotAStruct) {
  const std::string spirv = R"(
OpCapability Shader
OpExtension "SPV_KHR_storage_buffer_storage_class"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%ptr_ssbo = OpTypePointer StorageBuffer %uint
%var = OpVariable %ptr_ssbo StorageBuffer
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-Uniform-06807"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("From Vulkan spec:\nVariables identified with "
                "the StorageBuffer storage class are used to access "
                "transparent buffer backed resources. Such variables must be "
                "typed as OpTypeStruct, or an array of this type"));
}

TEST_F(ValidateMemory, VulkanStorageBufferRuntimeArrayNotAStruct) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability RuntimeDescriptorArrayEXT
OpExtension "SPV_KHR_storage_buffer_storage_class"
OpExtension "SPV_EXT_descriptor_indexing"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%array = OpTypeRuntimeArray %uint
%ptr_ssbo = OpTypePointer StorageBuffer %array
%var = OpVariable %ptr_ssbo StorageBuffer
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-Uniform-06807"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("From Vulkan spec:\nVariables identified with "
                "the StorageBuffer storage class are used to access "
                "transparent buffer backed resources. Such variables must be "
                "typed as OpTypeStruct, or an array of this type"));
}

TEST_F(ValidateMemory, VulkanStorageBufferArrayNotAStruct) {
  const std::string spirv = R"(
OpCapability Shader
OpExtension "SPV_KHR_storage_buffer_storage_class"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%uint_4 = OpConstant %uint 4
%array = OpTypeArray %uint %uint_4
%ptr_ssbo = OpTypePointer StorageBuffer %array
%var = OpVariable %ptr_ssbo StorageBuffer
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-Uniform-06807"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("From Vulkan spec:\nVariables identified with "
                "the StorageBuffer storage class are used to access "
                "transparent buffer backed resources. Such variables must be "
                "typed as OpTypeStruct, or an array of this type"));
}

TEST_F(ValidateMemory, VulkanInvariantOutputSuccess) {
  const std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Vertex %main "main" %var
OpDecorate %var Location 0
OpDecorate %var Invariant
%void = OpTypeVoid
%f32 = OpTypeFloat 32
%ptr_output = OpTypePointer Output %f32
%var = OpVariable %ptr_output Output
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_0));
}

TEST_F(ValidateMemory, VulkanInvariantInputStructSuccess) {
  const std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var
OpExecutionMode %main OriginUpperLeft
OpDecorate %var Location 0
OpMemberDecorate %struct 1 Invariant
%void = OpTypeVoid
%f32 = OpTypeFloat 32
%struct = OpTypeStruct %f32 %f32
%ptr_input = OpTypePointer Input %struct
%var = OpVariable %ptr_input Input
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_0));
}

TEST_F(ValidateMemory, VulkanInvariantWrongStorageClass) {
  const std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Vertex %main "main"
OpDecorate %var Invariant
%void = OpTypeVoid
%f32 = OpTypeFloat 32
%ptr_private = OpTypePointer Private %f32
%var = OpVariable %ptr_private Private
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-Invariant-04677"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Variable decorated with Invariant must only be identified with the "
          "Input or Output storage class in Vulkan environment."));
}

TEST_F(ValidateMemory, VulkanInvariantMemberWrongStorageClass) {
  const std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main"
OpExecutionMode %main OriginUpperLeft
OpMemberDecorate %struct 1 Invariant
%void = OpTypeVoid
%f32 = OpTypeFloat 32
%struct = OpTypeStruct %f32 %f32
%ptr_private = OpTypePointer Private %struct
%var = OpVariable %ptr_private Private
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-Invariant-04677"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Variable struct member decorated with Invariant must "
                        "only be identified with the Input or Output storage "
                        "class in Vulkan environment."));
}

TEST_F(ValidateMemory, PhysicalStorageBufferPtrEqual) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Int64
OpCapability PhysicalStorageBufferAddresses
OpMemoryModel PhysicalStorageBuffer64 GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
%void = OpTypeVoid
%bool = OpTypeBool
%long = OpTypeInt 64 0
%long_0 = OpConstant %long 0
%ptr_pssbo_long = OpTypePointer PhysicalStorageBuffer %long
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%conv = OpConvertUToPtr %ptr_pssbo_long %long_0
%eq = OpPtrEqual %bool %conv %conv
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_5);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_5));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Cannot use a pointer in the PhysicalStorageBuffer storage class"));
}

TEST_F(ValidateMemory, PhysicalStorageBufferPtrNotEqual) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Int64
OpCapability PhysicalStorageBufferAddresses
OpMemoryModel PhysicalStorageBuffer64 GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
%void = OpTypeVoid
%bool = OpTypeBool
%long = OpTypeInt 64 0
%long_0 = OpConstant %long 0
%ptr_pssbo_long = OpTypePointer PhysicalStorageBuffer %long
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%conv = OpConvertUToPtr %ptr_pssbo_long %long_0
%neq = OpPtrNotEqual %bool %conv %conv
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_5);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_5));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Cannot use a pointer in the PhysicalStorageBuffer storage class"));
}

TEST_F(ValidateMemory, PhysicalStorageBufferPtrDiff) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Int64
OpCapability PhysicalStorageBufferAddresses
OpCapability VariablePointers
OpMemoryModel PhysicalStorageBuffer64 GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
%void = OpTypeVoid
%long = OpTypeInt 64 0
%long_0 = OpConstant %long 0
%ptr_pssbo_long = OpTypePointer PhysicalStorageBuffer %long
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%conv = OpConvertUToPtr %ptr_pssbo_long %long_0
%diff = OpPtrDiff %long %conv %conv
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_5);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_5));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Cannot use a pointer in the PhysicalStorageBuffer storage class"));
}

TEST_F(ValidateMemory, VulkanInitializerWithWorkgroupStorageClassBad) {
  std::string spirv = R"(
OpCapability Shader
OpCapability VulkanMemoryModelKHR
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
%float = OpTypeFloat 32
%float_ptr = OpTypePointer Workgroup %float
%init_val = OpConstant %float 1.0
%1 = OpVariable %float_ptr Workgroup %init_val
%void = OpTypeVoid
%functy = OpTypeFunction %void
%func = OpFunction %void None %functy
%2 = OpLabel
OpReturn
OpFunctionEnd
)";
  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_3));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID(" VUID-StandaloneSpirv-OpVariable-04734"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("OpVariable, <id> '5[%5]', initializers are limited to "
                        "OpConstantNull in Workgroup storage class"));
}

TEST_F(ValidateMemory, VulkanInitializerWithWorkgroupStorageClassGood) {
  std::string spirv = R"(
OpCapability Shader
OpCapability VulkanMemoryModelKHR
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
OpEntryPoint Fragment %func "func"
OpExecutionMode %func OriginUpperLeft
%float = OpTypeFloat 32
%float_ptr = OpTypePointer Workgroup %float
%init_val = OpConstantNull %float
%1 = OpVariable %float_ptr Workgroup %init_val
%void = OpTypeVoid
%functy = OpTypeFunction %void
%func = OpFunction %void None %functy
%2 = OpLabel
OpReturn
OpFunctionEnd
)";
  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_3);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_3));
}

TEST_F(ValidateMemory, LoadRuntimeArray) {
  const std::string spirv = R"(
OpCapability Shader
OpExtension "SPV_KHR_storage_buffer_storage_class"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%rta = OpTypeRuntimeArray %int
%block = OpTypeStruct %rta
%ptr_rta = OpTypePointer StorageBuffer %rta
%ptr_block = OpTypePointer StorageBuffer %block
%var = OpVariable %ptr_block StorageBuffer
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep = OpAccessChain %ptr_rta %var %int_0
%ld = OpLoad %rta %gep
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Cannot load a runtime-sized array"));
}

TEST_F(ValidateMemory, LoadRuntimeArrayInStruct) {
  const std::string spirv = R"(
OpCapability Shader
OpExtension "SPV_KHR_storage_buffer_storage_class"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%rta = OpTypeRuntimeArray %int
%block = OpTypeStruct %rta
%ptr_rta = OpTypePointer StorageBuffer %rta
%ptr_block = OpTypePointer StorageBuffer %block
%var = OpVariable %ptr_block StorageBuffer
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%ld = OpLoad %block %var
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Cannot load a runtime-sized array"));
}

TEST_F(ValidateMemory, LoadRuntimeArrayInArray) {
  const std::string spirv = R"(
OpCapability Shader
OpExtension "SPV_KHR_storage_buffer_storage_class"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%int_4 = OpConstant %int 4
%rta = OpTypeRuntimeArray %int
%block = OpTypeStruct %rta
%array = OpTypeArray %block %int_4
%ptr_rta = OpTypePointer StorageBuffer %rta
%ptr_block = OpTypePointer StorageBuffer %block
%ptr_array = OpTypePointer StorageBuffer %array
%var = OpVariable %ptr_array StorageBuffer
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%ld = OpLoad %array %var
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Cannot load a runtime-sized array"));
}

TEST_F(ValidateMemory, Pre1p4WorkgroupMemoryBadLayoutOk) {
  const std::string spirv = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpDecorate %struct Block
OpMemberDecorate %struct 0 Offset 0
%void = OpTypeVoid
%bool = OpTypeBool
%struct = OpTypeStruct %bool
%ptr = OpTypePointer Workgroup %struct
%var = OpVariable %ptr Workgroup
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateMemory, UntypedVariableGood) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%ptr = OpTypeUntypedPointerKHR Private
%var = OpUntypedVariableKHR %ptr Private %int %int_0
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateMemory, UntypedVariableNoDataType) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpExtension "SPV_KHR_storage_buffer_storage_class"
OpMemoryModel Logical GLSL450
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%ptr = OpTypeUntypedPointerKHR StorageBuffer
%var = OpUntypedVariableKHR %ptr StorageBuffer
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateMemory, UntypedVariableNoDataTypeFunction) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%ptr = OpTypeUntypedPointerKHR Function
%var = OpUntypedVariableKHR %ptr Function
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Data type must be specified for Function, "
                        "Private, and Workgroup storage classes"));
}

TEST_F(ValidateMemory, UntypedVariableNoDataTypePrivate) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%ptr = OpTypeUntypedPointerKHR Private
%var = OpUntypedVariableKHR %ptr Private
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Data type must be specified for Function, "
                        "Private, and Workgroup storage classes"));
}

TEST_F(ValidateMemory, UntypedVariableNoDataTypeWorkgroup) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%ptr = OpTypeUntypedPointerKHR Workgroup
%var = OpUntypedVariableKHR %ptr Workgroup
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Data type must be specified for Function, "
                        "Private, and Workgroup storage classes"));
}

TEST_F(ValidateMemory, UntypedVariableNoDataTypeVulkan) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpExtension "SPV_KHR_storage_buffer_storage_class"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%ptr = OpTypeUntypedPointerKHR StorageBuffer
%var = OpUntypedVariableKHR %ptr StorageBuffer
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Vulkan requires that data type be specified"));
}

TEST_F(ValidateMemory, PtrAccessChainArrayStrideBad) {
  const std::string spirv = R"(
               OpCapability Shader
               OpCapability VariablePointersStorageBuffer
               OpExtension "SPV_KHR_storage_buffer_storage_class"
               OpExtension "SPV_KHR_variable_pointers"
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "foo" %var
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %var DescriptorSet 0
               OpDecorate %var Binding 0
       %uint = OpTypeInt 32 0
     %uint_0 = OpConstant %uint 0
     %uint_1 = OpConstant %uint 1
        %ptr = OpTypePointer StorageBuffer %uint
       %void = OpTypeVoid
       %func = OpTypeFunction %void
        %var = OpVariable %ptr StorageBuffer
       %main = OpFunction %void None %func
      %label = OpLabel
     %access = OpAccessChain %ptr %var
 %ptr_access = OpPtrAccessChain %ptr %access %uint_1
               OpReturn
               OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_5);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA,
            ValidateInstructions(SPV_ENV_UNIVERSAL_1_5));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("OpPtrAccessChain must have a Base whose type is "
                        "decorated with ArrayStride"));
}

TEST_F(ValidateMemory, PtrAccessChainArrayStrideSuccess) {
  const std::string spirv = R"(
               OpCapability Shader
               OpCapability VariablePointersStorageBuffer
               OpExtension "SPV_KHR_storage_buffer_storage_class"
               OpExtension "SPV_KHR_variable_pointers"
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "foo" %var
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %var DescriptorSet 0
               OpDecorate %var Binding 00
               OpDecorate %ptr ArrayStride 4
       %uint = OpTypeInt 32 0
     %uint_0 = OpConstant %uint 0
     %uint_1 = OpConstant %uint 1
        %ptr = OpTypePointer StorageBuffer %uint
       %void = OpTypeVoid
       %func = OpTypeFunction %void
        %var = OpVariable %ptr StorageBuffer
       %main = OpFunction %void None %func
      %label = OpLabel
     %access = OpAccessChain %ptr %var
 %ptr_access = OpPtrAccessChain %ptr %access %uint_1
               OpReturn
               OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_5);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_5));
}

TEST_F(ValidateMemory, VulkanPtrAccessChainStorageBufferSuccess) {
  const std::string spirv = R"(
               OpCapability Shader
               OpCapability VariablePointersStorageBuffer
               OpExtension "SPV_KHR_storage_buffer_storage_class"
               OpExtension "SPV_KHR_variable_pointers"
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "foo" %var
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %_runtimearr_uint ArrayStride 4
               OpMemberDecorate %_struct_10 0 Offset 0
               OpDecorate %_struct_10 Block
               OpDecorate %var DescriptorSet 0
               OpDecorate %var Binding 0
               OpDecorate %_ptr_StorageBuffer_uint ArrayStride 4
       %uint = OpTypeInt 32 0
     %uint_0 = OpConstant %uint 0
     %uint_1 = OpConstant %uint 1
%_runtimearr_uint = OpTypeRuntimeArray %uint
 %_struct_10 = OpTypeStruct %_runtimearr_uint
%_ptr_StorageBuffer__struct_10 = OpTypePointer StorageBuffer %_struct_10
%_ptr_StorageBuffer_uint = OpTypePointer StorageBuffer %uint
       %void = OpTypeVoid
      %func2 = OpTypeFunction %void %_ptr_StorageBuffer_uint
      %func1 = OpTypeFunction %void
         %var = OpVariable %_ptr_StorageBuffer__struct_10 StorageBuffer
     %called = OpFunction %void None %func2
      %param = OpFunctionParameter %_ptr_StorageBuffer_uint
     %label2 = OpLabel
 %ptr_access = OpPtrAccessChain %_ptr_StorageBuffer_uint %param %uint_1
               OpReturn
               OpFunctionEnd
       %main = OpFunction %void None %func1
     %label1 = OpLabel
     %access = OpAccessChain %_ptr_StorageBuffer_uint %var %uint_0 %uint_0
       %call = OpFunctionCall %void %called %access
               OpReturn
               OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_2);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_2));
}

TEST_F(ValidateMemory, VulkanPtrAccessChainStorageBufferCapability) {
  const std::string spirv = R"(
               OpCapability Shader
               OpCapability PhysicalStorageBufferAddresses
               OpExtension "SPV_KHR_storage_buffer_storage_class"
               OpExtension "SPV_KHR_variable_pointers"
               OpMemoryModel PhysicalStorageBuffer64 GLSL450
               OpEntryPoint GLCompute %main "foo" %var
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %_runtimearr_uint ArrayStride 4
               OpMemberDecorate %_struct_10 0 Offset 0
               OpDecorate %_struct_10 Block
               OpDecorate %var DescriptorSet 0
               OpDecorate %var Binding 0
               OpDecorate %_ptr_StorageBuffer_uint ArrayStride 4
       %uint = OpTypeInt 32 0
     %uint_0 = OpConstant %uint 0
     %uint_1 = OpConstant %uint 1
%_runtimearr_uint = OpTypeRuntimeArray %uint
 %_struct_10 = OpTypeStruct %_runtimearr_uint
%_ptr_StorageBuffer__struct_10 = OpTypePointer StorageBuffer %_struct_10
%_ptr_StorageBuffer_uint = OpTypePointer StorageBuffer %uint
       %void = OpTypeVoid
       %func = OpTypeFunction %void
         %var = OpVariable %_ptr_StorageBuffer__struct_10 StorageBuffer
       %main = OpFunction %void None %func
      %label = OpLabel
     %access = OpAccessChain %_ptr_StorageBuffer_uint %var %uint_0 %uint_0
 %ptr_access = OpPtrAccessChain %_ptr_StorageBuffer_uint %access %uint_1
               OpReturn
               OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_2);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_2));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-Base-07652"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("OpPtrAccessChain Base operand pointing to "
                        "StorageBuffer storage class must use VariablePointers "
                        "or VariablePointersStorageBuffer capability"));
}

TEST_F(ValidateMemory, VulkanPtrAccessChainWorkgroupCapability) {
  const std::string spirv = R"(
               OpCapability Shader
               OpCapability VariablePointersStorageBuffer
               OpExtension "SPV_KHR_storage_buffer_storage_class"
               OpExtension "SPV_KHR_variable_pointers"
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "foo" %var
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %_ptr_Workgroup_uint ArrayStride 4
       %uint = OpTypeInt 32 0
     %uint_0 = OpConstant %uint 0
     %uint_1 = OpConstant %uint 1
%_arr_uint = OpTypeArray %uint %uint_1
%_ptr_Workgroup__arr_uint = OpTypePointer Workgroup %_arr_uint
%_ptr_Workgroup_uint = OpTypePointer Workgroup %uint
       %void = OpTypeVoid
       %func = OpTypeFunction %void
        %var = OpVariable %_ptr_Workgroup__arr_uint Workgroup
       %main = OpFunction %void None %func
      %label = OpLabel
     %access = OpAccessChain %_ptr_Workgroup_uint %var %uint_0
 %ptr_access = OpPtrAccessChain %_ptr_Workgroup_uint %access %uint_1
               OpReturn
               OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_2);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_2));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-Base-07651"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("OpPtrAccessChain Base operand pointing to Workgroup "
                        "storage class must use VariablePointers capability"));
}

TEST_F(ValidateMemory, VulkanPtrAccessChainWorkgroupNoArrayStrideSuccess) {
  const std::string spirv = R"(
               OpCapability Shader
               OpCapability VariablePointers
               OpExtension "SPV_KHR_storage_buffer_storage_class"
               OpExtension "SPV_KHR_variable_pointers"
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "foo" %var
               OpExecutionMode %main LocalSize 1 1 1
       %uint = OpTypeInt 32 0
     %uint_0 = OpConstant %uint 0
     %uint_1 = OpConstant %uint 1
%_arr_uint = OpTypeArray %uint %uint_1
%_ptr_Workgroup__arr_uint = OpTypePointer Workgroup %_arr_uint
%_ptr_Workgroup_uint = OpTypePointer Workgroup %uint
       %void = OpTypeVoid
       %func = OpTypeFunction %void
        %var = OpVariable %_ptr_Workgroup__arr_uint Workgroup
       %main = OpFunction %void None %func
      %label = OpLabel
     %access = OpAccessChain %_ptr_Workgroup_uint %var %uint_0
 %ptr_access = OpPtrAccessChain %_ptr_Workgroup_uint %access %uint_1
               OpReturn
               OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_2);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_2));
}

TEST_F(ValidateMemory, AccessChainNegativeStructIndex32) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpMemoryModel Logical GLSL450
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%int = OpTypeInt 32 1
%_struct_4 = OpTypeStruct %int %int %int
%_ptr_Function__struct_4 = OpTypePointer Function %_struct_4
%_ptr_Function_int = OpTypePointer Function %int
%int_n224 = OpConstant %int -224
%fn = OpFunction %void Inline %void_fn
%entry = OpLabel
%var = OpVariable %_ptr_Function__struct_4 Function
%gep = OpInBoundsAccessChain %_ptr_Function_int %var %int_n224
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(), HasSubstr("is out of bounds"));
  EXPECT_THAT(getDiagnosticString(), HasSubstr("cannot find index -224"));
}

TEST_F(ValidateMemory, AccessChainNegativeStructIndex64) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability Int64
OpMemoryModel Logical GLSL450
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%int = OpTypeInt 32 1
%long = OpTypeInt 64 1
%_struct_4 = OpTypeStruct %int %int %int
%_ptr_Function__struct_4 = OpTypePointer Function %_struct_4
%_ptr_Function_int = OpTypePointer Function %int
%long_n224 = OpConstant %long -224
%fn = OpFunction %void Inline %void_fn
%entry = OpLabel
%var = OpVariable %_ptr_Function__struct_4 Function
%gep = OpInBoundsAccessChain %_ptr_Function_int %var %long_n224
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(), HasSubstr("is out of bounds"));
  EXPECT_THAT(getDiagnosticString(), HasSubstr("cannot find index -224"));
}

TEST_F(ValidateMemory, UntypedVariableFunctionOutsideFunction) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%ptr = OpTypeUntypedPointerKHR Function
%var = OpUntypedVariableKHR %ptr Function %int
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_ERROR_INVALID_LAYOUT, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Variables can not have a function[7] storage class "
                        "outside of a function"));
}

TEST_F(ValidateMemory, UntypedVariableBadResultType) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%ptr = OpTypeUntypedPointerKHR Workgroup
%var = OpUntypedVariableKHR %int Workgroup %int
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Result type must be an untyped pointer"));
}

TEST_F(ValidateMemory, UntypedVariableBadDataType) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%ptr = OpTypeUntypedPointerKHR Workgroup
%var = OpUntypedVariableKHR %ptr Workgroup %int_0
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Data type must be a type instruction"));
}

TEST_F(ValidateMemory, UntypedVariableBadStorageClass) {
  const std::string spirv = R"(
OpCapability Kernel
OpCapability GenericPointer
OpCapability Linkage
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical OpenCL
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%ptr = OpTypeUntypedPointerKHR Generic
%var = OpUntypedVariableKHR %ptr Generic %int
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_2);
  EXPECT_EQ(SPV_ERROR_INVALID_BINARY,
            ValidateInstructions(SPV_ENV_UNIVERSAL_1_2));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Variable storage class cannot be Generic"));
}

TEST_F(ValidateMemory, UntypedVariableMismatchedStorageClass) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%ptr = OpTypeUntypedPointerKHR Workgroup
%var = OpUntypedVariableKHR %ptr Private %int
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Storage class must match result type storage class"));
}

TEST_F(ValidateMemory, UntypedVariableBadInitializer) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
%int = OpTypeInt 32 0
%float = OpTypeFloat 32
%float_0 = OpConstant %float 0
%ptr = OpTypeUntypedPointerKHR Private
%var = OpUntypedVariableKHR %ptr Private %int %float_0
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Initializer type must match the data type"));
}

TEST_F(ValidateMemory, AccessChainBaseUntypedPointer) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_storage_buffer_storage_class"
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpName %var "var"
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%block = OpTypeStruct %int
%ptr_ssbo = OpTypePointer StorageBuffer %block
%ptr_ssbo_int = OpTypePointer StorageBuffer %int
%ptr = OpTypeUntypedPointerKHR StorageBuffer
%var = OpUntypedVariableKHR %ptr StorageBuffer %int
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep = OpAccessChain %ptr_ssbo_int %var %int_0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("The Base <id> '2[%var]' in OpAccessChain "
                        "instruction must be a pointer"));
}

using ValidateMemoryUntypedAccessChain = spvtest::ValidateBase<std::string>;

TEST_P(ValidateMemoryUntypedAccessChain, GoodTypedPointerBase) {
  const std::string opcode = GetParam();
  const bool ptr = opcode == "OpUntypedPtrAccessChainKHR" ||
                   opcode == "OpUntypedInBoundsPtrAccessChainKHR";
  const std::string extra_param = ptr ? "%int_0" : "";
  const std::string deco = ptr ? "OpDecorate %ptr_ssbo ArrayStride 4" : "";

  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability VariablePointers
OpExtension "SPV_KHR_variable_pointers"
OpExtension "SPV_KHR_storage_buffer_storage_class"
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
)" + deco + R"(
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%block = OpTypeStruct %int
%ptr_ssbo = OpTypePointer StorageBuffer %block
%var = OpVariable %ptr_ssbo StorageBuffer
%ptr = OpTypeUntypedPointerKHR StorageBuffer
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep = )" + opcode + R"( %ptr %block %var )" +
                            extra_param + R"( %int_0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_P(ValidateMemoryUntypedAccessChain, GoodUntypedPointerBase) {
  const std::string opcode = GetParam();
  const bool ptr = opcode == "OpUntypedPtrAccessChainKHR" ||
                   opcode == "OpUntypedInBoundsPtrAccessChainKHR";
  const std::string extra_param = ptr ? "%int_0" : "";
  const std::string deco = ptr ? "OpDecorate %ptr ArrayStride 4" : "";

  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability VariablePointers
OpExtension "SPV_KHR_variable_pointers"
OpExtension "SPV_KHR_storage_buffer_storage_class"
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
)" + deco + R"(
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%block = OpTypeStruct %int
%ptr_ssbo = OpTypePointer StorageBuffer %block
%ptr = OpTypeUntypedPointerKHR StorageBuffer
%var = OpUntypedVariableKHR %ptr StorageBuffer %int
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep = )" + opcode + R"( %ptr %block %var )" +
                            extra_param + R"( %int_0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_P(ValidateMemoryUntypedAccessChain, ResultTypedPointer) {
  const std::string opcode = GetParam();
  const bool ptr = opcode == "OpUntypedPtrAccessChainKHR" ||
                   opcode == "OpUntypedInBoundsPtrAccessChainKHR";
  const std::string extra_param = ptr ? "%int_0" : "";

  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability VariablePointers
OpExtension "SPV_KHR_variable_pointers"
OpExtension "SPV_KHR_storage_buffer_storage_class"
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpName %gep "gep"
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%block = OpTypeStruct %int
%ptr_ssbo = OpTypePointer StorageBuffer %block
%var = OpVariable %ptr_ssbo StorageBuffer
%ptr = OpTypeUntypedPointerKHR StorageBuffer
%ptr_int = OpTypePointer StorageBuffer %int
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep = )" + opcode + R"( %ptr_int %block %var )" +
                            extra_param + R"( %int_0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("The Result Type of " + opcode +
                        " <id> '2[%gep]' must be OpTypeUntypedPointer"));
}

TEST_P(ValidateMemoryUntypedAccessChain, BaseTypeNotAType) {
  const std::string opcode = GetParam();
  const bool ptr = opcode == "OpUntypedPtrAccessChainKHR" ||
                   opcode == "OpUntypedInBoundsPtrAccessChainKHR";
  const std::string extra_param = ptr ? "%int_0" : "";

  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability VariablePointers
OpExtension "SPV_KHR_variable_pointers"
OpExtension "SPV_KHR_storage_buffer_storage_class"
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpName %gep "gep"
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%block = OpTypeStruct %int
%ptr_ssbo = OpTypePointer StorageBuffer %block
%var = OpVariable %ptr_ssbo StorageBuffer
%ptr = OpTypeUntypedPointerKHR StorageBuffer
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep = )" + opcode + R"( %ptr %int_0 %var )" +
                            extra_param + R"( %int_0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Base type must be a non-pointer type"));
}

TEST_P(ValidateMemoryUntypedAccessChain, BaseTypedPointer) {
  const std::string opcode = GetParam();
  const bool ptr = opcode == "OpUntypedPtrAccessChainKHR" ||
                   opcode == "OpUntypedInBoundsPtrAccessChainKHR";
  const std::string extra_param = ptr ? "%int_0" : "";

  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability VariablePointers
OpExtension "SPV_KHR_variable_pointers"
OpExtension "SPV_KHR_storage_buffer_storage_class"
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpName %gep "gep"
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%block = OpTypeStruct %int
%ptr_ssbo = OpTypePointer StorageBuffer %block
%var = OpVariable %ptr_ssbo StorageBuffer
%ptr = OpTypeUntypedPointerKHR StorageBuffer
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep = )" + opcode + R"( %ptr %ptr_ssbo %var )" +
                            extra_param + R"( %int_0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Base type must be a non-pointer type"));
}

TEST_P(ValidateMemoryUntypedAccessChain, BaseUntypedPointer) {
  const std::string opcode = GetParam();
  const bool ptr = opcode == "OpUntypedPtrAccessChainKHR" ||
                   opcode == "OpUntypedInBoundsPtrAccessChainKHR";
  const std::string extra_param = ptr ? "%int_0" : "";

  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability VariablePointers
OpExtension "SPV_KHR_variable_pointers"
OpExtension "SPV_KHR_storage_buffer_storage_class"
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpName %gep "gep"
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%block = OpTypeStruct %int
%ptr_ssbo = OpTypePointer StorageBuffer %block
%var = OpVariable %ptr_ssbo StorageBuffer
%ptr = OpTypeUntypedPointerKHR StorageBuffer
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep = )" + opcode + R"( %ptr %ptr %var )" +
                            extra_param + R"( %int_0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Base type must be a non-pointer type"));
}

TEST_P(ValidateMemoryUntypedAccessChain, BaseNotAPointer) {
  const std::string opcode = GetParam();
  const bool ptr = opcode == "OpUntypedPtrAccessChainKHR" ||
                   opcode == "OpUntypedInBoundsPtrAccessChainKHR";
  const std::string extra_param = ptr ? "%int_0" : "";

  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability VariablePointers
OpExtension "SPV_KHR_variable_pointers"
OpExtension "SPV_KHR_storage_buffer_storage_class"
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpName %int_0 "int_0"
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%block = OpTypeStruct %int
%ptr_ssbo = OpTypePointer StorageBuffer %block
%var = OpVariable %ptr_ssbo StorageBuffer
%ptr = OpTypeUntypedPointerKHR StorageBuffer
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep = )" + opcode + R"( %ptr %int %int_0 )" +
                            extra_param + R"( %int_0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("The Base <id> '2[%int_0]' in " + opcode +
                        " instruction must be a pointer"));
}

TEST_P(ValidateMemoryUntypedAccessChain, StorageClassMismatch) {
  const std::string opcode = GetParam();
  const bool ptr = opcode == "OpUntypedPtrAccessChainKHR" ||
                   opcode == "OpUntypedInBoundsPtrAccessChainKHR";
  const std::string extra_param = ptr ? "%int_0" : "";

  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability VariablePointers
OpExtension "SPV_KHR_variable_pointers"
OpExtension "SPV_KHR_storage_buffer_storage_class"
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpName %int_0 "int_0"
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%block = OpTypeStruct %int
%ptr_wg = OpTypePointer Workgroup %block
%var = OpVariable %ptr_wg Workgroup
%ptr = OpTypeUntypedPointerKHR StorageBuffer
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep = )" + opcode + R"( %ptr %block %var )" +
                            extra_param + R"( %int_0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("The result pointer storage class and base pointer storage "
                "class in " +
                opcode + " do not match"));
}

TEST_P(ValidateMemoryUntypedAccessChain, NonCompositeBase) {
  const std::string opcode = GetParam();
  const bool ptr = opcode == "OpUntypedPtrAccessChainKHR" ||
                   opcode == "OpUntypedInBoundsPtrAccessChainKHR";
  const std::string extra_param = ptr ? "%int_0" : "";

  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability VariablePointers
OpExtension "SPV_KHR_variable_pointers"
OpExtension "SPV_KHR_storage_buffer_storage_class"
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpName %int_0 "int_0"
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%block = OpTypeStruct %int
%ptr_wg = OpTypePointer StorageBuffer %block
%var = OpVariable %ptr_wg StorageBuffer
%ptr = OpTypeUntypedPointerKHR StorageBuffer
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep = )" + opcode + R"( %ptr %int %var )" +
                            extra_param + R"( %int_0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr(opcode + " reached non-composite type while indexes "
                                 "still remain to be traversed"));
}

TEST_P(ValidateMemoryUntypedAccessChain, TooManyIndices) {
  const std::string opcode = GetParam();
  const bool ptr = opcode == "OpUntypedPtrAccessChainKHR" ||
                   opcode == "OpUntypedInBoundsPtrAccessChainKHR";
  const std::string extra_param = ptr ? "%int_0" : "";

  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability VariablePointers
OpExtension "SPV_KHR_variable_pointers"
OpExtension "SPV_KHR_storage_buffer_storage_class"
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpName %int_0 "int_0"
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%block = OpTypeStruct %int
%ptr_wg = OpTypePointer StorageBuffer %block
%var = OpVariable %ptr_wg StorageBuffer
%ptr = OpTypeUntypedPointerKHR StorageBuffer
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep = )" + opcode + R"( %ptr %block %var )" +
                            extra_param + R"( %int_0 %int_0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr(opcode + " reached non-composite type while indexes "
                                 "still remain to be traversed"));
}

INSTANTIATE_TEST_SUITE_P(
    ValidateUntypedAccessChains, ValidateMemoryUntypedAccessChain,
    Values("OpUntypedAccessChainKHR", "OpUntypedInBoundsAccessChainKHR",
           "OpUntypedPtrAccessChainKHR", "OpUntypedInBoundsPtrAccessChainKHR"));

TEST_F(ValidateMemory, LoadUntypedPointerGood) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability WorkgroupMemoryExplicitLayoutKHR
OpExtension "SPV_KHR_workgroup_memory_explicit_layout"
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main" %var
OpDecorate %struct Block
OpMemberDecorate %struct 0 Offset 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%float = OpTypeFloat 32
%struct = OpTypeStruct %int
%ptr = OpTypeUntypedPointerKHR Workgroup
%var = OpUntypedVariableKHR %ptr Workgroup %struct
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%load = OpLoad %float %var
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
}

TEST_F(ValidateMemory, StoreUntypedPointerGood) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability WorkgroupMemoryExplicitLayoutKHR
OpExtension "SPV_KHR_workgroup_memory_explicit_layout"
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main" %var
OpDecorate %struct Block
OpMemberDecorate %struct 0 Offset 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%float = OpTypeFloat 32
%float_0 = OpConstant %float 0
%struct = OpTypeStruct %int
%ptr = OpTypeUntypedPointerKHR Workgroup
%var = OpUntypedVariableKHR %ptr Workgroup %struct
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpStore %var %float_0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
}

TEST_F(ValidateMemory, CopyMemoryUntypedPointerSourceGood) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability WorkgroupMemoryExplicitLayoutKHR
OpExtension "SPV_KHR_workgroup_memory_explicit_layout"
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main" %var1 %var2
OpName %var1 "var1"
OpDecorate %struct Block
OpMemberDecorate %struct 0 Offset 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%struct = OpTypeStruct %int
%ptr = OpTypeUntypedPointerKHR Workgroup
%var1 = OpUntypedVariableKHR %ptr Workgroup %struct
%ptr_wg = OpTypePointer Workgroup %int
%var2 = OpVariable %ptr_wg Workgroup
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpCopyMemory %var2 %var1
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
}

TEST_F(ValidateMemory, CopyMemoryUntypedPointerTargetGood) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability WorkgroupMemoryExplicitLayoutKHR
OpExtension "SPV_KHR_workgroup_memory_explicit_layout"
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main" %var1 %var2
OpName %var1 "var1"
OpDecorate %struct Block
OpMemberDecorate %struct 0 Offset 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%struct = OpTypeStruct %int
%ptr = OpTypeUntypedPointerKHR Workgroup
%var1 = OpUntypedVariableKHR %ptr Workgroup %struct
%ptr_wg = OpTypePointer Workgroup %int
%var2 = OpVariable %ptr_wg Workgroup
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpCopyMemory %var1 %var2
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
}

TEST_F(ValidateMemory, CopyMemoryUntypedPointerTargetAndSourceBad) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability WorkgroupMemoryExplicitLayoutKHR
OpExtension "SPV_KHR_workgroup_memory_explicit_layout"
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main" %var1 %var2
OpName %var1 "var1"
OpDecorate %struct Block
OpMemberDecorate %struct 0 Offset 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%struct = OpTypeStruct %int
%ptr = OpTypeUntypedPointerKHR Workgroup
%var1 = OpUntypedVariableKHR %ptr Workgroup %struct
%var2 = OpUntypedVariableKHR %ptr Workgroup %struct
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpCopyMemory %var1 %var2
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("One of Source or Target must be a typed pointer"));
}

TEST_F(ValidateMemory, CopyMemorySizedUntypedPointersGood) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability WorkgroupMemoryExplicitLayoutKHR
OpExtension "SPV_KHR_workgroup_memory_explicit_layout"
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main" %v1 %v2
OpDecorate %struct Block
OpMemberDecorate %struct 0 Offset 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_4 = OpConstant %int 4
%struct = OpTypeStruct %int
%ptr = OpTypeUntypedPointerKHR Workgroup
%v1 = OpUntypedVariableKHR %ptr Workgroup %struct
%v2 = OpUntypedVariableKHR %ptr Workgroup %struct
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpCopyMemorySized %v2 %v1 %int_4
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
}

TEST_F(ValidateMemory, CopyMemorySizedUntypedPointersSizeBad1) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability StorageBuffer16BitAccess
OpCapability WorkgroupMemoryExplicitLayoutKHR
OpExtension "SPV_KHR_workgroup_memory_explicit_layout"
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main" %var_wg %var_ssbo
OpDecorate %struct Block
OpMemberDecorate %struct 0 Offset 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%short = OpTypeInt 16 0
%int_2 = OpConstant %int 2
%struct = OpTypeStruct %int
%ptr_ssbo = OpTypeUntypedPointerKHR StorageBuffer
%ptr_wg = OpTypeUntypedPointerKHR Workgroup
%var_ssbo = OpUntypedVariableKHR %ptr_ssbo StorageBuffer %struct
%var_wg = OpUntypedVariableKHR %ptr_wg Workgroup %struct
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpCopyMemorySized %var_ssbo %var_wg %int_2
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
  EXPECT_THAT(getDiagnosticString(), HasSubstr("Size must be a multiple of 4"));
}

TEST_F(ValidateMemory, CopyMemorySizedUntypedPointersSizeBad2) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability StorageBuffer16BitAccess
OpCapability WorkgroupMemoryExplicitLayoutKHR
OpExtension "SPV_KHR_workgroup_memory_explicit_layout"
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main" %var_ssbo %var_wg
OpDecorate %struct Block
OpMemberDecorate %struct 0 Offset 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%short = OpTypeInt 16 0
%int_2 = OpConstant %int 2
%struct = OpTypeStruct %int
%ptr_ssbo = OpTypeUntypedPointerKHR StorageBuffer
%ptr_wg = OpTypeUntypedPointerKHR Workgroup
%var_ssbo = OpUntypedVariableKHR %ptr_ssbo StorageBuffer %struct
%var_wg = OpUntypedVariableKHR %ptr_wg Workgroup %struct
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpCopyMemorySized %var_wg %var_ssbo %int_2
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
  EXPECT_THAT(getDiagnosticString(), HasSubstr("Size must be a multiple of 4"));
}

TEST_F(ValidateMemory, CopyMemorySizedUntypedPointersSizeBad3) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Int16
OpCapability UntypedPointersKHR
OpCapability StorageBuffer8BitAccess
OpCapability WorkgroupMemoryExplicitLayoutKHR
OpExtension "SPV_KHR_workgroup_memory_explicit_layout"
OpExtension "SPV_KHR_8bit_storage"
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main" %var_ssbo %var_wg
OpDecorate %struct Block
OpMemberDecorate %struct 0 Offset 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%short = OpTypeInt 16 0
%int_1 = OpConstant %int 1
%struct = OpTypeStruct %int
%ptr_ssbo = OpTypeUntypedPointerKHR StorageBuffer
%ptr_wg = OpTypeUntypedPointerKHR Workgroup
%var_ssbo = OpUntypedVariableKHR %ptr_ssbo StorageBuffer %struct
%var_wg = OpUntypedVariableKHR %ptr_wg Workgroup %struct
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpCopyMemorySized %var_ssbo %var_wg %int_1
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
  EXPECT_THAT(getDiagnosticString(), HasSubstr("Size must be a multiple of 2"));
}

TEST_F(ValidateMemory, CopyMemorySizedUntypedPointersSizeBad4) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Int16
OpCapability UntypedPointersKHR
OpCapability StorageBuffer8BitAccess
OpCapability WorkgroupMemoryExplicitLayoutKHR
OpExtension "SPV_KHR_workgroup_memory_explicit_layout"
OpExtension "SPV_KHR_8bit_storage"
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main" %var_ssbo %var_wg
OpDecorate %struct Block
OpMemberDecorate %struct 0 Offset 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%short = OpTypeInt 16 0
%int_1 = OpConstant %int 1
%struct = OpTypeStruct %int
%ptr_ssbo = OpTypeUntypedPointerKHR StorageBuffer
%ptr_wg = OpTypeUntypedPointerKHR Workgroup
%var_ssbo = OpUntypedVariableKHR %ptr_ssbo StorageBuffer %struct
%var_wg = OpUntypedVariableKHR %ptr_wg Workgroup %struct
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpCopyMemorySized %var_wg %var_ssbo %int_1
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
  EXPECT_THAT(getDiagnosticString(), HasSubstr("Size must be a multiple of 2"));
}

TEST_F(ValidateMemory, PtrEqualUntypedPointersGood) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VariablePointers
OpCapability UntypedPointersKHR
OpCapability WorkgroupMemoryExplicitLayoutKHR
OpExtension "SPV_KHR_workgroup_memory_explicit_layout"
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main" %v1 %v2
OpDecorate %struct Block
OpMemberDecorate %struct 0 Offset 0
%void = OpTypeVoid
%bool = OpTypeBool
%int = OpTypeInt 32 0
%struct = OpTypeStruct %int
%ptr = OpTypeUntypedPointerKHR Workgroup
%v1 = OpUntypedVariableKHR %ptr Workgroup %struct
%v2 = OpUntypedVariableKHR %ptr Workgroup %struct
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%res = OpPtrEqual %bool %v1 %v2
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
}

TEST_F(ValidateMemory, PtrNotEqualUntypedPointersGood) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VariablePointers
OpCapability UntypedPointersKHR
OpCapability WorkgroupMemoryExplicitLayoutKHR
OpExtension "SPV_KHR_workgroup_memory_explicit_layout"
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main" %v1 %v2
OpDecorate %struct Block
OpMemberDecorate %struct 0 Offset 0
%void = OpTypeVoid
%bool = OpTypeBool
%int = OpTypeInt 32 0
%struct = OpTypeStruct %int
%ptr = OpTypeUntypedPointerKHR Workgroup
%v1 = OpUntypedVariableKHR %ptr Workgroup %struct
%v2 = OpUntypedVariableKHR %ptr Workgroup %struct
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%res = OpPtrNotEqual %bool %v1 %v2
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
}

TEST_F(ValidateMemory, PtrDiffUntypedPointersGood) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VariablePointers
OpCapability UntypedPointersKHR
OpCapability WorkgroupMemoryExplicitLayoutKHR
OpExtension "SPV_KHR_workgroup_memory_explicit_layout"
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main" %v1
OpDecorate %struct Block
OpMemberDecorate %struct 0 Offset 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%struct = OpTypeStruct %int
%ptr = OpTypeUntypedPointerKHR Workgroup
%v1 = OpUntypedVariableKHR %ptr Workgroup %struct
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%res = OpPtrDiff %int %v1 %v1
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
}

TEST_F(ValidateMemory, UntypedVariableVulkanPushConstantGood) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %struct Block
OpMemberDecorate %struct 0 Offset 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%struct = OpTypeStruct %int
%ptr = OpTypeUntypedPointerKHR PushConstant
%var = OpUntypedVariableKHR %ptr PushConstant %struct
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_0));
}

TEST_F(ValidateMemory, UntypedVariableVulkanStorageBufferGood) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpExtension "SPV_KHR_storage_buffer_storage_class"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %struct Block
OpMemberDecorate %struct 0 Offset 0
OpDecorate %var DescriptorSet 0
OpDecorate %var Binding 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%struct = OpTypeStruct %int
%ptr = OpTypeUntypedPointerKHR StorageBuffer
%var = OpUntypedVariableKHR %ptr StorageBuffer %struct
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_0));
}

TEST_F(ValidateMemory, UntypedVariableVulkanUniformGood) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpExtension "SPV_KHR_storage_buffer_storage_class"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %struct Block
OpMemberDecorate %struct 0 Offset 0
OpDecorate %var DescriptorSet 0
OpDecorate %var Binding 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%struct = OpTypeStruct %int
%ptr = OpTypeUntypedPointerKHR Uniform
%var = OpUntypedVariableKHR %ptr Uniform %struct
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_0));
}

TEST_F(ValidateMemory, UntypedVariableVulkanWorkgroupGood) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability WorkgroupMemoryExplicitLayoutKHR
OpExtension "SPV_KHR_workgroup_memory_explicit_layout"
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %struct Block
OpMemberDecorate %struct 0 Offset 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%struct = OpTypeStruct %int
%ptr = OpTypeUntypedPointerKHR Workgroup
%var = OpUntypedVariableKHR %ptr Workgroup %struct
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_1_SPIRV_1_4);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_1_SPIRV_1_4));
}

TEST_F(ValidateMemory, UntypedPointerAsVariableType) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability Linkage
OpCapability VariablePointers
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
%void = OpTypeVoid
%float = OpTypeFloat 32
%ptr = OpTypeUntypedPointerKHR StorageBuffer
%priv_ptr = OpTypePointer Private %ptr
%var = OpVariable %priv_ptr Private
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateMemory, UntypedArrayLengthGood) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %var DescriptorSet 0
OpDecorate %var Binding 0
OpDecorate %block Block
OpMemberDecorate %block 0 Offset 0
OpDecorate %array ArrayStride 4
%void = OpTypeVoid
%int = OpTypeInt 32 0
%array = OpTypeRuntimeArray %int
%block = OpTypeStruct %array
%ptr = OpTypeUntypedPointerKHR StorageBuffer
%var = OpUntypedVariableKHR %ptr StorageBuffer %block
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%length = OpUntypedArrayLengthKHR %int %block %var 0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateMemory, UntypedArrayLengthBadResultType) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %var DescriptorSet 0
OpDecorate %var Binding 0
OpDecorate %block Block
OpMemberDecorate %block 0 Offset 0
OpDecorate %array ArrayStride 4
%void = OpTypeVoid
%int = OpTypeInt 32 0
%float = OpTypeFloat 32
%array = OpTypeRuntimeArray %int
%block = OpTypeStruct %array
%ptr = OpTypeUntypedPointerKHR StorageBuffer
%var = OpUntypedVariableKHR %ptr StorageBuffer %block
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%length = OpUntypedArrayLengthKHR %float %block %var 0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("must be OpTypeInt with width 32 and signedness 0"));
}

TEST_F(ValidateMemory, UntypedArrayLengthBadPointer) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %var DescriptorSet 0
OpDecorate %var Binding 0
OpDecorate %block Block
OpMemberDecorate %block 0 Offset 0
OpDecorate %array ArrayStride 4
%void = OpTypeVoid
%int = OpTypeInt 32 0
%array = OpTypeRuntimeArray %int
%block = OpTypeStruct %array
%ptr = OpTypeUntypedPointerKHR StorageBuffer
%typed_ptr = OpTypePointer StorageBuffer %block
%var = OpVariable %typed_ptr StorageBuffer
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%length = OpUntypedArrayLengthKHR %int %block %var 0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Pointer must be an untyped pointer"));
}

TEST_F(ValidateMemory, UntypedArrayLengtBadStruct) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %var DescriptorSet 0
OpDecorate %var Binding 0
OpDecorate %block Block
OpMemberDecorate %block 0 Offset 0
OpDecorate %array ArrayStride 4
%void = OpTypeVoid
%int = OpTypeInt 32 0
%array = OpTypeRuntimeArray %int
%block = OpTypeStruct %array
%ptr = OpTypeUntypedPointerKHR StorageBuffer
%var = OpUntypedVariableKHR %ptr StorageBuffer %block
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%length = OpUntypedArrayLengthKHR %int %int %var 0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(getDiagnosticString(), HasSubstr("to an OpTypeStruct"));
}

TEST_F(ValidateMemory, UntypedArrayLengthLastMemberNotArray) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %var DescriptorSet 0
OpDecorate %var Binding 0
OpDecorate %block Block
OpMemberDecorate %block 0 Offset 0
OpDecorate %array ArrayStride 4
%void = OpTypeVoid
%int = OpTypeInt 32 0
%array = OpTypeRuntimeArray %int
%block = OpTypeStruct %int
%ptr = OpTypeUntypedPointerKHR StorageBuffer
%var = OpUntypedVariableKHR %ptr StorageBuffer %block
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%length = OpUntypedArrayLengthKHR %int %block %var 0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("must be an OpTypeRuntimeArray"));
}

TEST_F(ValidateMemory, UntypedArrayLengthBadIndex) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %var DescriptorSet 0
OpDecorate %var Binding 0
OpDecorate %block Block
OpMemberDecorate %block 0 Offset 0
OpDecorate %array ArrayStride 4
%void = OpTypeVoid
%int = OpTypeInt 32 0
%array = OpTypeRuntimeArray %int
%block = OpTypeStruct %array
%ptr = OpTypeUntypedPointerKHR StorageBuffer
%var = OpUntypedVariableKHR %ptr StorageBuffer %block
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%length = OpUntypedArrayLengthKHR %int %block %var 1
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("must be the last member of the struct"));
}

TEST_F(ValidateMemory, UntypedCooperativeMatrixLoad) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability CooperativeMatrixKHR
OpCapability VulkanMemoryModel
OpExtension "SPV_KHR_untyped_pointers"
OpExtension "SPV_KHR_cooperative_matrix"
OpMemoryModel Logical Vulkan
OpEntryPoint GLCompute %main "main" %var
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %var DescriptorSet 0
OpDecorate %var Binding 0
OpDecorate %block Block
OpMemberDecorate %block 0 Offset 0
OpDecorate %array ArrayStride 4
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%untyped = OpTypeUntypedPointerKHR StorageBuffer
%float = OpTypeFloat 32
%array = OpTypeRuntimeArray %float
%block = OpTypeStruct %array
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%subgroup = OpConstant %int 3
%rows = OpSpecConstant %int 1
%cols = OpSpecConstant %int 1
%matrix_a = OpConstant %int 1
%stride = OpConstant %int 42
%matrix = OpTypeCooperativeMatrixKHR %float %subgroup %rows %cols %matrix_a
%var = OpUntypedVariableKHR %untyped StorageBuffer %block
%main = OpFunction %void None %void_fn
%entry = OpLabel
%ld = OpCooperativeMatrixLoadKHR %matrix %var %int_0 %stride
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_3);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_3));
}

TEST_F(ValidateMemory, UntypedCooperativeMatrixLoad2) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability CooperativeMatrixKHR
OpCapability VulkanMemoryModel
OpExtension "SPV_KHR_untyped_pointers"
OpExtension "SPV_KHR_cooperative_matrix"
OpMemoryModel Logical Vulkan
OpEntryPoint GLCompute %main "main" %var
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %var DescriptorSet 0
OpDecorate %var Binding 0
OpDecorate %block Block
OpMemberDecorate %block 0 Offset 0
OpDecorate %array ArrayStride 4
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%untyped = OpTypeUntypedPointerKHR StorageBuffer
%float = OpTypeFloat 32
%array = OpTypeRuntimeArray %float
%block = OpTypeStruct %array
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%subgroup = OpConstant %int 3
%rows = OpSpecConstant %int 1
%cols = OpSpecConstant %int 1
%matrix_a = OpConstant %int 1
%stride = OpConstant %int 42
%matrix = OpTypeCooperativeMatrixKHR %float %subgroup %rows %cols %matrix_a
%var = OpUntypedVariableKHR %untyped StorageBuffer %block
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep = OpUntypedAccessChainKHR %untyped %block %var %int_0 %int_0
%ld = OpCooperativeMatrixLoadKHR %matrix %gep %int_0 %stride
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_3);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_3));
}

TEST_F(ValidateMemory, UntypedCooperativeMatrixStore) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability CooperativeMatrixKHR
OpCapability VulkanMemoryModel
OpExtension "SPV_KHR_untyped_pointers"
OpExtension "SPV_KHR_cooperative_matrix"
OpMemoryModel Logical Vulkan
OpEntryPoint GLCompute %main "main" %var1 %var2
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %var1 DescriptorSet 0
OpDecorate %var1 Binding 0
OpDecorate %var2 DescriptorSet 0
OpDecorate %var2 Binding 1
OpDecorate %block Block
OpMemberDecorate %block 0 Offset 0
OpDecorate %array ArrayStride 4
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%untyped = OpTypeUntypedPointerKHR StorageBuffer
%float = OpTypeFloat 32
%array = OpTypeRuntimeArray %float
%block = OpTypeStruct %array
%ptr = OpTypePointer StorageBuffer %block
%ptr_float = OpTypePointer StorageBuffer %float
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%subgroup = OpConstant %int 3
%rows = OpSpecConstant %int 1
%cols = OpSpecConstant %int 1
%matrix_a = OpConstant %int 1
%stride = OpConstant %int 42
%matrix = OpTypeCooperativeMatrixKHR %float %subgroup %rows %cols %matrix_a
%var1 = OpVariable %ptr StorageBuffer
%var2 = OpUntypedVariableKHR %untyped StorageBuffer %block
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep = OpAccessChain %ptr_float %var1 %int_0 %int_0
%ld = OpCooperativeMatrixLoadKHR %matrix %gep %int_0 %stride
OpCooperativeMatrixStoreKHR %var2 %ld %int_0 %stride
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_3);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_3));
}

TEST_F(ValidateMemory, UntypedCooperativeMatrixStore2) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability CooperativeMatrixKHR
OpCapability VulkanMemoryModel
OpExtension "SPV_KHR_untyped_pointers"
OpExtension "SPV_KHR_cooperative_matrix"
OpMemoryModel Logical Vulkan
OpEntryPoint GLCompute %main "main" %var1 %var2
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %var1 DescriptorSet 0
OpDecorate %var1 Binding 0
OpDecorate %var2 DescriptorSet 0
OpDecorate %var2 Binding 1
OpDecorate %block Block
OpMemberDecorate %block 0 Offset 0
OpDecorate %array ArrayStride 4
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%untyped = OpTypeUntypedPointerKHR StorageBuffer
%float = OpTypeFloat 32
%array = OpTypeRuntimeArray %float
%block = OpTypeStruct %array
%ptr = OpTypePointer StorageBuffer %block
%ptr_float = OpTypePointer StorageBuffer %float
%int = OpTypeInt 32 0
%int_0 = OpConstant %int 0
%subgroup = OpConstant %int 3
%rows = OpSpecConstant %int 1
%cols = OpSpecConstant %int 1
%matrix_a = OpConstant %int 1
%stride = OpConstant %int 42
%matrix = OpTypeCooperativeMatrixKHR %float %subgroup %rows %cols %matrix_a
%var1 = OpVariable %ptr StorageBuffer
%var2 = OpUntypedVariableKHR %untyped StorageBuffer %block
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep = OpAccessChain %ptr_float %var1 %int_0 %int_0
%ld = OpCooperativeMatrixLoadKHR %matrix %gep %int_0 %stride
%gep2 = OpUntypedAccessChainKHR %untyped %block %var2 %int_0 %int_0
OpCooperativeMatrixStoreKHR %gep2 %ld %int_0 %stride
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_3);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_3));
}

TEST_F(ValidateMemory, PtrAccessChainElementNotInteger) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VariablePointers
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %ptr_int ArrayStride 4
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_4 = OpConstant %int 4
%int_0 = OpConstant %int 0
%float = OpTypeFloat 32
%float_0 = OpConstant %float 0
%array = OpTypeArray %int %int_4
%ptr_array = OpTypePointer Workgroup %array
%ptr_int = OpTypePointer Workgroup %int
%var = OpVariable %ptr_array Workgroup
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep = OpAccessChain %ptr_int %var %int_0
%ptr_gep = OpPtrAccessChain %ptr_int %gep %float_0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(), HasSubstr("Element must be an integer"));
}

TEST_F(ValidateMemory, PtrAccessChainElementNotIntegerUntyped) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VariablePointers
OpCapability UntypedPointersKHR
OpCapability WorkgroupMemoryExplicitLayoutKHR
OpExtension "SPV_KHR_untyped_pointers"
OpExtension "SPV_KHR_workgroup_memory_explicit_layout"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main" %var
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %ptr_int ArrayStride 4
OpDecorate %array ArrayStride 4
OpDecorate %block Block
OpMemberDecorate %block 0 Offset 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_4 = OpConstant %int 4
%int_0 = OpConstant %int 0
%float = OpTypeFloat 32
%float_0 = OpConstant %float 0
%array = OpTypeArray %int %int_4
%block = OpTypeStruct %array
%ptr_block = OpTypeUntypedPointerKHR Workgroup
%ptr_int = OpTypeUntypedPointerKHR Workgroup
%var = OpUntypedVariableKHR %ptr_block Workgroup %block
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep = OpUntypedAccessChainKHR %ptr_int %block %var %int_0 %int_0
%ptr_gep = OpUntypedPtrAccessChainKHR %ptr_int %int %gep %float_0
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_2);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_2));
  EXPECT_THAT(getDiagnosticString(), HasSubstr("Element must be an integer"));
}

TEST_F(ValidateMemory, PtrAccessChainElementBlockArrayNonZeroConstant) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VariablePointers
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %var DescriptorSet 0
OpDecorate %var Binding 0
OpDecorate %block Block
OpMemberDecorate %block 0 Offset 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_4 = OpConstant %int 4
%int_1 = OpConstant %int 1
%int_0 = OpConstant %int 0
%block = OpTypeStruct %int
%array = OpTypeArray %block %int_4
%ptr_array = OpTypePointer StorageBuffer %array
%ptr_block = OpTypePointer StorageBuffer %block
%var = OpVariable %ptr_array StorageBuffer
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep = OpAccessChain %ptr_block %var %int_0
%ptr_gep = OpPtrAccessChain %ptr_block %gep %int_1
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Element must be 0 if the interpretation type is a "
                        "Block- or BufferBlock-decorated structure"));
}

TEST_F(ValidateMemory, PtrAccessChainElementBlockArrayNonZeroConstantUntyped) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability VariablePointers
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %var DescriptorSet 0
OpDecorate %var Binding 0
OpDecorate %block Block
OpMemberDecorate %block 0 Offset 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_4 = OpConstant %int 4
%int_1 = OpConstant %int 1
%int_0 = OpConstant %int 0
%block = OpTypeStruct %int
%array = OpTypeArray %block %int_4
%ptr_array = OpTypeUntypedPointerKHR StorageBuffer
%ptr_block = OpTypeUntypedPointerKHR StorageBuffer
%var = OpUntypedVariableKHR %ptr_array StorageBuffer %array
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep = OpUntypedAccessChainKHR %ptr_block %array %var %int_0
%ptr_gep = OpUntypedPtrAccessChainKHR %ptr_block %block %gep %int_1
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Element must be 0 if the interpretation type is a "
                        "Block- or BufferBlock-decorated structure"));
}

TEST_F(ValidateMemory, UntypedAccessChainBlockArrayMismatch1) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability VariablePointers
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %var DescriptorSet 0
OpDecorate %var Binding 0
OpDecorate %block Block
OpMemberDecorate %block 0 Offset 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_1 = OpConstant %int 1
%int_4 = OpConstant %int 4
%int_0 = OpConstant %int 0
%block = OpTypeStruct %int
%array1 = OpTypeArray %block %int_4
%array2 = OpTypeArray %block %int_4
%ptr = OpTypeUntypedPointerKHR StorageBuffer
%var = OpUntypedVariableKHR %ptr StorageBuffer %array1
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep = OpUntypedAccessChainKHR %ptr %array2 %var
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("If Base or Base Type is a Block or BufferBlock array, "
                        "the other must also be the same array"));
}

TEST_F(ValidateMemory, UntypedAccessChainBlockArrayMismatch2) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability VariablePointers
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %var DescriptorSet 0
OpDecorate %var Binding 0
OpDecorate %block Block
OpMemberDecorate %block 0 Offset 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_1 = OpConstant %int 1
%int_4 = OpConstant %int 4
%int_0 = OpConstant %int 0
%block = OpTypeStruct %int
%array1 = OpTypeArray %block %int_4
%ptr = OpTypeUntypedPointerKHR StorageBuffer
%var = OpUntypedVariableKHR %ptr StorageBuffer %array1
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%gep = OpUntypedAccessChainKHR %ptr %block %var
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Both Base Type and Base must be Block or BufferBlock "
                        "arrays or neither can be"));
}

TEST_F(ValidateMemory, UntypedAccessChainBlockArrayMismatch3) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability VariablePointers
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %var DescriptorSet 0
OpDecorate %var Binding 0
OpDecorate %block Block
OpMemberDecorate %block 0 Offset 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_1 = OpConstant %int 1
%int_4 = OpConstant %int 4
%int_0 = OpConstant %int 0
%block = OpTypeStruct %int
%array1 = OpTypeArray %block %int_4
%array2 = OpTypeArray %block %int_4
%ptr = OpTypeUntypedPointerKHR StorageBuffer
%ptr_block_array = OpTypePointer StorageBuffer %array1
%var = OpVariable %ptr_block_array StorageBuffer
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%copy1 = OpCopyObject %ptr_block_array %var
%copy2 = OpCopyObject %ptr_block_array %copy1
%gep = OpUntypedAccessChainKHR %ptr %array2 %copy2
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("If Base or Base Type is a Block or BufferBlock array, "
                        "the other must also be the same array"));
}

TEST_F(ValidateMemory, UntypedAccessChainBlockArrayMismatch4) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability VariablePointers
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %var DescriptorSet 0
OpDecorate %var Binding 0
OpDecorate %block Block
OpMemberDecorate %block 0 Offset 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_1 = OpConstant %int 1
%int_4 = OpConstant %int 4
%int_0 = OpConstant %int 0
%block = OpTypeStruct %int
%array1 = OpTypeArray %block %int_4
%ptr = OpTypeUntypedPointerKHR StorageBuffer
%ptr_block_array = OpTypePointer StorageBuffer %array1
%var = OpVariable %ptr_block_array StorageBuffer
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%copy = OpCopyObject %ptr_block_array %var
%gep = OpUntypedAccessChainKHR %ptr %block %copy
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(spirv, SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Both Base Type and Base must be Block or BufferBlock "
                        "arrays or neither can be"));
}

std::string GenCoopMat2Shader(const std::string& extra_types,
                              const std::string& main_body,
                              const std::string& after_main = "",
                              const std::string& extra_decorations = "") {
  const std::string prefix = R"(
OpCapability Shader
OpCapability Float16
OpCapability PhysicalStorageBufferAddresses
OpCapability VulkanMemoryModel
OpCapability CooperativeMatrixKHR
OpCapability TensorAddressingNV
OpCapability CooperativeMatrixTensorAddressingNV
OpCapability CooperativeMatrixBlockLoadsNV
OpExtension "SPV_KHR_physical_storage_buffer"
OpExtension "SPV_KHR_storage_buffer_storage_class"
OpExtension "SPV_NV_tensor_addressing"
OpExtension "SPV_NV_cooperative_matrix2"
OpExtension "SPV_KHR_cooperative_matrix"
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical VulkanKHR
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1

OpDecorate %f16_arr ArrayStride 2
OpDecorate %46 Block
OpMemberDecorate %46 0 Offset 0
OpDecorate %48 Binding 0
OpDecorate %48 DescriptorSet 0
OpDecorate %psb Restrict
)" + extra_decorations + R"(
%void = OpTypeVoid
%bool = OpTypeBool
%func = OpTypeFunction %void
%f16 = OpTypeFloat 16
%f32 = OpTypeFloat 32
%u32 = OpTypeInt 32 0
%s32 = OpTypeInt 32 1

%s32_0 = OpConstant %s32 0
%f16_0 = OpConstant %f16 0
%u32_2 = OpConstant %u32 2
%u32_8 = OpConstant %u32 8
%use_A = OpConstant %u32 0
%workgroup = OpConstant %u32 2
%subgroup = OpConstant %u32 3

%f16_arr = OpTypeRuntimeArray %f16
%46 = OpTypeStruct %f16_arr
%47 = OpTypePointer StorageBuffer %46
%48 = OpVariable %47 StorageBuffer
%51 = OpTypePointer StorageBuffer %f16_arr
%psbptr = OpTypePointer PhysicalStorageBuffer %f16_arr

%f16mat = OpTypeCooperativeMatrixKHR %f16 %workgroup %u32_8 %u32_8 %use_A
%f32mat = OpTypeCooperativeMatrixKHR %f32 %subgroup %u32_8 %u32_8 %use_A

%arr2 = OpTypeArray %u32 %u32_2
%functy = OpTypeFunction %f16 %psbptr %arr2 %arr2
)";

  const std::string decode_func =
      R"(
%decodefunc = OpFunction %f16 None %functy
%psb = OpFunctionParameter %psbptr
%c0 =  OpFunctionParameter %arr2
%c1 =  OpFunctionParameter %arr2
%entry2 = OpLabel
OpReturnValue %f16_0
OpFunctionEnd
)";

  const std::string func_begin =
      R"(
%main = OpFunction %void None %func
%main_entry = OpLabel

%array_ptr = OpAccessChain %51 %48 %s32_0
)";

  const std::string suffix =
      R"(
OpReturn
OpFunctionEnd)";

  return prefix + extra_types + func_begin + main_body + suffix + decode_func +
         after_main;
}

TEST_F(ValidateMemory, CoopMat2TensorLayoutAndViewSuccess) {
  std::string spirv = GenCoopMat2Shader(
      R"(
      %clamp = OpConstant %u32 0
      %dim = OpConstant %u32 2
      %p0 = OpConstant %u32 0
      %p1 = OpConstant %u32 1
      %hasdim = OpConstantFalse %bool
      %layout = OpTypeTensorLayoutNV %dim %clamp
      %view = OpTypeTensorViewNV %dim %hasdim %p0 %p1
      )",
      R"(
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateMemory, CoopMat2TensorLayoutInvalidDimFail) {
  std::string spirv = GenCoopMat2Shader(
      R"(
      %clamp = OpConstant %u32 0
      %dim = OpConstant %u32 6
      %layout = OpTypeTensorLayoutNV %dim %clamp
      )",
      R"(
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(getDiagnosticString(), HasSubstr("must be between 1 and 5"));
}

TEST_F(ValidateMemory, CoopMat2TensorLayoutInvalidClampFail) {
  std::string spirv = GenCoopMat2Shader(
      R"(
      %clamp = OpConstant %u32 6
      %dim = OpConstant %u32 2
      %layout = OpTypeTensorLayoutNV %dim %clamp
      )",
      R"(
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("must be a valid TensorClampMode"));
}

TEST_F(ValidateMemory, CoopMat2TensorViewInvalidDimFail) {
  std::string spirv = GenCoopMat2Shader(
      R"(
      %dim = OpConstant %u32 6
      %p0 = OpConstant %u32 0
      %p1 = OpConstant %u32 1
      %hasdim = OpConstantFalse %bool
      %view = OpTypeTensorViewNV %dim %hasdim %p0 %p1
      )",
      R"(
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(getDiagnosticString(), HasSubstr("must be between 1 and 5"));
}

TEST_F(ValidateMemory, CoopMat2TensorViewInvalidPermutationFail) {
  std::string spirv = GenCoopMat2Shader(
      R"(
      %dim = OpConstant %u32 3
      %p0 = OpConstant %u32 0
      %p1 = OpConstant %u32 1
      %hasdim = OpConstantFalse %bool
      %view = OpTypeTensorViewNV %dim %hasdim %p0 %p1 %p1
      )",
      R"(
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Permutation values don't form a valid permutation"));
}

TEST_F(ValidateMemory, CoopMat2TensorViewInvalidPermutation2Fail) {
  std::string spirv = GenCoopMat2Shader(
      R"(
      %dim = OpConstant %u32 3
      %p0 = OpConstant %u32 0
      %p1 = OpConstant %u32 1
      %hasdim = OpConstantFalse %bool
      %view = OpTypeTensorViewNV %dim %hasdim %p0 %p1
      )",
      R"(
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Incorrect number of permutation values."));
}

TEST_F(ValidateMemory, CoopMat2TensorLayoutBlockSizePass) {
  std::string spirv = GenCoopMat2Shader(
      R"(
      %clamp = OpConstant %u32 0
      %dim = OpConstant %u32 3
      %b = OpConstant %u32 1
      %layout = OpTypeTensorLayoutNV %dim %clamp
      )",
      R"(
      %tl = OpCreateTensorLayoutNV %layout
      %tl2 = OpTensorLayoutSetBlockSizeNV %layout %tl %b %b %b
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateMemory, CoopMat2TensorLayoutBlockSizeFail) {
  std::string spirv = GenCoopMat2Shader(
      R"(
      %clamp = OpConstant %u32 0
      %dim = OpConstant %u32 3
      %b = OpConstant %u32 1
      %layout = OpTypeTensorLayoutNV %dim %clamp
      )",
      R"(
      %tl = OpCreateTensorLayoutNV %layout
      %tl2 = OpTensorLayoutSetBlockSizeNV %layout %tl %b %b %b %b
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("unexpected number of operands"));
}

TEST_F(ValidateMemory, CoopMat2TensorLayoutDimensionPass) {
  std::string spirv = GenCoopMat2Shader(
      R"(
      %clamp = OpConstant %u32 0
      %dim = OpConstant %u32 3
      %b = OpConstant %u32 1
      %layout = OpTypeTensorLayoutNV %dim %clamp
      )",
      R"(
      %tl = OpCreateTensorLayoutNV %layout
      %tl2 = OpTensorLayoutSetDimensionNV %layout %tl %b %b %b
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateMemory, CoopMat2TensorLayoutDimensionFail) {
  std::string spirv = GenCoopMat2Shader(
      R"(
      %clamp = OpConstant %u32 0
      %dim = OpConstant %u32 3
      %b = OpConstant %u32 1
      %layout = OpTypeTensorLayoutNV %dim %clamp
      )",
      R"(
      %tl = OpCreateTensorLayoutNV %layout
      %tl2 = OpTensorLayoutSetDimensionNV %layout %tl %b %b %b %b
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("unexpected number of operands"));
}

TEST_F(ValidateMemory, CoopMat2TensorLayoutStridePass) {
  std::string spirv = GenCoopMat2Shader(
      R"(
      %clamp = OpConstant %u32 0
      %dim = OpConstant %u32 3
      %b = OpConstant %u32 1
      %layout = OpTypeTensorLayoutNV %dim %clamp
      )",
      R"(
      %tl = OpCreateTensorLayoutNV %layout
      %tl2 = OpTensorLayoutSetStrideNV %layout %tl %b %b %b
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateMemory, CoopMat2TensorLayoutStrideFail) {
  std::string spirv = GenCoopMat2Shader(
      R"(
      %clamp = OpConstant %u32 0
      %dim = OpConstant %u32 3
      %b = OpConstant %u32 1
      %layout = OpTypeTensorLayoutNV %dim %clamp
      )",
      R"(
      %tl = OpCreateTensorLayoutNV %layout
      %tl2 = OpTensorLayoutSetStrideNV %layout %tl %b %b %b %b
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("unexpected number of operands"));
}

TEST_F(ValidateMemory, CoopMat2TensorLayoutSlicePass) {
  std::string spirv = GenCoopMat2Shader(
      R"(
      %clamp = OpConstant %u32 0
      %dim = OpConstant %u32 3
      %b = OpConstant %u32 1
      %layout = OpTypeTensorLayoutNV %dim %clamp
      )",
      R"(
      %tl = OpCreateTensorLayoutNV %layout
      %tl2 = OpTensorLayoutSliceNV %layout %tl %b %b %b %b %b %b
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateMemory, CoopMat2TensorLayoutSliceFail) {
  std::string spirv = GenCoopMat2Shader(
      R"(
      %clamp = OpConstant %u32 0
      %dim = OpConstant %u32 3
      %b = OpConstant %u32 1
      %layout = OpTypeTensorLayoutNV %dim %clamp
      )",
      R"(
      %tl = OpCreateTensorLayoutNV %layout
      %tl2 = OpTensorLayoutSliceNV %layout %tl %b %b %b
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("unexpected number of operands"));
}

TEST_F(ValidateMemory, CoopMat2TensorLayoutSetClampValuePass) {
  std::string spirv = GenCoopMat2Shader(
      R"(
      %clamp = OpConstant %u32 0
      %dim = OpConstant %u32 3
      %b = OpConstant %u32 1
      %layout = OpTypeTensorLayoutNV %dim %clamp
      )",
      R"(
      %tl = OpCreateTensorLayoutNV %layout
      %tl2 = OpTensorLayoutSetClampValueNV %layout %tl %b
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateMemory, CoopMat2TensorViewDimensionPass) {
  std::string spirv = GenCoopMat2Shader(
      R"(
      %dim = OpConstant %u32 3
      %hasdim = OpConstantFalse %bool
      %p0 = OpConstant %u32 0
      %p1 = OpConstant %u32 1
      %p2 = OpConstant %u32 2
      %view = OpTypeTensorViewNV %dim %hasdim %p0 %p1 %p2
      %b = OpConstant %u32 1
      )",
      R"(
      %tv = OpCreateTensorViewNV %view
      %tv2 = OpTensorViewSetDimensionNV %view %tv %b %b %b
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateMemory, CoopMat2TensorViewDimensionFail) {
  std::string spirv = GenCoopMat2Shader(
      R"(
      %dim = OpConstant %u32 3
      %hasdim = OpConstantFalse %bool
      %p0 = OpConstant %u32 0
      %p1 = OpConstant %u32 1
      %p2 = OpConstant %u32 2
      %view = OpTypeTensorViewNV %dim %hasdim %p0 %p1 %p2
      %b = OpConstant %u32 1
      )",
      R"(
      %tv = OpCreateTensorViewNV %view
      %tv2 = OpTensorViewSetDimensionNV %view %tv %b %b %b %b
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("unexpected number of operands"));
}

TEST_F(ValidateMemory, CoopMat2TensorViewStridePass) {
  std::string spirv = GenCoopMat2Shader(
      R"(
      %dim = OpConstant %u32 3
      %hasdim = OpConstantFalse %bool
      %p0 = OpConstant %u32 0
      %p1 = OpConstant %u32 1
      %p2 = OpConstant %u32 2
      %view = OpTypeTensorViewNV %dim %hasdim %p0 %p1 %p2
      %b = OpConstant %u32 1
      )",
      R"(
      %tv = OpCreateTensorViewNV %view
      %tv2 = OpTensorViewSetStrideNV %view %tv %b %b %b
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateMemory, CoopMat2TensorViewStrideFail) {
  std::string spirv = GenCoopMat2Shader(
      R"(
      %dim = OpConstant %u32 3
      %hasdim = OpConstantFalse %bool
      %p0 = OpConstant %u32 0
      %p1 = OpConstant %u32 1
      %p2 = OpConstant %u32 2
      %view = OpTypeTensorViewNV %dim %hasdim %p0 %p1 %p2
      %b = OpConstant %u32 1
      )",
      R"(
      %tv = OpCreateTensorViewNV %view
      %tv2 = OpTensorViewSetStrideNV %view %tv %b %b %b %b
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("unexpected number of operands"));
}

TEST_F(ValidateMemory, CoopMat2TensorViewClipPass) {
  std::string spirv = GenCoopMat2Shader(
      R"(
      %dim = OpConstant %u32 3
      %hasdim = OpConstantFalse %bool
      %p0 = OpConstant %u32 0
      %p1 = OpConstant %u32 1
      %p2 = OpConstant %u32 2
      %view = OpTypeTensorViewNV %dim %hasdim %p0 %p1 %p2
      %b = OpConstant %u32 1
      )",
      R"(
      %tv = OpCreateTensorViewNV %view
      %tv2 = OpTensorViewSetClipNV %view %tv %b %b %b %b
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateMemory, CoopMat2LoadStoreTensorPass) {
  std::string spirv = GenCoopMat2Shader(
      R"(
      %clamp = OpConstant %u32 0
      %dim = OpConstant %u32 2
      %p0 = OpConstant %u32 0
      %p1 = OpConstant %u32 1
      %hasdim = OpConstantFalse %bool
      %layout = OpTypeTensorLayoutNV %dim %clamp
      %view = OpTypeTensorViewNV %dim %hasdim %p0 %p1
      )",
      R"(
      %mat = OpUndef %f16mat
      %tl = OpCreateTensorLayoutNV %layout
      %tv = OpCreateTensorViewNV %view
      %mat2 = OpCooperativeMatrixLoadTensorNV %f16mat %array_ptr %mat %tl None None
      %mat3 = OpCooperativeMatrixLoadTensorNV %f16mat %array_ptr %mat %tl Aligned 4 None
      %mat4 = OpCooperativeMatrixLoadTensorNV %f16mat %array_ptr %mat %tl None TensorView %tv
      %mat5 = OpCooperativeMatrixLoadTensorNV %f16mat %array_ptr %mat %tl None DecodeFunc %decodefunc
      %mat6 = OpCooperativeMatrixLoadTensorNV %f16mat %array_ptr %mat %tl None TensorView|DecodeFunc %tv %decodefunc
      %mat7 = OpCooperativeMatrixLoadTensorNV %f16mat %array_ptr %mat %tl Aligned 4 TensorView|DecodeFunc %tv %decodefunc
      OpCooperativeMatrixStoreTensorNV %array_ptr %mat %tl None None
      OpCooperativeMatrixStoreTensorNV %array_ptr %mat %tl Aligned 4 None
      OpCooperativeMatrixStoreTensorNV %array_ptr %mat %tl None TensorView %tv
      OpCooperativeMatrixStoreTensorNV %array_ptr %mat %tl Aligned 4 TensorView %tv
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateMemory, CoopMat2LoadTensorWrongLayoutTypeFail) {
  std::string spirv = GenCoopMat2Shader(
      R"(
      %clamp = OpConstant %u32 0
      %dim = OpConstant %u32 2
      %p0 = OpConstant %u32 0
      %p1 = OpConstant %u32 1
      %hasdim = OpConstantFalse %bool
      %layout = OpTypeTensorLayoutNV %dim %clamp
      %view = OpTypeTensorViewNV %dim %hasdim %p0 %p1
      )",
      R"(
      %mat = OpUndef %f16mat
      %tl = OpCreateTensorLayoutNV %layout
      %tv = OpCreateTensorViewNV %view
      %mat2 = OpCooperativeMatrixLoadTensorNV %f16mat %array_ptr %mat %tv None None
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("does not have a tensor layout type"));
}

TEST_F(ValidateMemory, CoopMat2LoadTensorWrongObjectTypeFail) {
  std::string spirv = GenCoopMat2Shader(
      R"(
      %clamp = OpConstant %u32 0
      %dim = OpConstant %u32 2
      %p0 = OpConstant %u32 0
      %p1 = OpConstant %u32 1
      %hasdim = OpConstantFalse %bool
      %layout = OpTypeTensorLayoutNV %dim %clamp
      %view = OpTypeTensorViewNV %dim %hasdim %p0 %p1
      )",
      R"(
      %mat = OpUndef %f32mat
      %tl = OpCreateTensorLayoutNV %layout
      %tv = OpCreateTensorViewNV %view
      %mat2 = OpCooperativeMatrixLoadTensorNV %f16mat %array_ptr %mat %tl None None
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("type does not match Result Type"));
}

TEST_F(ValidateMemory, CoopMat2LoadTensorDecodeFuncTypeFail) {
  std::string spirv = GenCoopMat2Shader(
      R"(
      %clamp = OpConstant %u32 0
      %dim = OpConstant %u32 2
      %p0 = OpConstant %u32 0
      %p1 = OpConstant %u32 1
      %hasdim = OpConstantFalse %bool
      %layout = OpTypeTensorLayoutNV %dim %clamp
      %view = OpTypeTensorViewNV %dim %hasdim %p0 %p1
      )",
      R"(
      %mat = OpUndef %f32mat
      %tl = OpCreateTensorLayoutNV %layout
      %tv = OpCreateTensorViewNV %view
      %mat2 = OpCooperativeMatrixLoadTensorNV %f32mat %array_ptr %mat %tl None DecodeFunc %decodefunc
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("return type must match matrix component type"));
}

TEST_F(ValidateMemory, CoopMat2LoadTensorDecodeFuncArrayTypeFail) {
  std::string spirv = GenCoopMat2Shader(
      R"(
      %clamp = OpConstant %u32 0
      %dim = OpConstant %u32 2
      %u32_3 = OpConstant %u32 3
      %p0 = OpConstant %u32 0
      %p1 = OpConstant %u32 1
      %hasdim = OpConstantFalse %bool
      %layout = OpTypeTensorLayoutNV %dim %clamp
      %view = OpTypeTensorViewNV %dim %hasdim %p0 %p1
      %arr3 = OpTypeArray %u32 %u32_3
      %functy2 = OpTypeFunction %f16 %psbptr %arr3 %arr3
      )",
      R"(
      %mat = OpUndef %f16mat
      %tl = OpCreateTensorLayoutNV %layout
      %tv = OpCreateTensorViewNV %view
      %mat2 = OpCooperativeMatrixLoadTensorNV %f16mat %array_ptr %mat %tl None DecodeFunc %decodefunc2
      )",
      R"(
      %decodefunc2 = OpFunction %f16 None %functy2
      %psb2 = OpFunctionParameter %psbptr
      %c02 =  OpFunctionParameter %arr3
      %c12 =  OpFunctionParameter %arr3
      %entry3 = OpLabel
      OpReturnValue %f16_0
      OpFunctionEnd
      )",
      R"(
      OpDecorate %psb2 Restrict
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("dimension equal to the tensor dimension"));
}

TEST_F(ValidateMemory, CoopMat2LoadTensorDecodeFuncPointerTypeFail) {
  std::string spirv = GenCoopMat2Shader(
      R"(
      %clamp = OpConstant %u32 0
      %dim = OpConstant %u32 2
      %p0 = OpConstant %u32 0
      %p1 = OpConstant %u32 1
      %hasdim = OpConstantFalse %bool
      %layout = OpTypeTensorLayoutNV %dim %clamp
      %view = OpTypeTensorViewNV %dim %hasdim %p0 %p1
      %sbptr = OpTypePointer StorageBuffer %f16_arr
      %functy2 = OpTypeFunction %f16 %sbptr %arr2 %arr2
      )",
      R"(
      %mat = OpUndef %f16mat
      %tl = OpCreateTensorLayoutNV %layout
      %tv = OpCreateTensorViewNV %view
      %mat2 = OpCooperativeMatrixLoadTensorNV %f16mat %array_ptr %mat %tl None DecodeFunc %decodefunc2
      )",
      R"(
      %decodefunc2 = OpFunction %f16 None %functy2
      %sb = OpFunctionParameter %sbptr
      %c02 =  OpFunctionParameter %arr2
      %c12 =  OpFunctionParameter %arr2
      %entry3 = OpLabel
      OpReturnValue %f16_0
      OpFunctionEnd
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("first parameter must be pointer to PhysicalStorageBuffer"));
}

TEST_F(ValidateMemory, PtrAccessChainNodePayloadArray) {
  const std::string spirv = R"(
OpCapability Shader
OpCapability ShaderEnqueueAMDX
OpExtension "SPV_AMDX_shader_enqueue"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main" %input
%uint = OpTypeInt 32 0
%uint_0 = OpConstant %uint 0
%uint_1 = OpConstant %uint 1
%node0 = OpConstantStringAMDX "node0"
%node1 = OpConstantStringAMDX "node1"
%node2 = OpConstantStringAMDX "node2"
%S = OpTypeStruct %uint
%_payloadarr_S = OpTypeNodePayloadArrayAMDX %S
%_ptr_NodePayloadAMDX__payloadarr_S = OpTypePointer NodePayloadAMDX %_payloadarr_S
%_ptr_NodePayloadAMDX_uint = OpTypePointer NodePayloadAMDX %uint
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%input = OpVariable %_ptr_NodePayloadAMDX__payloadarr_S NodePayloadAMDX
%main = OpFunction %void None %void_fn
%entry = OpLabel
%x = OpAccessChain %_ptr_NodePayloadAMDX_uint %input %uint_0 %uint_0
OpReturn
OpFunctionEnd
)";

  spv_target_env env = SPV_ENV_UNIVERSAL_1_4;
  CompileSuccessfully(spirv, env);
  EXPECT_THAT(SPV_SUCCESS, ValidateInstructions(env));
}

std::string GenCoopVecLoadStoreShader(const std::string& storeMemoryAccess,
                                      const std::string& loadMemoryAccess) {
  std::string s = R"(
OpCapability Shader
OpCapability Float16
OpCapability StorageBuffer16BitAccess
OpCapability VulkanMemoryModel
OpCapability CooperativeVectorNV
OpCapability ReplicatedCompositesEXT
OpExtension "SPV_EXT_replicated_composites"
OpExtension "SPV_KHR_vulkan_memory_model"
OpExtension "SPV_NV_cooperative_vector"
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical Vulkan
OpEntryPoint GLCompute %4 "main" %48 %73
OpExecutionMode %4 LocalSize 1 1 1

OpDecorate %45 ArrayStride 2
OpDecorate %46 Block
OpMemberDecorate %46 0 Offset 0
OpDecorate %48 Binding 0
OpDecorate %48 DescriptorSet 0

%2 = OpTypeVoid
%3 = OpTypeFunction %2
%6 = OpTypeInt 32 0
%49 = OpTypeInt 32 1
%41 = OpTypeFloat 16

%14 = OpConstant %6 1
%50 = OpConstant %49 0
%82 = OpConstant %6 5

%42 = OpTypeCooperativeVectorNV %41 %14
%43 = OpTypePointer Function %42

%45 = OpTypeRuntimeArray %41
%46 = OpTypeStruct %45
%47 = OpTypePointer StorageBuffer %46
%48 = OpVariable %47 StorageBuffer
%51 = OpTypePointer StorageBuffer %45

%57 = OpTypePointer Private %42
%73 = OpVariable %57 Private

%4 = OpFunction %2 None %3
%5 = OpLabel
%52 = OpAccessChain %51 %48 %50
%56 = OpCooperativeVectorLoadNV %42 %52 %50 )" +
                  loadMemoryAccess + R"( %82
%77 = OpLoad %42 %73
OpCooperativeVectorStoreNV %52 %50 %77 )" + storeMemoryAccess + R"( %82
OpReturn
OpFunctionEnd
)";

  return s;
}

TEST_F(ValidateMemory, CoopVecLoadStoreSuccess) {
  std::string spirv =
      GenCoopVecLoadStoreShader("MakePointerAvailableKHR|NonPrivatePointerKHR",
                                "MakePointerVisibleKHR|NonPrivatePointerKHR");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1_SPIRV_1_4);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_1_SPIRV_1_4));
}

TEST_F(ValidateMemory, CoopVecStoreMemoryAccessFail) {
  std::string spirv =
      GenCoopVecLoadStoreShader("MakePointerVisibleKHR|NonPrivatePointerKHR",
                                "MakePointerVisibleKHR|NonPrivatePointerKHR");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1_SPIRV_1_4);
  ASSERT_EQ(SPV_ERROR_INVALID_ID,
            ValidateInstructions(SPV_ENV_VULKAN_1_1_SPIRV_1_4));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("MakePointerVisibleKHR cannot be used with OpStore"));
}

TEST_F(ValidateMemory, CoopVecLoadMemoryAccessFail) {
  std::string spirv =
      GenCoopVecLoadStoreShader("MakePointerAvailableKHR|NonPrivatePointerKHR",
                                "MakePointerAvailableKHR|NonPrivatePointerKHR");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1_SPIRV_1_4);
  ASSERT_EQ(SPV_ERROR_INVALID_ID,
            ValidateInstructions(SPV_ENV_VULKAN_1_1_SPIRV_1_4));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("MakePointerAvailableKHR cannot be used with OpLoad"));
}

TEST_F(ValidateMemory, CoopVecInvalidStorageClassFail) {
  const std::string body = R"(
OpCapability Shader
OpCapability Float16
OpCapability CooperativeVectorNV
OpCapability ReplicatedCompositesEXT
OpExtension "SPV_NV_cooperative_vector"
OpExtension "SPV_EXT_replicated_composites"
OpExtension "SPV_KHR_vulkan_memory_model"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
%void = OpTypeVoid
%func = OpTypeFunction %void
%f16 = OpTypeFloat 16
%u32 = OpTypeInt 32 0

%u32_8 = OpConstant %u32 8
%use_A = OpConstant %u32 0
%subgroup = OpConstant %u32 3

%f16vec = OpTypeCooperativeVectorNV %f16 %u32_8

%str = OpTypeStruct %f16vec
%str_ptr = OpTypePointer Workgroup %str
%sh = OpVariable %str_ptr Workgroup

%main = OpFunction %void None %func
%main_entry = OpLabel

OpReturn
OpFunctionEnd)";

  CompileSuccessfully(body.c_str(), SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Cooperative vector types (or types containing them) can only be "
          "allocated in Function or Private storage classes or as function "
          "parameters"));
}

std::string GenCoopVecShader(const std::string& extra_types,
                             const std::string& main_body) {
  const std::string prefix =
      R"(
OpCapability Shader
OpCapability Float16
OpCapability Int64
OpCapability StorageBuffer16BitAccess
OpCapability VulkanMemoryModel
OpCapability CooperativeVectorNV
OpCapability CooperativeVectorTrainingNV
OpCapability ReplicatedCompositesEXT
OpExtension "SPV_EXT_replicated_composites"
OpExtension "SPV_KHR_vulkan_memory_model"
OpExtension "SPV_NV_cooperative_vector"
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical Vulkan
OpEntryPoint GLCompute %main "main" %48 %73
OpExecutionMode %main LocalSize 1 1 1

OpDecorate %f16_arr ArrayStride 2
OpDecorate %46 Block
OpMemberDecorate %46 0 Offset 0
OpDecorate %48 Binding 0
OpDecorate %48 DescriptorSet 0

%void = OpTypeVoid
%func = OpTypeFunction %void
%u32 = OpTypeInt 32 0
%s32 = OpTypeInt 32 1
%f16 = OpTypeFloat 16
%bool = OpTypeBool

%false = OpConstantFalse %bool
%u32_4 = OpConstant %u32 4
%u32_8 = OpConstant %u32 8
%s32_0 = OpConstant %s32 0
%f16_0 = OpConstant %f16 0

%f16vec4 = OpTypeCooperativeVectorNV %f16 %u32_4
%f16vec8 = OpTypeCooperativeVectorNV %f16 %u32_8

%f16_arr = OpTypeRuntimeArray %f16
%46 = OpTypeStruct %f16_arr
%47 = OpTypePointer StorageBuffer %46
%48 = OpVariable %47 StorageBuffer
%51 = OpTypePointer StorageBuffer %f16_arr

%57 = OpTypePointer Private %f16vec4
%73 = OpVariable %57 Private
%u32ptr = OpTypePointer Function %u32

%input4 = OpConstantCompositeReplicateEXT %f16vec4 %f16_0
%input8 = OpConstantCompositeReplicateEXT %f16vec8 %f16_0
%interp = OpConstant %u32 0
%offset = OpConstant %u32 0

)";

  const std::string func_begin =
      R"(
%main = OpFunction %void None %func
%main_entry = OpLabel
%u32var = OpVariable %u32ptr Function
%array_ptr = OpAccessChain %51 %48 %s32_0
)";

  const std::string suffix =
      R"(
OpReturn
OpFunctionEnd)";

  return prefix + extra_types + func_begin + main_body + suffix;
}

TEST_F(ValidateMemory, CoopVecMatMulSuccess) {
  std::string spirv = GenCoopVecShader("",
                                       R"(
%result0 = OpCooperativeVectorMatrixMulAddNV %f16vec4 %input4 %interp %array_ptr %offset %interp %array_ptr %offset %interp %u32_4 %u32_4 %s32_0 %false
%result1 = OpCooperativeVectorMatrixMulAddNV %f16vec4 %input8 %interp %array_ptr %offset %interp %array_ptr %offset %interp %u32_4 %u32_8 %s32_0 %false
%result2 = OpCooperativeVectorMatrixMulAddNV %f16vec8 %input4 %interp %array_ptr %offset %interp %array_ptr %offset %interp %u32_8 %u32_4 %s32_0 %false
%result3 = OpCooperativeVectorMatrixMulNV %f16vec4 %input4 %interp %array_ptr %offset %interp %u32_4 %u32_4 %s32_0 %false
%result4 = OpCooperativeVectorMatrixMulNV %f16vec4 %input8 %interp %array_ptr %offset %interp %u32_4 %u32_8 %s32_0 %false
%result5 = OpCooperativeVectorMatrixMulNV %f16vec8 %input4 %interp %array_ptr %offset %interp %u32_8 %u32_4 %s32_0 %false

OpCooperativeVectorReduceSumAccumulateNV %array_ptr %offset %input4
OpCooperativeVectorOuterProductAccumulateNV %array_ptr %offset %input4 %input8 %interp %interp
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1_SPIRV_1_4);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_1_SPIRV_1_4));
}

TEST_F(ValidateMemory, CoopVecMatMulKMismatchFail) {
  std::string spirv = GenCoopVecShader(R"()",
                                       R"(
%result1 = OpCooperativeVectorMatrixMulAddNV %f16vec4 %input8 %interp %array_ptr %offset %interp %array_ptr %offset %interp %u32_4 %u32_4 %s32_0 %false
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1_SPIRV_1_4);
  ASSERT_EQ(SPV_ERROR_INVALID_ID,
            ValidateInstructions(SPV_ENV_VULKAN_1_1_SPIRV_1_4));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("OpCooperativeVectorMatrixMulAddNV input number of "
                        "components 8 does not match K 4"));
}

TEST_F(ValidateMemory, CoopVecMatMulPackedKMismatchPass) {
  std::string spirv = GenCoopVecShader(
      R"(
%packed = OpConstant %u32 1000491001
      )",
      R"(
%result1 = OpCooperativeVectorMatrixMulAddNV %f16vec4 %input8 %packed %array_ptr %offset %interp %array_ptr %offset %interp %u32_4 %u32_4 %s32_0 %false
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1_SPIRV_1_4);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_1_SPIRV_1_4));
}

TEST_F(ValidateMemory, CoopVecMatMulMMismatchFail) {
  std::string spirv = GenCoopVecShader(R"()",
                                       R"(
%result1 = OpCooperativeVectorMatrixMulAddNV %f16vec8 %input8 %interp %array_ptr %offset %interp %array_ptr %offset %interp %u32_4 %u32_8 %s32_0 %false
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1_SPIRV_1_4);
  ASSERT_EQ(SPV_ERROR_INVALID_ID,
            ValidateInstructions(SPV_ENV_VULKAN_1_1_SPIRV_1_4));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("OpCooperativeVectorMatrixMulAddNV result type number "
                        "of components 8 does not match M 4"));
}

TEST_F(ValidateMemory, CoopVecMatMulTransposeTypeFail) {
  std::string spirv = GenCoopVecShader(R"()",
                                       R"(
%result0 = OpCooperativeVectorMatrixMulAddNV %f16vec4 %input4 %interp %array_ptr %offset %interp %array_ptr %offset %interp %u32_4 %u32_4 %s32_0 %s32_0
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1_SPIRV_1_4);
  ASSERT_EQ(SPV_ERROR_INVALID_ID,
            ValidateInstructions(SPV_ENV_VULKAN_1_1_SPIRV_1_4));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("OpCooperativeVectorMatrixMulAddNV Transpose <id> "
                        "'16[%int_0]' is not a scalar boolean"));
}

TEST_F(ValidateMemory, CoopVecMatMulInputInterpretationNotConstantFail) {
  std::string spirv = GenCoopVecShader(
      R"(
      )",
      R"(
%u32val = OpLoad %u32 %u32var
%result0 = OpCooperativeVectorMatrixMulAddNV %f16vec4 %input4 %u32val %array_ptr %offset %interp %array_ptr %offset %interp %u32_4 %u32_4 %s32_0 %false
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1_SPIRV_1_4);
  ASSERT_EQ(SPV_ERROR_INVALID_ID,
            ValidateInstructions(SPV_ENV_VULKAN_1_1_SPIRV_1_4));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("OpCooperativeVectorMatrixMulAddNV InputInterpretation "
                        "<id> '31[%31]' is not a constant instruction"));
}

TEST_F(ValidateMemory, CoopVecMatMulMatrixInterpretationNotConstantFail) {
  std::string spirv = GenCoopVecShader(
      R"(
      )",
      R"(
%u32val = OpLoad %u32 %u32var
%result0 = OpCooperativeVectorMatrixMulAddNV %f16vec4 %input4 %interp %array_ptr %offset %u32val %array_ptr %offset %interp %u32_4 %u32_4 %s32_0 %false
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1_SPIRV_1_4);
  ASSERT_EQ(SPV_ERROR_INVALID_ID,
            ValidateInstructions(SPV_ENV_VULKAN_1_1_SPIRV_1_4));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("OpCooperativeVectorMatrixMulAddNV MatrixInterpretation <id> "
                "'31[%31]' is not a constant instruction"));
}

TEST_F(ValidateMemory, CoopVecMatMulBiasInterpretationNotConstantFail) {
  std::string spirv = GenCoopVecShader(
      R"(
      )",
      R"(
%u32val = OpLoad %u32 %u32var
%result0 = OpCooperativeVectorMatrixMulAddNV %f16vec4 %input4 %interp %array_ptr %offset %interp %array_ptr %offset %u32val %u32_4 %u32_4 %s32_0 %false
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1_SPIRV_1_4);
  ASSERT_EQ(SPV_ERROR_INVALID_ID,
            ValidateInstructions(SPV_ENV_VULKAN_1_1_SPIRV_1_4));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("OpCooperativeVectorMatrixMulAddNV BiasInterpretation "
                        "<id> '31[%31]' is not a constant instruction"));
}

TEST_F(ValidateMemory, CoopVecMatMulInputInterpretationNotInt32Fail) {
  std::string spirv = GenCoopVecShader(
      R"(
      )",
      R"(
%result0 = OpCooperativeVectorMatrixMulAddNV %f16vec4 %input4 %false %array_ptr %offset %interp %array_ptr %offset %interp %u32_4 %u32_4 %s32_0 %false
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1_SPIRV_1_4);
  ASSERT_EQ(SPV_ERROR_INVALID_ID,
            ValidateInstructions(SPV_ENV_VULKAN_1_1_SPIRV_1_4));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("OpCooperativeVectorMatrixMulAddNV InputInterpretation "
                        "type <id> '12[%bool]' is not a 32 bit integer"));
}

TEST_F(ValidateMemory, CoopVecOuterProductABMismatchFail) {
  std::string spirv = GenCoopVecShader(
      R"(
%f32 = OpTypeFloat 32
%f32vec8 = OpTypeCooperativeVectorNV %f32 %u32_8
%f32_0 = OpConstant %f32 0
%input8f32 = OpConstantCompositeReplicateEXT %f32vec8 %f32_0
      )",
      R"(
OpCooperativeVectorOuterProductAccumulateNV %array_ptr %offset %input4 %input8f32 %interp %interp
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1_SPIRV_1_4);
  ASSERT_EQ(SPV_ERROR_INVALID_ID,
            ValidateInstructions(SPV_ENV_VULKAN_1_1_SPIRV_1_4));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("OpCooperativeVectorOuterProductAccumulateNV A and B component "
                "types '11[%half]' and '28[%float]' do not match"));
}

TEST_F(ValidateMemory, CoopVecOuterProductInt32OffsetFail) {
  std::string spirv = GenCoopVecShader(
      R"(
%u64 = OpTypeInt 64 0
%u64_0 = OpConstant %u64 0
      )",
      R"(
OpCooperativeVectorOuterProductAccumulateNV %array_ptr %u64_0 %input4 %input8 %interp %interp
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1_SPIRV_1_4);
  ASSERT_EQ(SPV_ERROR_INVALID_ID,
            ValidateInstructions(SPV_ENV_VULKAN_1_1_SPIRV_1_4));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("OpCooperativeVectorOuterProductAccumulateNV Offset "
                        "type <id> '28[%ulong]' is not a 32 bit integer"));
}

TEST_F(ValidateMemory, CoopVecOuterProductInt32MatrixStrideFail) {
  std::string spirv = GenCoopVecShader(
      R"(
%u64 = OpTypeInt 64 0
%u64_0 = OpConstant %u64 0
      )",
      R"(
OpCooperativeVectorOuterProductAccumulateNV %array_ptr %offset %input4 %input8 %interp %interp %u64_0
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1_SPIRV_1_4);
  ASSERT_EQ(SPV_ERROR_INVALID_ID,
            ValidateInstructions(SPV_ENV_VULKAN_1_1_SPIRV_1_4));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("OpCooperativeVectorOuterProductAccumulateNV MatrixStride type "
                "<id> '28[%ulong]' is not a 32 bit integer"));
}

TEST_F(ValidateMemory, CoopVecOuterProductVectorTypeFail) {
  std::string spirv = GenCoopVecShader(
      R"(
%f16v4 = OpTypeVector %f16 4
%f16c = OpConstantCompositeReplicateEXT %f16v4 %f16_0
      )",
      R"(
OpCooperativeVectorOuterProductAccumulateNV %array_ptr %offset %f16c %input8 %interp %interp
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1_SPIRV_1_4);
  ASSERT_EQ(SPV_ERROR_INVALID_ID,
            ValidateInstructions(SPV_ENV_VULKAN_1_1_SPIRV_1_4));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("OpCooperativeVectorOuterProductAccumulateNV A type "
                        "<id> '28[%v4half]' is not a cooperative vector type"));
}

TEST_F(ValidateMemory, CoopVecReduceSumInt32OffsetFail) {
  std::string spirv = GenCoopVecShader(
      R"(
%u64 = OpTypeInt 64 0
%u64_0 = OpConstant %u64 0
      )",
      R"(
OpCooperativeVectorReduceSumAccumulateNV %array_ptr %u64_0 %input4
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1_SPIRV_1_4);
  ASSERT_EQ(SPV_ERROR_INVALID_ID,
            ValidateInstructions(SPV_ENV_VULKAN_1_1_SPIRV_1_4));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("OpCooperativeVectorReduceSumAccumulateNV Offset type "
                        "<id> '28[%ulong]' is not a 32 bit integer"));
}

TEST_F(ValidateMemory, CoopVecReduceSumVectorTypeFail) {
  std::string spirv = GenCoopVecShader(
      R"(
%f16v4 = OpTypeVector %f16 4
%f16c = OpConstantCompositeReplicateEXT %f16v4 %f16_0
      )",
      R"(
OpCooperativeVectorReduceSumAccumulateNV %array_ptr %offset %f16c
      )");

  CompileSuccessfully(spirv.c_str(), SPV_ENV_VULKAN_1_1_SPIRV_1_4);
  ASSERT_EQ(SPV_ERROR_INVALID_ID,
            ValidateInstructions(SPV_ENV_VULKAN_1_1_SPIRV_1_4));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("OpCooperativeVectorReduceSumAccumulateNV V type <id> "
                        "'28[%v4half]' is not a cooperative vector type."));
}

TEST_F(ValidateMemory, CoopMatMatrixBFloatFAdd) {
  const std::string body =
      R"(
               OpCapability Shader
               OpCapability Float16
               OpCapability BFloat16TypeKHR
               OpCapability BFloat16CooperativeMatrixKHR
               OpCapability VulkanMemoryModel
               OpCapability CooperativeMatrixKHR
               OpExtension "SPV_KHR_bfloat16"
               OpExtension "SPV_KHR_vulkan_memory_model"
               OpExtension "SPV_KHR_cooperative_matrix"
               OpMemoryModel Logical Vulkan
               OpEntryPoint GLCompute %main "main" %_ %__0 %__1
               OpExecutionMode %main LocalSize 32 1 1
               OpDecorate %_arr_bfloat16_uint_64 ArrayStride 2
               OpDecorate %A Block
               OpMemberDecorate %A 0 Offset 0
               OpDecorate %_ Binding 0
               OpDecorate %_ DescriptorSet 0
               OpDecorate %_arr_bfloat16_uint_64_0 ArrayStride 2
               OpDecorate %B Block
               OpMemberDecorate %B 0 Offset 0
               OpDecorate %__0 Binding 1
               OpDecorate %__0 DescriptorSet 0
               OpDecorate %_arr_bfloat16_uint_64_1 ArrayStride 2
               OpDecorate %R Block
               OpMemberDecorate %R 0 Offset 0
               OpDecorate %__1 Binding 2
               OpDecorate %__1 DescriptorSet 0
       %void = OpTypeVoid
          %4 = OpTypeFunction %void
   %bfloat16 = OpTypeFloat 16 BFloat16KHR
       %uint = OpTypeInt 32 0
     %uint_3 = OpConstant %uint 3
     %uint_8 = OpConstant %uint 8
     %uint_0 = OpConstant %uint 0
         %12 = OpTypeCooperativeMatrixKHR %bfloat16 %uint_3 %uint_8 %uint_8 %uint_0
%_ptr_Function_12 = OpTypePointer Function %12
    %uint_64 = OpConstant %uint 64
%_arr_bfloat16_uint_64 = OpTypeArray %bfloat16 %uint_64
          %A = OpTypeStruct %_arr_bfloat16_uint_64
%_ptr_StorageBuffer_A = OpTypePointer StorageBuffer %A
          %_ = OpVariable %_ptr_StorageBuffer_A StorageBuffer
        %int = OpTypeInt 32 1
      %int_0 = OpConstant %int 0
%_ptr_StorageBuffer_bfloat16 = OpTypePointer StorageBuffer %bfloat16
%_arr_bfloat16_uint_64_0 = OpTypeArray %bfloat16 %uint_64
          %B = OpTypeStruct %_arr_bfloat16_uint_64_0
%_ptr_StorageBuffer_B = OpTypePointer StorageBuffer %B
        %__0 = OpVariable %_ptr_StorageBuffer_B StorageBuffer
     %v3uint = OpTypeVector %uint 3
    %uint_32 = OpConstant %uint 32
     %uint_1 = OpConstant %uint 1
         %35 = OpConstantComposite %v3uint %uint_32 %uint_1 %uint_1
%_arr_bfloat16_uint_64_1 = OpTypeArray %bfloat16 %uint_64
          %R = OpTypeStruct %_arr_bfloat16_uint_64_1
%_ptr_StorageBuffer_R = OpTypePointer StorageBuffer %R
        %__1 = OpVariable %_ptr_StorageBuffer_R StorageBuffer
       %main = OpFunction %void None %4
          %6 = OpLabel
       %matX = OpVariable %_ptr_Function_12 Function
       %matY = OpVariable %_ptr_Function_12 Function
         %23 = OpAccessChain %_ptr_StorageBuffer_bfloat16 %_ %int_0 %uint_0
         %24 = OpCooperativeMatrixLoadKHR %12 %23 %int_0 %uint_8 None
               OpStore %matX %24
         %30 = OpAccessChain %_ptr_StorageBuffer_bfloat16 %__0 %int_0 %uint_0
         %31 = OpCooperativeMatrixLoadKHR %12 %30 %int_0 %uint_8 None
               OpStore %matY %31
         %32 = OpLoad %12 %matX
         %33 = OpLoad %12 %matY
         %34 = OpFAdd %12 %32 %33
               OpReturn
               OpFunctionEnd
)";

  CompileSuccessfully(body.c_str(), SPV_ENV_VULKAN_1_3);
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_3));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("FAdd doesn't support BFloat16 type"));
}

TEST_F(ValidateMemory, CoopMatMatrixFloat8FAdd) {
  const std::string body =
      R"(
               OpCapability Shader
               OpCapability Float8EXT
               OpCapability Float8CooperativeMatrixEXT
               OpCapability VulkanMemoryModel
               OpCapability CooperativeMatrixKHR
               OpExtension "SPV_EXT_float8"
               OpExtension "SPV_KHR_cooperative_matrix"
               OpExtension "SPV_KHR_vulkan_memory_model"
               OpMemoryModel Logical Vulkan
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 32 1 1
               OpDecorate %gl_WorkGroupSize BuiltIn WorkgroupSize
       %void = OpTypeVoid
          %4 = OpTypeFunction %void
    %fp8e4m3 = OpTypeFloat 8 Float8E4M3EXT
       %uint = OpTypeInt 32 0
     %uint_3 = OpConstant %uint 3
    %uint_16 = OpConstant %uint 16
     %uint_0 = OpConstant %uint 0
         %12 = OpTypeCooperativeMatrixKHR %fp8e4m3 %uint_3 %uint_16 %uint_16 %uint_0
%_ptr_Function_12 = OpTypePointer Function %12
     %v3uint = OpTypeVector %uint 3
    %uint_32 = OpConstant %uint 32
     %uint_1 = OpConstant %uint 1
%gl_WorkGroupSize = OpConstantComposite %v3uint %uint_32 %uint_1 %uint_1
       %main = OpFunction %void None %4
          %6 = OpLabel
       %matR = OpVariable %_ptr_Function_12 Function
       %matX = OpVariable %_ptr_Function_12 Function
       %matY = OpVariable %_ptr_Function_12 Function
         %16 = OpLoad %12 %matX
         %18 = OpLoad %12 %matY
         %19 = OpFAdd %12 %16 %18
               OpStore %matR %19
               OpReturn
               OpFunctionEnd
)";

  CompileSuccessfully(body.c_str(), SPV_ENV_VULKAN_1_3);
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_3));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("FAdd doesn't support FP8 E4M3/E5M2 types"));
}

}  // namespace
}  // namespace val
}  // namespace spvtools
