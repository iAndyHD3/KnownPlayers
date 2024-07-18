#include "Geode/Enums.hpp"
#include "Geode/binding/MenuGameLayer.hpp"
#include "Geode/binding/PlayerObject.hpp"
#include "Geode/loader/Mod.hpp"
#include "Geode/modify/Modify.hpp"
#include <Geode/ui/Notification.hpp>
#include <Geode/loader/Dirs.hpp>
#include <Geode/modify/MenuGameLayer.hpp>


#include <glaze/core/common.hpp>
#include <glaze/glaze.hpp>
#include <random>
#include <stdexcept>

using namespace cocos2d;

//#define MEMBERBYOFFSET(type, class, offset) *reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(class) + offset)
//#define MBO MEMBERBYOFFSET


int randomInt(int min, int max) {
    static std::random_device device;
    static std::mt19937 generator(device());
    std::uniform_int_distribution<int> distribution(min, max);

    return distribution(generator);
}


struct JsonPlayer
{
    std::string name;
    int iconid = 0;
    int color1 = 0;
    int color2 = 0;
    std::optional<bool> glow;
    std::optional<int> glowid; 

    std::optional<int> ship;
    std::optional<int> ball;
    std::optional<int> ufo;
    std::optional<int> wave;
    std::optional<int> robot;
    std::optional<int> spider;
    std::optional<int> swing;

    IconType getRandomAvailableMode()
    {
        std::vector<IconType> availableModes {IconType::Cube};

        for(int i = static_cast<int>(IconType::Ship);
            const auto& opt : {ship, ball, ufo, wave, robot, spider, swing})
        {
            if(opt) availableModes.push_back(static_cast<IconType>(i));
            i++;
        }
        try { return availableModes.at(randomInt(0, availableModes.size() - 1)); }
        catch(std::out_of_range& e) { return IconType::Cube; }
    }

    std::optional<int> iconIdForGameMode(IconType icon)
    {
        using enum IconType;
        switch(icon)
        {
            case Cube: return iconid;
            case Ship: return ship;
            case Ball: return ball;
            case Ufo:  return ufo;
            case Wave: return wave;
            case Robot: return robot;
            case Spider: return spider;
            case Swing: return swing;
            default: return {};
        }
    }

    void log()
    {
        geode::log::info("Player: {} iconid {} color1 {} color2 {}", name, iconid, color1, color2);
    }
};

struct JsonDocument
{
    std::optional<std::string> schema;
    std::vector<JsonPlayer> players;
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
    JsonPlayer* next()
    {		
        if(jsonDoc.players.empty()) loadPlayers();
        if(order.empty()) newOrder();

        if(order.empty())
        {
            geode::log::error("Order is empty! something went wrong");
            return nullptr;
        }

        int index = order.back();
        currentPlayer = [&]() -> JsonPlayer*
        {
            try { return &jsonDoc.players.at(index); }
            catch(std::out_of_range& e) { return nullptr; }
        }();

        order.pop_back();
        return currentPlayer;
    }

    JsonPlayer* current()
    {
        return this->currentPlayer;
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
private:
    JsonDocument jsonDoc;
    std::vector<int> order;

    JsonPlayer* currentPlayer = nullptr;
    bool loadedKnownPlayers = false;
};

struct MGMHook : geode::Modify<MGMHook, MenuGameLayer>
{
    //PlayerObject* getplayer()
    //{
    //	#if defined(GEODE_IS_WINDOWS)
    //		return m_playerObject;
    //	#elif defined(GEODE_IS_ANDROID32)
    //		return MBO(PlayerObject*, this, 0x180);
    //	#elif defined(GEODE_IS_ANDROID64)
    //		return MBO(PlayerObject*, this, 0x1B0);
    //	#else
    //		#error Unsupported platform
    //	#endif
    //}

    void onPlayerClicked(const std::string& name)
    {
        constexpr float time = 0.2f;
        geode::Notification::create(name, geode::NotificationIcon::Success, time)->show();
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

        auto* po = m_playerObject;
        if(!po) return;

        auto player = PlayerChooser::get()->next();
        if(!player)
        {
            geode::log::error("Player is nullptr");
            return;
        }

        IconType iconType = player->getRandomAvailableMode();
        int vehicleID = player->iconIdForGameMode(iconType).value_or(-1);

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

        po->updatePlayerFrame(player->iconid);

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
        po->setColor(gm->colorForIdx(player->color1));
        po->setSecondColor(gm->colorForIdx(player->color2));

        if(player->glowid)
        {
            po->m_hasGlow = true;
            po->enableCustomGlowColor(gm->colorForIdx(player->glowid.value()));
        }
        else if(player->glow)
        {
            po->m_hasGlow = true;
            po->enableCustomGlowColor(gm->colorForIdx(player->color2));
        }
        else
        {
            po->m_hasGlow = false;
            po->m_hasNoGlow = true;
        }
        po->updatePlayerGlow();
        po->updateGlowColor();
    }
};




#define SCHEMA_COPY
#ifdef SCHEMA_COPY

#include <Geode/modify/MenuLayer.hpp>

struct MenuLayerHook : geode::Modify<MenuLayerHook, MenuLayer>
{
    void onNewgrounds(CCObject*)
    {
        auto schema = glz::write_json_schema<JsonDocument>();
        geode::utils::clipboard::write([&](){
            return schema ? *schema : std::string("Error generating schema") + glz::format_error(schema.error());
        }());
    }
};

#endif
