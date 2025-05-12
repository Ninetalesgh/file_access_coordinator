#define NO_GODOT

#include "access_coordinator.cpp"

int main(int argc, char** argv)
{
  AccessCoordinator coordinator;

  for (int i = 1; i < argc; ++i)
  {
    if (string_begins_with(argv[i], "-download"))
    {
      coordinator.download();
    }
    else if (string_begins_with(argv[i], "-upload"))
    {
      coordinator.upload();
    }
    else if (string_begins_with(argv[i], "-reserve"))
    {
      coordinator.reserve();
    }
    else if (string_begins_with(argv[i], "-release"))
    {
      coordinator.release(false);
    }
    else if (string_begins_with(argv[i], "-forcerelease"))
    {
      coordinator.release(true);
    }
  }

  coordinator.shutdown_session();

  return 0;
}
