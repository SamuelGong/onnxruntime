# Zhifeng's Guidance

[This](./README-original.md) is the original README for the LookOnceToHear project

## 1. Common Steps

```bash
git clone --recursive git@github.com:SamuelGong/onnxruntime.git  # may take ten minutes to finish
git submodule update --init --recursive 
```

*This means that the branch you use should be no older than v1.22.2.*

## 2. Compile for MacBook (Apple Silicon) on MacBook (Apple Silicon)

```bash
brew install cmake ninja
```

### 2.1 Normal build

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

### 2.2 Build minimal CPU-only library

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
  --include_ops_by_config /Users/samuel/Repositories/code/LookOnceToHear/harmony_deploy/required_operators_and_types.config
 
cd build_macos_arm64/MinSizeRel
cmake --install . --prefix "$(pwd)/install"

deactivate
# conda activate
```

Why these changes
* `--build_dir build_macos_arm64_min`: keep this minimal build isolated from your full build. 
* `--minimal_build` (no “extended”): for CPU EP it’s the smallest. “Extended” is for EPs that need dynamic kernel creation (CoreML/XNNPACK/etc.). If you later add those, switch to extended. 
* `--include_ops_by_config <file>`: trims kernels down to just what your models need. 
* `--enable_reduced_operator_type_support`: strips unneeded tensor dtypes for included ops. 
* `--disable_ml_ops`: drops ai.onnx.ml domain; keep it only if your config needs ML ops (LabelEncoder, etc.). If your config includes any ai.onnx.ml ops, remove this flag.
* `MinSizeRel`: enables size-oriented optimizations with symbols off—good default for tiny builds.