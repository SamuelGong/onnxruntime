# Zhifeng's Guidance

[This](./README-original.md) is the original README for the LookOnceToHear project

## 1. Common Steps

```bash
git clone --recursive git@github.com:SamuelGong/onnxruntime.git  # may take ten minutes to finish
cd onnxruntime
git submodule update --init --recursive 
```

*This means that the branch you use should be no older than v1.22.2.*

## Case A: Compile for MacBook (Apple Silicon) on MacBook (Apple Silicon)

```bash
brew install cmake ninja
```

### Case A.1 Normal build

```bash
# to avoid the use of system/conda's protobuf
conda deactivate
brew uninstall protobuf

# may need to set up proxy (export http_proxy=... && export https_proxy=... ) if there is network issue

rm -rf build_macos_arm64/Release
./build.sh \
  --config Release \
  --build_shared_lib \
  --parallel \
  --compile_no_warning_as_error \
  --skip_submodule_sync \
  --build_dir build_macos_arm64 \
  --cmake_extra_defines CMAKE_OSX_ARCHITECTURES=arm64 \
  --cmake_extra_defines CMAKE_IGNORE_PATH=/opt/homebrew

cd build_macos_arm64/Release
cmake --install . --prefix "$(pwd)/install"
```

<details> <summary><b>Example file output (Tab here to expand)</b></summary>

```
build_macos_arm64/Release/install
├── bin
│   └── onnx_test_runner
├── include
│   └── onnxruntime
│       ├── core
│       │   └── providers
│       │       ├── custom_op_context.h
│       │       └── resource.h
│       ├── cpu_provider_factory.h
│       ├── onnxruntime_c_api.h
│       ├── onnxruntime_cxx_api.h
│       ├── onnxruntime_cxx_inline.h
│       ├── onnxruntime_ep_c_api.h
│       ├── onnxruntime_ep_device_ep_metadata_keys.h
│       ├── onnxruntime_float16.h
│       ├── onnxruntime_lite_custom_op.h
│       ├── onnxruntime_run_options_config_keys.h
│       ├── onnxruntime_session_options_config_keys.h
│       └── provider_options.h
└── lib
    ├── cmake
    │   └── onnxruntime
    │       ├── onnxruntimeConfig.cmake
    │       ├── onnxruntimeConfigVersion.cmake
    │       ├── onnxruntimeTargets-release.cmake
    │       └── onnxruntimeTargets.cmake
    ├── libonnxruntime.1.23.0.dylib
    ├── libonnxruntime.dylib -> libonnxruntime.1.23.0.dylib
    └── pkgconfig
        └── libonnxruntime.pc
```

In particular, one can copy `build_macos_arm64/Release/install/lib` together with `build_macos_arm64/Release/install/include` for later compilation use.

</details>

### Case A.2 Minimal Build

Make sure you have converted your onnx models and get the merged version of configuration files.
Let's assume the absolute path to that merged file is `$OP_CONFIG` (e.g., `/Users/samuel/Repositories/code/LookOnceToHear/harmony_deploy/required_operators_and_types.config`).

```bash
# may need to set up proxy (export http_proxy=... && export https_proxy=... ) if there is network issue

python3.13 -m venv ~/venvs/ortbuild  # the Python in the environment should be >= 3.12, otherwise will have syntax errors
source ~/venvs/ortbuild/bin/activate
python -m pip install --upgrade pip
python -m pip install flatbuffers

rm -rf build_macos_arm64/MinSizeRel
./build.sh \
  --config MinSizeRel \
  --build_shared_lib \
  --parallel \
  --compile_no_warning_as_error \
  --skip_submodule_sync \
  --build_dir build_macos_arm64 \
  --cmake_extra_defines CMAKE_OSX_ARCHITECTURES=arm64 \
  --cmake_extra_defines CMAKE_IGNORE_PATH=/opt/homebrew \
  --skip_tests \
  --minimal_build \
  --disable_ml_ops \
  --enable_reduced_operator_type_support \
  --include_ops_by_config $OP_CONFIG
 
cd build_macos_arm64/MinSizeRel
cmake --install . --prefix "$(pwd)/install"
```

**Remarks**:
* `--minimal_build`: for CPU EP it’s the smallest.
* `--include_ops_by_config <file>`: trims kernels down to just what your models need. 
* `--enable_reduced_operator_type_support`: strips unneeded tensor dtypes for included ops. 
* `--disable_ml_ops`: drops ai.onnx.ml domain; keep it only if your config needs ML ops (LabelEncoder, etc.). If your config includes any ai.onnx.ml ops, remove this flag.

<details> <summary><b>Example file output (Tab here to expand)</b></summary>

```
build_macos_arm64/MinSizeRel/install
├── bin
│   └── onnx_test_runner
├── include
│   └── onnxruntime
│       ├── core
│       │   └── providers
│       │       ├── custom_op_context.h
│       │       └── resource.h
│       ├── cpu_provider_factory.h
│       ├── onnxruntime_c_api.h
│       ├── onnxruntime_cxx_api.h
│       ├── onnxruntime_cxx_inline.h
│       ├── onnxruntime_ep_c_api.h
│       ├── onnxruntime_ep_device_ep_metadata_keys.h
│       ├── onnxruntime_float16.h
│       ├── onnxruntime_lite_custom_op.h
│       ├── onnxruntime_run_options_config_keys.h
│       ├── onnxruntime_session_options_config_keys.h
│       └── provider_options.h
└── lib
    ├── cmake
    │   └── onnxruntime
    │       ├── onnxruntimeConfig.cmake
    │       ├── onnxruntimeConfigVersion.cmake
    │       ├── onnxruntimeTargets-minsizerel.cmake
    │       └── onnxruntimeTargets.cmake
    ├── libonnxruntime.1.23.0.dylib
    ├── libonnxruntime.dylib -> libonnxruntime.1.23.0.dylib
    └── pkgconfig
        └── libonnxruntime.pc
```

In particular, one can copy `build_macos_arm64/MinSizeRel/install/lib` together with `build_macos_arm64/MinSizeRel/install/include` for later compilation use.

## Case B: Compile for HarmonyOS (Mate 60/70 Pro) on MacBook (Apple Silicon)

Make sure you have HarmonyOS NDK installed, and the absolute path to it is `$NDK_PATH` (e.g., `/Applications/DevEco-Studio.app/Contents/sdk/default/openharmony/native`)

Also, create a new file `ort_ohos_compat.h` containing:

```c++
#pragma once

#if defined(__linux__) && !defined(__GLIBC__)
  #include <sched.h>
  #include <pthread.h>
  #include <unistd.h>
  #include <sys/syscall.h>

  extern "C" inline int pthread_setaffinity_np(pthread_t /*thread*/,
                                               size_t cpusetsize,
                                               const cpu_set_t* mask) {
  #ifdef SYS_gettid
    pid_t tid = (pid_t)syscall(SYS_gettid);
  #else
    pid_t tid = getpid();
  #endif
    return sched_setaffinity(tid, cpusetsize, mask);
  }
#endif
```

at any place you like.
Let us refer to the absolute path to this file (e.g., `/Users/samuel/Repositories/code/onnxruntime/ort_ohos_compat.h`) as `$ORT_PATH`.

Finally, run

```bash
brew install cmake ninja
```

### Case B.1 Normal build

```bash
# may need to set up proxy (export http_proxy=... && export https_proxy=... ) if there is network issue

mkdir -p build_ohos_arm64/Release
cd build_ohos_arm64/Release

cmake -S ../../cmake \
  -B . \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$NDK_PATH/build/cmake/ohos.toolchain.cmake" \
  -DOHOS_ARCH=arm64-v8a \
  -DOHOS_STL=c++_shared \
  -Donnxruntime_BUILD_SHARED_LIB=ON \
  -Donnxruntime_ENABLE_PYTHON=OFF \
  -Donnxruntime_BUILD_UNIT_TESTS=OFF \
  -Donnxruntime_RUN_ONNX_TESTS=OFF \
  -Donnxruntime_ENABLE_CPUINFO=OFF \
  -DCMAKE_C_FLAGS="-Wno-unused-command-line-argument -Wno-error=unused-command-line-argument -Xclang -target-feature -fno-emulated-tls -Xclang +bf16" \
  -DCMAKE_CXX_FLAGS="-Wno-unused-command-line-argument -Wno-error=unused-command-line-argument -Xclang -target-feature -fno-emulated-tls -Xclang +bf16 -include $ORT_PATH" \
  -DCMAKE_ASM_FLAGS="-Wno-unused-command-line-argument -Wno-error=unused-command-line-argument -Xclang -target-feature -Xclang +bf16"

cmake --build . --parallel
cmake --install . --prefix "$(pwd)/install"
```

**Remark**: Upon failure, one should remove the built intermediate files using `rm -rf CMakeCache.txt CMakeFiles _deps` before running `cmake -S` and `cmake --build`.

<details> <summary><b>Example file output (Tab here to expand)</b></summary>

```
build_ohos_arm64/Release/install
├── include
│   └── onnxruntime
│       ├── core
│       │   └── providers
│       │       ├── custom_op_context.h
│       │       └── resource.h
│       ├── cpu_provider_factory.h
│       ├── onnxruntime_c_api.h
│       ├── onnxruntime_cxx_api.h
│       ├── onnxruntime_cxx_inline.h
│       ├── onnxruntime_ep_c_api.h
│       ├── onnxruntime_ep_device_ep_metadata_keys.h
│       ├── onnxruntime_float16.h
│       ├── onnxruntime_lite_custom_op.h
│       ├── onnxruntime_run_options_config_keys.h
│       ├── onnxruntime_session_options_config_keys.h
│       └── provider_options.h
└── lib
    ├── cmake
    │   └── onnxruntime
    │       ├── onnxruntimeConfig.cmake
    │       ├── onnxruntimeConfigVersion.cmake
    │       ├── onnxruntimeTargets-release.cmake
    │       └── onnxruntimeTargets.cmake
    ├── libonnxruntime.so -> libonnxruntime.so.1
    ├── libonnxruntime.so.1 -> libonnxruntime.so.1.23.0
    ├── libonnxruntime.so.1.23.0
    ├── libonnxruntime_providers_shared.so
    └── pkgconfig
        └── libonnxruntime.pc
```

In particular, one can copy `build_ohos_arm64/Release/install/lib` together with `build_ohos_arm64/Release/install/include` for later compilation use.

### Case B.2 Minimal build

```bash
# may need to set up proxy (export http_proxy=... && export https_proxy=... ) if there is network issue

mkdir -p build_ohos_arm64/MinSizeRel
cd build_ohos_arm64/MinSizeRel

cmake -S ../../cmake \
  -B . \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=MinSizeRel \
  -DCMAKE_TOOLCHAIN_FILE="$NDK_PATH/build/cmake/ohos.toolchain.cmake" \
  -DOHOS_ARCH=arm64-v8a \
  -DOHOS_STL=c++_shared \
  -Donnxruntime_BUILD_SHARED_LIB=ON \
  -Donnxruntime_ENABLE_PYTHON=OFF \
  -Donnxruntime_BUILD_UNIT_TESTS=OFF \
  -Donnxruntime_RUN_ONNX_TESTS=OFF \
  -Donnxruntime_ENABLE_CPUINFO=OFF \
  -Donnxruntime_MINIMAL_BUILD=ON \
  -Donnxruntime_DISABLE_ML_OPS=ON \
  -Donnxruntime_ENABLE_REDUCED_OPERATOR_TYPE_SUPPORT=ON \
  -Donnxruntime_INCLUDE_OPS_BY_CONFIG="$OP_CONFIG" \
  -DCMAKE_C_FLAGS="-Wno-unused-command-line-argument -Wno-error=unused-command-line-argument -Xclang -target-feature -fno-emulated-tls -Xclang +bf16" \
  -DCMAKE_CXX_FLAGS="-Wno-unused-command-line-argument -Wno-error=unused-command-line-argument -Xclang -target-feature -fno-emulated-tls -Xclang +bf16 -include $ORT_PATH" \
  -DCMAKE_ASM_FLAGS="-Wno-unused-command-line-argument -Wno-error=unused-command-line-argument -Xclang -target-feature -Xclang +bf16"

cmake --build . --parallel
cmake --install . --prefix "$(pwd)/install"
```

**Remark**: Upon failure, one should remove the built intermediate files using `rm -rf CMakeCache.txt CMakeFiles _deps` before running `cmake -S` and `cmake --build`.

<details> <summary><b>Example file output (Tab here to expand)</b></summary>

```
build_ohos_arm64/MinSizeRel/install

```

</details>

In particular, one can copy `build_ohos_arm64/MinSizeRel/install/lib` together with `build_ohos_arm64/MinSizeRel/install/include` for later compilation use.