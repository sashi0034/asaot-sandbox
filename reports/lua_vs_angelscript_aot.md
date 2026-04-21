# Lua vs AngelScript Benchmark Report

測定日: 2026-04-21

## 比較対象

- Lua: `build/Lua/lua_benchmark`
- AngelScript aot無し: `build/AngelScript/angelscript_benchmark_noaot`
- AngelScript aot有り: `build/AngelScript/angelscript_benchmark_aot`

各ワークロードは 2 回ウォームアップ後、7 回測定しました。表では中央値を主な比較値として使っています。

## 結果

| workload | repeat | Lua median ms | AS no AOT median ms | AS AOT median ms | AOT vs no AOT | AOT vs Lua |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| native_loop | 1 | 66.620 | 41.434 | 16.018 | 2.59x | 4.16x |
| fibonacci_loop | 3 | 9.215 | 9.350 | 2.875 | 3.25x | 3.21x |
| fibonacci_recursive | 1 | 103.246 | 189.329 | 162.909 | 1.16x | 0.63x |
| mandelbrot | 1 | 10.932 | 11.705 | 3.847 | 3.04x | 2.84x |

## 要約

AngelScript AOT は、ループ主体の `native_loop`、`fibonacci_loop`、`mandelbrot` で AngelScript aot無しより約 2.6-3.3 倍高速でした。Lua 比でも同 3 ワークロードでは約 2.8-4.2 倍高速です。

一方、`fibonacci_recursive` は AOT 化しても改善が小さく、Lua より遅い結果でした。再帰呼び出しのオーバーヘッドが残りやすく、今回の asaot 生成コードでは単純ループほど効いていません。

## 実装メモ

- 共通の測定ヘルパーを `common/benchmark_common.h` と `common/benchmark_workloads.h` に追加しました。
- AngelScript は `angelscript_benchmark_noaot` と `angelscript_benchmark_aot` を分離しました。
- AOT 版は `angelscript_generate` で `build/AngelScript/angelscript_aot_generated.cpp` を生成してからビルドします。
- asaot 側の制約で `uint64` 戻り値の AOT 実行が不安定だったため、AngelScript 側のベンチ戻り値は `uint` に寄せています。
- 固定配列を使う `primes_loop` と、AOT 実行時に落ちた `queen` は今回の比較対象から外しました。

## 生成物

- `Lua/results/lua_results.json`
- `AngelScript/results/angelscript_noaot_results.json`
- `AngelScript/results/angelscript_aot_results.json`
