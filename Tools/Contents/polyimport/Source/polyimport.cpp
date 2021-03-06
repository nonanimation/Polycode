
#include "polyimport.h"
#include "OSBasics.h"
#include "PolyObject.h"

#include "physfs.h"
#ifdef WIN32
#include "getopt.h"
#define getopt getopt_a
#else
#include <unistd.h>
#endif

using namespace Polycode;

const struct aiScene* scene = NULL;
bool hasWeights = false;
vector<aiBone*> bones;
unsigned int numBones = 0;


bool writeNormals = false;
bool writeTangents = false;
bool writeColors = false;
bool writeBoneWeights = false;
bool writeUVs = false;
bool writeSecondaryUVs = false;


unsigned int addBone(aiBone *bone) {
	for(int i=0; i < bones.size(); i++) {
		if(bones[i]->mName == bone->mName)
			return i;
	}
	bones.push_back(bone);
	return bones.size()-1;
}

void addToMesh(String prefix, Polycode::Mesh *tmesh, const struct aiScene *sc, const struct aiNode* nd, bool swapZY, bool addSubmeshes, bool listOnly, ObjectEntry *parentSceneObject) {
	int i, nIgnoredPolygons = 0;
	unsigned int n = 0, t;
	// draw all meshes assigned to this node

	for (; n < nd->mNumMeshes; ++n) {
	
		if(!addSubmeshes) {
			tmesh = new Polycode::Mesh(Mesh::TRI_MESH);
            tmesh->indexedMesh = true;
		}
	
		const struct aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];
        
        Vector3 bBox;
        
		if(listOnly) {
			if(!addSubmeshes) {
				printf("%s%s.mesh\n", prefix.c_str(), nd->mName.data);
			}
		} else {
			printf("Importing mesh:%s (%d vertices) (%d faces) \n", mesh->mName.data, mesh->mNumVertices, mesh->mNumFaces);
		}
		//apply_material(sc->mMaterials[mesh->mMaterialIndex]);
        
		for (t = 0; t < mesh->mNumVertices; ++t) {
            Vertex *vertex = new Vertex();
            int index = t;
            if(mesh->mColors[0] != NULL) {
                vertex->vertexColor.setColorRGBA(mesh->mColors[0][index].r, mesh->mColors[0][index].g, mesh->mColors[0][index].b, mesh->mColors[0][index].a);
            }

            if(mesh->mTangents != NULL)  {
                if(swapZY)
                    vertex->tangent = Vector3(mesh->mTangents[index].x, mesh->mTangents[index].z, -mesh->mTangents[index].y);
                else
                    vertex->tangent = Vector3(mesh->mTangents[index].x, mesh->mTangents[index].y, mesh->mTangents[index].z);
            }

            if(mesh->mNormals != NULL)  {
                if(swapZY)
                    vertex->setNormal(mesh->mNormals[index].x, mesh->mNormals[index].z, -mesh->mNormals[index].y);
                else
                    vertex->setNormal(mesh->mNormals[index].x, mesh->mNormals[index].y, mesh->mNormals[index].z);
            }

            if(mesh->HasTextureCoords(0))
            {
                vertex->setTexCoord(mesh->mTextureCoords[0][index].x, mesh->mTextureCoords[0][index].y);
            }

            if(mesh->HasTextureCoords(1))
            {
                vertex->setSecondaryTexCoord(mesh->mTextureCoords[1][index].x, mesh->mTextureCoords[1][index].y);
            }

            
            for( unsigned int a = 0; a < mesh->mNumBones; a++) {
                aiBone* bone = mesh->mBones[a];
                unsigned int boneIndex = addBone(bone);

                for( unsigned int b = 0; b < bone->mNumWeights; b++) {
                    if(bone->mWeights[b].mVertexId == index) {
                        vertex->addBoneAssignment(boneIndex, bone->mWeights[b].mWeight);
                        hasWeights = true;
                    }
                }
            }

            if(swapZY) {
					vertex->set(mesh->mVertices[index].x, mesh->mVertices[index].z, -mesh->mVertices[index].y);
            } else {
                vertex->set(mesh->mVertices[index].x, mesh->mVertices[index].y, mesh->mVertices[index].z);
            }
            
            if(fabs(vertex->x) > bBox.x) {
                bBox.x = vertex->x;
            }
            if(fabs(vertex->y) > bBox.y) {
                bBox.y = vertex->y;
            }
            if(fabs(vertex->z) > bBox.z) {
                bBox.z = vertex->z;
            }
            
            tmesh->addVertex(vertex);
		}
        

         for (t = 0; t < mesh->mNumFaces; ++t) {
             const struct aiFace* face = &mesh->mFaces[t];
			 if (face->mNumIndices != 3) {
				 nIgnoredPolygons++;
				 continue;
			 }

             for(i = 0; i < face->mNumIndices; i++) {
                 int index = face->mIndices[i];
                 tmesh->addIndex(index);
             }
         }
		
		if(!addSubmeshes && !listOnly) {
			String fileNameMesh = prefix+String(nd->mName.data)+".mesh";			
			OSFILE *outFile = OSBasics::open(fileNameMesh.c_str(), "wb");	
			tmesh->saveToFile(outFile, writeNormals, writeTangents, writeColors, writeBoneWeights, writeUVs, writeSecondaryUVs);
			OSBasics::close(outFile);
			delete tmesh;
            
            ObjectEntry *meshEntry = parentSceneObject->addChild("child");
            meshEntry->addChild("id", String(nd->mName.data));
            meshEntry->addChild("tags", "");
            meshEntry->addChild("type", "SceneMesh");
            meshEntry->addChild("cR", "1");
            meshEntry->addChild("cG", "1");
            meshEntry->addChild("cB", "1");
            meshEntry->addChild("cA", "1");
            meshEntry->addChild("blendMode", "0");
            
            aiVector3D p;
            aiVector3D s;
            aiQuaternion r;
            nd->mTransformation.Decompose(s, r, p);
            
            meshEntry->addChild("sX", s.x);
            meshEntry->addChild("sY", s.y);
            meshEntry->addChild("sZ", s.z);

            meshEntry->addChild("rX", r.x);
            meshEntry->addChild("rY", r.y);
            meshEntry->addChild("rZ", r.z);
            meshEntry->addChild("rW", r.w);
            
            meshEntry->addChild("pX", p.x);
            meshEntry->addChild("pY", p.y);
            meshEntry->addChild("pZ", p.z);
            
            meshEntry->addChild("bbX", bBox.x);
            meshEntry->addChild("bbY", bBox.y);
            meshEntry->addChild("bbZ", bBox.z);
            
            ObjectEntry *sceneMeshEntry = meshEntry->addChild("SceneMesh");
            sceneMeshEntry->addChild("file", fileNameMesh);
            
            String materialName = "Default";
            int materialIndex = mesh->mMaterialIndex;
            if(materialIndex < scene->mNumMaterials) {
                aiString name;
                scene->mMaterials[materialIndex]->Get(AI_MATKEY_NAME,name);
                if(name.length > 0) {
                    materialName = String(name.data);
                }
            }
            sceneMeshEntry->addChild("material", materialName);
		}
		if (nIgnoredPolygons) {
			printf("Ignored %d non-triangular polygons\n", nIgnoredPolygons);
		}
	}
	   
	// draw all children
	for (n = 0; n < nd->mNumChildren; ++n) {
		addToMesh(prefix, tmesh, sc, nd->mChildren[n], swapZY, addSubmeshes, listOnly, parentSceneObject);
	}
}

int getBoneID(aiString name) {
	for(int i=0; i  < bones.size(); i++) {
		if(bones[i]->mName == name) {
			return i;
		}
	}
	return 666;
}

void addToISkeleton(ISkeleton *skel, IBone *parent, const struct aiScene *sc, const struct aiNode* nd) {
	IBone *bone = new IBone();
	bone->parent = parent;
	bone->name = nd->mName;
	bone->t = nd->mTransformation;

	for(int i=0; i < bones.size(); i++) {
		if(bones[i]->mName == bone->name) {
			bone->bindMatrix = bones[i]->mOffsetMatrix;
		}
	}

	for (int n = 0; n < nd->mNumChildren; ++n) {
		addToISkeleton(skel, bone, sc, nd->mChildren[n]);
	}
	skel->addIBone(bone, getBoneID(bone->name));
}

int exportToFile(String prefix, bool swapZY, bool addSubmeshes, bool listOnly, bool exportEntity) {

    Object sceneObject;
    sceneObject.root.name = "entity";
    ObjectEntry *parentEntry = sceneObject.root.addChild("root");
    
    parentEntry->addChild("id", "");
    parentEntry->addChild("tags", "");
    parentEntry->addChild("type", "Entity");
    parentEntry->addChild("cR", "1");
    parentEntry->addChild("cG", "1");
    parentEntry->addChild("cB", "1");
    parentEntry->addChild("cA", "1");
    parentEntry->addChild("blendMode", "0");
    parentEntry->addChild("sX", 1.0);
    parentEntry->addChild("sY", 1.0);
    parentEntry->addChild("sZ", 1.0);
    
    parentEntry->addChild("rX", 0.0);
    parentEntry->addChild("rY", 0.0);
    parentEntry->addChild("rZ", 0.0);
    parentEntry->addChild("rW", 1.0);
    
    parentEntry->addChild("pX", 0.0);
    parentEntry->addChild("pY", 0.0);
    parentEntry->addChild("pZ", 0.0);
    
    parentEntry->addChild("bbX", 0.0);
    parentEntry->addChild("bbY", 0.0);
    parentEntry->addChild("bbZ", 0.0);
    
    ObjectEntry *children = parentEntry->addChild("children");

		
	Polycode::Mesh *mesh = new Polycode::Mesh(Mesh::TRI_MESH);
    mesh->indexedMesh = true;
	addToMesh(prefix, mesh, scene, scene->mRootNode, swapZY, addSubmeshes, listOnly, children);
    
	
	if(addSubmeshes) {
		String fileNameMesh;
		if(prefix != "") {
			fileNameMesh = prefix+".mesh";			
		} else {
			fileNameMesh = "out.mesh";
		}		

		if(listOnly) {
			printf("%s\n", fileNameMesh.c_str());
		} else {
			OSFILE *outFile = OSBasics::open(fileNameMesh.c_str(), "wb");	
			mesh->saveToFile(outFile, writeNormals, writeTangents, writeColors, writeBoneWeights, writeUVs, writeSecondaryUVs);
			OSBasics::close(outFile);
		}
	}
		
	if(hasWeights) {		
		if(listOnly) {
			printf("%s.skeleton\n", prefix.c_str());
		} else {
			printf("Mesh has weights, exporting skeleton...\n");
		}	
		
		String fileNameSkel;
        if(prefix != "") {
            fileNameSkel = prefix+".skeleton";
        } else {
            fileNameSkel = "out.skeleton";
        }
		ISkeleton *skeleton = new ISkeleton();
	
		for (int n = 0; n < scene->mRootNode->mNumChildren; ++n) {
			if(scene->mRootNode->mChildren[n]->mNumChildren > 0) {
				addToISkeleton(skeleton, NULL, scene, scene->mRootNode->mChildren[n]);
			}
		}

		if(scene->HasAnimations()) {
			printf("Importing animations...\n");
			for(int i=0; i < scene->mNumAnimations;i++) {
				aiAnimation *a = scene->mAnimations[i];
				
				if(listOnly) {
					printf("%s%s.anim\n", prefix.c_str(), a->mName.data);
				} else {
					printf("Importing '%s' (%d tracks)\n", a->mName.data, a->mNumChannels);					
				}	
				
			
				IAnimation *anim = new IAnimation();
				anim->tps = a->mTicksPerSecond;
				anim->name = a->mName.data;
				anim->numTracks = a->mNumChannels;
				anim->length = a->mDuration/a->mTicksPerSecond;
	
				for(int c=0; c < a->mNumChannels; c++) {
					aiNodeAnim *nodeAnim = a->mChannels[c];

					ITrack *track = new ITrack();
					track->nodeAnim = nodeAnim;
					anim->tracks.push_back(track);
				}


				skeleton->addAnimation(anim);
				
			}
		} else {
			printf("No animations in file...\n");
		}

		if(!listOnly) {
			skeleton->saveToFile(fileNameSkel.c_str(), swapZY);
		}
	} else {
		if(!listOnly) {
			printf("No weight data, skipping skeleton export...\n");
		}
	}
    
    if(!listOnly && exportEntity) {
		String entityFileName;
		if(prefix != "") {
			entityFileName = prefix+".entity";
		} else {
			entityFileName = "out.entity";
		}        sceneObject.saveToXML(entityFileName);
    }

	if(mesh) {
		delete mesh;
	}
	return 1;
}

int main(int argc, char **argv) {


	bool argsValid = true;
	bool showHelp = false;

	bool swapZYAxis = false;
	bool generateTangents = false;
	bool addSubmeshes = false;
	bool listOnly = false;
	bool showAssimpDebug = false;
    bool generateNormals = false;
    bool exportEntity = false;
	   
	String prefix;
	
	int opt;
	while ((opt = getopt(argc, argv, "engcwuvadlhp:stm")) != -1) {
		switch ((char)opt) {
            case 'e':
                exportEntity = true;
            break;
            case 'n':
                writeNormals = true;
            break;
            case 'g':
                writeTangents = true;
            break;
            case 'c':
                writeColors = true;
            break;
            case 'w':
                writeBoneWeights = true;
            break;
            case 'u':
                writeUVs = true;
            break;
            case 'v':
                writeSecondaryUVs = true;
            break;
			case 's':
				swapZYAxis = true;
			break;
			case 't':
				generateTangents = true;
			break;
			case 'm':
				generateNormals = true;
                break;
			case 'a':
				addSubmeshes = true;
			break;
			case 'd':
				showAssimpDebug = true;
			break;				
			case 'l':
				listOnly = true;
			break;				
			case 'p':
				prefix = String(optarg);
			break;
			case 'h':
				showHelp = true;
			break;			
			default:
				argsValid = false;
			break;
		}
	}

	if(listOnly && argc < 3) {
		argsValid = false;
	}
		
	if(!listOnly) {
		printf("Polycode import tool v"POLYCODE_VERSION_STRING"\n");	
	}
	
	if(!argsValid) {
		printf("Invalid arguments! Run with -h to see available options.\n\n");
		return 0;		
	}
	
	if(showHelp || argc < 2) {
		printf("usage: polyimport [-adhlstngcwuvme] [-p output_prefix] source_file\n\n");
		printf("Misc options:\n");
		printf("d: Show Assimp debug info.\n");
		printf("h: Show this help.\n");
		printf("l: List output files, but do not convert.\n");
		printf("p: Specify a file prefix for exported files.\n\n");
		printf("Mesh import options:\n");
		printf("a: Add all meshes to a single mesh.\n");
		printf("s: Swap Z/Y axis (e.g. import from Blender)\n");
		printf("m: Generate normals.\n");
		printf("t: Generate tangents.\n\n");
		printf("Mesh export options:\n");
		printf("n: Export normals\n");
		printf("g: Export tangents\n");
		printf("c: Export colors\n");
		printf("w: Export bone weights\n");
		printf("u: Export UV coordinates\n");
		printf("v: Export secondary UV coordinates\n");
		printf("e: Export entity scene\n");
		printf("\n");
		return 0;
	}
	
	PHYSFS_init(argv[0]);
	
	if(showAssimpDebug) {
		struct aiLogStream stream;
		stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT,NULL);
		aiAttachLogStream(&stream);
	}
		
	int inputArg = argc-1;

	if(!listOnly) {
		printf("Loading %s...\n", argv[inputArg]);
	}
	
	scene = aiImportFile(argv[inputArg], aiProcess_JoinIdenticalVertices|
                         aiProcess_Triangulate);
    
    if(scene) {

        if(generateTangents && !listOnly) {
            aiApplyPostProcessing(scene, aiProcess_CalcTangentSpace);
        }
        
        if(generateNormals && !listOnly) {
            aiApplyPostProcessing(scene, aiProcess_GenSmoothNormals);
        }
        
		exportToFile(prefix, swapZYAxis, addSubmeshes, listOnly, exportEntity);
	} else {
		printf("Error opening scene (%s)\n", aiGetErrorString());
	}

	aiReleaseImport(scene);
	return 1;
}
