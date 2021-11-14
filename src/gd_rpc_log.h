#pragma once

// logging config, just remove it if not needed
#define  GDRPC_LOG_INFO(...) tm_settings::get()->log_info (__VA_ARGS__) 
#define GDRPC_LOG_ERROR(...) tm_settings::get()->log_error(__VA_ARGS__) 