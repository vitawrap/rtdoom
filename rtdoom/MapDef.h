#pragma once

#include "MapStore.h"
#include "MapStructs.h"

#include <memory>
#include <deque>
#include <optional>

namespace rtdoom
{
class MapDef
{
protected:
    MapStore m_store;

    static bool IsInFrontOf(const Point& pov, const MapStore::Node& node) noexcept;
    static bool IsInFrontOf(const Point& pov, const Vertex& sv, const Vertex& ev) noexcept;

    void LookupVertex(unsigned short vertexNo, float& x, float& y);
    void ProcessSegment(float sx, float sy, float ex, float ey, unsigned short lineDefNo, signed short direction, signed short offset);
    void ProcessNode(const Point& pov, const MapStore::Node& node, std::deque<SubSector*>& subSectors) const;
    void ProcessChildRef(unsigned short childRef, const Point& pov, std::deque<SubSector*>& subSectors) const;
    void ProcessSubsector(SubSector* subSector, std::deque<SubSector*>& subSectors) const;
    void OpenDoors();
    void Initialize();
    void BuildWireframe();
    void BuildSegments();
    void BuildSectors();
    void BuildSubSectors();
    void BuildThings();

public:
    static bool                             IsInFrontOf(const Point& pov, const Line& line) noexcept;
    static float                            SignedDist(const Point& pov, const Line& line) noexcept;
    static Point                            ProjectPointOnLine(const Point& pov, const Line& line) noexcept;
    static bool                             IsPointOnLine(const Point& pov, const Line& line, float bias = 0.f) noexcept;
    bool                                    HasGL() const;
    std::vector<Line>                       m_wireframe;
    std::vector<Sector>                     m_sectors;
    std::vector<std::vector<Thing>>         m_things;
    std::vector<Segment*>                   m_segments;
    std::vector<SubSector*>                 m_subSectors;

    Thing                                   GetStartingPosition() const;
    std::optional<Sector>                   GetSector(const Point& pov) const;
    std::deque<Segment*>                    GetSegmentsToDraw(const Point& pov) const;
    std::deque<SubSector*>                  GetSubSectorsToDraw(const Point& pov) const;

    MapDef(const std::string& mapFolder);
    MapDef(const MapStore& mapStore);
    ~MapDef();
};
} // namespace rtdoom
