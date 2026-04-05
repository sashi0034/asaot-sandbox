# asaot-sandbox

`sandbox` のサンプルをビルドして実行する手順です。

## 必要なもの

- `cmake`
- C++17 対応コンパイラ
- `python3`

## 実行方法

リポジトリのルートで以下を実行します。

```bash
cmake -S sandbox -B build/sandbox
cmake --build build/sandbox --target sandbox_run -j2
./build/sandbox/sandbox_run sandbox/sample.as build/sandbox/sandbox_aot_generated.cpp
```

## 補足

- `sandbox_run` のビルド時に `sandbox_generate` も自動で実行され、`build/sandbox/sandbox_aot_generated.cpp` が生成されます。
- サンプルスクリプトは `sandbox/sample.as` です。

## 実行例

```text
sin=1
script returned: 1000
linked AOT table size: 3
```
