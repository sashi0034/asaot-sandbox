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
cmake -S AngelScript -B build/AngelScript-release -DCMAKE_BUILD_TYPE=Release
cmake --build build/AngelScript-release --target angelscript_benchmark -j2
./build/AngelScript-release/angelscript_benchmark_noaot AngelScript/scripts/sample.as AngelScript/results/angelscript_noaot_results.json
./build/AngelScript-release/angelscript_benchmark_aot AngelScript/scripts/sample.as AngelScript/results/angelscript_aot_results.json

cmake -S Lua -B build/Lua-release -DCMAKE_BUILD_TYPE=Release
cmake --build build/Lua-release --target lua_benchmark -j2
./build/Lua-release/lua_benchmark
```

## 補足

- `angelscript_benchmark` のビルド時に `angelscript_generate` も自動で実行され、`build/AngelScript-release/angelscript_aot_generated.cpp` が生成されます。
- AngelScript の実測バイナリは aot無しが `angelscript_benchmark_noaot`、aot有りが `angelscript_benchmark_aot` です。
- AngelScript のサンプルスクリプトは `AngelScript/scripts/sample.as` です。
- 今回の比較レポートは `reports/lua_vs_angelscript_aot.md` です。
- `AngelScript/asaot/` は Git submodule なので、clone 直後は `git submodule update --init --recursive` を実行してください。

## 実行例

```text
sin=1
script returned: 1000
linked AOT table size: 3
```
