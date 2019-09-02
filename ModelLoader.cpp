#include "stdafx.h"
#include "ModelLoader.h"

#include <assimp/material.h>
#include <assimp/matrix4x4.h>
#include <assimp/vector3.h>
#include <assimp/quaternion.h>

#include <IL\il.h>
#include <atlstr.h>

using std::make_shared;
using std::static_pointer_cast;

using glm::mat4;
using glm::vec3;
using glm::dquat;

namespace Jarvis
{
  ModelLoader::ModelLoader()
  {
    ilInit();
  }


  ModelLoader::~ModelLoader()
  {
  }

  shared_ptr<Entity> ModelLoader::loadAssimpModel(string filename)
  {
    Assimp::Importer importer;
    shared_ptr<Entity> rootEntity = NULL;
    shared_ptr<Mesh> rlMesh;
    shared_ptr<Material> rlMaterial;
    shared_ptr<Renderable> renderable;

    //const aiScene* scene = importer.ReadFile(filename.c_str(),
    //  aiProcess_CalcTangentSpace |
    //  aiProcess_Triangulate |
    //  aiProcess_JoinIdenticalVertices |
    //  aiProcess_SortByPType |
    //  aiProcess_GenSmoothNormals);

    uint32_t AssimpFlags = aiProcessPreset_TargetRealtime_MaxQuality |
      aiProcess_OptimizeGraph |
      aiProcess_CalcTangentSpace |
      aiProcess_MakeLeftHanded |
      //aiProcess_FlipUVs |
      // aiProcess_FixInfacingNormals | // causes incorrect facing normals for crytek-sponza
      0;
    //AssimpFlags &= ~(aiProcess_CalcTangentSpace);

    const aiScene* scene = importer.ReadFile(filename.c_str(), AssimpFlags);

    // If the import failed, report it
    if (!scene)
    {
      const char* error = importer.GetErrorString();
      return NULL;
    }

    uint32_t numMeshes = scene->mNumMeshes;
    uint32_t numMaterials = scene->mNumMaterials;

    m_materials.resize(numMaterials);
    for (uint32_t i = 0; i < numMaterials; ++i)
    {
      int texIndex = 0;
      aiString texturePath;
      aiMaterial* material = scene->mMaterials[i];

      if (material->GetTexture(aiTextureType_DIFFUSE, texIndex, &texturePath) == AI_SUCCESS)
      {
        int normalIndex = 0;
        aiString normalPath;
        if (material->GetTexture(aiTextureType_HEIGHT, normalIndex, &normalPath) == AI_SUCCESS && texturePath != normalPath)
        {
          rlMaterial = make_shared<Material>("ModelMaterial", Material::LIT);
        }
        else
        {
          rlMaterial = make_shared<Material>("ModelMaterial", Material::LIT);
        }
      }
      else
      {
        rlMaterial = make_shared<Material>("ModelMaterial", Material::LIT_NOTEXTURE);
        if (material->GetTexture(aiTextureType_HEIGHT, texIndex, &texturePath) == AI_SUCCESS)
        {
          printLog("UNLIT NORMAL MAP");
        }
      }
      m_materials[i] = rlMaterial;
      populateMaterial(scene, i, rlMaterial);
    }

    rootEntity = make_shared<Entity>(scene->mRootNode->mName.C_Str());
    if (scene->mRootNode->mNumMeshes > 0)
    {
      // Create Entity for this node
      renderable = make_shared<Renderable>(scene->mRootNode->mName.C_Str());

      for (unsigned int i = 0; i<scene->mRootNode->mNumMeshes; i++)
      {
        aiMesh* mesh = scene->mMeshes[scene->mRootNode->mMeshes[i]];
        unsigned int numVerts = mesh->mNumVertices;
        unsigned int numBuffers = 1;

        if (mesh->HasNormals())
        {
          numBuffers++;
        }

        if (mesh->HasTextureCoords(0))
        {
          numBuffers++;
        }

        if (mesh->HasTangentsAndBitangents())
        {
          numBuffers += 2;
        }

        rlMesh = make_shared<Mesh>(scene->mRootNode->mName.C_Str(), Mesh::TRIANGLES, numVerts, numBuffers);
        rlMaterial = m_materials[mesh->mMaterialIndex];

        populateMesh(scene, rlMesh, mesh);
        rlMesh->setMaterial(rlMaterial);
        renderable->addGeometry(rlMesh);
      }
      rootEntity->addComponent(renderable);
    }
    mat4 transform = glm::make_mat4((float*)&(scene->mRootNode->mTransformation));
    rootEntity->setTransform(transform);

    // Process the children
    for (unsigned int i = 0; i<scene->mRootNode->mNumChildren; i++)
    {
      processNode(scene, rootEntity, scene->mRootNode->mChildren[i]);
    }

    printLog("Done Loaded Mesh");

    return (rootEntity);
  }

  void ModelLoader::processNode(const aiScene* scene, shared_ptr<Entity> parent, aiNode* node)
  {
    shared_ptr<Entity> entity = NULL;
    shared_ptr<Mesh> rlMesh;
    shared_ptr<Material> rlMaterial;
    shared_ptr<Renderable> renderable;

    entity = make_shared<Entity>(node->mName.C_Str());
    if (node->mNumMeshes > 0)
    {
      renderable = make_shared<Renderable>(node->mName.C_Str());

      for (unsigned int i = 0; i<node->mNumMeshes; i++)
      {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        unsigned int numVerts = mesh->mNumVertices;
        unsigned int numBuffers = 1;

        if (mesh->HasNormals())
        {
          numBuffers++;
        }

        if (mesh->HasTextureCoords(0))
        {
          numBuffers++;
        }

        if (mesh->HasTangentsAndBitangents())
        {
          numBuffers += 2;
        }

        rlMesh = make_shared<Mesh>(node->mName.C_Str(), Mesh::TRIANGLES, numVerts, numBuffers);
        rlMaterial = m_materials[mesh->mMaterialIndex];

        populateMesh(scene, rlMesh, mesh);
        rlMesh->setMaterial(rlMaterial);
        renderable->addGeometry(rlMesh);
      }
      entity->addComponent(renderable);
    }
    mat4 transform = glm::make_mat4((float*)&(node->mTransformation));
    entity->setTransform(transform);
    parent->addChild(entity);

    // Process the children
    for (unsigned int i = 0; i<node->mNumChildren; i++)
    {
      processNode(scene, entity, node->mChildren[i]);
    }
  }

  void ModelLoader::populateMesh(const aiScene* scene, shared_ptr<Mesh> rlMesh, aiMesh* mesh)
  {
    unsigned int numFaces = mesh->mNumFaces;
    unsigned int numVerts = mesh->mNumVertices;
    unsigned int bufferIndex = 0;

    float* verts = new float[numVerts * 3];
    unsigned int vindex = 0;
    for (unsigned int i = 0; i<numVerts; i++)
    {
      verts[vindex++] = mesh->mVertices[i].x;
      verts[vindex++] = mesh->mVertices[i].y;
      verts[vindex++] = mesh->mVertices[i].z;
    }
    rlMesh->addVertexBuffer(bufferIndex++, 3, sizeof(float)*numVerts * 3, verts);

    if (mesh->HasNormals())
    {
      float* normals = new float[numVerts * 3];
      unsigned int nindex = 0;
      for (unsigned int i = 0; i<numVerts; i++)
      {
        normals[nindex++] = mesh->mNormals[i].x;
        normals[nindex++] = mesh->mNormals[i].y;
        normals[nindex++] = mesh->mNormals[i].z;
      }
      rlMesh->addVertexBuffer(bufferIndex++, 3, sizeof(float)*numVerts * 3, normals);
    }

    if (mesh->HasTextureCoords(0))
    {
      float* texCoords = new float[numVerts * 2];
      unsigned int tindex = 0;
      for (unsigned int i = 0; i<numVerts; i++)
      {
        texCoords[tindex++] = mesh->mTextureCoords[0][i].x;
        texCoords[tindex++] = mesh->mTextureCoords[0][i].y;
      }
      rlMesh->addVertexBuffer(bufferIndex++, 2, sizeof(float)*numVerts * 2, texCoords);
    }

    if (mesh->HasTangentsAndBitangents())
    {
      float* tangents = new float[numVerts * 3];
      float* bitangents = new float[numVerts * 3];
      unsigned int tbindex = 0;
      for (unsigned int i = 0; i<numVerts; i++)
      {
        tangents[tbindex] = mesh->mTangents[i].x;
        bitangents[tbindex++] = mesh->mBitangents[i].x;
        tangents[tbindex] = mesh->mTangents[i].y;
        bitangents[tbindex++] = mesh->mBitangents[i].y;
        tangents[tbindex] = mesh->mTangents[i].z;
        bitangents[tbindex++] = mesh->mBitangents[i].z;
      }
      rlMesh->addVertexBuffer(bufferIndex++, 3, sizeof(float)*numVerts * 3, tangents);
      rlMesh->addVertexBuffer(bufferIndex++, 3, sizeof(float)*numVerts * 3, bitangents);
    }

    unsigned int* indexBuffer = new unsigned int[numFaces * 3];
    unsigned int iindex = 0;
    for (unsigned int i = 0; i<numFaces; i++)
    {
      const struct aiFace* face = &mesh->mFaces[i];
      indexBuffer[iindex++] = face->mIndices[0];
      indexBuffer[iindex++] = face->mIndices[1];
      indexBuffer[iindex++] = face->mIndices[2];
    }
    rlMesh->addIndexBuffer(numFaces * 3, indexBuffer);
  }

  void ModelLoader::populateMaterial(const aiScene* scene, int materialIndex, shared_ptr<Material> rlMaterial)
  {
    aiMaterial* material = scene->mMaterials[materialIndex];

    for (unsigned int i = 0; i < material->mNumProperties;)
    {
      aiMaterialProperty* property = material->mProperties[i];
      i++;
    }

    aiString name;

    glm::vec3 color;
    glm::vec4 color4;
    aiColor3D aColor;
    if (material->Get(AI_MATKEY_COLOR_DIFFUSE, aColor) == AI_SUCCESS)
    {
      color4.r = aColor.r;
      color4.g = aColor.g;
      color4.b = aColor.b;
      color4.a = 1.0f;
      rlMaterial->setAlbedoColor(color4);
    }

    if (material->Get(AI_MATKEY_COLOR_EMISSIVE, aColor) == AI_SUCCESS)
    {
      color.r = aColor.r;
      color.g = aColor.g;
      color.b = aColor.b;
      rlMaterial->setEmissiveColor(color);
    }

    int twoSided;
    if (material->Get(AI_MATKEY_TWOSIDED, twoSided) == AI_SUCCESS)
    {
      rlMaterial->setTwoSided((twoSided == 0 ? false : true));
    }

    int wireframe;
    if (material->Get(AI_MATKEY_ENABLE_WIREFRAME, wireframe) == AI_SUCCESS)
    {
      rlMaterial->setTwoSided((wireframe == 0 ? false : true));
    }

    int texIndex = 0;
    aiString texturePath;
    if (material->GetTexture(aiTextureType_DIFFUSE, texIndex, &texturePath) == AI_SUCCESS)
    {
      shared_ptr<Texture> texture = loadTexture((const char*)texturePath.C_Str());
      rlMaterial->setAlbedoTexture(texture);
      printLog("Loaded Texture: " + std::string(texturePath.C_Str()));
    }

    int normalIndex = 0;
    aiString normalPath;
    if (material->GetTexture(aiTextureType_HEIGHT, normalIndex, &normalPath) == AI_SUCCESS && texturePath != normalPath)
    {
      shared_ptr<Texture> normalTexture = loadTexture((const char*)normalPath.C_Str());
      rlMaterial->setNormalTexture(normalTexture);
      printLog("Loaded Normal Texture: " + std::string(normalPath.C_Str()));
    }

    if (material->GetTexture(aiTextureType_NORMALS, normalIndex, &normalPath) == AI_SUCCESS && texturePath != normalPath)
    {
      shared_ptr<Texture> normalTexture = loadTexture((const char*)normalPath.C_Str());
      rlMaterial->setNormalTexture(normalTexture);
      printLog("Loaded Normal Texture: " + std::string(normalPath.C_Str()));
    }

    //int metallicIndex = 0;
    //aiString metallicPath;
    //if (material->GetTexture(aiTextureType_AMBIENT, metallicIndex, &metallicPath) == AI_SUCCESS && texturePath != metallicPath)
    //{
    //  shared_ptr<Texture> metallicTexture = loadTexture((const char*)metallicPath.C_Str());
    //  rlMaterial->setMetallicTexture(metallicTexture);
    //  printLog("Loaded Metallic Texture: " + std::string(metallicPath.C_Str()));
    //}

    int roughnessIndex = 0;
    aiString roughnessPath;
    if (material->GetTexture(aiTextureType_SHININESS, roughnessIndex, &roughnessPath) == AI_SUCCESS && texturePath != roughnessPath)
    {
      shared_ptr<Texture> roughnessTexture = loadTexture((const char*)roughnessPath.C_Str());
      rlMaterial->setMetallicRoughnessTexture(roughnessTexture);
      printLog("Loaded Roughness Texture: " + std::string(roughnessPath.C_Str()));
    }
  }

  shared_ptr<Texture> ModelLoader::loadTexture(const char* filename)
  {
    ILuint ilDiffuseID;
    ILboolean success;
    shared_ptr<Texture> texture = nullptr;

    map<string, shared_ptr<Texture>>::iterator it = m_textureMap.find(filename);
    if (it != m_textureMap.end())
    {
      //element found;
      return it->second;
    }

    /* generate DevIL Image IDs */
    ilGenImages(1, &ilDiffuseID);
    ilBindImage(ilDiffuseID); /* Binding of DevIL image name */
    success = ilLoadImage((const char*)filename);
    //ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

    if (success) /* If no error occured: */
    {
      success = ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
      if (!success)
      {
        return NULL;
      }

      texture = make_shared<Texture>(filename, ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT), ilGetInteger(IL_IMAGE_DEPTH),
        ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL), ilGetInteger(IL_IMAGE_SIZE_OF_DATA), ilGetInteger(IL_IMAGE_FORMAT));
      texture->setData(ilGetData());

      m_textureMap[filename] = texture;
    }

    return texture;
  }

  void ModelLoader::printLog(string s)
  {
    string st = s + "\n";
    TCHAR name[256];
    _tcscpy_s(name, CA2T(st.c_str()));
    OutputDebugString(name);
  }
}
