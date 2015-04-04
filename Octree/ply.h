

#ifndef Octree_ply_h
#define Octree_ply_h

class Model_PLY 
{
public:
	int Load(const char *filename);
	void Draw();
    void calculateNormal(float norm[3], float *coord1, float *coord2, float *coord3 );
	Model_PLY();
    
    float* Faces_Triangles;
    float* Faces_Quads;
	float* Vertex_Buffer;
	float* Normals;
    
	int TotalConnectedTriangles;	
	int TotalConnectedQuads;	
	int TotalConnectedPoints;
	int TotalFaces;
    
    
};

#endif
