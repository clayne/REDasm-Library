#include "surfacerenderer.h"
#include "../document/document.h"
#include "../context.h"

#define BLANK_CELL         { Theme_Default, Theme_Default, ' ' }

SurfaceRenderer::SurfaceRenderer(Context* ctx, rd_flag flags): Object(ctx), m_flags(flags) { }
SafeDocument& SurfaceRenderer::document() const { return this->context()->document(); }
const SurfaceRenderer::Rows& SurfaceRenderer::rows() const { return m_rows; }
bool SurfaceRenderer::hasFlag(rd_flag flag) const { return m_flags & flag; }

int SurfaceRenderer::row(int row, const RDSurfaceCell** cells) const
{
    Lock lock(m_mutex);
    if(row >= this->lastRow()) return 0;
    int c = m_ncols != -1 ? std::min<int>(m_rows[row].cells.size(), m_ncols) : m_rows[row].cells.size();

    if(cells)
    {
        m_reqrows.resize(c);
        std::copy_n(m_rows[row].cells.begin(), c, m_reqrows.data());
        *cells = m_reqrows.data();
    }

    return c;
}

int SurfaceRenderer::indexOf(rd_address address) const
{
    for(size_t i = 0; i < m_rows.size(); i++)
    {
        if(m_rows[i].address == address)
            return i;
    }

    return -1;
}

int SurfaceRenderer::lastIndexOf(rd_address address) const
{
    for(size_t i = m_rows.size(); i-- > 0; )
    {
        if(m_rows[i].address == address)
            return i;
    }

    return -1;
}

void SurfaceRenderer::setLastColumn(int col) { m_lastcolumn = std::max<int>(this->lastColumn(), col); }
int SurfaceRenderer::firstColumn() const { return m_firstcol; }
int SurfaceRenderer::lastColumn() const { return m_ncols != -1 ? m_ncols : m_lastcolumn; }
int SurfaceRenderer::lastRow() const { return std::min<int>(m_nrows, m_rows.size()); }

void SurfaceRenderer::updateSegment(const RDSegment* segment, size_t segmentidx, rd_address startaddress)
{
    const auto* addressspace = this->document()->addressSpace();
    auto* blocks = addressspace->getBlocks(segment->address);
    if(!blocks) return;

    auto it = blocks->find(startaddress);
    if(it == blocks->end()) return;

    if((it->address == segment->address) && !this->hasFlag(RendererFlags_NoSegmentLine))
    {
        if(segmentidx) this->createEmptyLine(it->address);
        this->createLine(it->address).renderSegment();
    }

    for( ; this->needsRows() && (it != blocks->end()); it++)
    {
        auto& laststate = m_laststate[segment->address];
        rd_flag flags = addressspace->getFlags(it->address);

        switch(it->type)
        {
            case BlockType_Code: {
                if(flags & AddressFlags_Function) {
                    if((it->address != segment->address)) this->createEmptyLine(it->address);
                    if(!this->hasFlag(RendererFlags_NoFunctionLine)) this->createLine(it->address).renderFunction();
                }
                else if(flags & AddressFlags_Location) {
                    this->createEmptyLine(it->address);
                    this->createLine(it->address).renderLocation();
                }

                this->createLine(it->address).renderInstruction();
                break;
            }

            case BlockType_String:
            case BlockType_Data: {
                if(flags & AddressFlags_Type) {
                    this->createEmptyLine(it->address);
                    this->createLine(it->address).renderType();
                }

                if(flags & AddressFlags_TypeField) this->createLine(it->address).renderTypeField();
                else if(it->type == BlockType_String) this->createLine(it->address).renderString();
                else this->createLine(it->address).renderData();
                break;
            }

            case BlockType_Unknown: {
                if(laststate.flags && (flags != laststate.flags)) this->createEmptyLine(it->address);
                this->createLine(it->address).renderUnknown(BlockContainer::size(std::addressof(*it)));
                if(laststate.flags && (flags != laststate.flags)) this->createEmptyLine(it->address);
                break;
            }

            default: this->createLine(it->address).renderLine("Block #" + std::to_string(it->type)); break;
        }

        laststate = {it->type, flags};
    }
}

void SurfaceRenderer::updateSegments()
{
    const auto* addressspace = this->document()->addressSpace();

    RDSegment startsegment;
    if(!addressspace->addressToSegment(m_range.first, &startsegment)) return;

    RDSegment segment;
    size_t segmentidx = addressspace->indexOfSegment(&startsegment);

    m_lastempty = false;

    for( ; (this->lastRow() < m_nrows) && (segmentidx < addressspace->size()); segmentidx++)
    {
        if(!addressspace->indexToSegment(segmentidx, &segment)) break;

        m_laststate[segment.address] = { };
        this->updateSegment(&segment, segmentidx, (segment == startsegment) ? m_range.first : segment.address);
    }

    // Fill remaining cells with blank characters
    for(auto& row : m_rows)
    {
        for(int i = row.cells.size(); i < m_lastcolumn; i++)
            row.cells.push_back(BLANK_CELL);
    }
}

SurfaceRow& SurfaceRenderer::insertRow(rd_address address)
{
    auto& row = m_rows.emplace_back();
    row.address = address;
    return row;
}

bool SurfaceRenderer::needsRows() const { return this->lastRow() < m_nrows; }

void SurfaceRenderer::update(rd_address currentaddress)
{
    if((m_range.first == RD_NVAL) || !m_nrows || !m_ncols) return;

    m_rows.clear();
    m_lastcolumn = 0;

    this->updateSegments();
    this->updateCompleted(currentaddress);
    this->notify<RDEventArgs>(Event_SurfaceUpdated, this);
}

RDSurfaceCell& SurfaceRenderer::cell(int row, int col) { return m_rows[row].cells.at(col); }
