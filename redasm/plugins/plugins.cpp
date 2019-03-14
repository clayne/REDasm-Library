#define WRAP_TO_STRING(...)         #__VA_ARGS__
#define LOADER_PLUGIN(loadername)   WRAP_TO_STRING(../loaders/loadername/loadername.h)
#define ASSEMBLER_PLUGIN(assembler) WRAP_TO_STRING(../assemblers/assembler/assembler.h)

#include <algorithm>
#include "plugins.h"

/* *** Loaders *** */
#include LOADER_PLUGIN(binary)
#include LOADER_PLUGIN(chip8)
#include LOADER_PLUGIN(pe)
#include LOADER_PLUGIN(elf)
#include LOADER_PLUGIN(psxexe)
#include LOADER_PLUGIN(dex)
#include LOADER_PLUGIN(xbe)
#include LOADER_PLUGIN(gba)
#include LOADER_PLUGIN(n64)

/* *** Assemblers *** */
#include ASSEMBLER_PLUGIN(x86)
#include ASSEMBLER_PLUGIN(mips)
#include ASSEMBLER_PLUGIN(dalvik)
#include ASSEMBLER_PLUGIN(cil)
#include ASSEMBLER_PLUGIN(metaarm)
//#include ASSEMBLER_PLUGIN(arm64)
#include ASSEMBLER_PLUGIN(chip8)

#define EXT_LIST(...) { __VA_ARGS__ }
#define REGISTER_LOADER_PLUGIN_EXT(ext, desc, id) registerLoaderByExt(#ext, desc, &id##_loaderPlugin); Plugins::loaderCount++

#define REGISTER_LOADER_PLUGIN_EXT_LIST(extlist, desc, id) for(const auto& ext : extlist) \
                                                             registerLoaderByExt(ext, desc, &id##_loaderPlugin); \
                                                           Plugins::loadersCount++;

#define REGISTER_LOADER_PLUGIN(id)                REDasm::Plugins::loaders.emplace_front(&id##_loaderPlugin); Plugins::loadersCount++
#define REGISTER_ASSEMBLER_PLUGIN(id)             REDasm::Plugins::assemblers[#id] = &id##_assemblerPlugin

namespace REDasm {

size_t Plugins::loadersCount = 0;
EntryListT<LoaderPlugin_Entry>::Type Plugins::loaders;
EntryMapT<LoaderEntryListByExt>::Type Plugins::loadersByExt;
EntryMapT<AssemblerPlugin_Entry>::Type Plugins::assemblers;

static void registerLoaderByExt(std::string ext, const std::string& description, const LoaderPlugin_Entry& cb)
{
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    auto it = Plugins::loadersByExt.find(ext);

    if(it != Plugins::loadersByExt.end())
        it->second.emplace_front(std::make_pair(cb, description));
    else
        Plugins::loadersByExt[ext] = { std::make_pair(cb, description) };
}

void init(const std::string& temppath, const std::string& searchpath)
{
    Runtime::rntTempPath = temppath;
    Runtime::rntSearchPath = searchpath;

    REGISTER_LOADER_PLUGIN(binary); // Always last choice
    REGISTER_LOADER_PLUGIN(n64rom);
    REGISTER_LOADER_PLUGIN(gbarom);
    REGISTER_LOADER_PLUGIN(xbe);
    REGISTER_LOADER_PLUGIN(dex);
    REGISTER_LOADER_PLUGIN(psxexe);
    REGISTER_LOADER_PLUGIN(elf64be);
    REGISTER_LOADER_PLUGIN(elf64le);
    REGISTER_LOADER_PLUGIN(elf32be);
    REGISTER_LOADER_PLUGIN(elf32le);
    REGISTER_LOADER_PLUGIN(pe64);
    REGISTER_LOADER_PLUGIN(pe32);

    REGISTER_LOADER_PLUGIN_EXT_LIST(EXT_LIST("chip8", "ch8", "rom"), "CHIP-8 Rom", chip8);

    // Assemblers
    REGISTER_ASSEMBLER_PLUGIN(x86_16);
    REGISTER_ASSEMBLER_PLUGIN(x86_32);
    REGISTER_ASSEMBLER_PLUGIN(x86_64);
    REGISTER_ASSEMBLER_PLUGIN(mips32le);
    REGISTER_ASSEMBLER_PLUGIN(mips64le);
    REGISTER_ASSEMBLER_PLUGIN(mips32r6le);
    REGISTER_ASSEMBLER_PLUGIN(mips2le);
    REGISTER_ASSEMBLER_PLUGIN(mips3le);
    REGISTER_ASSEMBLER_PLUGIN(mipsmicrole);
    REGISTER_ASSEMBLER_PLUGIN(mips32be);
    REGISTER_ASSEMBLER_PLUGIN(mips64be);
    REGISTER_ASSEMBLER_PLUGIN(mips32r6be);
    REGISTER_ASSEMBLER_PLUGIN(mips2be);
    REGISTER_ASSEMBLER_PLUGIN(mips3be);
    REGISTER_ASSEMBLER_PLUGIN(mipsmicrobe);
    REGISTER_ASSEMBLER_PLUGIN(arm);
    REGISTER_ASSEMBLER_PLUGIN(armthumb);
    REGISTER_ASSEMBLER_PLUGIN(metaarm);
    //REGISTER_ASSEMBLER_PLUGIN(arm64);
    REGISTER_ASSEMBLER_PLUGIN(dalvik);
    REGISTER_ASSEMBLER_PLUGIN(cil);
    REGISTER_ASSEMBLER_PLUGIN(chip8);
}

LoaderPlugin *getLoader(AbstractBuffer *buffer)
{
    for(const LoaderPlugin_Entry& loaderentry : Plugins::loaders)
    {
        LoaderPlugin* lp = loaderentry(buffer);

        if(lp)
            return lp;
    }

    return nullptr;
}

AssemblerPlugin *getAssembler(const std::string& id)
{
    auto it = REDasm::findPluginEntry<AssemblerPlugin_Entry>(id, Plugins::assemblers);

    if(it != Plugins::assemblers.end())
        return it->second();

    return nullptr;
}

void setLoggerCallback(const Runtime::LogCallback& logcb) { Runtime::rntLogCallback = logcb; }
void setStatusCallback(const Runtime::LogCallback& logcb) { Runtime::rntStatusCallback = logcb; }
void setProgressCallback(const Runtime::ProgressCallback& pcb) { Runtime::rntProgressCallback = pcb; }

bool getLoaderByExt(std::string ext, LoaderEntryListByExt **entries)
{
    auto it = Plugins::loadersByExt.find(ext);

    if(it == Plugins::loadersByExt.end())
        return false;

    *entries = &it->second;
    return true;
}

}
