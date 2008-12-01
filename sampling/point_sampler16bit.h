#ifndef RTRACER_SAMPLING_POINT_SAMPLER_16BIT_H
#define RTRACER_SAMPLING_POINT_SAMPLER_16BIT_H

#include "rtbase.h"
#include <gfxlib_texture.h>

namespace sampling {

	class PointSampler16bit {
	public:
		PointSampler16bit(const gfxlib::Texture&);
		PointSampler16bit() { }

	//	template<class Vec2>
	//	Vec3<typename Vec2::TScalar> operator[](const Vec2 &uv) const {
	//
	//	}

		Vec3f operator()(const Vec2f &uv) const; // biggest mipmap will be used
		Vec3q operator()(const Vec2q &uv) const; // biggest mipmap will be used
		Vec3q operator()(const Vec2q &uv,const Vec2q &diff) const;

	protected:
		void Init(const gfxlib::Texture&);

		gfxlib::Texture tex;
		uint wMask,hMask,mips,w,h,wShift,wShift1;
		float hMul,wMul;
	};

}

#endif
