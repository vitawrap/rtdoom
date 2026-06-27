
#include "rtdoom.h"
#include "GameState.h"
#include "Projection.h"
//#include <stdio.h>

using std::string;

#define PLAYER_RADIUS 8.f
#define PLAYER_INERTIA 2.5f
#define PLAYER_SWIM_INERTIA 1.0f

static float clampf(float x, float min, float max) {
    return x > min? (x < max? x : max) : min;
}

static float minf(float x, float y) {
    return x < y? x : y;
}

static float signf(float x) {
    return x > 0.f? 1.f : -1.f;
}

namespace rtdoom
{
/**
 * it turns out GetSector naively returns the nearest sector even when we walked out
 * because in the BSP we're still technically closest to that sector, according to clipping planes.
 * we need to actually test segments to check if we're actually CONTAINED in the active subsector.
 */
void GameState::Player::Step(MapDef* mapDef, int m, int r, float step, float time)
{
    a += step * 16 * -0.15f * r;
    a = Projection::NormalizeAngle(a);

    // init vars necessary for both animation and navigation up there
    float to_x = 0.f, to_y = 0.f;
    float sectZ = 0.f;
    bool isLiquid = false;
    
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
        sectZ = sect.floorHeight - (sect.isLiquid? 30.f : 0.f);
        isLiquid = sect.isLiquid;

        // compute walking math here once we know if we're in a liquid sector
        const float inertia = isLiquid? PLAYER_SWIM_INERTIA : PLAYER_INERTIA;
        if (m != 0.f)
            m_walkInertia = clampf(m_walkInertia + (m * step * inertia), -1.f, 1.f);
        else {
            const auto wiSign = signf(m_walkInertia);
            m_walkInertia -= (step * inertia * signf(m_walkInertia));
            if (signf(m_walkInertia) != wiSign)
                m_walkInertia = 0.f;
        }

        float to_x = x + (step * 32 * 10.0f * m_walkInertia * cosf(a));
        float to_y = y + (step * 32 * 10.0f * m_walkInertia * sinf(a));
        
        // resume sector checks
        const auto& predict = Point(to_x, to_y);
        const auto& _toSect = mapDef->GetSector(Point(to_x, to_y));
        Point response = predict;

        // no obvious blocking sector, test segments from the sector we were in at the time of walking
        for(auto& segment : segments) {
            if (!(segment->isSolid || segment->isBlocking) /*|| segment->frontSide.sideless || segment->backSide.sideless */)
                continue;

            float dist = mapDef->SignedDist(response, *segment);
            if (dist < PLAYER_RADIUS &&
                // + make sure we didn't just collide with the infinitely extending wall plane
                segment->DistIntersection(response, PLAYER_RADIUS))
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
                goto movement_stopped; /* TODO: This might still cause the player to enter inaccessible sectors (too high up or too low down) */
            }
        }

        // nothing is stopping our movement
        m_targetZ = sectZ + 40.f;

        x = response.x;
        y = response.y;

    movement_stopped:;
        //printf("Sector %d, Floor: %f, Ceiling: %f\n", sect.sectorId, sect.floorHeight, sect.ceilingHeight);
    } else {
        //puts("No sector");
    }

    // animate
    if (m_targetZ > z) {
        m_velocityZ = 0.f;
        float allowedSectZ = z < (sectZ + 20.f)? (sectZ + 20.f) : z = minf(z + (step * 128.f), m_targetZ);
        z = allowedSectZ;
    } else {
        m_velocityZ = m_targetZ < z? m_velocityZ + (step * 7.5f) : 0.f;
        float allowedSectZ = (z - m_velocityZ > m_targetZ)? z - m_velocityZ : m_targetZ;
        z = allowedSectZ;
    }

    if (m_walkInertia != 0.f || isLiquid) {
        z += sinf(time * 6.f) * (isLiquid? 1.f : m_walkInertia);
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

    m_player.Step(m_mapDef.get(), m, r, step, m_step);
}

GameState::~GameState() {}
} // namespace rtdoom
