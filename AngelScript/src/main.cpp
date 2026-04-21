#include "../../common/benchmark_common.h"
#include "../../common/benchmark_workloads.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <angelscript.h>

#include "AOTCompiler.h"

namespace {

class MessageCallback {
public:
    void Callback(const asSMessageInfo *msg) const {
        const char *kind = "ERR ";
        if (msg->type == asMSGTYPE_WARNING) {
            kind = "WARN";
        } else if (msg->type == asMSGTYPE_INFORMATION) {
            kind = "INFO";
        }

        std::cerr << msg->section << " (" << msg->row << ", " << msg->col << ") : "
                  << kind << " : " << msg->message << '\n';
    }
};

class CCodeStream : public asIBinaryStream {
public:
    explicit CCodeStream(const char *filename) : file_(filename, std::ios::binary) {}

    int Read(void *, asUINT) override {
        return 0;
    }

    int Write(const void *ptr, asUINT size) override {
        file_.write(static_cast<const char *>(ptr), static_cast<std::streamsize>(size));
        return file_ ? 0 : -1;
    }

private:
    std::ofstream file_;
};

struct AOTLinkerEntry {
    const char name[256];
    asJITFunction entry;
};

class SimpleAOTLinker : public AOTLinker {
public:
    SimpleAOTLinker(AOTLinkerEntry *entries, unsigned int size)
        : entries_(entries), size_(size) {}

    LinkerResult LookupFunction(AOTFunction *function, asJITFunction *jitFunction) override {
        if (entries_ != nullptr) {
            for (unsigned int i = 0; i < size_; ++i) {
                if (function->GetName() == entries_[i].name) {
                    *jitFunction = entries_[i].entry;
                    return LinkSuccessful;
                }
            }
        }

        return GenerateCode;
    }

    void LinkTimeCodeGeneration(std::string &code, std::vector<AOTFunction> &compiledFunctions) override {
        code += "typedef struct\n";
        code += "{\n";
        code += "    const char name[256];\n";
        code += "    asJITFunction entry;\n";
        code += "} AOTLinkerEntry;\n";
        code += "\n";
        code += "unsigned int AOTLinkerTableSize = " + std::to_string(compiledFunctions.size()) + ";\n";
        code += "AOTLinkerEntry AOTLinkerTable[] =\n{\n";
        for (const AOTFunction &function : compiledFunctions) {
            code += "{\"" + function.GetName() + "\", " + function.GetName() + "},\n";
        }
        code += "};\n";
    }

private:
    AOTLinkerEntry *entries_;
    unsigned int size_;
};

std::string read_all(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("failed to read script: " + path.string());
    }
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

void require_as(int code, const std::string &message) {
    if (code < 0) {
        throw std::runtime_error(message + ": " + std::to_string(code));
    }
}

asIScriptModule *build_module(asIScriptEngine *engine, const std::filesystem::path &script_path) {
    const std::string script = read_all(script_path);
    asIScriptModule *module = engine->GetModule("benchmark", asGM_ALWAYS_CREATE);
    require_as(module->AddScriptSection(script_path.string().c_str(),
                                        script.c_str(),
                                        static_cast<unsigned int>(script.size())),
               "AddScriptSection failed");
    require_as(module->Build(), "Build failed");
    return module;
}

std::uint64_t execute_benchmark_function(asIScriptEngine *engine,
                                         asIScriptModule *module,
                                         const std::string &name,
                                         int repeat_count) {
    const std::string decl = "uint benchmark_" + name + "(int)";
    asIScriptFunction *function = module->GetFunctionByDecl(decl.c_str());
    if (function == nullptr) {
        throw std::runtime_error("script function not found: " + decl);
    }

    asIScriptContext *ctx = engine->CreateContext();
    if (ctx == nullptr) {
        throw std::runtime_error("failed to create AngelScript context");
    }

    int r = ctx->Prepare(function);
    if (r >= 0) {
        ctx->SetArgDWord(0, static_cast<asDWORD>(repeat_count));
        r = ctx->Execute();
    }

    std::uint64_t result = 0;
    if (r == asEXECUTION_FINISHED) {
        result = static_cast<std::uint64_t>(ctx->GetReturnDWord());
    } else {
        std::string detail = "AngelScript execution failed: " + name + " code=" + std::to_string(r);
        if (r == asEXECUTION_EXCEPTION) {
            detail += " exception=";
            detail += ctx->GetExceptionString();
        }
        ctx->Release();
        throw std::runtime_error(detail);
    }

    ctx->Release();
    return result;
}

} // namespace

#if SANDBOX_USE_AOT
extern unsigned int AOTLinkerTableSize;
extern AOTLinkerEntry AOTLinkerTable[];
#endif

int main(int argc, char **argv) {
#if SANDBOX_GENERATE_AOT
    if (argc != 3) {
        std::cerr << "usage: " << argv[0] << " <script.as> <generated.cpp>\n";
        return 2;
    }
#else
    if (argc != 3) {
        std::cerr << "usage: " << argv[0] << " <script.as> <results.json>\n";
        return 2;
    }
#endif

    const std::filesystem::path script_path = argv[1];

    asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
    if (engine == nullptr) {
        std::cerr << "failed to create AngelScript engine\n";
        return 3;
    }

    MessageCallback callback;
    engine->SetMessageCallback(asMETHOD(MessageCallback, Callback), &callback, asCALL_THISCALL);
    engine->SetEngineProperty(asEP_BUILD_WITHOUT_LINE_CUES, 1);
    engine->SetEngineProperty(asEP_OPTIMIZE_BYTECODE, 1);

    AOTCompiler *jit = nullptr;
    SimpleAOTLinker *linker = nullptr;
#if SANDBOX_GENERATE_AOT
    engine->SetEngineProperty(asEP_INCLUDE_JIT_INSTRUCTIONS, 1);
    linker = new SimpleAOTLinker(nullptr, 0);
    jit = new AOTCompiler(linker);
    engine->SetJITCompiler(jit);
#elif SANDBOX_USE_AOT
    engine->SetEngineProperty(asEP_INCLUDE_JIT_INSTRUCTIONS, 1);
    linker = new SimpleAOTLinker(AOTLinkerTable, AOTLinkerTableSize);
    jit = new AOTCompiler(linker);
    engine->SetJITCompiler(jit);
#endif

    try {
        asIScriptModule *module = build_module(engine, script_path);

#if SANDBOX_GENERATE_AOT
        for (const auto &item : slc::benchmark_items()) {
            slc::consume(execute_benchmark_function(engine, module, item.name, 1));
        }
        CCodeStream code_stream(argv[2]);
        jit->SaveCode(&code_stream);
        std::cout << "generated AOT source: " << argv[2] << "\n";
        std::cout << "sink: " << static_cast<unsigned long long>(slc::g_sink) << "\n";
#else
        std::vector<slc::BenchmarkSample> samples;
        for (const auto &item : slc::benchmark_items()) {
            slc::BenchmarkSample sample = slc::run_benchmark_sample(item, [&](int repeat_count) {
                return execute_benchmark_function(engine, module, item.name, repeat_count);
            });
            std::cout << "["
#if SANDBOX_USE_AOT
                      << "AngelScript AOT"
#else
                      << "AngelScript"
#endif
                      << "] " << sample.name
                      << " best=" << slc::format_double(sample.best_ms)
                      << "ms median=" << slc::format_double(sample.median_ms)
                      << "ms repeat=" << sample.repeat_count << "\n";
            samples.push_back(sample);
        }

        const char *engine_name =
#if SANDBOX_USE_AOT
            "AngelScript AOT";
#else
            "AngelScript";
#endif
        slc::write_results_json(argv[2], engine_name, samples);
        std::cout << "results: " << argv[2] << "\n";
        std::cout << "sink: " << static_cast<unsigned long long>(slc::g_sink) << "\n";
#endif
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\n";
        engine->ShutDownAndRelease();
        delete jit;
        delete linker;
        return 1;
    }

    engine->ShutDownAndRelease();
    delete jit;
    delete linker;
    return 0;
}
