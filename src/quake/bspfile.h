#pragma once

#include "common/macro.h"

PIM_C_BEGIN

enum
{
    Q_BspVersion = 29,
    Q_ToolVersion = 2,
    Q_MaxKey = 32,
    Q_MaxValue = 1024,
    Q_NumMipLevels = 4,
};

typedef enum
{
    MaxMap_Hulls = 4,
    MaxMap_Models = 256,
    MaxMap_Brushes = 4096,
    MaxMap_Entities = 1024,
    MaxMap_EntString = 65536,

    MaxMap_Planes = 32767,
    MaxMap_Nodes = 32767,
    MaxMap_ClipNodes = 32767,
    MaxMap_Leaves = 8192,
    MaxMap_Verts = 65535,
    MaxMap_Faces = 65535,
    MaxMap_MarkSurfaces = 65535,
    MaxMap_TexInfo = 4096,
    MaxMap_Edges = 256000,
    MaxMap_SurfEdges = 512000,
    MaxMap_Textures = 512,
    MaxMap_MipTex = 0x200000,
    MaxMap_Lighting = 0x100000,
    MaxMap_Visibility = 0x100000,

    MaxMap_Portals = 65536,
} MaxMap;

typedef enum
{
    LumpId_Entities = 0,
    LumpId_Planes,
    LumpId_Textures,
    LumpId_Vertices,
    LumpId_Visibility,
    LumpId_Nodes,
    LumpId_TexInfo,
    LumpId_Faces,
    LumpId_Lighting,
    LumpId_ClipNodes,
    LumpId_Leaves,
    LumpId_MarkSurfaces,
    LumpId_Edges,
    LumpId_SurfEdges,
    LumpId_Models,

    LumpId_COUNT
} LumpId;

typedef struct bsp_lump_s
{
    i32 offset;
    i32 length;
} bsp_lump_t;

typedef struct bsp_header_s
{
    i32 version;
    bsp_lump_t lumps[LumpId_COUNT];
} bsp_header_t;

typedef struct bsp_model_s
{
    float mins[3];
    float maxs[3];
    float origin[3];
    i32 headNode[MaxMap_Hulls];
    i32 visLeaves; // not including the solid leaf 0
    i32 firstFace;
    i32 numFaces;
} bsp_model_t;

typedef struct bsp_miptex_lump_s
{
    i32 count;
    i32 offsets[4];
} bsp_miptex_lump_t;

typedef struct bsp_miptex_s
{
    char name[16];
    u32 width;
    u32 height;
    u32 offsets[Q_NumMipLevels];
} bsp_miptex_t;

typedef struct bsp_vertex_s
{
    float point[3];
} bsp_vertex_t;

typedef enum
{
    // axial planes
    PlaneId_X = 0,
    PlaneId_Y,
    PlaneId_Z,

    // non-axial, snapped to nearest
    PlaneId_AnyX,
    PlaneId_AnyY,
    PlaneId_AnyZ,

    PlaneId_Count
} PlaneId;

typedef struct bsp_plane_s
{
    float normal[3];
    float distance;
    i32 type;   // PlaneId
} bsp_plane_t;

typedef enum
{
    Contents_Empty = -1,
    Contents_Solid = -2,
    Contents_Water = -3,
    Contents_Slime = -4,
    Contents_Lava = -5,
    Contents_Sky = -6,
    Contents_Origin = -7, // removed at csg time
    Contents_Clip = -8,   // changed to Contents_Solid

    Contents_Current_0 = -9,
    Contents_Current_90 = -10,
    Contents_Current_180 = -11,
    Contents_Current_270 = -12,
    Contents_Current_Up = -13,
    Contents_Current_Down = -14,
} Contents;

typedef struct bsp_node_s
{
    i32 planeNum;
    i16 children[2];    // negative numbers are -(leafs+1), not nodes
    i16 mins[3];        // for sphere culling
    i16 maxs[3];
    u16 firstFace;
    u16 numFaces;       // counting both sides
} bsp_node_t;

typedef struct bsp_clipnode_s
{
    i32 planeNum;
    i16 children[2]; // negative numbers are contents
} bsp_clipnode_t;

typedef struct bsp_texinfo_s
{
    float vecs[2][4]; // [s/t][xyz offset]
    i32 mipTex;
    i32 flags;
} bsp_texinfo_t;

#define TexSpecial 1 // sky or slime, no lightmap or 256 subdivision

// edge 0 is never used; negative edge nums are for CCW use of the edge in a face
typedef struct bsp_edge_s
{
    u16 v[2]; // vertex numbers
} bsp_edge_t;

#define MaxLightMaps 4

typedef struct bsp_face_s
{
    i16 planeNum;
    i16 side;

    i32 firstEdge; // must support > 64k edges
    i16 numEdges;
    i16 texInfo;

    u8 styles[MaxLightMaps];
    i32 lightOffset; // start of [numStyles * surfSize] samples
} bsp_face_t;

typedef enum
{
    Ambient_Water = 0,
    Ambient_Sky,
    Ambient_Slime,
    Ambient_Lava,

    Ambient_Count
} Ambient;

typedef struct bsp_leaf_s
{
    i32 contents;
    i32 visOffset;

    i16 mins[3];
    i16 maxs[3];

    u16 firstMarkSurface;
    u16 numMarkSurfaces;

    u8 ambientLevel[Ambient_Count];
} bsp_leaf_t;

typedef struct bsp_pair_s
{
    struct bsp_pair_s* next;
    char* key;
    char* value;
} bsp_pair_t;

// https://quakewiki.org/wiki/Entity_guide
typedef struct bsp_entity_s
{
    float origin[3];
    i32 firstBrush;
    i32 numBrushes;
    struct bsp_pair_s* pairs;
} bsp_entity_t;

typedef struct bsp_mapdata_s
{
    static const i32 AngleUp = -1;
    static const i32 AngleDown = -2;

    i32 numModels;
    bsp_model_t models[MaxMap_Models];

    i32 visDataSize;
    u8 visData[MaxMap_Visibility];

    i32 lightDataSize;
    u8 lightData[MaxMap_Lighting];

    i32 texDataSize;
    u8 texData[MaxMap_MipTex]; // (MipTexLump)

    i32 entDataSize;
    char entData[MaxMap_EntString];

    i32 numLeaves;
    bsp_leaf_t leaves[MaxMap_Leaves];

    i32 numPlanes;
    bsp_plane_t planes[MaxMap_Planes];

    i32 numVertices;
    bsp_vertex_t vertices[MaxMap_Verts];

    i32 numNodes;
    bsp_node_t nodes[MaxMap_Nodes];

    i32 numTexInfo;
    bsp_texinfo_t texInfo[MaxMap_TexInfo];

    i32 numFaces;
    bsp_face_t faces[MaxMap_Faces];

    i32 numClipNodes;
    bsp_clipnode_t clipNodes[MaxMap_ClipNodes];

    i32 numEdges;
    bsp_edge_t edges[MaxMap_Edges];

    i32 numMarkSurfaces;
    u16 markSurfaces[MaxMap_MarkSurfaces];

    i32 numSurfEdges;
    i32 surfEdges[MaxMap_SurfEdges];

    i32 numEntities;
    bsp_entity_t entities[MaxMap_Entities];
} bsp_mapdata_t;

void bsp_decompress_vis(u8* input, u8* output);
i32 bsp_compress_vis(u8* vis, u8* dst);

void bsp_load(const char* filename);
void bsp_write(const char* filename);
void bsp_print_file_sizes(void);

PIM_C_END
