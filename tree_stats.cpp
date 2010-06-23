#include "rtbase.h"
#include "gfxlib_texture.h"
#include <stdio.h>

/*
	void MemPattern::Init(int size,int r) {
		if(enabled) {
			res=r; mul=double(res)/double(size);
			data.resize(res);
			for(int n=0;n<res;n++) data[n]=0;
		}
	}
	void MemPattern::Draw(gfxlib::Texture &img) const {
		if(enabled) {
			int width=Min(data.size(),img.width);
			for(int n=0;n<width;n++) {
				int color=data[n];
				int r=Clamp(color/255,0,255),g=Clamp(color,0,255),b=0;
				img.Pixel(n,0,r,g,b); img.Pixel(n,1,r,g,b); img.Pixel(n,2,r,g,b);
				img.Pixel(n,img.height-1-0,r,g,b); img.Pixel(n,img.height-1-1,r,g,b); img.Pixel(n,img.height-1-2,r,g,b);
			}
		}
	} */

	template <bool enabled>
	string TreeStats<enabled>::GenInfo(int resx,int resy,double msRenderTime,double msBuildTime) {
		double raysPerSec = double(data[2]) * (1000.0 / msRenderTime);
		double nPixels = double(resx*resy);

		if(!enabled) {
			char buf[2048];
			sprintf(buf,"ms/frame:%6.2f  MPixels/sec:%6.2f\n",msRenderTime,
					(resx*resy*(1000.0/msRenderTime))*0.000001);
			return string(buf);
		}

		char buf[2048];
		sprintf(buf,
				/*"isct,iter:"*/"%5.2f %5.2f  ms:%6.2f  mrs:%5.2f (%d rays) %d"
				/*"Coh:%.2f%% " "br:%.2f%% fa:%.2f%%" " Build:%6.2f"*/,
				double(data[0]) / nPixels, double(data[1]) / nPixels, msRenderTime, raysPerSec * 0.000001, data[2],
				data[9]
				/*,GetCoherent()*100.0f, GetBreaking()*100.0f, GetIntersectFail()*100.0f, msBuildTime*/);
		return string(buf);
	}

	template string TreeStats<0>::GenInfo(int,int,double,double);
	template string TreeStats<1>::GenInfo(int,int,double,double);

