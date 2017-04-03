#include "blur.h"
#include "ShaderSettingsVisor.txt"
float4 ___z1103(float2 s,float4 V,float e,int f)
{
	float4 R=0;
	float2 r = float2(.5,.5), i = float2 (s.x,(s.y - .5) * VIS_SCREENRATIO + .5);
	float x = length(i.xy - float2(.5, .5)), y = 10, t =.35, F =.05, T = 2.,O =.8, z =.15,c = 5.2;
	if (x >= t ) 
	{
		float p = saturate ( (x / t - 1) * T);
           for(int L = 0; L < y; L++)
		    {
				  float N = 1 - p / O + z * (L / ( y - 1) ) * p;
				  float2 m = (float2(.5, .5) - s.xy) * - N + r;
				  R += tex2Dlod(s_image, float4(m, 0, 0));
		    }
			R *= 1 / y;
			R *= 1 - saturate((x - t - F) * c);
	}
	else 
	{
		R = tex2Dlod(s_image, float4(s, 0, 0));
		return (V + VIS_CRUNCHINTENSITY * R) / (1.f + VIS_CRUNCHINTENSITY);
	}
}
float4 __i_21(float2 s,float4 V,float e,int f)
	{
		float4 R = 0;
		float r = f == 1 ? VIS_CENTER.x : VIS_CENTER.y,O = f == 1 ? VIS_BLACKASPECT.x : VIS_BLACKASPECT.y, F = f == 1 ? VIS_BLACKRADIUSMIN.x : VIS_BLACKRADIUSMIN.y, y = f == 1 ? VIS_BLACKRADIUSMAX.x : VIS_BLACKRADIUSMAX.y, z = 0, t = 0, x = 0, i = 0, L = 0, T = 0, c = 0;
		z = VIS_XFACTOR * (s.x-.5);
		t = VIS_YFACTOR*(s.y-.5);
		x = sqrt(z*z*O+t*t);
		L = x;
		i = x;
		if( f != 1)
		{
			float p = VIS_XFACTOR * (s.x - .5), N =- (p -.5) +.5;
			p -= .3;
			N -= 1.3;
			x = sqrt(p * p * tan(p + r * t) + t * t * cos(t));
			L = sqrt(N * N * tan(N + r * t) + t * t * cos(t));
		}
		T = 1 - saturate((x - F) / (y - F));
		c = 1 - saturate((L - F) / (y - F));
		x = i;
		static const float p = 3.14159f;
		float2 N = float2(s.x, (s.y - .5) * VIS_SCREENRATIO +.5);
		float m = length(N.xy - float2(.5, .5)), l= x? - x / tan(x) / .4:0., o = l * z,d = l * t, n = fmod(o / (VIS_BREAK * p) + .5,1.f), Y = d / (VIS_BREAK * p) +.5;
		float2 D = float2(n, Y);
		float v = tex2Dlod(s_position, float4(D, 0, 0)).z, K = saturate(1 - (VIS_MIRRORMINDIST - v) / VIS_MIRRORMINDIST) * VIS_MIRRORINTENSITY;
        #ifdef VIS_BLOOM
           #ifdef VIS_BLUR
                 float4 X=(tex2Dlod(s_bloom,float4(D+VIS_BLUR*(.5f/1024.f),0,0))+tex2Dlod(s_bloom,float4(D-VIS_BLUR*(.5f/1024.f),0,0)))*.5f;
           #else
                 float4 X=tex2Dlod(s_bloom,float4(D,0,0));
           #endif
        #else
           #ifdef VIS_BLUR
                  float4 X=(tex2Dlod(s_image,float4(D+VIS_BLUR*(.5f/1024.f),0,0))+tex2Dlod(s_image,float4(D-VIS_BLUR*(.5f/1024.f),0,0)))*.5f;
           #else
                  float4 X=tex2Dlod(s_image,float4(D,0,0));
           #endif
        #endif
        float C = dot(X.xyz, float3(.3, .59, .11));
        X.xyz = lerp(float3(C, C, C), X.xyz, VIS_SATURATION + dot(Ldynamic_color.xyz, float3(1, 1, 1)) / 3 * VIS_SAT_SUN_COEF);
        float3 A = X.xyz * X.xyz;
        X.xyz = lerp(X.xyz, A, VIS_CONTRAST);
        R = V + K * pow(m, VIS_RADIALCOEFF) * X * pow(VIS_COLOR, VIS_COLORCOEFF);
        #ifdef VIS_BLACKSMOOTH
          if(s.x > .5) 
			  return R * smoothstep(0, 1, T);
		  else 
			  return R * smoothstep(0, 1, c);
        #else
          if(s.x>.5)
			  return R*T;
	      else 
		      return R*c;
        #endif
}
float4 visor_reflect(float2 s,float4 f,float V)
{
   #ifdef VIS
     if(e_weights.x > .99)
         {
	         int t = (float)((e_weights.x - .99f) * 1000.f);
	         float4 R = __i_21(s, f, V, t);
	         R = ___z1103(s, R, V, t);
	         return R * VIS_COLOR;
	     }
	else 
	         return f;
    #else
        return f;
    #endif
}