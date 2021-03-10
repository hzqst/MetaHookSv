#ifndef __MODEL__
#define __MODEL__

void Mod_Init(void);
void Mod_ClearAll(void);
model_t *Mod_ForName(const char *name, qboolean crash, qboolean trackCRC);
void *Mod_Extradata(model_t *mod);
void Mod_TouchModel(char *name);

mleaf_t *Mod_PointInLeaf(float *p, model_t *model);
byte *Mod_LeafPVS(mleaf_t *leaf, model_t *model);

#endif