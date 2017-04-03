#ifndef _SHADERS_CONFIG_H
#define _SHADERS_CONFIG_H

/////////////////////////////////////////////////////////////
// improved parallax occlusion mapping
	#define POM_PARALLAX_OFFSET float(0.015) //(0.02)			// смещение. Чем больше, тем дальше будут выступать кирпичи.

/////////////////////////////////////////////////////////////
// Вода
	#define SW_USE_FOAM								// включить "пену" прибоя
	#define SW_FOAM_THICKNESS float (0.035)			// толщина "пены"
	#define SW_WATER_INTENSITY float (1.0)			// глубина цвета воды

/////////////////////////////////////////////////////////////
// Волны по траве
	#define USE_GRASS_WAVE							// включить "волны" от ветра по траве
	#define GRASS_WAVE_POWER float(1.4) //(2.0)				// "яркость", заметность волн
	#define GRASS_WAVE_FREQ float(0.5) //(0.7)				// частота появления волн
	
#endif // _SHADERS_CONFIG_H	