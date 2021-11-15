#pragma once

// common project macro settings, change if needed
#define  GDRPC_LOG_INFO(...) ::tm_settings::get()->log_info (__VA_ARGS__) 
#define GDRPC_LOG_ERROR(...) ::tm_settings::get()->log_error(__VA_ARGS__) 
#define   GDRPC_ENABLED      ::tm_settings::get()->gd_rpc_enable.active