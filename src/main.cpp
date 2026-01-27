#include "Geode/Enums.hpp"
#include "Geode/binding/MenuGameLayer.hpp"
#include "Geode/binding/PlayerObject.hpp"
#include "Geode/loader/Event.hpp"
#include "Geode/loader/Mod.hpp"
#include "Geode/modify/Modify.hpp"
#include <Geode/ui/Notification.hpp>
#include <Geode/loader/Dirs.hpp>
#include <Geode/modify/MenuGameLayer.hpp>


#include <glaze/core/common.hpp>
#include <glaze/glaze.hpp>
#include <random>
#include <stdexcept>
#include "Geode/utils/random.hpp"
#include "KnownPlayers.h"

using namespace cocos2d;
using namespace known_players;


//#define MEMBERBYOFFSET(type, class, offset) *reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(class) + offset)
//#define MBO MEMBERBYOFFSET


int randomInt(int min, int max) {
    return geode::utils::random::generate(min, max);
}

IconType getRandomAvailableMode(PlayerData& p)
{
    std::vector<IconType> availableModes {IconType::Cube};
    for(int i = static_cast<int>(IconType::Ship);
        const auto& opt : {p.shipID, p.ballID, p.ufoID, p.waveID, p.robotID, p.spiderID, p.swingID})
    {
        if(opt) availableModes.push_back(static_cast<IconType>(i));
        i++;
    }
    try { return availableModes.at(randomInt(0, availableModes.size() - 1)); }
    catch(std::out_of_range& e) { return IconType::Cube; }
}

std::optional<int> iconIdForGameMode(PlayerData& p, IconType icon)
{
    using enum IconType;
    switch(icon)
    {
        case Cube: return p.iconID;
        case Ship: return p.shipID;
        case Ball: return p.ballID;
        case Ufo:  return p.ufoID;
        case Wave: return p.waveID;
        case Robot: return p.robotID;
        case Spider: return p.spiderID;
        case Swing: return p.swingID;
        default: return {};
    }
}


struct JsonDocument
{
    std::optional<std::string> schema;
    std::vector<PlayerData> players;
};

template <>
struct glz::meta<JsonDocument> {
    using T = JsonDocument;
    static constexpr auto value = object(
        "$schema", &T::schema,
        "players", &T::players
    );
};


class PlayerChooser
{
public:
    static PlayerChooser* get()
    {
        static PlayerChooser instance;
        return &instance;
    }

    //Check nullptr for error!
    PlayerData* next()
    {		
        if(jsonDoc.players.empty()) loadPlayers();
        if(order.empty()) newOrder();

        if(order.empty())
        {
            geode::log::error("Order is empty! something went wrong");
            return nullptr;
        }

        int index = order.back();
        m_currentPlayer = [&]() -> PlayerData*
        {
            try { return &jsonDoc.players.at(index); }
            catch(std::out_of_range& e) { return nullptr; }
        }();

        order.pop_back();
        return m_currentPlayer;
    }

    PlayerData* current()
    {
        return this->m_currentPlayer;
    }

    std::string getJsonStr()
    {
        std::ifstream file(geode::Mod::get()->getConfigDir(true) / "players.json");
        if(file) return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        
        //this is not expected to change, load just once
        if(!loadedKnownPlayers)
        {
            file.open(geode::Mod::get()->getResourcesDir() / "known_players.json");
            if(file)
            {
                geode::log::info("Loaded known_players.json");
                loadedKnownPlayers = true;
                return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
            }
        }

        return {};
    }

    void loadPlayers()
    {
        std::string jsonStr = getJsonStr();
        if(jsonStr.empty())
        {
            return geode::log::error("Empty string! Could not load new players");
        }

        jsonDoc.players.clear();
        auto error = glz::read_json(jsonDoc, jsonStr);
        if(error != 0)
        {
            geode::log::error("Error reading json! {}", glz::format_error(error));
            return;
        }
    }

    void newOrder()
    {
        order.clear();
        order.resize(jsonDoc.players.size());
        std::iota(order.begin(), order.end(), 0);
        std::shuffle(order.begin(), order.end(), std::mt19937{std::random_device{}()});
    }

    const std::vector<PlayerData>& getPlayers()
    {
        return jsonDoc.players;
    }
private:
    JsonDocument jsonDoc;
    std::vector<int> order;

    PlayerData* m_currentPlayer = nullptr;
    bool loadedKnownPlayers = false;
};



void modifyPlayer(PlayerObject* po, known_players::PlayerData* data)
{
    IconType iconType = getRandomAvailableMode(*data);
    int vehicleID = iconIdForGameMode(*data, iconType).value_or(-1);

    if(vehicleID == -1)
    {
        iconType = IconType::Cube;
    }

    po->toggleFlyMode(false, false);
    po->toggleRollMode(false, false);
    po->toggleBirdMode(false, false);
    po->toggleBirdMode(false, false);
    po->toggleDartMode(false, false);
    po->toggleRobotMode(false, false);
    po->toggleSpiderMode(false, false);
    po->toggleSwingMode(false, false);

    po->updatePlayerFrame(data->iconID);

    if(iconType == IconType::Ship)
    {
        po->toggleFlyMode(true, true);
        po->updatePlayerShipFrame(vehicleID);
    }

#define HELPER(ICONTYPE, TOGGLEFUNC, UPDATEFUNC) \
    if(iconType == IconType::ICONTYPE) \
    {\
        po->TOGGLEFUNC(true, true);\
        po->UPDATEFUNC(vehicleID);\
    }
    HELPER(Ship, toggleFlyMode, updatePlayerShipFrame)
    HELPER(Ball, toggleRollMode, updatePlayerRollFrame)
    HELPER(Ufo, toggleBirdMode, updatePlayerBirdFrame)
    HELPER(Wave, toggleDartMode, updatePlayerDartFrame)
    HELPER(Robot, toggleRobotMode, updatePlayerRobotFrame)
    HELPER(Spider, toggleSpiderMode, updatePlayerSpiderFrame)
    HELPER(Swing, toggleSwingMode, updatePlayerSwingFrame)

    auto gm = GameManager::sharedState();
    po->setColor(gm->colorForIdx(data->color1));
    po->setSecondColor(gm->colorForIdx(data->color2));

    if(data->glowID)
    {
        po->m_hasGlow = true;
        po->enableCustomGlowColor(gm->colorForIdx(data->glowID.value()));
    }
    else if(data->glow)
    {
        po->m_hasGlow = true;
        po->enableCustomGlowColor(gm->colorForIdx(data->color2));
    }
    else
    {
        po->m_hasGlow = false;
        po->m_hasNoGlow = true;
    }
    po->updatePlayerGlow();
    po->updateGlowColor();
}

struct MGMHook : geode::Modify<MGMHook, MenuGameLayer>
{
    void onPlayerClicked(const std::string& name)
    {
        constexpr float time = 0.2f;
        geode::Notification::create(name.c_str(), geode::NotificationIcon::Success, time)->show();
    }

    $override
    void destroyPlayer()
    {
        if(!geode::Mod::get()->getSettingValue<bool>("notification"))
        {
            return MenuGameLayer::destroyPlayer();
        }

        auto current = PlayerChooser::get()->current();
        if(current && m_playerObject->getPositionX() < CCDirector::get()->getWinSize().width)
        {
            onPlayerClicked(current->name);
        }
        MenuGameLayer::destroyPlayer();
    }

    $override
    void resetPlayer()
    {
        MenuGameLayer::resetPlayer();

        if(!m_playerObject) return;

        auto player = PlayerChooser::get()->next();
        if(!player)
        {
            geode::log::error("Player is nullptr");
            return;
        }
        modifyPlayer(m_playerObject, player);
    }
};


//my mod
$execute
{
    using namespace geode;
    using namespace known_players::events;
    new EventListener<EventFilter<NextIcon>>(+[](NextIcon* ev){
        ev->m_callback(PlayerChooser::get()->next());
        return ListenerResult::Stop;
    });


    new EventListener<EventFilter<CurrentIcon>>(+[](CurrentIcon* ev){
        ev->m_callback(PlayerChooser::get()->current());
        return ListenerResult::Stop;
    });

    new EventListener<EventFilter<NextIconModifyPlayerObject>>(+[](NextIconModifyPlayerObject* ev){
        modifyPlayer(ev->m_player, PlayerChooser::get()->next());
        ev->done = true;
        return ListenerResult::Stop;
    });


    new EventListener<EventFilter<CurrentIconModifyPlayerObject>>(+[](CurrentIconModifyPlayerObject* ev){
        modifyPlayer(ev->m_player, PlayerChooser::get()->next());
        ev->done = true;
        return ListenerResult::Stop;
    });
}



//This is not relevant (disabled in the release)
//just generating useful stuff from the json and writing to clipboard
//#define SCHEMA_COPY
#ifdef SCHEMA_COPY

#include <Geode/modify/MenuLayer.hpp>
#include <set>
#include <fmt/format.h>

struct MenuLayerHook : geode::Modify<MenuLayerHook, MenuLayer>
{
    void onGarage(CCObject*)
    {
        auto schema = glz::write_json_schema<JsonDocument>();
        geode::utils::clipboard::write([&](){
            return schema ? *schema : std::string("Error generating schema") + glz::format_error(schema.error());
        }());


        std::set<std::string_view> alphabeticalSortedPlayerNames;
        for(const auto& p : PlayerChooser::get()->getPlayers())
        {
            alphabeticalSortedPlayerNames.insert(p.name);
        }

        std::string writestr = fmt::format("There are {} players added in this version:\n", alphabeticalSortedPlayerNames.size());

        for(const auto& name : alphabeticalSortedPlayerNames)
        {
            writestr += fmt::format("- {}\n", name);
        }
        geode::utils::clipboard::write(writestr);
    }
};

#endif
