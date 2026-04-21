# Lua vs AngelScript Benchmark Report

測定日: 2026-04-21

## 比較対象

- Lua: `build/Lua-release/lua_benchmark`
- AngelScript aot無し: `build/AngelScript-release/angelscript_benchmark_noaot`
- AngelScript aot有り: `build/AngelScript-release/angelscript_benchmark_aot`

各ワークロードは Release ビルドで 2 回ウォームアップ後、7 回測定しました。表では中央値を主な比較値として使っています。
測定対象の checksum は Lua、AngelScript aot無し、AngelScript aot有りですべて一致しています。

## 結果

| workload | repeat | Lua median ms | AS no AOT median ms | AS AOT median ms | AOT vs no AOT | AOT vs Lua |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| native_loop | 1 | 116.695 | 21.679 | 6.476 | 3.35x | 18.02x |
| fibonacci_loop | 3 | 22.028 | 3.544 | 0.989 | 3.58x | 22.27x |
| fibonacci_recursive | 1 | 53.288 | 56.508 | 61.879 | 0.91x | 0.86x |
| mandelbrot | 1 | 4.609 | 4.430 | 1.338 | 3.31x | 3.44x |

## 要約

AngelScript AOT は、ループ主体の `native_loop`、`fibonacci_loop`、`mandelbrot` で AngelScript aot無しより約 3.3-3.6 倍高速でした。Lua 比でも同 3 ワークロードでは約 3.4-22.3 倍高速です。

一方、`fibonacci_recursive` は AOT 化で速くならず、AngelScript aot無しと Lua の両方より遅い結果でした。再帰呼び出しのオーバーヘッドが残りやすく、今回の asaot 生成コードでは単純ループほど効いていません。

## 実装メモ

- 共通の測定ヘルパーを `common/benchmark_common.h` と `common/benchmark_workloads.h` に追加しました。
- AngelScript は `angelscript_benchmark_noaot` と `angelscript_benchmark_aot` を分離しました。
- AOT 版は `angelscript_generate` で `build/AngelScript-release/angelscript_aot_generated.cpp` を生成してからビルドします。
- asaot 側の制約で `uint64` 戻り値の AOT 実行が不安定だったため、AngelScript 側のベンチ戻り値は `uint` に寄せています。
- Lua 側も比較対象ワークロードは 32-bit unsigned 相当の wrap に寄せ、AngelScript 側と checksum が一致することを確認しています。
- 固定配列を使う `primes_loop` と、AOT 実行時に落ちた `queen` は今回の比較対象から外しました。

## 生成物

- `Lua/results/lua_results.json`
- `AngelScript/results/angelscript_noaot_results.json`
- `AngelScript/results/angelscript_aot_results.json`
