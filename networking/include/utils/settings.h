#ifndef SETTINGS_H
#define SETTINGS_H

namespace Core::Networking::Settings {
    constexpr int MESSAGE_MAX_SIZE = 1024;
    constexpr int POINTS_PER_PACKAGE = 20; // The most optimal number of points in one package
    constexpr int SERVER_ID = 0; // Default server ID
}

#endif //SETTINGS_H
