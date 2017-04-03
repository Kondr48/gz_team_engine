#ifndef lensFlaresPass
#define lensFlaresPass

// Copyright (c) 2009-2015 Gilcher Pascal aka Marty McFly

// DEBUG
//#define LENZ_DEPTH_CHECK
//#define CHAPMAN_DEPTH_CHECK
//#define FLARE_DEPTH_CHECK

#define fLenzThreshold float(0.8)
#define fLenzIntensity float(0.1)

#define fChapFlareDispersal float(0.25)
#define fChapFlareSize float(0.45)
#define fChapFlareCA float3(0.0, 0.01, 0.02)
#define iChapFlareCount int(15)
#define fChapFlareTreshold float(0.9)
#define fChapFlareIntensity float(5.0)

#define fLensdirtIntensity float(0.4)
#define fLensdirtSaturation float(2.0)
#define iLensdirtMixmode int(1)

#define fFlareBlur float(200.0)
#define fFlareTint float3(0.137, 0.216, 1.0)
#define fFlareIntensity float(0.67)
#define fFlareLuminance float(0.344) //(0.025)

//uniform sampler2D SamplerLensFlare1;
//uniform sampler2D SamplerLensFlare2;
uniform sampler2D 	SamplerDirt;
/*
float3 GaussBlur22(float2 coord, sampler2D tex, float mult, float lodlevel, bool isBlurVert) {
	float3 sum = 0;
	float2 axis = isBlurVert ? float2(0, 1) : float2(1, 0);
	
	const float weight[11] = {
		0.082607,
		0.080977,
		0.076276,
		0.069041,
		0.060049,
		0.050187,
		0.040306,
		0.031105,
		0.023066,
		0.016436,
		0.011254		
	};
	
	for (int i = -10; i < 11; i++) {
		float currweight = weight[abs(i)];
		sum += tex2Dlod(tex, float4(coord.xy + axis.xy * (float)i * screen_resolution.zw * mult, 0, lodlevel)) * currweight;
	}
	
	return sum;
}
*/
float3 GetDnB(sampler2D tex, float2 coords) {
	float3 color = max(0, dot(tex2Dlod(tex, float4(coords.xy, 0, 4)).rgb, 0.333) - fChapFlareTreshold) * fChapFlareIntensity;
	
	//#ifdef CHAPMAN_DEPTH_CHECK
	//	if (tex2Dlod(s_position, float4(coords.xy, 0, 3)).z < 0.99999)
	//		color = 0;
	//#endif // CHAPMAN_DEPTH_CHECK	
	
	return color;
}

float3 GetDistortedTex(sampler2D tex, float2 sample_center, float2 sample_vector, float3 distortion) {
	float2 final_vector = sample_center + sample_vector * min(min(distortion.r, distortion.g), distortion.b);
	
	if (final_vector.x > 1.0 || final_vector.y > 1.0 || final_vector.x < -1.0 || final_vector.y < -1.0)
		return float3(0, 0, 0);
	else
		return float3(GetDnB(tex, sample_center + sample_vector * distortion.r).r,GetDnB(tex, sample_center + sample_vector * distortion.g).g,GetDnB(tex, sample_center + sample_vector * distortion.b).b);
}

float3 GetBrightPass(float2 init_coords) {
	float3 c = tex2D(s_image, init_coords).rgb;
	float3 bC = max(c - fFlareLuminance.xxx, 0.0);
	
	float bright = dot(bC, 1.0);
	bright = smoothstep(0.0f, 0.5, bright);
	
	float3 result = lerp(0.0, c, bright);
	
	//#ifdef FLARE_DEPTH_CHECK
	//	float checkdepth = tex2D(s_position, init_coords).z;
	//	if (checkdepth < 0.99999)
	//		result = 0;
	//#endif // FLARE_DEPTH_CHECK	
	
	return result;
}

float3 GetAnamorphicSample(int axis, float2 init_coords, float blur) {
	init_coords = 2.0 * init_coords - 1.0;
	init_coords.x /= -blur;
	init_coords = 0.5 * init_coords + 0.5;
	
	return GetBrightPass(init_coords);
}

float3 LensFlarePass0(float3 init_img, float2 init_center) {
	
	bool bLenzEnable = true;
	// Lenz
	if (bLenzEnable) {
		const float3 lfoffset[19] = {
			float3(0.9, 0.01, 4),
			float3(0.7, 0.25, 25),
			float3(0.3, 0.25, 15),
			float3(1, 1.0, 5),
			float3(-0.15, 20, 1),
			float3(-0.3, 20, 1),
			float3(6, 6, 6),
			float3(7, 7, 7),
			float3(8, 8, 8),
			float3(9, 9, 9),
			float3(0.24, 1, 10),
			float3(0.32, 1, 10),
			float3(0.4, 1, 10),
			float3(0.5, -0.5, 2),
			float3(2, 2, -5),
			float3(-5, 0.2, 0.2),
			float3(20, 0.5, 0),
			float3(0.4, 1, 10),
			float3(0.00001, 10, 20)
		};
		const float3 lffactors[19] = {
			float3(1.5, 1.5, 0),
			float3(0, 1.5, 0),
			float3(0, 0, 1.5),
			float3(0.2, 0.25, 0),
			float3(0.15, 0, 0),
			float3(0, 0, 0.15),
			float3(1.4, 0, 0),
			float3(1, 1, 0),
			float3(0, 1, 0),
			float3(0, 0, 1.4),
			float3(1, 0.3, 0),
			float3(1, 1, 0),
			float3(0, 2, 4),
			float3(0.2, 0.1, 0),
			float3(0, 0, 1),
			float3(1, 1, 0),
			float3(1, 1, 0),
			float3(0, 0, 0.2),
			float3(0.012,0.313,0.588)
		};
		
		float2 lfcoord = 0;
		float3 lenstemp = 0;
		float2 distfact = init_center - 0.5;
		distfact.x *= screen_resolution.x*screen_resolution.w;
		
		for (int i = 0; i < 19; i++) {
			lfcoord.xy = lfoffset[i].x * distfact;
			lfcoord.xy *= pow(2.0 * length(distfact), lfoffset[i].y * 3.5);
			lfcoord.xy *= lfoffset[i].z;
			lfcoord.xy = 0.5 - lfcoord.xy;
			float2 tempfact = (lfcoord.xy - 0.5) * 2;
			float templensmult = clamp(1.0 - dot(tempfact, tempfact), 0, 1);
			float3 lenstemp1 = dot(tex2Dlod(s_image, float4(lfcoord.xy, 0, 1)).rgb, 0.333);
			
			//#ifdef LENZ_DEPTH_CHECK
			//	float templensdepth = tex2D(s_position, lfcoord.xy).z;
			//	if (templensdepth < 0.99999)
			//		lenstemp1 = 0;
			//#endif // LENZ_DEPTH_CHECK	
			
			lenstemp1 = max(0, lenstemp1.xyz - fLenzThreshold);
			lenstemp1 *= lffactors[i] * templensmult;
			
			lenstemp += lenstemp1;
		}
		
		init_img.rgb += lenstemp * fLenzIntensity;
	}
	
	bool bChapFlareEnable = false; //true;
	// Chapman Lens
	if (bChapFlareEnable) {
		float2 sample_vector = (float2(0.5, 0.5) - init_center.xy) * fChapFlareDispersal;
		float2 halo_vector = normalize(sample_vector) * fChapFlareSize;
		
		float3 chaplens = GetDistortedTex(s_image, init_center.xy + halo_vector, halo_vector, fChapFlareCA * 2.5f).rgb;
		
		for (int j = 0; j < iChapFlareCount; j++) {
			float2 foffset = sample_vector * float(j);
			chaplens += GetDistortedTex(s_image, init_center.xy + foffset, foffset, fChapFlareCA).rgb;
		}
		
		chaplens *= 1.0/iChapFlareCount;
		init_img.xyz += chaplens;
	}
	
	bool bAnamFlareEnable = true;
	// Anamorphic flare
	if (bAnamFlareEnable) {
		float3 anamFlare = 0;
		const float gaussweight[5] = {
			0.2270270270,
			0.1945945946,
			0.1216216216,
			0.0540540541,
			0.0162162162
		};
		
		for (int z = -4; z < 5; z++) {
			anamFlare += GetAnamorphicSample(0, init_center.xy + float2(0, z * screen_resolution.w * 2), fFlareBlur) * fFlareTint * gaussweight[abs(z)];
		}
		
		init_img.xyz += anamFlare * fFlareIntensity;
		
		// LensFlarePass1
		//init_img.xyz = GaussBlur22(init_center, SamplerLensFlare1, 2, 0, 1);
		
		// LensFlarePass2
		//init_img.xyz = GaussBlur22(init_center, SamplerLensFlare2, 2, 0, 0);
	}
	
	bool bLensdirtEnable = false; //true;
	// Lens dirt
	if (bLensdirtEnable) {
		float lensdirtmult = dot(init_img.rgb, 0.333);
		float3 dirttex = tex2D(SamplerDirt, init_center).rgb;
		float3 lensdirt = dirttex * lensdirtmult * fLensdirtIntensity;
		
		lensdirt = lerp(dot(lensdirt.xyz, 0.333), lensdirt.xyz, fLensdirtSaturation);
		
		if (iLensdirtMixmode == 0)
			init_img.rgb += lensdirt;
		else if (iLensdirtMixmode == 1)
			init_img.rgb = 1 - (1 - init_img.rgb) * (1 - lensdirt);
		else if (iLensdirtMixmode == 2)
			init_img.rgb = max(0.0f, max(init_img.rgb, lerp(init_img.rgb, (1 - (1 - saturate(lensdirt)) * (1 - saturate(lensdirt))), 1.0)));
		else if (iLensdirtMixmode == 3)
			init_img.rgb = max(init_img.rgb, lensdirt);
	}
	
	return init_img.rgb;
}

#endif // lensFlaresPass