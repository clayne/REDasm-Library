#include "disassembler.h"
#include "../plugins/assembler/algorithm/algorithm.h"
#include <algorithm>
#include <memory>

namespace REDasm {

Disassembler::Disassembler(AssemblerPlugin *assembler, FormatPlugin *format): DisassemblerBase(format)
{
    if(!format->isBinary())
        assembler->setEndianness(format->endianness());

    m_assembler = std::unique_ptr<AssemblerPlugin>(assembler);
    m_algorithm = REDasm::safe_ptr<AssemblerAlgorithm>(m_assembler->createAlgorithm(this));

    m_analyzejob.work(std::bind(&Disassembler::analyzeStep, this), true); // Deferred
    m_jobs.stateChanged += [&](Job*) { busyChanged(); };
}

void Disassembler::disassembleStep(Job* job)
{
    if(m_algorithm->hasNext())
        m_algorithm->next();
    else
        job->stop();

    if(!m_jobs.active())
        m_analyzejob.start();
}

void Disassembler::analyzeStep()
{
    m_algorithm->analyze();
    m_analyzejob.stop();
}

void Disassembler::disassemble()
{
    if(!m_document->segmentsCount())
    {
        REDasm::log("ERROR: Segment list is empty");
        return;
    }

    SymbolTable* symboltable = m_document->symbols();

    // Preload format functions for analysis
    symboltable->iterate(SymbolTypes::FunctionMask, [=](SymbolPtr symbol) -> bool {
        m_algorithm->enqueue(symbol->address);
        return true;
    });

    SymbolPtr entrypoint = m_document->documentEntry();

    if(entrypoint)
        m_algorithm->enqueue(entrypoint->address); // Push entry point

    REDasm::log("Disassembling with " + std::to_string(m_jobs.concurrency()) + " threads");
    this->disassembleJob();
}

Printer *Disassembler::createPrinter() { return m_assembler->createPrinter(this); }
AssemblerPlugin *Disassembler::assembler() { return m_assembler.get(); }

void Disassembler::disassemble(address_t address)
{
    m_algorithm->enqueue(address);

    if(m_jobs.active())
        return;

    this->disassembleJob();
}

void Disassembler::stop() { m_jobs.stop(); }
void Disassembler::pause() { m_jobs.pause(); }
void Disassembler::resume() { m_jobs.resume(); }
size_t Disassembler::state() const { return m_jobs.state(); }
bool Disassembler::busy() const { return m_jobs.active(); }
void Disassembler::disassembleJob() { m_jobs.work(std::bind(&Disassembler::disassembleStep, this, std::placeholders::_1)); }

InstructionPtr Disassembler::disassembleInstruction(address_t address)
{
    InstructionPtr instruction = m_document->instruction(address);

    if(instruction)
        return instruction;

    instruction = std::make_shared<Instruction>();
    m_algorithm->disassembleInstruction(address, instruction);
    return instruction;
}

}
