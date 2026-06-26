
#include "rtdoom.h"
#include "GameState.h"
#include "Projection.h"
//#include <stdio.h>

using std::string;

namespace rtdoom
{
GameState::GameState() : m_player {0, 0, 0, 0}, m_mapDef {nullptr}, m_step {0} {}

void GameState::NewGame(const MapStore& mapStore)
{
    m_mapDef = std::make_unique<MapDef>(mapStore);
    m_player = m_mapDef->GetStartingPosition();
    m_step   = 0;
}

/**
 * it turns out GetSector naively returns the nearest sector even when we walked out
 * because in the BSP we're still technically closest to that sector, according to clipping planes.
 * we need to actually test segments to check if we're actually CONTAINED in the active subsector.
 */
void GameState::Move(int m, int r, float step)
{
    m_step += step;

    m_player.a += step * 16 * -0.15f * r;
    m_player.a = Projection::NormalizeAngle(m_player.a);

    float to_x = m_player.x + (step * 32 * 10.0f * m * cosf(m_player.a));
    float to_y = m_player.y + (step * 32 * 10.0f * m * sinf(m_player.a));
    
    std::deque<std::shared_ptr<Segment>> segments;
    const auto&                subSectors = m_mapDef->GetSubSectorsToDraw(Point(m_player.x, m_player.y));
    //for(auto& subSector : subSectors)
    {
        for(auto& segment : subSectors[0]->segments)
            segments.push_back(segment);
    }
    if(subSectors.size())
    {
        const auto& sect = m_mapDef->m_sectors[subSectors[0]->sectorId];
        
        const auto& predict = Point(to_x, to_y);
        const auto& _toSect = m_mapDef->GetSector(Point(to_x, to_y));

        // no obvious blocking sector, test segments from the sector we were in at the time of walking
        for(auto& segment : segments) {
            if (!(segment->isSolid || segment->isBlocking) /*|| segment->frontSide.sideless || segment->backSide.sideless */)
                continue;

            float dist = m_mapDef->SignedDist(predict, *segment);
            if (dist < 0.f &&    
                // + make sure we didn't just collide with the infinitely extending wall plane
                segment->DistIntersection(predict))
            {
                goto movement_stopped;
            }
        }

        // if we're stepping on another adjacent sector, can we actually go?
        if (_toSect.has_value() && _toSect.value().sectorId != sect.sectorId) {
            const auto& toSect = _toSect.value();
            if ((toSect.floorHeight > (m_player.z + 1.f)) || ((toSect.ceilingHeight - toSect.floorHeight) < 45.f)) {
                goto movement_stopped;
            }
        }

        // nothing is stopping our movement
        m_player.z = sect.floorHeight + 40.f;

        m_player.x = to_x;
        m_player.y = to_y;

    movement_stopped:;
        //printf("Sector %d, Floor: %f, Ceiling: %f\n", sect.sectorId, sect.floorHeight, sect.ceilingHeight);
    } else {
        //puts("No sector");
    }
}

GameState::~GameState() {}
} // namespace rtdoom
