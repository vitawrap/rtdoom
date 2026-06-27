#pragma once

#include "MapDef.h"

namespace rtdoom
{
class GameState
{
public:
    struct Player : Thing {
        Player(float x, float y, float z, Angle a) :
            Thing(x, y, z, a)
        {}

        Player(Thing const& thing) :
            Thing(thing)
        {}

        void Step(MapDef* mapDef, int m, int r, float step, float time);

        float m_targetZ;
        float m_velocityZ;
        float m_walkInertia;
    };

    std::unique_ptr<MapDef> m_mapDef;
    float                   m_step;
    Player                  m_player;

    void Move(int m, int r, float step);
    void NewGame(const MapStore& mapStore);

    float GetSecondsElapsed() const { return m_step; }

    GameState();
    ~GameState();
};
} // namespace rtdoom
