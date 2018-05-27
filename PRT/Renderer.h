#ifndef RENDERER_H_
#define RENDERER_H_

#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "hdrTextureCube.h"
#include "Scene.h"
#include "Object.h"
#include "Lighting.h"
#include "DiffuseObject.h"
#include "GeneralObject.h"

// Camera intrinsic parameters definetion.
#define ZOOM 45.0f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 10000.0f

using glm::vec3;

struct MeshVertex
{
    float x;
    float y;
    float z;
    float r;
    float g;
    float b;
};

class Renderer
{
public:
    Renderer() = default;
    virtual ~Renderer();
	void init(Scene *scene,Lighting *light)
	{
		_scene = scene;
		_lighting = light;
	}
	void init(DiffuseObject *obj,Lighting *light)
	{
		_diffObject = obj;
		_lighting = light;
		_genObject = NULL;
	}

	void init(GeneralObject *obj,Lighting *light)
	{
		_genObject = obj;
		_lighting = light;
		_diffObject = NULL;
	}

	void toGPUObject();
	void naiveObjRender();

	void initColorBuffer(int type,vec3 viewDir,bool diffornot);
	void initDiffuseBuffer(int type);
	void initGeneralBuffer(int type,vec3 viewDir);

    void init();
    void render();

private:
	Scene *_scene;
	DiffuseObject *_diffObject;
	GeneralObject *_genObject; 

	
	Lighting *_lighting;
    std::vector<float> _colorBuffer;
    std::vector<MeshVertex> _meshBuffer;

    HDRTextureCube* hdrTextures;
    glm::mat4 projection;

    GLuint _VAO;
    GLuint _VBO;
};

#endif
