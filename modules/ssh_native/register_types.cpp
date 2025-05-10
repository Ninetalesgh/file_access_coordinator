#include "register_types.h"

#include "ssh_native/access_coordinator.cpp"


void initialize_ssh_native_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
	{
		return;
	}

	ClassDB::register_class<AccessCoordinator>();
}

void uninitialize_ssh_native_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
	{
		return;
	}

}
