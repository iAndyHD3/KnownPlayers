#pragma once

#include "Geode/binding/PlayerObject.hpp"
#include <optional>
#include <string>
#include <Geode/loader/Event.hpp>



namespace known_players
{
struct PlayerData
{
    std::string name;
    int iconID = 0;
    int color1 = 0;
    int color2 = 0;
    std::optional<bool> glow;
    std::optional<int> glowID;

    std::optional<int> shipID;
    std::optional<int> ballID;
    std::optional<int> ufoID;
    std::optional<int> waveID;
    std::optional<int> robotID;
    std::optional<int> spiderID;
    std::optional<int> swingID;

};

namespace events
{
struct NextIcon : public geode::Event<NextIcon, bool(PlayerData*)> {
    using Event::Event;
};

struct NextIconModifyPlayerObject : public geode::Event<NextIconModifyPlayerObject, bool(PlayerObject*)> {
    using Event::Event;
    PlayerObject* m_player;
    bool done = false;
    NextIconModifyPlayerObject(PlayerObject* player) : m_player(player) {}
};

struct CurrentIconModifyPlayerObject : public geode::Event<CurrentIconModifyPlayerObject, bool(PlayerObject*)> {
    using Event::Event;
    PlayerObject* m_player;
    bool done = false;
    CurrentIconModifyPlayerObject(PlayerObject* player) : m_player(player) {}
};

struct CurrentIcon : public geode::Event<CurrentIcon, bool(PlayerData*)> {
    using Event::Event;
};


}

}