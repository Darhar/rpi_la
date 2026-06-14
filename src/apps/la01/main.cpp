#include "la_app.h"

int main()
{
    LaApp app;

    if(!app.init())
        return 1;

    app.run();
    return 0;
}

