#include "Renderer.h"
#include "UI.h"
#include "resource_manager.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

extern bool simpleLight;
extern int objectIndex;
extern Lighting simpleL;
extern int lightNumber;
extern int lightingIndex;
extern std::string lightings[];
extern int WIDTH;
extern int HEIGHT;

extern float camera_dis;
extern float last_camera_pos[];
extern float camera_pos[];
extern float camera_dir[];
extern float camera_up[];

extern int g_AutoRotate;
extern float rotateMatrix[4 * 4]; // Rotation matrix.

extern int vertices;
extern int faces;

extern bool drawCubemap;

extern DiffuseObject* diffObject;
extern GeneralObject* genObject;
extern Lighting* lighting;

using glm::mat3;

void Renderer::toGPUObject()
{
    // glGenVertexArrays(1, &_vao);
    // glBindVertexArray(_vao);
    //
    // glGenBuffers(1, &_vboVertex);
    // glBindBuffer(GL_ARRAY_BUFFER, _vboVertex);
    // glBufferData(
    //     GL_ARRAY_BUFFER,
    //     sizeof(float) * _scene->_vertexes.size(),
    //     _scene->_vertexes.data(),
    //     GL_STATIC_DRAW
    // );
    // glEnableClientState(GL_VERTEX_ARRAY);
    // glVertexPointer(3,GL_FLOAT, 0, 0);
    //
    // glGenBuffers(1, &_vboNormal);
    // glBindBuffer(GL_ARRAY_BUFFER, _vboNormal);
    // glBufferData(
    //     GL_ARRAY_BUFFER,
    //     sizeof(float) * _scene->_normals.size(),
    //     _scene->_normals.data(),
    //     GL_STATIC_DRAW
    // );
    //
    // glGenBuffers(1, &_vboColor);
    // glBindBuffer(GL_ARRAY_BUFFER, _vboColor);
    /*glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(float) * _scene->co*/
}

void Renderer::initDiffuseBuffer(int type)
{
    if (_diffObject->band() != _lighting->band())
    {
        std::cout << "different number band!" << std::endl;
    }
    int vertexnumber = _diffObject->_vertexes.size() / 3;
    int band2 = _diffObject->band() * _diffObject->band();

    // Generate color buffer.
    _colorBuffer.clear();
    for (int i = 0; i < vertexnumber; ++i)
    {
        float cr = 0, cg = 0, cb = 0;
        float lightcoeff[3];
        for (int j = 0; j < band2; ++j)
        {
            lightcoeff[0] = _lighting->_Vcoeffs[0](j);
            lightcoeff[1] = _lighting->_Vcoeffs[1](j);
            lightcoeff[2] = _lighting->_Vcoeffs[2](j);

            if (simpleLight)
            {
                for (int k = 0; k < 3; ++k)
                    lightcoeff[k] = simpleL._Vcoeffs[k](j);
            }

            cr += lightcoeff[0] * _diffObject->_DTransferFunc[type][i][j][0];
            cg += lightcoeff[1] * _diffObject->_DTransferFunc[type][i][j][1];
            cb += lightcoeff[2] * _diffObject->_DTransferFunc[type][i][j][2];
        }

        cr *= _lighting->HDRaffect().x;
        cg *= _lighting->HDRaffect().y;
        cb *= _lighting->HDRaffect().z;

        _colorBuffer.push_back(cr);
        _colorBuffer.push_back(cg);
        _colorBuffer.push_back(cb);
    }
    // Generate mesh buffer.
    _meshBuffer.clear();
    int facenumber = _diffObject->_renderIndex.size() / 3;
    for (int i = 0; i < facenumber; ++i)
    {
        int offset = 3 * i;
        int index[3] = {
            _diffObject->_renderIndex[offset + 0],
            _diffObject->_renderIndex[offset + 1],
            _diffObject->_renderIndex[offset + 2]
        };

        for (int j = 0; j < 3; ++j)
        {
            int Vindex = 3 * index[j];
            MeshVertex vertex = {
                _diffObject->_vertexes[Vindex + 0],
                _diffObject->_vertexes[Vindex + 1],
                _diffObject->_vertexes[Vindex + 2],
                _colorBuffer[Vindex + 0],
                _colorBuffer[Vindex + 1],
                _colorBuffer[Vindex + 2]
            };
            _meshBuffer.push_back(vertex);
        }
    }

    // Set the objects we need in the rendering process (namely, VAO, VBO and EBO).
    glGenVertexArrays(1, &_VAO);
    glGenBuffers(1, &_VBO);
    glBindVertexArray(_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, _VBO);
    glBufferData(GL_ARRAY_BUFFER, _meshBuffer.size() * sizeof(MeshVertex), &(_meshBuffer[0]), GL_STATIC_DRAW);

    // Position attribute.
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (GLvoid*)0);
    // Color attribute.
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (GLvoid*)(3 * sizeof(float)));

    // Unbind.
    glBindVertexArray(0);
}

void Renderer::initColorBuffer(int type, vec3 viewDir, bool diffornot = true)
{
    if (diffornot)
        initDiffuseBuffer(type);
    else
        initGeneralBuffer(type, viewDir);
}

void Renderer::initGeneralBuffer(int type, vec3 viewDir)
{
    std::cout << "type" << type << std::endl;
    std::cout << "name" << _genObject->_modelname << std::endl;
    if (objectIndex == 0)
    {
        if (type > 1)
            type = 1;
    }
    else
    {
        type = 0;
    }

    std::cout << "buffer type " << type << std::endl;

    if (_genObject->band() != _lighting->band())
    {
        std::cout << "different number band!" << std::endl;
    }
    int vertexnumber = _genObject->_vertexes.size() / 3;
    int band = _genObject->band();
    int band2 = band * band;

    // Generate color buffer.
    _colorBuffer.clear();
    _colorBuffer.resize(_genObject->_vertexes.size());

    //std::cout << "viewDir"  << viewDir.x << ' '<< viewDir.y << ' ' << viewDir.z << std::endl;
    int validNumber = 0;
#pragma omp parallel for
    for (int i = 0; i < vertexnumber; ++i)
    {
        int offset = 3 * i;
        vec3 position = vec3(_genObject->_vertexes[offset], _genObject->_vertexes[offset + 1],
                             _genObject->_vertexes[offset + 2]);
        vec3 normal = vec3(_genObject->_normals[offset], _genObject->_normals[offset + 1],
                           _genObject->_normals[offset + 2]);


        float color[3];
        //float lightcoeff[3];

        VectorXf transferedLight[3];
        if (type == 0)
        {
            transferedLight[0] = _genObject->_TransferMatrix[0][i] * _lighting->_Vcoeffs[0];
            transferedLight[1] = _genObject->_TransferMatrix[0][i] * _lighting->_Vcoeffs[1];
            transferedLight[2] = _genObject->_TransferMatrix[0][i] * _lighting->_Vcoeffs[2];
        }
        else
        {
            transferedLight[0] = _genObject->_TransferMatrix[1][i] * _lighting->_Vcoeffs[0];
            transferedLight[1] = _genObject->_TransferMatrix[1][i] * _lighting->_Vcoeffs[1];
            transferedLight[2] = _genObject->_TransferMatrix[1][i] * _lighting->_Vcoeffs[2];
        }


        Lighting lightingtemp(band, transferedLight);

        float rotateMatrix[3][3];
        vec3 tangent = vec3(_genObject->_tangent[i].x, _genObject->_tangent[i].y, _genObject->_tangent[i].z);
        vec3 binormal = glm::cross(normal, tangent) * _genObject->_tangent[i].w;
        //vec3 binormal = glm::cross(normal,tangent);

        //TEST DATA 1 PASSED
        /*tangent = vec3(0.0f,0.0f,1.0f);
        normal = vec3(0.0f,1.0f,0.0f);
        binormal = glm::cross(normal,tangent) ;*/


        //TEST DATA 2 PASSED
        /*tangent = vec3(0.0,-1.0f,0.0f);
        normal = vec3(0.0f,0.0f,1.0f);
        binormal = glm::cross(normal,tangent);

        //TEST DATA 3  PASSED
        tangent = vec3(1.0f,0.0f,0.0f);
        normal = vec3(0.0f,1.0f,0.0f);
        binormal = glm::cross(normal,tangent) ;

        //TEST DATA 4 PASSED
        tangent = vec3(0.0f,-1.0f,0.0f);
        normal = vec3(1.0,0.0,0.0);
        binormal = glm::cross(normal,tangent);*/

        //TEST DATA5
        /*tangent = vec3(0.0f,0.0f,1.0f);
        normal = vec3(-1.0f,0.0f,0.0f);
        binormal = glm::cross(normal,tangent);*/

        /*	if(fabs(normal.x + 1.0f) < 0.02f)
            {
                std::cout << "local coord check" << std::endl;
    
                std::cout << "normal " << normal.x << ' ' << normal.y << ' ' << normal.z<< std::endl;
                std::cout << "tangent " << tangent.x << ' ' << tangent.y << ' ' << tangent.z<< std::endl;
                std::cout << "binormal " << binormal.x << ' ' << binormal.y << ' ' << binormal.z<< std::endl;
    
                system("pause");
    
    
    
            }*/

        //std::cout << binormal.x << ' '<< binormal.y << ' '<< binormal.z << std::endl;


        mat3 changeCoord;


        /*tangent = vec3(1.0f,0.0f,0.0f);
        normal = vec3(0.0f,0.0f,1.0f);
        binormal = glm::cross(normal,tangent) ;*/

        //yzx(OpenGL) to zxy(not OpenGL)
        vec3 mattangent, matnormal, matbinormal;
        matnormal = vec3(normal.z, normal.x, normal.y);
        mattangent = vec3(tangent.z, tangent.x, tangent.y);
        matbinormal = vec3(binormal.z, binormal.x, binormal.y);


        //rotate matrix in zxy
        for (int m = 0; m < 3; ++m)rotateMatrix[m][0] = mattangent[m];
        for (int m = 0; m < 3; ++m)rotateMatrix[m][1] = matbinormal[m];
        for (int m = 0; m < 3; ++m)rotateMatrix[m][2] = matnormal[m];

        float alpha, beta, gamma;

        rotateMatrixtoYZY(rotateMatrix, alpha, beta, gamma);

        vector<vec2> paraResult;
        paraResult.push_back(vec2(gamma, beta));
        paraResult.push_back(vec2(alpha, 0.0f));


        lightingtemp.rotateZYZ(paraResult);

        //CONVOLUTION wih BRDF
        for (int l = 0; l < band; ++l)
        {
            float alpha_l_0 = sqrt((4.0f * (float)M_PI) / (2 * l + 1));

            int BRDFindex = l * (l + 1);
            for (int m = -l; m <= l; ++m)
            {
                int index = BRDFindex + m;
                for (int k = 0; k < 3; ++k)
                {
                    lightingtemp._Vcoeffs[k](index) *= (alpha_l_0 * _genObject->_BRDFcoeff(BRDFindex));
                }
            }
        }

        vec3 dir = viewDir;

        dir = glm::normalize(dir);


        float theta = 0.0f, phi = 0.0f;

        if (glm::dot(normal, dir) < 0)
        {
            _colorBuffer[3 * i + 0] = 0.0f;
            _colorBuffer[3 * i + 1] = 0.0f;
            _colorBuffer[3 * i + 2] = 0.0f;

            continue;
        }

        theta = acos(glm::clamp(glm::dot(normal, dir), -1.0f, 1.0f));
        //phi = acos(glm::clamp(glm::dot(normal,dir),-1.0f,1.0f));
        //	std::cout <<"Theta" << theta << std::endl;
        validNumber ++;

        for (int s = 0; s < 3; ++s)
        {
            color[s] = 0.0f;
            for (int l = 0; l < band; ++l)
            {
                for (int m = -l; m <= l; ++m)
                {
                    int index = l * (l + 1) + m;
                    color[s] += lightingtemp._Vcoeffs[s](index) * (float)SphericalH::SHvalue(theta, phi, l, m);
                }
            }

            color[s] *= _lighting->Glossyaffect()[s];
            _colorBuffer[3 * i + s] = color[s];
        }
    }
    // Generate mesh buffer.
    _meshBuffer.clear();
    int facenumber = _genObject->_renderIndex.size() / 3;
    for (int i = 0; i < facenumber; ++i)
    {
        int offset = 3 * i;
        int index[3] = {
            _genObject->_renderIndex[offset + 0],
            _genObject->_renderIndex[offset + 1],
            _genObject->_renderIndex[offset + 2]
        };

        for (int j = 0; j < 3; ++j)
        {
            int Vindex = 3 * index[j];
            MeshVertex vertex = {
                _genObject->_vertexes[Vindex + 0],
                _genObject->_vertexes[Vindex + 1],
                _genObject->_vertexes[Vindex + 2],
                _colorBuffer[Vindex + 0],
                _colorBuffer[Vindex + 1],
                _colorBuffer[Vindex + 2]
            };
            _meshBuffer.push_back(vertex);
        }
    }

    // Set the objects we need in the rendering process (namely, VAO, VBO and EBO).
    glGenVertexArrays(1, &_VAO);
    glGenBuffers(1, &_VBO);
    glBindVertexArray(_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, _VBO);
    glBufferData(GL_ARRAY_BUFFER, _meshBuffer.size() * sizeof(MeshVertex), &(_meshBuffer[0]), GL_STATIC_DRAW);

    // Position attribute.
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (GLvoid*)0);
    // Color attribute.
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (GLvoid*)(3 * sizeof(float)));

    // Unbind.
    glBindVertexArray(0);

    std::cout << validNumber << std::endl;
    std::cout << "Color buffer done." << std::endl;
}

void Renderer::naiveObjRender()
{
    glBindVertexArray(_VAO);
    if (_genObject == NULL)
    {
        vertices = _diffObject->_vertexes.size() / 3;
        faces = _diffObject->_renderIndex.size() / 3;
    }
    else if (_diffObject == NULL)
    {
        vertices = _genObject->_vertexes.size() / 3;
        faces = _genObject->_renderIndex.size() / 3;
    }

    glDrawArrays(GL_TRIANGLES, 0, _meshBuffer.size());

    // Unbind.
    glBindVertexArray(0);
}

Renderer::~Renderer()
{
    delete[]hdrTextures;
}

void Renderer::init()
{
    // Initialize cubemap.
    hdrTextures = new HDRTextureCube[lightNumber];
    for (size_t i = 0; i < lightNumber; ++i)
    {
        hdrTextures[i].Init("LightingCube/hdr/" + lightings[i] + ".hdr");
    }

    // Initialize projection matrix.
    projection = glm::perspective(ZOOM, (float)WIDTH / (float)HEIGHT, NEAR_PLANE, FAR_PLANE);
}

void Renderer::render()
{
    // Render objects.
    glm::mat4 view = glm::lookAt(
        glm::vec3(camera_dis * camera_pos[0], camera_dis * camera_pos[1], camera_dis * camera_pos[2]),
        glm::vec3(camera_dir[0], camera_dir[1], camera_dir[2]), glm::vec3(camera_up[0], camera_up[1], camera_up[2]));
    glm::mat4 model;
    bool b_rotate = false;
    if (g_AutoRotate)
    {
        float axis[3] = {0, 1, 0};
        float angle = glfwGetTime() - g_RotateTime;
        float quat[4];
        AxisAngletoQuat(quat, axis, angle);
        Multi(g_RotateStart, quat, g_Rotation);
        b_rotate = true;
    }
    QuattoMatrix(g_Rotation, rotateMatrix);
    model = glm::make_mat4(rotateMatrix) * model;
    Shader shader = ResourceManager::GetShader("prt");
    shader.Use();
    shader.SetMatrix4("model", model);
    shader.SetMatrix4("view", view);
    shader.SetMatrix4("projection", projection);

    // @TODO: Rotation will cause memory leak!!
    vec3 rotateVector;
    bool b_rotateLight = false;
    float thetatemp;
    //float phitemp;
    if (b_rotate)
    {
        mat4 rM = glm::make_mat4(rotateMatrix);
        vec4 dir = rM * vec4(0.0f, 0.0f, 1.0f, 0.0f);
        rotateVector = vec3(dir.x, dir.y, dir.z);
        rotateVector = glm::clamp(rotateVector, -1.0f, 1.0f);
        thetatemp = acos(rotateVector.z);
        if (dir.x < 0)
            thetatemp = 2 * (float)M_PI - thetatemp;

        b_rotateLight = true;
    }
    if (simpleLight)
    {
        //rotateVector = vec3(light_dir[0],light_dir[1],light_dir[2]);
        rotateVector = vec3(light_dir[2], light_dir[0], light_dir[1]);

        b_rotateLight = true;
    }

    if (b_rotateLight)
    {
        rotateVector = glm::normalize(rotateVector);
        float theta, phi;

        rotateVector[2] = glm::clamp(rotateVector[2], -1.0f, 1.0f);

        theta = acos(rotateVector[2]);
        float sintheta = sin(theta);
        if (fabs(sintheta) < M_ZERO)
        {
            phi = 0.0f;
        }
        else
        {
            float cosphi = rotateVector[0] / sintheta;
            float sinphi = rotateVector[1] / sintheta;
            phi = inverseSC(sinphi, cosphi);
        }
        vector<vec2> rotatePara;
        rotatePara.clear();

        if (simpleLight)
        {
            rotatePara.push_back(vec2(theta, phi));
            simpleL.rotateZYZ(rotatePara);
        }
        if (b_rotate)
        {
            rotatePara.push_back(vec2(0.0f, 2.0f * M_PI - thetatemp));
            //rotatePara.push_back(vec2(3.0f*M_PI/2.0f,M_PI/2.0f));
            lighting[lightingIndex].rotateZYZ(rotatePara);
        }
        if (materialIndex == 0)
        {
            init(&diffObject[objectIndex], &lighting[lightingIndex]);
            initColorBuffer(transferFIndex, vec3(0.0f, 0.0f, 0.0f), true);
        }
        else
        {
            init(&genObject[objectIndex], &lighting[lightingIndex]);
            initColorBuffer(transferFIndex, camera_dis * vec3(camera_pos[0], camera_pos[1], camera_pos[2]),
                            false);
        }
    }

    if (materialIndex == 1)
    {
        if ((last_camera_pos[0] != camera_pos[0]) || (last_camera_pos[1] != camera_pos[1]) || (last_camera_pos[2] !=
            camera_pos[2]))
        {
            init(&genObject[objectIndex], &lighting[lightingIndex]);
            initColorBuffer(transferFIndex, camera_dis * vec3(camera_pos[0], camera_pos[1], camera_pos[2]),
                            false);
            last_camera_pos[0] = camera_pos[0];
            last_camera_pos[1] = camera_pos[1];
            last_camera_pos[2] = camera_pos[2];
        }
    }

    naiveObjRender();

    if (drawCubemap)
    {
        // Render cubemap.
        shader = ResourceManager::GetShader("cubemap");
        shader.Use();
        // Remove translation from the view matrix.
        view = glm::mat4(glm::mat3(view));
        shader.SetMatrix4("view", view);
        shader.SetMatrix4("projection", projection);
        // cubemap[lightingIndex].render();
        // std::cout << "lighting index: " << lightingIndex << std::endl;
        hdrTextures[lightingIndex].Draw();
    }
}
