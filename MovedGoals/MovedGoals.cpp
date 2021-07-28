#include "pch.h"
#include "MovedGoals.h"


BAKKESMOD_PLUGIN(MovedGoals, "write a plugin description here", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

int backWall = 5030;
bool blueEnabled = false;
bool orangeEnabled = false;

int goalWidth = 1786;
int goalHeight = 642;
int backWallLength = 5888;
//int topBackWall = 2030;
int topBackWall = 1000;

Vector goalLocBlue;
Vector goalLocOrange;

void MovedGoals::onLoad()
{
	_globalCvarManager = cvarManager;

	Netcode = std::make_shared<NetcodeManager>(cvarManager, gameWrapper, exports,
		[this](const std::string& Message, PriWrapper Sender) { OnMessageReceived(Message, Sender); });

	std::random_device dev;
	std::mt19937 rng(dev());

	RandomDevice = std::make_shared<std::mt19937>(rng);

	cvarManager->registerCvar("moved_goals_back", std::to_string(backWall), "back wall location")
		.addOnValueChanged([this](std::string, CVarWrapper cvar) {
			backWall = cvar.getIntValue();
			});

	cvarManager->registerCvar("moved_goals_blue", "0", "makes the blue team have random goal location", true, true, 0, true, 1)
		.addOnValueChanged([this](std::string, CVarWrapper cvar) { 
			if (gameWrapper->IsInOnlineGame()) { return; }

			Netcode->SendNewMessage("benable" + cvar.getStringValue()); 
		});

	cvarManager->registerCvar("moved_goals_orange", "0", "makes the orange team have random goal location", true, true, 0, true, 1)
		.addOnValueChanged([this](std::string, CVarWrapper cvar) { 
			if (gameWrapper->IsInOnlineGame()) { return; }

			Netcode->SendNewMessage("oenable" + cvar.getStringValue());
		});

	gameWrapper->HookEventPost("Function TAGame.GameEvent_TA.AddCar",
		[this](...) {
			if (gameWrapper->IsInOnlineGame()) { return; }
			Vector newGoalBlue = generateGoalLocation();
			Vector newGoalOrange = generateGoalLocation();

			Netcode->SendNewMessage("bgoalx" + std::to_string((int)newGoalBlue.X) + "z" + std::to_string((int)newGoalBlue.Z));
			Netcode->SendNewMessage("ogoalx" + std::to_string((int)newGoalOrange.X) + "z" + std::to_string((int)newGoalOrange.Z));

			Netcode->SendNewMessage("benable" + cvarManager->getCvar("moved_goals_blue").getStringValue());
			Netcode->SendNewMessage("oenable" + cvarManager->getCvar("moved_goals_orange").getStringValue());

			cvarManager->log("sent messages");
		});
	
	gameWrapper->HookEventWithCallerPost<CarWrapper>("Function TAGame.Car_TA.SetVehicleInput",
		[this](CarWrapper caller, void*, std::string) { onTick(caller); });
}

void MovedGoals::onUnload()
{
}

// returns a valid serverwrapper only if the user is in a plugin environment that can use netcode
ServerWrapper MovedGoals::GetCurrentGameState() {
	if (gameWrapper->IsInReplay()) {
		return NULL;
	} else if (gameWrapper->IsInOnlineGame()) {
		// client is in an online game
		ServerWrapper sw = gameWrapper->GetOnlineGame();

		if (!sw) {
			return sw;
		}

		auto playlist = sw.GetPlaylist();

		if (!playlist) {
			return NULL;
		}

		// playlist 24 is a LAN match for a client
		if (playlist.GetPlaylistId() != 24) {
			return NULL;
		}

		return sw;
	} else {
		// host is in an offline game
		return gameWrapper->GetGameEventAsServer();
	}
}

// FULFILL REQUEST //
void MovedGoals::OnMessageReceived(const std::string& Message, PriWrapper Sender)
{
    if(Sender.IsNull()) { return; }

	ServerWrapper sw = GetCurrentGameState();

	if (!sw) {
		// exits if not in Netcode-friendly environment
		return;
	}

	// Do your netcode here
	cvarManager->log(Message);

	if (Message.length() < 1) {
		return;
	}

	std::string teamString = Message.substr(0, 1);
	std::string teamName;
	bool isBlue;

	if (teamString == "b") {
		isBlue = true;
		teamName = "blue";
	} else if (teamString == "o") {
		isBlue = false;
		teamName = "orange";
	} else {
		cvarManager->log("error in receving (invalid team) on message: " + Message);
		return;
	}

	std::string updateType = Message.substr(1);

	if (updateType.find("enable") == 0) {
		std::string enableString = updateType.substr(6);
		bool enabled;

		try {
			int enabledTemp = std::stoi(enableString);
			enabled = enabledTemp;
		} catch (...) {
			cvarManager->log("error in receving enabled (invalid number) on message: " + Message);
			return;
		}

		if (isBlue) {
			blueEnabled = enabled;
		} else {
			orangeEnabled = enabled;
		}
	} else if (updateType.find("goal") == 0) {
		std::string xValString = updateType.substr(5);
		int xValue;

		try {
			xValue = std::stoi(xValString);
		} catch (...) {
			cvarManager->log("error in receving new goal (invalid number) on message: " + Message);
			return;
		}

		int zLoc = updateType.find("z");
		if (zLoc == std::string::npos) {
			cvarManager->log("error in receving new goal (no z value) on message: " + Message);
			return;
		}

		std::string zValString = updateType.substr(zLoc + 1);
		int zValue;

		try {
			zValue = std::stoi(zValString);
		} catch (...) {
			cvarManager->log("error in receving new goal (invalid number) on message: " + Message);
			return;
		}

		if (isBlue) {
			goalLocBlue = Vector(xValue, backWall, zValue);
		} else {
			goalLocOrange = Vector(xValue, backWall, zValue);
		}
	}
}

void MovedGoals::onTick(CarWrapper caller) {
	auto sw = GetCurrentGameState();

	if (!sw) return;

	BallWrapper ball = sw.GetBall();

	if (!ball) {
		return;
	}

	ArrayWrapper<GoalWrapper> goals = sw.GetGoals();

	if (goals.Count() < 2) {
		return;
	}

	// end of goal is +-5200
	auto ballLoc = ball.GetLocation();
	auto velocity = ball.GetVelocity();
	auto speed = velocity.magnitude();
	// converts speed to km/h from cm/s
	speed *= 0.036f;
	speed += 0.5f;

	//  5028
	if (ballLoc.Y > backWall && velocity.Y > 0) {
		// ball is going in orange net
		cvarManager->log("shot taken at orange wall");
		if (!blueEnabled) {
			return;
		}

		if (isWithin(goalLocOrange, ballLoc)) {
			if (!gameWrapper->IsInOnlineGame()) {
				auto teams = sw.GetTeams();

				teams.Get(0).ScorePoint(1);
				return;
			}
		}

		velocity.Y = -velocity.Y;
		ball.SetVelocity(velocity);
	}
	else if (ballLoc.Y < -backWall && velocity.Y < 0) {
		// ball is going in blue net
		cvarManager->log("shot taken at blue wall");
		if (!orangeEnabled) {
			return;
		}

		if (isWithin(goalLocBlue, ballLoc)) {
			if (!gameWrapper->IsInOnlineGame()) {
				auto teams = sw.GetTeams();

				teams.Get(0).ScorePoint(1);
				return;
				/*
				GoalWrapper blueGoal = goals.Get(0);

				if (!blueGoal) {
					return;
				}

				ball.EventHitGoal(ball, blueGoal);
				return;*/
			}
		}

		velocity.Y = -velocity.Y;
		ball.SetVelocity(velocity);
	}
}

Vector MovedGoals::generateGoalLocation() {
	int maxX = (backWallLength / 2) - (goalWidth / 2);
	int minX = -maxX;

	int maxZ = topBackWall - (goalHeight / 2);
	int minZ = goalHeight / 2;
	
	std::uniform_int_distribution<std::mt19937::result_type> distX(minX, maxX);

	int xVal = distX(*RandomDevice.get());

	std::uniform_int_distribution<std::mt19937::result_type> distZ(minZ, maxZ);

	int zVal = distZ(*RandomDevice.get());

	cvarManager->log("new goal: " + std::to_string(xVal) + ", " + std::to_string(zVal));

	return Vector(xVal, backWall, zVal);
}

bool MovedGoals::isWithin(Vector goalLoc, Vector ballLoc) {
	float goalMinX = goalLoc.X - (goalWidth / 2);
	float goalMaxX = goalLoc.X + (goalWidth / 2);

	float goalMinZ = goalLoc.Z - (goalHeight / 2);
	float goalMaxZ = goalLoc.Z + (goalHeight / 2);

	return ballLoc.X > goalMinX && ballLoc.X < goalMaxX&& ballLoc.Z > goalMinZ && ballLoc.Z < goalMaxZ;
}