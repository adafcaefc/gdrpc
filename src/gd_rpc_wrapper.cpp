#include "pch.h"
#include "gd_rpc_wrapper.h"

namespace rpc
{
	static void handleDiscordReady(const DiscordUser* connectedUser) 
	{
		discord::get()->set_status(0); // success is code of 0
	}

	static void handleDiscordDisconnected(
		int errcode, 
		const char* message) 
	{
		discord::get()->set_status(errcode);
	}

	static void handleDiscordError(
		int errcode, 
		const char* message) 
	{
		discord::get()->set_status(errcode);
	}

	static void handleDiscordJoinGame(const char* secret) { return; }

	static void handleDiscordSpectateGame(const char* secret) { return; }

	static void handleDiscordJoinRequest(const DiscordUser* request)
	{
		Discord_Respond(request->userId, DISCORD_REPLY_NO);
	}

	void discord::init(const char* application_id)
	{
		DiscordEventHandlers handlers;
		std::memset(&handlers, 0, sizeof(handlers));
		handlers.ready = handleDiscordReady;
		handlers.errored = handleDiscordError;
		handlers.disconnected = handleDiscordDisconnected;
		handlers.joinGame = handleDiscordJoinGame;
		handlers.spectateGame = handleDiscordSpectateGame;
		handlers.joinRequest = handleDiscordJoinRequest;

		Discord_Initialize(application_id, &handlers, 1, "322170");
	}

	void discord::update(
		const char* details,
		const char* largeText,
		const char* smallText,
		const char* statetext,
		const char* smallImage,
		const std::time_t timestamp)
	{
		DiscordRichPresence instance;
		std::memset(&instance, 0, sizeof(instance));
		if (std::strlen(statetext) != 0) instance.state = statetext;

		instance.details = details;
		instance.startTimestamp = timestamp;
		instance.largeImageKey = "logo";
		instance.largeImageText = largeText;

		if (std::strcmp(smallImage, "none") != 0)
		{
			instance.smallImageKey = smallImage;
			instance.smallImageText = smallText;
		}

		Discord_UpdatePresence(&instance);
	}
}
