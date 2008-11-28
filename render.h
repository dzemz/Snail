#include "camera.h"
#include "rtbase.h"
#include "tree_stats.h"

struct Options {
	Options(ShadingMode sm,bool refl,bool rdtsc) :shading(sm),reflections(refl),rdtscShader(rdtsc) { }
	Options() { reflections=rdtscShader=0; shading=smFlat; }

	ShadingMode shading;
	bool reflections,rdtscShader;
};

template <class AccStruct>
TreeStats<1> Render(int quadLevels,const AccStruct &tree,const Camera &camera,Image &image,
					const Options options,uint tasks);

