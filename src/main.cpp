#include "Geode/binding/MenuGameLayer.hpp"
#include "Geode/binding/PlayerObject.hpp"
#include "Geode/loader/Mod.hpp"
#include <Geode/ui/Notification.hpp>
#include <Geode/loader/Dirs.hpp>
#include <Geode/modify/MenuGameLayer.hpp>


#include <glaze/glaze.hpp>
#include <random>
#include <stdexcept>

#define MEMBERBYOFFSET(type, class, offset) *reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(class) + offset)
#define MBO MEMBERBYOFFSET

using namespace cocos2d;


struct JsonPlayer
{
	std::string name;
	int iconid = 0;
	int color1 = 0;
	int color2 = 0;
	bool glow = 0;


	void log()
	{
		geode::log::info("Player: {} iconid {} color1 {} color2 {}", name, iconid, color1, color2);
	}
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
		if(players.empty()) loadPlayers();
		if(order.empty()) newOrder();

		if(order.empty())
		{
			geode::log::error("Order is empty! something went wrong");
			return nullptr;
		}

		int index = order.back();
		currentPlayer = [&]() -> JsonPlayer*
		{
			try { return &players.at(index); }
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

		players.clear();
		auto error = glz::read_json(players, jsonStr);
		if(error != 0)
		{
			geode::log::error("Error reading json! {}", glz::format_error(error));
			return;
		}
	}

	void newOrder()
	{
		order.clear();
		order.resize(players.size());
		std::iota(order.begin(), order.end(), 0);
		std::shuffle(order.begin(), order.end(), std::mt19937{std::random_device{}()});
	}
private:
	std::vector<JsonPlayer> players;
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

	void resetPlayer()
	{
		auto* po = m_playerObject;
		if(!po) return MenuGameLayer::resetPlayer();

		if(auto current = PlayerChooser::get()->current();
			current && po->getPositionX() < CCDirector::get()->getWinSize().width)
		{
			onPlayerClicked(current->name);
		}

		MenuGameLayer::resetPlayer();

		auto player = PlayerChooser::get()->next();
		if(!player)
		{
			geode::log::error("Player is nullptr");
			return;
		}

		po->toggleFlyMode(false, false);
		po->toggleRollMode(false, false);
		po->toggleBirdMode(false, false);
		po->toggleDartMode(false, false);
		po->toggleRobotMode(false, false);
		po->toggleSpiderMode(false, false);
		po->toggleSwingMode(false, false);



		po->updatePlayerFrame(player->iconid);

		auto gm = GameManager::sharedState();
		po->setColor(gm->colorForIdx(player->color1));
		po->setSecondColor(gm->colorForIdx(player->color2));

		if(player->glow)
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