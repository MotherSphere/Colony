#include "app/application.hpp"

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    colony::Application app;
    return app.Run();
}
