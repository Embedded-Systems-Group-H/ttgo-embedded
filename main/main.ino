#include "config.hpp"
#include "app.hpp"

App* app = nullptr;

void setup(){
    app = &App::Get();
    app->Init();
}

void loop(){
    app->Update();
}
