#include "ClientApplication.h"
#include "utils/log.h"

int main()
{
    ClientApplication app;
    if (!app.Init()) {
        LOG_LINE("Failed to init application.");
        return 1;
    }
    app.Run();

    return 0;
}
