#pragma once
#include <string>

namespace Engine
{
	class Config
	{
	public:
		std::string app_name = "Engine";
		bool enable_validation_layers = true;
		bool allow_integrated_gpus = false;
	};



}
