#pragma once

#include <type_traits>
#include "../disassembler/listing/listingdocument.h"
#include "../disassembler/disassembler.h"
#include "../context.h"
#include "demangler.h"
#include "utils.h"

#define REDASM_SYMBOLIZE(T, disassembler, address) REDasm::symbolize<T>(disassembler, address, #T)

namespace REDasm {

struct StructVisitor {
    template<typename T, typename = void> struct VisitorImpl {
        static bool visit(Disassembler* disassembler, address_t address, const std::string& basename, const char* name) { // Basic Types
            u64 value = 0;

            if(disassembler->dereference(address, &value) && disassembler->document()->segment(value))
                disassembler->document()->lock(address, basename + "." + std::string(name), SymbolType::Data | SymbolType::Pointer);
            else
                disassembler->document()->lock(address, basename + "." + std::string(name), SymbolType::Data);

            return true;
        }
    };

    template<typename T> struct VisitorImpl<T, typename std::enable_if<std::is_array<T>::value && std::is_convertible<T, std::string>::value>::type> { // Arrays
        static bool visit(Disassembler* disassembler, address_t address, const std::string& basename, const char* name) {
            disassembler->document()->lock(address, basename + "." + std::string(name), SymbolType::String);
            return true;
        }
    };

    template<typename T> struct VisitorImpl<T, typename std::enable_if<std::is_class<T>::value>::type> { // Classes
        static bool visit(Disassembler* disassembler, address_t address, const std::string& basename, const char* name) {
            if(!StructVisitor::symbolize<T>(disassembler, address, basename + "." + name))
                return false;

            disassembler->document()->lock(address, basename + "." + std::string(name), SymbolType::Data);
            return true;
        }
    };

    StructVisitor(Disassembler* disassembler, address_t address, const std::string& basename): address(address), disassembler(disassembler), basename(basename) {
        failed = false;
    }

    template<typename T> void operator()(const char* name, const visit_struct::type_c<T>& tag) {
        if(failed)
            return;

        if(!VisitorImpl<T>::visit(disassembler, address, basename, name))
            failed = true;

        address += sizeof(T);
    }

    template<typename T> static bool symbolize(Disassembler* disassembler, address_t address, const std::string& name) {
        if(!std::is_trivial<T>::value) {
            r_ctx->log("Type " + Utils::quoted(Demangler::typeName<T>()) + "is not Trivial");
            return false;
        }

        auto document = disassembler->document();
        std::string symbolname = name + "_" + Utils::hex(address); // Generate an unique name
        StructVisitor visitor(disassembler, address, symbolname);
        visit_struct::visit_types<T>(visitor);
        document->type(address, symbolname);
        return true;
    }

    address_t address;
    Disassembler* disassembler;
    const std::string& basename;
    bool failed;
};

template<typename T> static inline void symbolize(Disassembler* disassembler, address_t address, const std::string& name) { StructVisitor::symbolize<T>(disassembler, address, name); }

} // namespace REDasm
