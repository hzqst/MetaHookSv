# Studio Model 平滑法线生成流程

本文档描述了 Renderer 插件中 Studio Model（.mdl 模型）平滑法线（Smooth Normal）的生成过程。

## 概述

平滑法线是一种用于改善模型渲染效果的技术。GoldSrc 引擎原始的 Studio Model 法线是基于面法线计算的，在相邻面之间可能会产生明显的硬边（hard edge）。通过计算平滑法线，可以在渲染时将平滑法线用于法线外扩描边等特效。

## 数据结构

### 顶点数据结构

```cpp
// 基础顶点数据
class studiovertexbase_t
{
public:
    vec3_t  pos{};           // 顶点位置
    vec3_t  normal{};        // 原始法线
    vec2_t  texcoord{};      // 纹理坐标
    byte    packedbone[4]{}; // 骨骼索引（packedbone[0]为顶点骨骼，packedbone[1]为法线骨骼）
};

// TBN（切线、副切线、平滑法线）数据
class studiovertextbn_t
{
public:
    vec3_t  tangent{};       // 切线
    vec3_t  bitangent{};     // 副切线
    vec3_t  smoothnormal{};  // 平滑法线
};
```

### 量化向量（用于哈希查找）

```cpp
class CQuantizedVector
{
public:
    int64_t m_x{};           // 量化后的 X 坐标（原始值 × 1000）
    int64_t m_y{};           // 量化后的 Y 坐标
    int64_t m_z{};           // 量化后的 Z 坐标
    int m_boneindex{ -1 };   // 骨骼索引
};
```

量化向量将浮点坐标转换为整数（精度为 0.001），用于在哈希表中快速查找相同位置的顶点。同时包含骨骼索引，确保不同骨骼上的相同位置顶点不会被错误合并。

### 面法线存储

```cpp
class CFaceNormalStorage
{
public:
    CVector3List weightedNormals;    // 加权法线列表
    float normalTotalFactor{ 0 };    // 权重因子总和
    vec3_t averageNormal{};          // 计算后的平均法线
    
    CVector3List edges;              // 边向量列表（用于处理退化情况）
    vec3_t averageEdge{};            // 平均边向量
    bool bHasAverageEdge{ false };   // 是否使用边向量作为法线
};
```

## 完整流程

### 1. 入口函数

平滑法线的计算在 `R_PrepareSmoothNormalForRenderMesh` 函数中完成：

```
R_PrepareTBNForRenderSubmodel
    └── R_PrepareSmoothNormalForRenderMesh  ← 平滑法线入口
            ├── CalculateFaceNormalHashMap   ← 步骤1：构建面法线哈希表
            ├── CalculateAverageNormal       ← 步骤2：计算平均法线
            └── GetSmoothNormal              ← 步骤3：获取每个顶点的平滑法线
```

### 2. 步骤一：构建面法线哈希表

**函数**: `CalculateFaceNormalHashMap`

遍历所有三角形，为每个顶点收集其所属面的加权法线信息。

#### 2.1 遍历三角形

```cpp
for (int i = 0; i < vIndicesBuffer.size(); i += 3)
{
    // 获取三角形的三个顶点索引（CCW 逆时针顺序）
    int idx0 = vIndicesBuffer[i + 2];
    int idx1 = vIndicesBuffer[i + 1];
    int idx2 = vIndicesBuffer[i + 0];
    
    // 获取顶点位置、法线、骨骼信息
    // ...
}
```

#### 2.2 计算面法线

```cpp
// 计算两条边向量
vec3_t edge1, edge2;
VectorSubtract(vTrianglePos[1], vTrianglePos[0], edge1);
VectorSubtract(vTrianglePos[2], vTrianglePos[0], edge2);

// 叉积得到面法线（长度 = 三角形面积 × 2）
vec3_t faceNormalWeighted;
CrossProduct(edge1, edge2, faceNormalWeighted);
float triangleArea = VectorLength(faceNormalWeighted);

// 归一化得到单位面法线
vec3_t faceNormal;
VectorScale(faceNormalWeighted, 1.0f / triangleArea, faceNormal);
```

#### 2.3 计算顶点角度权重

对于三角形的每个顶点，计算该顶点处的内角作为权重因子：

```cpp
// nextId 用于获取三角形中下一个顶点的索引
int nextId[4] = { 1, 2, 0, 1 };

for (int j = 0; j < 3; j++)
{
    // 计算当前顶点的两条邻边
    vec3_t edgeA, edgeB;
    VectorSubtract(vTrianglePos[nextId[j]], vTrianglePos[j], edgeA);
    VectorSubtract(vTrianglePos[nextId[j + 1]], vTrianglePos[j], edgeB);
    
    // 归一化边向量
    vec3_t edgeANorm, edgeBNorm;
    VectorScale(edgeA, 1.0f / edgeALength, edgeANorm);
    VectorScale(edgeB, 1.0f / edgeBLength, edgeBNorm);
    
    // 计算顶点内角（弧度）
    float dotProduct = DotProduct(edgeANorm, edgeBNorm);
    dotProduct = math_clamp(dotProduct, -1.0f, 1.0f);
    float angle = std::acos(dotProduct);
    
    // 组合权重 = 角度 × 面积
    float combinedWeight = angle * triangleArea;
}
```

**权重算法说明**：

使用**角度-面积加权**（Angle-Area Weighting）方法：
- **角度权重**：顶点内角越大，该面对该顶点法线的贡献越大
- **面积权重**：面积越大的三角形对法线的贡献越大
- 组合权重 = 角度 × 面积

这种方法比简单平均能产生更自然的平滑效果。

#### 2.4 存储到哈希表

```cpp
// 将顶点位置量化为哈希键
CQuantizedVector quantizedVertexPos(vTrianglePos[j], vertbones[j]);

// 计算加权法线
vertex3f_t vWeightedNormal{};
VectorScale(faceNormal, combinedWeight, vWeightedNormal.v);

// 存储到哈希表
auto it = FaceNormalHashMap.find(quantizedVertexPos);
if (it == FaceNormalHashMap.end())
{
    // 创建新条目
    faceNormalStorage = std::make_shared<CFaceNormalStorage>();
    faceNormalStorage->weightedNormals.emplace_back(vWeightedNormal);
    faceNormalStorage->normalTotalFactor += combinedWeight;
    // 同时存储边向量（用于退化情况处理）
    faceNormalStorage->edges.emplace_back(negEdgeA);
    faceNormalStorage->edges.emplace_back(negEdgeB);
    FaceNormalHashMap[quantizedVertexPos] = faceNormalStorage;
}
else
{
    // 累加到现有条目
    faceNormalStorage = it->second;
    faceNormalStorage->weightedNormals.emplace_back(vWeightedNormal);
    faceNormalStorage->normalTotalFactor += combinedWeight;
    faceNormalStorage->edges.emplace_back(negEdgeA);
    faceNormalStorage->edges.emplace_back(negEdgeB);
}
```

### 3. 步骤二：计算平均法线

**函数**: `CalculateAverageNormal`

遍历哈希表中的每个条目，计算该位置的平均法线。

```cpp
for (auto it = FaceNormalHashMap.begin(); it != FaceNormalHashMap.end(); it++)
{
    const auto& FaceNormalStorage = it->second;
    vec3_t averageNormal = { 0, 0, 0 };
    
    // 累加所有加权法线
    for (const auto& weightedNormal : FaceNormalStorage->weightedNormals)
    {
        VectorAdd(averageNormal, weightedNormal.v, averageNormal);
    }
    
    // 归一化
    float averageNormalLength = VectorLength(averageNormal);
    
    if (averageNormalLength < 0.001f)
    {
        // 退化情况：无厚度面片（如双面渲染的面片）
        // 使用边向量的平均值作为法线
        // ...
        FaceNormalStorage->bHasAverageEdge = true;
    }
    else
    {
        VectorScale(averageNormal, 1.0f / averageNormalLength, FaceNormalStorage->averageNormal);
    }
}
```

#### 退化情况处理

当平均法线长度接近零时（通常发生在无厚度的双面面片上，正反面法线相互抵消），使用边向量的平均值作为替代法线：

```cpp
vec3_t averageEdge = { 0, 0, 0 };
for (const auto& edge : FaceNormalStorage->edges)
{
    VectorAdd(averageEdge, edge.v, averageEdge);
}
VectorScale(averageEdge, 1.0f / (float)FaceNormalStorage->edges.size(), averageEdge);

float averageEdgeLength = VectorLength(averageEdge);
if (averageEdgeLength > 0.001f)
{
    VectorScale(averageEdge, 1.0f / averageEdgeLength, FaceNormalStorage->averageEdge);
    FaceNormalStorage->bHasAverageEdge = true;
}
```

### 4. 步骤三：获取每个顶点的平滑法线

**函数**: `GetSmoothNormal`

根据顶点位置从哈希表中查找对应的平均法线，并进行平滑混合。

```cpp
void GetSmoothNormal(
    CStudioModelRenderData* pRenderData,
    const vec3_t VertexPos,
    const vec3_t VertexNorm,
    int vertbone,
    const CFaceNormalHashMap& FaceNormalHashMap,
    vec3_t outNormal)
{
    CQuantizedVector quantizedVertexPos(VertexPos, vertbone);
    auto it = FaceNormalHashMap.find(quantizedVertexPos);
    
    if (it != FaceNormalHashMap.end())
    {
        const auto& FaceNormalStorage = it->second;
        
        // 情况1：退化面片，使用边向量
        if (FaceNormalStorage->bHasAverageEdge)
        {
            VectorCopy(FaceNormalStorage->averageEdge, outNormal);
            return;
        }
        
        // 情况2：检查平均法线与原始法线的差异
        float dotProduct = DotProduct(FaceNormalStorage->averageNormal, VertexNorm);
        
        // 如果夹角超过60度（cos(60°) = 0.5），认为差异过大
        if (dotProduct < 0.5f)
        {
            // 保留原始法线（保持硬边效果）
            VectorCopy(VertexNorm, outNormal);
            return;
        }
        
        // 情况3：使用插值平滑过渡
        float blendFactor = math_clamp((dotProduct - 0.5f) / 0.5f, 0.0f, 1.0f);
        vec3_t lerpResult;
        vec3_lerp(VertexNorm, FaceNormalStorage->averageNormal, blendFactor, lerpResult);
        VectorNormalize(lerpResult);
        VectorCopy(lerpResult, outNormal);
        return;
    }
    
    // 情况4：找不到对应条目，使用原始法线
    VectorCopy(VertexNorm, outNormal);
}
```

#### 平滑混合策略

为了避免在硬边处产生不自然的平滑效果，算法使用了基于角度的混合策略：

| 条件 | 处理方式 |
|------|----------|
| 夹角 > 60° (dot < 0.5) | 保留原始法线，保持硬边 |
| 夹角 ∈ [0°, 60°] | 使用插值混合，blendFactor ∈ [0, 1] |

混合公式：
```
blendFactor = clamp((dotProduct - 0.5) / 0.5, 0, 1)
smoothNormal = lerp(originalNormal, averageNormal, blendFactor)
```

### 5. 应用到顶点缓冲区

最后，将计算出的平滑法线存储到 `vVertexTBNBuffer` 中：

```cpp
for (size_t i = 0; i < vVertexTBNBuffer.size(); ++i)
{
    GetSmoothNormal(
        pRenderData,
        vVertexBaseBuffer[i].pos,
        vVertexBaseBuffer[i].normal,
        vVertexBaseBuffer[i].packedbone[0],
        FaceNormalHashMap,
        vVertexTBNBuffer[i].smoothnormal  // 输出到这里
    );
}
```

## 流程图

```
┌─────────────────────────────────────────────────────────────────────┐
│                    平滑法线生成流程                                   │
└─────────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│  步骤1: CalculateFaceNormalHashMap                                  │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │ 遍历所有三角形                                                  │  │
│  │   ├── 计算面法线                                               │  │
│  │   ├── 计算每个顶点的角度权重                                     │  │
│  │   ├── 组合权重 = 角度 × 面积                                    │  │
│  │   └── 存储加权法线到哈希表 (key = 量化位置 + 骨骼索引)            │  │
│  └───────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│  步骤2: CalculateAverageNormal                                      │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │ 遍历哈希表每个条目                                              │  │
│  │   ├── 累加所有加权法线                                          │  │
│  │   ├── 归一化得到平均法线                                        │  │
│  │   └── 处理退化情况（使用边向量）                                  │  │
│  └───────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│  步骤3: GetSmoothNormal (每个顶点)                                   │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │ 查找哈希表获取平均法线                                          │  │
│  │   ├── 退化情况 → 使用边向量                                     │  │
│  │   ├── 夹角 > 60° → 保留原始法线（硬边）                          │  │
│  │   ├── 夹角 ≤ 60° → 插值混合                                     │  │
│  │   └── 未找到 → 使用原始法线                                     │  │
│  └───────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│  输出: vVertexTBNBuffer[i].smoothnormal                             │
└─────────────────────────────────────────────────────────────────────┘
```

## GPU 使用

计算完成后，平滑法线数据通过 VBO 上传到 GPU：

```cpp
// 上传 TBN 数据到 VBO
m_pRenderData->hVBO[STUDIO_VBO_TBN] = GL_GenBuffer();
GL_UploadDataToVBOStaticDraw(
    m_pRenderData->hVBO[STUDIO_VBO_TBN],
    m_vVertexTBNBuffer.size() * sizeof(studiovertextbn_t),
    m_vVertexTBNBuffer.data()
);

// 设置顶点属性
glVertexAttribPointer(
    STUDIO_VA_SMOOTHNORMAL,  // 属性位置
    3,                       // 分量数
    GL_FLOAT,                // 数据类型
    false,                   // 是否归一化
    sizeof(studiovertextbn_t),
    OFFSET(studiovertextbn_t, smoothnormal)
);
```

在着色器中，平滑法线被用于：
- 法线外扩描边 (Outline)
- 法线外扩发光 (Entity Glow Effects)

## 相关文件

- `Plugins/Renderer/gl_studio.cpp` - 平滑法线计算实现
- `Plugins/Renderer/gl_studio.h` - 顶点数据结构定义
- `Plugins/Renderer/mathlib2.h` - `CQuantizedVector` 和哈希器定义