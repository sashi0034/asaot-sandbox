#include <fstream>
#include <iostream>
#include <string>

#include <angelscript.h>
#include <scriptmath/scriptmath.h>
#include <scriptstdstring/scriptstdstring.h>

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

        std::cout << msg->section << " (" << msg->row << ", " << msg->col << ") : "
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

void Print(const std::string &text) {
    std::cout << text;
}

std::string ReadAll(const char *path) {
    std::ifstream file(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

}  // namespace

#if !SANDBOX_GENERATE_AOT
extern unsigned int AOTLinkerTableSize;
extern AOTLinkerEntry AOTLinkerTable[];
#endif

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "usage: " << argv[0] << " <script.as> <generated.cpp>\n";
        return 2;
    }

    const char *scriptPath = argv[1];
    const char *generatedPath = argv[2];
    const std::string script = ReadAll(scriptPath);
    if (script.empty()) {
        std::cerr << "failed to read script: " << scriptPath << '\n';
        return 3;
    }

    asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
    MessageCallback callback;
    engine->SetMessageCallback(asMETHOD(MessageCallback, Callback), &callback, asCALL_THISCALL);
    engine->SetEngineProperty(asEP_BUILD_WITHOUT_LINE_CUES, 1);
    engine->SetEngineProperty(asEP_INCLUDE_JIT_INSTRUCTIONS, 1);
    engine->SetEngineProperty(asEP_OPTIMIZE_BYTECODE, 1);

    RegisterStdString(engine);
    engine->SetDefaultNamespace("math");
    RegisterScriptMath(engine);
    engine->SetDefaultNamespace("");
    int r = engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(Print), asCALL_CDECL);
    if (r < 0) {
        std::cerr << "RegisterGlobalFunction failed: " << r << '\n';
        return 4;
    }

#if SANDBOX_GENERATE_AOT
    SimpleAOTLinker linker(nullptr, 0);
#else
    SimpleAOTLinker linker(AOTLinkerTable, AOTLinkerTableSize);
#endif
    asIJITCompiler *jit = new AOTCompiler(&linker);
    engine->SetJITCompiler(jit);

    asIScriptModule *module = engine->GetModule("sandbox", asGM_ALWAYS_CREATE);
    module->AddScriptSection(scriptPath, script.c_str(), static_cast<unsigned int>(script.size()));
    r = module->Build();
    if (r < 0) {
        std::cerr << "Build failed: " << r << '\n';
        delete jit;
        engine->ShutDownAndRelease();
        return 5;
    }

    asIScriptFunction *entry = module->GetFunctionByDecl("int main()");
    if (entry == nullptr) {
        std::cerr << "Script entry 'int main()' not found\n";
        delete jit;
        engine->ShutDownAndRelease();
        return 6;
    }

    asIScriptContext *ctx = engine->CreateContext();
    ctx->Prepare(entry);
    r = ctx->Execute();
    if (r != asEXECUTION_FINISHED) {
        std::cerr << "Execution failed: " << r << '\n';
        if (r == asEXECUTION_EXCEPTION) {
            std::cerr << "Exception: " << ctx->GetExceptionString() << '\n';
        }
        ctx->Release();
        delete jit;
        engine->ShutDownAndRelease();
        return 7;
    }

    std::cout << "script returned: " << ctx->GetReturnDWord() << '\n';

#if SANDBOX_GENERATE_AOT
    CCodeStream codeStream(generatedPath);
    static_cast<AOTCompiler *>(jit)->SaveCode(&codeStream);
    std::cout << "generated AOT source: " << generatedPath << '\n';
#else
    std::cout << "linked AOT table size: " << AOTLinkerTableSize << '\n';
#endif

    ctx->Release();
    engine->ShutDownAndRelease();
    delete jit;
    return 0;
}
