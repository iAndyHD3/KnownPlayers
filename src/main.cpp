#include <Geode/ui/Notification.hpp>
#include <matjson.hpp>
#include <Geode/modify/MenuGameLayer.hpp>
#include <random>
#include <numeric>

#define MEMBERBYOFFSET(type, class, offset) *reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(class) + offset)
#define MBO MEMBERBYOFFSET

using namespace cocos2d;

constexpr const char* JSON_STRING = R"JSON(
[
    [ "AeonAir", 109, 29, 3, false ],
    [ "Colon", 60, 18, 10, true ],
    [ "Cool", 37, 20, 17, true ],
    [ "Cyclic", 30, 3, 12, false ],
    [ "DanZmeN", 104, 34, 12, true ],
    [ "envylol", 73, 20, 1, true ],
    [ "EVW", 28, 12, 9, false ],
    [ "Flub", 25, 3, 12, true ],
    [ "Flaaroni", 62, 12, 15, false ],
    [ "Jayuff", 22, 19, 11, true ],
    [ "Jeyzor", 99, 24, 25, true ],
    [ "Juniper", 98, 40, 12, true ],
    [ "Knobbelboy", 37, 10, 14, false ],
    [ "Knots", 50, 40, 3, true ],
    [ "KrmaL", 30, 9, 12, true ],
    [ "lemoncak3", 107, 11, 12, false ],
    [ "Lemons", 93, 7, 11, true ],
    [ "Nexus", 35, 9, 12, true ],
    [ "Michigun", 22, 15, 12, true ],
    [ "MiKhaXx", 11, 1, 15, false ],
    [ "mulpan", 90, 2, 12, true ],
    [ "npesta", 30, 2, 12, false ],
    [ "Norcda Childa", 53, 37, 12, true ],
    [ "OmegaFalcon", 11, 22, 4, true ],
    [ "Partition", 3, 4, 3, false ],
    [ "RedHuseey", 98, 9, 12, false ],
    [ "Riot", 35, 7, 3, true ],
    [ "Smiffy777", 115, 29, 14, true ],
    [ "SpKale", 51, 41, 12, true ],
    [ "SrGuillester", 23, 12, 15, true ],
    [ "Technical", 48, 0, 13, false ],
    [ "TheRealSdSlAyEr", 74, 3, 12, true ],
    [ "TriAxis", 51, 18, 12, true ],
    [ "TrusTa", 30, 7, 3, true ],
    [ "ViPriN", 133, 11, 13, true ],
    [ "Wulzy", 44, 29, 11, false ],
    [ "XShadowWizardX", 115, 12, 7, true ],
    [ "FunnyGame", 13, 18, 9, false ],
    [ "Zobros", 36, 12, 15, true ],
    [ "ZenthicAlpha", 41, 12, 3, true ],
    [ "Alkali", 4, 18, 8, true ],
    [ "Danolex", 46, 3, 11, true ],
    [ "Cirtrax", 93, 18, 41, false ],
    [ "CastriX", 120, 6, 12, true ]
]
)JSON";

struct JsonPlayer
{
	std::string name;
	int iconid = 0;
	int color1 = 0;
	int color2 = 0;
	bool glow = 0;

	explicit JsonPlayer(const matjson::Array& playerarr)
	{
		name = playerarr[0].as_string();
		iconid = playerarr[1].as_int();
		color1 = playerarr[2].as_int();
		color2 = playerarr[3].as_int();
		glow = playerarr[4].as_bool();
	}
	void log()
	{
		geode::log::info("Player: {} iconid {} color1 {} color2 {}", name, iconid, color1, color2);
	}
};


struct MGMHook : geode::Modify<MGMHook, MenuGameLayer>
{
	struct Fields
	{
		matjson::Value players;
		std::vector<int> order;
		int currentIndex = 0;
	};

	PlayerObject* getplayer()
	{
		#if defined(GEODE_IS_WINDOWS)
			return m_playerObject;
		#elif defined(GEODE_IS_ANDROID32)
			return MBO(PlayerObject*, this, 0x180);
		#elif defined(GEODE_IS_ANDROID64)
			return MBO(PlayerObject*, this, 0x1B0);
		#else
			#error Unsupported platform
		#endif
	}

	void resetOrder(size_t count)
	{
		auto& order = m_fields->order;
		order.resize(count);
		std::iota(order.begin(), order.end(), 0);
		std::ranges::shuffle(order, std::mt19937(std::random_device{}()));
	}

	JsonPlayer getNextPlayer()
	{
		auto& order = m_fields->order;
		if(!m_fields->players.is_array()) m_fields->players = matjson::parse(JSON_STRING);

		auto& players = m_fields->players.as_array();
		if(order.empty()) resetOrder(players.size());
		int index = order.back();
		m_fields->currentIndex = index;
		order.pop_back();

		return JsonPlayer(players[index].as_array());
	}

	void onPlayerClicked()
	{
		if(!m_fields->players.is_array() || m_fields->currentIndex == 0) return;
		std::string name = m_fields->players.as_array()[m_fields->currentIndex][0].as_string();
		constexpr float time = 0.2f;
		geode::Notification::create(name, geode::NotificationIcon::Success, time)->show();
	}

	void resetPlayer()
	{
		auto* po = getplayer();
		if(po && po->getPositionX() < CCDirector::get()->getWinSize().width)
		{
			onPlayerClicked();
		}

		MenuGameLayer::resetPlayer();

		po->toggleFlyMode(false, false);
		po->toggleRollMode(false, false);
		po->toggleBirdMode(false, false);
		po->toggleDartMode(false, false);
		po->toggleRobotMode(false, false);
		po->toggleSpiderMode(false, false);
		po->toggleSwingMode(false, false);

		auto player = getNextPlayer();

		po->updatePlayerFrame(player.iconid);

		auto gm = GameManager::sharedState();
		po->setColor(gm->colorForIdx(player.color1));
		po->setSecondColor(gm->colorForIdx(player.color2));

		if(player.glow)
		{
			po->m_hasGlow = true;
			po->enableCustomGlowColor(gm->colorForIdx(player.color2));
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