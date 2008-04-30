#ifndef RTRACER_RAY_GROUP_H
#define RTRACER_RAY_GROUP_H

#include "rtbase.h"

extern f32x4b GetSSEMaskFromBits_array[16];

INLINE f32x4b GetSSEMaskFromBits(u8 bits) {
	assert(bits<=15);
	/*
	union { u32 tmp[4]; __m128 out; };

	tmp[0]=bits&1?~0:0;
	tmp[1]=bits&2?~0:0;
	tmp[2]=bits&4?~0:0;
	tmp[3]=bits&8?~0:0;

	return out; */
	return GetSSEMaskFromBits_array[bits];
}

INLINE int CountMaskBits(u8 bits) {
	assert(bits<=15);
	char count[16]= { 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4 };
	return count[bits];
}

typedef int RayIndex;

// this class stores a list of active rays
// with their masks
template <int size_>
class RaySelector {
public:
	enum { size=size_ };
private:
	u8 idx[size];
	char bits[size];
	int num;
public:
	INLINE RaySelector() :num(0) { }

	INLINE int Num() const					{ return num; }
	INLINE RayIndex Last() const			{ return idx[num-1]; }
	INLINE RayIndex Idx(int n) const		{ return idx[n]; }
	INLINE RayIndex operator[](int n) const { return idx[n]; }

	// Moves last index to n,
	// decreases number of selected rays
	INLINE void Disable(int n)				{ idx[n]=idx[--num]; bits[n]=bits[num]; }
	INLINE bool DisableWithMask(int n,const f32x4b &m) {
		int newMask=_mm_movemask_ps(m.m);
		bits[n]&=~newMask;
		bool disable=!bits[n];
		if(disable) Disable(n);
		return disable;
	}
	INLINE bool DisableWithBitMask(int n,int newMask) {
		bits[n]&=~newMask;
		bool disable=!bits[n];
		if(disable) Disable(n);
		return disable;
	}

	INLINE void Add(RayIndex i,int bitMask=15) {
		assert(num<size);
		bits[num]=bitMask;
		idx[num++]=i;
	}
	template <class Selector>
	INLINE void Add(const Selector &other,int n)	{ Add(other[n],other.BitMask(n)); }

	INLINE int  BitMask(int n) const			{ return bits[n]; }
	INLINE f32x4b Mask(int n) const				{ return GetSSEMaskFromBits(bits[n]); }
	INLINE void SetBitMask(int n,int m)			{ bits[n]=m; }
	INLINE void SetMask(int n,const f32x4b &m)	{ return bits[n]=_mm_movemask_ps(m.m); }

	INLINE void Clear() { num=0; }
	INLINE void SelectAll() {
		num=size;
		for(int n=0;n<size;n++) { idx[n]=n; bits[n]=15; }
	}
};

// returns 8 for mixed rays
inline int GetVecSign(const Vec3q &dir) {
	int x=SignMask(dir.x),y=SignMask(dir.y),z=SignMask(dir.z);
	int out=(x?1:0)+(y?2:0)+(z?4:0);

	if((x>0&&x<15)||(y>0&&y<15)||(z>0&&z<15)) out=8;
	else if(ForAny(	Min(Abs(dir.x),Min(Abs(dir.y),Abs(dir.z)))<ConstEpsilon<f32x4>() )) out=8;

	return out;
}

template <class Selector1,class Selector2>
inline void SplitSelectorsBySign(const Selector1 &in,Selector2 out[9],Vec3q *dir) {
	for(int n=0;n<9;n++) out[n].Clear();
	for(int i=0;i<in.Num();i++) {
		int n=in[i];
		out[GetVecSign(dir[n])].Add(in,i);
	}
}
	

template <int size_,bool tSharedOrigin=0,bool tPrecomputedInverses=0>
class RayGroup
{
public:
	enum { sharedOrigin=tSharedOrigin, precomputedInverses=tPrecomputedInverses, size=size_ };

	template <class Selector1,class Selector2>
	void GenSelectors(const Selector1 &oldSelector,Selector2 sel[9]) {
		SplitSelectorsBySign(oldSelector,sel,&Dir(0));
	}

	INLINE RayGroup(Vec3q *d,Vec3q *o,Vec3q *i=0) :dir(d),origin(o),idir(i) { assert(!precomputedInverses||idir); }

	template <int tSize,bool tPrecomputed>
	INLINE RayGroup(RayGroup<tSize,sharedOrigin,tPrecomputed> &other,int shift)
		:dir(other.dir+shift)
		,origin(sharedOrigin?other.origin:other.origin+shift)
		,idir(precomputedInverses?tPrecomputed?other.idir+shift:0:0) { }

	INLINE Vec3q &Dir(int n)				{ return dir[n]; }
	INLINE const Vec3q &Dir(int n) const	{ return dir[n]; }

	INLINE Vec3q &Origin(int n)				{ return origin[sharedOrigin?0:n]; }
	INLINE const Vec3q &Origin(int n) const { return origin[sharedOrigin?0:n]; }

	INLINE Vec3q IDir(int n) const			{ return precomputedInverses?idir[n]:VInv(dir[n]); }

	INLINE Vec3q *DirPtr()		{ return dir; }
	INLINE Vec3q *OriginPtr()	{ return origin; }
	INLINE Vec3q *IDirPtr()		{ return precomputedInverses?idir:0; }

private:
	Vec3q *dir,*idir,*origin;
};

#endif

