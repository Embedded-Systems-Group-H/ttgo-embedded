#include "config.hpp"
#include "app.hpp"

App* app = nullptr;

void setup(){
    app = &App::Get();
    app->Init();
    // delay(1000);
}

void loop(){
    app->Update();
    // delay(5);
}
