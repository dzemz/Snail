#include <iostream>
#include <baselib_threads.h>
#include "ray_generator.h"
#include "scene.h"
#include "camera.h"

#include "bih/tree.h"
#include "gl_window.h"
#include "formats/loader.h"
#include "base_scene.h"
#include "tree_box.h"
#include "font.h"

struct Options {
	Options(ShadingMode sm,bool refl,bool rdtsc) :shading(sm),reflections(refl),rdtscShader(rdtsc) { }
	Options() { reflections=rdtscShader=0; shading=smFlat; }

	ShadingMode shading;
	bool reflections,rdtscShader;
};

inline i32x4 ConvColor(const Vec3q &rgb) {
	i32x4 tr=Trunc(Clamp(rgb.x*255.0f,floatq(0.0f),floatq(255.0f)));
	i32x4 tg=Trunc(Clamp(rgb.y*255.0f,floatq(0.0f),floatq(255.0f)));
	i32x4 tb=Trunc(Clamp(rgb.z*255.0f,floatq(0.0f),floatq(255.0f)));

	__m128i c1,c2,c3,c4;
	c1=_mm_packs_epi32(tr.m,tr.m);
	c2=_mm_packs_epi32(tg.m,tg.m);
	c3=_mm_packs_epi32(tb.m,tb.m);
	c1=_mm_packus_epi16(c1,c1);
	c2=_mm_packus_epi16(c2,c2);
	c3=_mm_packus_epi16(c3,c3);

	__m128i c12=_mm_unpacklo_epi8(c1,c2);
	__m128i c33=_mm_unpacklo_epi8(c3,c3);
	return _mm_unpacklo_epi16(c12,c33);
}

template <class AccStruct,int QuadLevels>
struct RenderTask {
	RenderTask(const AccStruct *tr,const Camera &cam,Image *tOut,const Options &opt,uint tx,uint ty,
					uint tw,uint th,TreeStats<1> *outSt) :tree(tr),camera(cam),out(tOut),options(opt),
					startX(tx),startY(ty),width(tw),height(th),outStats(outSt) {
		}

	uint startX,startY;
	uint width,height;

	TreeStats<1> *outStats;
	const AccStruct *tree;
	Camera camera;
	Image *out;
	Options options;

	void Work() {
		float ratio=float(out->width)/float(out->height);

		enum { NQuads=1<<(QuadLevels*2), PWidth=2<<QuadLevels, PHeight=2<<QuadLevels };

		Matrix<Vec4f> rotMat(Vec4f(camera.right),Vec4f(camera.up),Vec4f(camera.front),Vec4f(0,0,0,1));

		Vec3q origin; Broadcast(camera.pos,origin);
		RayGenerator rayGen(QuadLevels,out->width,out->height,camera.plane_dist);

		uint pitch=out->width*3;
		u8 *outPtr=(u8*)&out->buffer[startY*pitch+startX*3];
		ShadowCache shadowCache;

		for(int y=0;y<height;y+=PHeight) {
			for(int x=0;x<width;x+=PWidth) {
				Vec3q dir[NQuads];
				rayGen.Generate(PWidth,PHeight,startX+x,startY+y,dir);

				for(int n=0;n<NQuads;n++) {
					dir[n]=rotMat*dir[n];
					dir[n]*=RSqrt(dir[n]|dir[n]);
					dir[n].x+=floatq(0.000000000001f);
					dir[n].y+=floatq(0.000000000001f);
					dir[n].z+=floatq(0.000000000001f);
				}

				RayGroup<NQuads,isct::fShOrig|isct::fInvDir|isct::fPrimary> rays(origin,dir);
				Result<NQuads> result=RayTrace(*tree,rays,FullSelector<NQuads>());
				Vec3q *rgb=result.color;
				*outStats += result.stats;

				if(NQuads==1) {
					i32x4 col=ConvColor(rgb[0]);
					const u8 *c=(u8*)&col;

					u8 *p1=outPtr+y*pitch+x*3;
					u8 *p2=p1+pitch;

					p1[ 0]=c[ 0]; p1[ 1]=c[ 1]; p1[ 2]=c[ 2];
					p1[ 3]=c[ 4]; p1[ 4]=c[ 5]; p1[ 5]=c[ 6];
					p2[ 0]=c[ 8]; p2[ 1]=c[ 9]; p2[ 2]=c[10];
					p2[ 3]=c[12]; p2[ 4]=c[13]; p2[ 5]=c[14];
				}
				else {
					rayGen.Decompose(rgb,rgb);

					Vec3q *src=rgb;
					u8 *dst=outPtr+x*3+y*pitch;
					int lineDiff=pitch-PWidth*3;

					for(int ty=0;ty<PHeight;ty++) {
						for(int tx=0;tx<PWidth;tx+=4) {
							i32x4 col=ConvColor(*src++);
							const u8 *c=(u8*)&col;
	
							dst[ 0]=c[ 0]; dst[ 1]=c[ 1]; dst[ 2]=c[ 2];
							dst[ 3]=c[ 4]; dst[ 4]=c[ 5]; dst[ 5]=c[ 6];
							dst[ 6]=c[ 8]; dst[ 7]=c[ 9]; dst[ 8]=c[10];
							dst[ 9]=c[12]; dst[10]=c[13]; dst[11]=c[14];

							

							dst+=12;
						}
						dst+=lineDiff;
					}
				}
			}
		}
	}
};

template <int QuadLevels,class AccStruct>
TreeStats<1> Render(const AccStruct &tree,const Camera &camera,Image &image,const Options options,uint tasks) {
	enum { taskSize=64 };

	uint numTasks=(image.width+taskSize-1)*(image.height+taskSize-1)/(taskSize*taskSize);

	vector<TreeStats<1> > taskStats(numTasks);

	TaskSwitcher<RenderTask<AccStruct,QuadLevels> > switcher(numTasks);
	uint num=0;
	for(uint y=0;y<image.height;y+=taskSize) for(uint x=0;x<image.width;x+=taskSize) {
		switcher.AddTask(RenderTask<AccStruct,QuadLevels> (&tree,camera,&image,options,x,y,
					Min((int)taskSize,int(image.width-x)),Min((int)taskSize,int(image.height-y)),
					&taskStats[num]) );
		num++;
	}

	switcher.Work(tasks);

	TreeStats<1> stats;
	for(uint n=0;n<numTasks;n++)
		stats+=taskStats[n];
	return stats;
}

template <class AccStruct>
TreeStats<1> Render(int quadLevels,const AccStruct &tree,const Camera &camera,Image &image,const Options options,uint tasks) {
	switch(quadLevels) {
//	case 0: return Render<0>(tree,camera,image,options,tasks);
//	case 1: return Render<1>(tree,camera,image,options,tasks);
	case 2: return Render<2>(tree,camera,image,options,tasks);
//	case 3: return Render<3>(tree,camera,image,options,tasks);
//	case 4: return Render<4>(tree,camera,image,options,tasks);
	default: throw Exception("Quad level not supported.");
	}
}

typedef bih::Tree<Triangle,ShTriangle> StaticTree;
typedef bih::Tree<TreeBox<StaticTree>,ShTriangle> FullTree;

template TreeStats<1> Render<StaticTree>(int,const StaticTree&,const Camera&,Image&,const Options,uint);
template TreeStats<1> Render<FullTree  >(int,const FullTree  &,const Camera&,Image&,const Options,uint);
	
