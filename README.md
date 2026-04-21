# asaot-sandbox

AngelScript と Lua のベンチマーク用サンプルをビルドして実行する手順です。

## 必要なもの

- `cmake`
- C++17 対応コンパイラ
- `python3`

## ディレクトリ構成

- `AngelScript/`: AngelScript + asaot のサンプル
- `Lua/`: Lua のベンチマーク
- `AngelScript/asaot/`: Git submodule

## 実行方法

リポジトリのルートで以下を実行します。

```bash
cmake -S AngelScript -B build/AngelScript
cmake --build build/AngelScript --target angelscript_benchmark -j2
./build/AngelScript/angelscript_benchmark AngelScript/scripts/sample.as build/AngelScript/angelscript_aot_generated.cpp

cmake -S Lua -B build/Lua
cmake --build build/Lua --target lua_benchmark -j2
./build/Lua/lua_benchmark
```

## 補足

- `angelscript_benchmark` のビルド時に `angelscript_generate` も自動で実行され、`build/AngelScript/angelscript_aot_generated.cpp` が生成されます。
- AngelScript のサンプルスクリプトは `AngelScript/scripts/sample.as` です。
- `AngelScript/asaot/` は Git submodule なので、clone 直後は `git submodule update --init --recursive` を実行してください。

## 実行例

```text
sin=1
script returned: 1000
linked AOT table size: 3
```
