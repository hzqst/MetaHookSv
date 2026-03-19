# Decal Culling

## Summary

本方案在 `R_DrawDecals` 的单个 decal 循环内，为每个 decal 增加两层可见性裁剪：

- 使用 `R_CullBox(mins, maxs)` 做单 decal 的 AABB Frustum Culling。
- 使用 `PVSNode((*cl_worldmodel)->nodes, mins, maxs)` 做单 decal 的 PVS Culling。

关键约束如下：

- PVS 判断必须发生在绘制阶段，不能提前放进 `R_PrepareDecals`，因为 `visframe` 由 `R_MarkLeaves` 在场景绘制前更新。
- 包围盒以 decal 裁剪后的世界空间顶点为准，不退化为 surface 级粗盒。
- 首次遇到不可见 decal 时，允许先做一次 CPU clipping 生成 bounds；后续帧优先使用缓存的 bounds 直接早退。

## Data Changes

扩展 `CCachedDecal`：

- `mins`
- `maxs`
- `boundsValid`

这些字段与现有 VBO/material 缓存同生命周期，在 `R_ClearDecalCache` 时一并清空。

## Render Flow

`R_DrawDecals` 中每个 decal 的处理顺序调整为：

1. 如果已有有效 bounds，先做 `R_CullBox`。
2. 通过后，再做 `PVSNode` 判定。
3. 如果当前 decal 没有有效 VBO，或者 decal cache 已失效，则重新执行 clip/noclip 顶点生成。
4. 对新生成的顶点计算精确 AABB，并做一个很小的保守扩张，避免平面 decal 因零厚度盒被误剔除。
5. 新 bounds 再次通过 AABB/PVS 后，才允许上传纹理、顶点和索引数据，并设置 `FDECAL_VBO`。
6. 如果本次生成出了有效 bounds 但当前不可见，则保留 bounds，跳过 upload 和 batch，等待后续可见帧再重建。

## Implementation Points

- `Plugins/Renderer/gl_rsurf.cpp`
  - 新增 `R_DecalBoxInPVS(mins, maxs)`。
  - 新增 cached decal bounds 的构建与清理 helper。
  - 调整 `R_IsDecalCacheInvalidated`，当 Renderer 侧 bounds 或 draw range 不完整时也触发重建。
  - 调整 `R_DrawDecals`，把单 decal 的 AABB/PVS early-out 放在 batch 收集之前。

- `Plugins/Renderer/gl_wsurf.h`
  - 扩展 `CCachedDecal` 的 bounds 字段。

- `Plugins/Renderer/gl_wsurf.cpp`
  - `R_ClearDecalCache` 同时清空 `origin/mins/maxs/boundsValid`。

## Verification Checklist

- 视野内 decal 正常显示，普通 clip decal 与 `FDECAL_NOCLIP` decal 都能绘制。
- 视锥外 decal 不进入 draw batch。
- 非当前 PVS 的 decal 即使方向上位于前方，也会被 `PVSNode` 拦截。
- 首帧不可见的 decal 在后续仍不可见时不会反复 upload；重新进入可见区域后能恢复绘制。
- `R_ClearDecalCache` 之后不会复用陈旧 bounds。
