#include "enum.hpp"
#include "logger.hpp"

int main()
{

    Logger::get().enable_logs(Logger::LOG_LEVEL_DEBUG | Logger::LOG_LEVEL_ERROR | Logger::LOG_LEVEL_FATAL | Logger::LOG_LEVEL_INFO | Logger::LOG_LEVEL_WARNING);

    ASSERT_EQ(Reflection::Enum::get_enums().size(), 3, "Invalid registered enum number")
}