#include "loader.h"
#include <iostream>
#include <fstream>
#include <string.h>

using std::cout;
using std::endl;

uint LoadWavefrontObj(const char *fileName,Vector<Triangle> &out,float scale,uint maxTris) {

	std::filebuf fb;
	if(!fb.open (fileName,std::ios::in)) return 0;

	std::istream is(&fb);
	vector<Vec3f> verts,normals,tex;
	bool flipSides=0;

	float sx=scale,sy=scale,sz=scale;
	int count=0;

	for(;;) {
		char line[1000],type[100],a[100],b[100],c[100],d[100],e[100],f[100];
		if(!is.getline(line,1000))
			break;

		sscanf(line,"%s",type);

		if(strcmp(type,"v")==0) {
			Vec3f vert;
			sscanf(line,"%s %s %s %s",type,a,b,c);
			vert.x=atof(a)*sx;
			vert.y=-atof(b)*sy;
			vert.z=atof(c)*sz;
			verts.push_back(vert);
		}
		else if(strcmp(type,"f")==0) {
			int v[3];

			char *buf;
			buf=strchr(line,' ')+1; v[0]=atoi(buf)-1; 
			buf=strchr(buf ,' ')+1; v[1]=atoi(buf)-1;
			buf=strchr(buf ,' ')+1; v[2]=atoi(buf)-1;

			if(flipSides) out[count++]=Triangle(verts[v[2]],verts[v[1]],verts[v[0]]);
			else out[count++]=Triangle(verts[v[0]],verts[v[1]],verts[v[2]]);
			if(count==maxTris) break;
			
			/*char *buf=strchr(line,' ')+1;
			while(buf=strchr(buf,' ')) {
				buf++;
				v[1]=v[2];
				v[2]=atoi(buf)-1;
			}*/
		}
		else if(strcmp(type,"flip")==0)
			flipSides=1;
		else if(strcmp(type,"scale")==0) {
			sscanf(line,"%s %s %s %s",type,a,b,c);
			sx=sx*atof(a);
			sy=sy*atof(b);
			sz=sz*atof(c);
		}
		/*else if(strcmp(type,"blocker")==0) {
			Vec3f min,max;
			sscanf(line,"%s %s %s %s %s %s %s",type,a,b,c,d,e,f);
			min.x=atof(a); min.y=atof(b); min.z=atof(c);
			max.x=atof(d); max.y=atof(e); max.z=atof(f);
			Triangle blocker(min,max,Lerp(min,max,0.5f));
			blocker.SetFlag1(1337);
			out.push_back(blocker); 
		}*/

	}
	fb.close();
//	printf("Done loading %s\n",fileName);

	return count;
}		

uint LoadRaw(const char *filename,Vector<Triangle> &out,float scale,uint maxTris) {
	FILE *f=fopen(filename,"rb");

	int count=0;

	while(1) {
		float x[3],y[3],z[3];
		if(fscanf(f,"%f %f %f %f %f %f %f %f %f",&x[0],&y[0],&z[0],&x[1],&y[1],&z[1],&x[2],&y[2],&z[2])!=9)
			break;
		for(int n=0;n<3;n++) {
			x[n]*=scale;
			y[n]*=-scale;
			z[n]*=-scale;
		}

		out[count++]=Triangle(Vec3f(x[2],z[2],y[2]),Vec3f(x[1],z[1],y[1]),Vec3f(x[0],z[0],y[0]));
		if(count>maxTris) break;
	}

	fclose(f);
	return count;
}

