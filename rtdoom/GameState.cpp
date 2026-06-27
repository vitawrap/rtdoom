
#include "rtdoom.h"
#include "GameState.h"
#include "Projection.h"
//#include <stdio.h>

using std::string;

#define PLAYER_RADIUS 8.f

namespace rtdoom
{
/**
 * it turns out GetSector naively returns the nearest sector even when we walked out
 * because in the BSP we're still technically closest to that sector, according to clipping planes.
 * we need to actually test segments to check if we're actually CONTAINED in the active subsector.
 */
void GameState::Player::Step(MapDef* mapDef, int m, int r, float step)
{
    a += step * 16 * -0.15f * r;
    a = Projection::NormalizeAngle(a);

    float to_x = x + (step * 32 * 10.0f * m * cosf(a));
    float to_y = y + (step * 32 * 10.0f * m * sinf(a));
    
    std::deque<std::shared_ptr<Segment>> segments;
    const auto&                subSectors = mapDef->GetSubSectorsToDraw(Point(x, y));
    //for(auto& subSector : subSectors)
    {
        for(auto& segment : subSectors[0]->segments)
            segments.push_back(segment);
    }
    if(subSectors.size())
    {
        const auto& sect = mapDef->m_sectors[subSectors[0]->sectorId];
        
        const auto& predict = Point(to_x, to_y);
        const auto& _toSect = mapDef->GetSector(Point(to_x, to_y));
        Point response = predict;

        // no obvious blocking sector, test segments from the sector we were in at the time of walking
        for(auto& segment : segments) {
            if (!(segment->isSolid || segment->isBlocking) /*|| segment->frontSide.sideless || segment->backSide.sideless */)
                continue;

            float dist = mapDef->SignedDist(predict, *segment);
            if (dist < PLAYER_RADIUS &&
                // + make sure we didn't just collide with the infinitely extending wall plane
                segment->DistIntersection(predict))
            {
                Point project = mapDef->ProjectPointOnLine(response, *segment);
                Point norm = (segment->s - segment->e).Cross().Normalized();
                response = (project + (norm * PLAYER_RADIUS));
            }
        }

        // if we're stepping on another adjacent sector, can we actually go?
        if (_toSect.has_value() && _toSect.value().sectorId != sect.sectorId) {
            const auto& toSect = _toSect.value();
            if ((toSect.floorHeight > (z + 1.f)) || ((toSect.ceilingHeight - toSect.floorHeight) < 45.f)) {
                goto movement_stopped;
            }
        }

        // nothing is stopping our movement
        z = sect.floorHeight + 40.f;

        x = response.x;
        y = response.y;

    movement_stopped:;
        //printf("Sector %d, Floor: %f, Ceiling: %f\n", sect.sectorId, sect.floorHeight, sect.ceilingHeight);
    } else {
        //puts("No sector");
    }
}

GameState::GameState() : m_player {0, 0, 0, 0}, m_mapDef {nullptr}, m_step {0} {}

void GameState::NewGame(const MapStore& mapStore)
{
    m_mapDef = std::make_unique<MapDef>(mapStore);
    m_player = m_mapDef->GetStartingPosition();
    m_step   = 0;
}


void GameState::Move(int m, int r, float step)
{
    m_step += step;

    m_player.Step(m_mapDef.get(), m, r, step);
}

GameState::~GameState() {}
} // namespace rtdoom
