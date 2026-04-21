#include "../../common/benchmark_common.h"
#include "../../common/benchmark_workloads.h"

extern "C" {
#include "../lua-5.5.0/lauxlib.h"
#include "../lua-5.5.0/lua.h"
#include "../lua-5.5.0/lualib.h"
}

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require_lua(bool ok, lua_State *L, const std::string &message) {
    if (!ok) {
        const char *err = lua_tostring(L, -1);
        throw std::runtime_error(message + (err ? (": " + std::string(err)) : ""));
    }
}

std::uint64_t execute_benchmark_function(lua_State *L, const std::string &fn_name, int repeat_count) {
    lua_getglobal(L, fn_name.c_str());
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        return 0;
    }

    lua_pushinteger(L, repeat_count);
    require_lua(lua_pcall(L, 1, 1, 0) == LUA_OK, L, "lua function call failed");

    if (!lua_isinteger(L, -1)) {
        lua_pop(L, 1);
        throw std::runtime_error("lua benchmark function did not return integer: " + fn_name);
    }
    const auto value = static_cast<std::uint64_t>(lua_tointeger(L, -1));
    lua_pop(L, 1);
    return value;
}

} // namespace

int main() {
    const std::filesystem::path project_root = SLC_PROJECT_ROOT;
    const std::filesystem::path script_path = project_root / "scripts" / "bench.lua";
    const std::filesystem::path results_path = project_root / "results" / "lua_results.json";

    lua_State *L = luaL_newstate();
    if (!L) {
        std::cerr << "failed to create Lua state\n";
        return 1;
    }

    try {
        luaL_openlibs(L);
        require_lua(luaL_dofile(L, script_path.string().c_str()) == LUA_OK, L, "failed to load Lua script");

        std::vector<slc::BenchmarkSample> samples;
        for (const auto &item : slc::benchmark_items()) {
            const std::string fn_name = std::string("benchmark_") + item.name;
            lua_getglobal(L, fn_name.c_str());
            const bool has_script_function = lua_isfunction(L, -1);
            lua_pop(L, 1);

            slc::BenchmarkSample sample;
            if (!has_script_function) {
                std::cerr << "[Lua] missing script function '" << item.name << "', recording zero time\n";
                sample.name = item.name;
                sample.repeat_count = item.repeat_count;
            } else {
                sample = slc::run_benchmark_sample(item, [&](int repeat_count) {
                    return execute_benchmark_function(L, fn_name, repeat_count);
                });
            }

            std::cout << "[Lua] " << sample.name
                      << " best=" << slc::format_double(sample.best_ms)
                      << "ms median=" << slc::format_double(sample.median_ms)
                      << "ms repeat=" << sample.repeat_count << "\n";
            samples.push_back(sample);
        }

        slc::write_results_json(results_path, "Lua", samples);
        std::cout << "results: " << results_path.string() << "\n";
        std::cout << "sink: " << static_cast<unsigned long long>(slc::g_sink) << "\n";
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\n";
        lua_close(L);
        return 1;
    }

    lua_close(L);
    return 0;
}
