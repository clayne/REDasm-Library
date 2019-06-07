#include "loader_impl.h"
#include <redasm/plugins/loader/analyzer.h>
#include <redasm/macros.h>

namespace REDasm {

LoaderImpl::LoaderImpl(): PluginImpl(), m_buffer(nullptr) { }
AbstractBuffer *LoaderImpl::buffer() const { return m_buffer; }
BufferView LoaderImpl::viewOffset(offset_t offset) const { return m_buffer->view(offset); }
const BufferView &LoaderImpl::view() const { return m_view; }

BufferView LoaderImpl::view(address_t address) const
{
    offset_location offset = this->offset(address);

    if(!offset.valid)
        return BufferView();

    return this->viewOffset(offset);
}

BufferView LoaderImpl::viewSegment(const Segment *segment) const
{
    if(!segment || segment->empty() || segment->is(SegmentType::Bss))
        return BufferView();

    return m_buffer->view(segment->offset, segment->rawSize());
}

ListingDocument &LoaderImpl::createDocument() { m_document = ListingDocument(); return m_document; }
const ListingDocument &LoaderImpl::document() const { return m_document;  }
ListingDocument &LoaderImpl::document() { return m_document;  }
SignatureIdentifiers &LoaderImpl::signatures() { return m_signatures; }
void LoaderImpl::signature(const std::string &sig) { m_signatures.insert(sig); }

offset_location LoaderImpl::offset(address_t address) const
{
    for(const Segment& segment : m_document->segments())
    {
        if(!segment.contains(address))
            continue;

        offset_t offset = (address - segment.address) + segment.offset;
        return REDasm::make_location(offset, segment.containsOffset(offset));
    }

    return REDasm::invalid_location<offset_t>();
}

address_location LoaderImpl::address(offset_t offset) const
{
    for(const Segment& segment : m_document->segments())
    {
        if(!segment.containsOffset(offset))
            continue;

        address_t address = (offset - segment.offset) + segment.address;
        return REDasm::make_location(address, segment.contains(address));
    }

    return REDasm::invalid_location<address_t>();
}

void LoaderImpl::build(const std::string &assembler, offset_t offset, address_t baseaddress, address_t entrypoint) { throw std::runtime_error("Invalid call to LoaderPlugin::build()"); }

Analyzer *LoaderImpl::analyzer(Disassembler *disassembler)
{
    PIMPL_Q(Loader);

    m_analyzer = std::unique_ptr<Analyzer>(q->createAnalyzer(disassembler));
    return m_analyzer.get();
}

void LoaderImpl::load(const LoadRequest *loader)
{
    m_buffer = loader->buffer();
    m_view = loader->view();
    this->createDocument();
}

std::string LoaderImpl::assembler() const { return std::string(); }

} // namespace REDasm
