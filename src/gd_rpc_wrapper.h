#pragma once
#include <discord_rpc.h>
#include <ctime>
#include <cstring>

namespace rpc
{
	class discord 
	{
	private:
		discord() : status(-1) {}

	public:
		discord(discord const&) = delete;
		void operator=(discord const&) = delete;
		static discord* get()
		{
			static discord instance; 
			return &instance;
		}

	private:
		int status = -1;

	public:
		void init(const char* application_id);

		int get_status()
		{
			return this->status;
		}

		void set_status(const int value)
		{
			this->status = value;
		}

		void update(
			const char* details,
			const char* largeText,
			const char* smallText,
			const char* statetext,
			const char* smallImage,
			const std::time_t timestamp);

		void run_callbacks()
		{
			Discord_RunCallbacks();
		}

		void shutdown()
		{
			Discord_Shutdown();
		}
	};
}