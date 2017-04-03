#ifndef TONEMAPPING_H
#define TONEMAPPING_H

/*
//////////////////////////////////////////////////////////////////////////////////////////
 * Tonemap version 1.1
 * by Christian Cann Schuldt Jensen ~ CeeJay.dk
//////////////////////////////////////////////////////////////////////////////////////////
*/

// debug
//#define Gamma half(1.4) 		// [0...2]
#define Gamma float(1.4) 		// [0...2]
#define Exposure float(1) 		// [-1...1]
#define Saturation float(1) 		// [-1...1]
#define Bleach float(0.8) 		// [0...1]
//#define Defog half(0.03) 		// [0...1]
#define Defog float(0.03) 		// [0...1]
//#define FogColor half3(0.0, 0.34, 0.1) 		// [0...1, 0...1, 0...1]
//#define FogColor half3(0.0, 0.6, 0.7) 		// [0...1, 0...1, 0...1]

float3 tonemapPass(float3 init_color/*, float2 tc*/) {
	init_color = saturate(init_color - Defog * c_tnmp_defog.xyz * 2.55).rgb; // Defog
	init_color *= pow(2.f, Exposure); // Exposure
	init_color = pow(init_color, Gamma); // Gamma
	
	//const half3 coeLuma = half3(0.3, 0.48, 0.22);
	float lum = dot(/*coeLuma*/LUMINANCE_VECTOR, init_color);
	
	float L = saturate(10 * (lum - 0.45));
	float3 A2 = Bleach * init_color;
	
	float3 result1 = 2.f * init_color * lum;
	float3 result2 = 1.f - 2.f * (1.f - lum) * (1.f - init_color);
	
	float3 newColor = lerp(result1, result2, L);
	float3 mixRGB = A2 * newColor;
	init_color += ((1.f - A2) * mixRGB);
	
	float3 middlegray = dot(init_color, (1/3));
	float3 diffcolor = init_color - middlegray;
	init_color = (init_color + diffcolor * Saturation)/(1 + (diffcolor * Saturation)); // Saturation
	
	return init_color;
}

#endif // TONEMAPPING_H