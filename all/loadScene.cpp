#include "loadScene.h"
#include "mainScene.h"

void LoadScene::loadSavedMaps(){
}

LoadScene::LoadScene(){}

Scene* LoadScene::DrawControl()
{
	return new MainScene();
}
