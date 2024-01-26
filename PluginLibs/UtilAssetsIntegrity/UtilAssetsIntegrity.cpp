#include <IUtilAssetsIntegrity.h>

#include <metahook.h>
#include <memory>
#include <stdint.h>

#include <studio.h>

size_t safe_strlen(const char* str, size_t maxChars)
{
	size_t		count;

	count = 0;
	while (str[count] && count < maxChars)
		count++;

	return count;
}

class CUtilAssetsIntegrity : public IUtilAssetsIntegrity
{
private:

public:

	UtilAssetsIntegrityCheckReason CheckStudioModel_Textures(void* buf, size_t bufSize, studiohdr_t* studiohdr, UtilAssetsIntegrityCheckResult* checkResult)
	{
		if (studiohdr->textureindex < 0)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->textureindex (%d).", studiohdr->textureindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (studiohdr->numtextures < 0)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->numtextures (%d).", studiohdr->numtextures);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto ptexture_base = (mstudiotexture_t*)((byte*)studiohdr + studiohdr->textureindex);

		if ((byte*)ptexture_base < (byte*)buf)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptexture_base.");
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto ptexture_end = ptexture_base + studiohdr->numtextures;

		if ((byte*)ptexture_end > (byte*)buf + bufSize)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptexture_end.");
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel_TextureData(void* buf, size_t bufSize, studiohdr_t* studiohdr, UtilAssetsIntegrityCheckResult* checkResult)
	{
		auto ptexture = (mstudiotexture_t*)((byte*)studiohdr + studiohdr->textureindex);

		for (int i = 0; i < studiohdr->numtextures; i++)
		{
			if (ptexture[i].index < 0)
			{
				snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptexture[%d].index (%d).", i, ptexture[i].index);
				return UtilAssetsIntegrityCheckReason::OutOfBound;
			}

			auto pal = (byte*)buf + ptexture[i].index;
			auto palsize = ptexture[i].width * ptexture[i].height;

			if (ptexture[i].width < 0)
			{
				snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptexture[%d].width (%d).", i, ptexture[i].width);
				return UtilAssetsIntegrityCheckReason::OutOfBound;
			}

			if (ptexture[i].height < 0)
			{
				snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptexture[%d].height (%d).", i, ptexture[i].height);
				return UtilAssetsIntegrityCheckReason::OutOfBound;
			}

			if (pal < (byte*)buf)
			{
				snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptexture[%d].", i);
				return UtilAssetsIntegrityCheckReason::OutOfBound;
			}

			if (pal + palsize > (byte*)buf + bufSize)
			{
				snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptexture[%d].", i);
				return UtilAssetsIntegrityCheckReason::OutOfBound;
			}

			if (safe_strlen(ptexture[i].name, sizeof(ptexture[i].name)) >= sizeof(ptexture[i].name))
			{
				snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptexture[%d].name.", i);
				return UtilAssetsIntegrityCheckReason::OutOfBound;
			}
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel_Skins(void* buf, size_t bufSize, studiohdr_t* studiohdr, UtilAssetsIntegrityCheckResult* checkResult)
	{
		if (studiohdr->numskinref < 0)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->numskinref (%d).", studiohdr->numskinref);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (studiohdr->numskinfamilies == 0)
		{
			return UtilAssetsIntegrityCheckReason::OK;
		}

		if (studiohdr->numskinref == 0)
		{
			return UtilAssetsIntegrityCheckReason::OK;
		}

		auto pskinref_base = (short*)((byte*)studiohdr + studiohdr->skinindex);

		if ((byte*)pskinref_base < (byte*)buf)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pskinref_base.");
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto pskinref_end = pskinref_base + (studiohdr->numskinfamilies * studiohdr->numskinref);

		if ((byte*)pskinref_end > (byte*)buf + bufSize)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pskinref_end.");
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		for (int i = 0; i < studiohdr->numskinfamilies; ++i)
		{
			auto pskinref = pskinref_base + i * studiohdr->numskinref;

			for (int j = 0; j < studiohdr->numskinref; ++j)
			{
				auto ref = pskinref[j];

				if (ref < 0 || ref >= studiohdr->numtextures)
				{
					snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pskinref[%d][%d]=%d.", i, j, ref);
					return UtilAssetsIntegrityCheckReason::OutOfBound;
				}
			}
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel_Event(void* buf, size_t bufSize, studiohdr_t* studiohdr, int i, int j, mstudioseqdesc_t* pseqdesc, mstudioevent_t *pevent, UtilAssetsIntegrityCheckResult* checkResult)
	{
		if (safe_strlen(pevent->options, sizeof(pevent->options)) >= sizeof(pevent->options))
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pevent->options.");
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel_SeqDesc(void* buf, size_t bufSize, studiohdr_t* studiohdr, int i, mstudioseqdesc_t *pseqdesc, UtilAssetsIntegrityCheckResult* checkResult)
	{
		if (pseqdesc->numevents < 0)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pseqdesc->numevents (%d).", pseqdesc->numevents);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (pseqdesc->eventindex < 0)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pseqdesc->eventindex (%d).", pseqdesc->eventindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto pevent_base = (mstudioevent_t*)((byte*)studiohdr + pseqdesc->eventindex);

		if ((byte*)pevent_base < (byte*)buf)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pevent_base.");
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto pevent_end = pevent_base + pseqdesc->numevents;

		if ((byte*)pevent_end > (byte*)buf + bufSize)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pevent_end.");
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		for (int j = 0; j < pseqdesc->numevents; ++j)
		{
			auto pevent = pevent_base + j;

			auto r = CheckStudioModel_Event(buf, bufSize, studiohdr, i, j, pseqdesc, pevent, checkResult);

			if (r != UtilAssetsIntegrityCheckReason::OK)
				return r;
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel_Sequences(void* buf, size_t bufSize, studiohdr_t* studiohdr, UtilAssetsIntegrityCheckResult* checkResult)
	{
		if (studiohdr->numseq < 0)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->numseq (%d).", studiohdr->numseq);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}
		if (studiohdr->seqindex < 0)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->seqindex (%d).", studiohdr->seqindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto pseqdesc_base = (mstudioseqdesc_t*)((byte*)studiohdr + studiohdr->seqindex);

		if ((byte*)pseqdesc_base < (byte*)buf)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pseqdesc_base.");
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto pseqdesc_end = pseqdesc_base + studiohdr->numseq;

		for (int i = 0; i < studiohdr->numseq; ++i)
		{
			auto pseqdesc = pseqdesc_base + i;

			auto r = CheckStudioModel_SeqDesc(buf, bufSize, studiohdr, i, pseqdesc, checkResult);

			if (r != UtilAssetsIntegrityCheckReason::OK)
				return r;
		}

		return UtilAssetsIntegrityCheckReason::OK;

	}

	UtilAssetsIntegrityCheckReason CheckStudioModel_Mesh(void* buf, size_t bufSize, studiohdr_t* studiohdr, int i, int j, int k, mstudiomodel_t* psubmodel, mstudiomesh_t*pmesh, UtilAssetsIntegrityCheckResult* checkResult)
	{
		auto pstudioverts = (vec3_t*)((byte*)studiohdr + psubmodel->vertindex);
		auto pstudionorms = (vec3_t*)((byte*)studiohdr + psubmodel->normindex);
		auto pvertbone = ((byte*)studiohdr + psubmodel->vertinfoindex);
		auto pnormbone = ((byte*)studiohdr + psubmodel->norminfoindex);

		if (pmesh->triindex < 0)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pmesh->triindex (%d).", pmesh->triindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto ptricmds = (short*)((byte*)studiohdr + pmesh->triindex);

		if ((byte*)(ptricmds + 1) > (byte*)buf + bufSize)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pmesh->triindex (%d).", pmesh->triindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		int t;
		while (t = *(ptricmds++))
		{
			if (t < 0)
			{
				t = -t;
				//GL_TRIANGLE_FAN;
				for (; t > 0; t--, ptricmds += 4)
				{
					if ((byte*)(ptricmds + 4) > (byte*)buf + bufSize)
					{
						snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pmesh->triindex (%d).", pmesh->triindex);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}

					auto vertindex = ptricmds[0];
					auto normindex = ptricmds[1];

					if (vertindex < 0)
					{
						snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptricmds[0] (%d).", ptricmds[0]);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}

					if (normindex < 0)
					{
						snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptricmds[1] (%d).", ptricmds[1]);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}

					if ((byte*)(pstudioverts + vertindex) > (byte*)buf + bufSize)
					{
						snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptricmds[0] (%d).", ptricmds[0]);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}
					if ((byte*)(pvertbone + vertindex) > (byte*)buf + bufSize)
					{
						snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptricmds[0] (%d).", ptricmds[0]);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}
					if ((byte*)(pstudionorms + normindex) > (byte*)buf + bufSize)
					{
						snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptricmds[1] (%d).", ptricmds[1]);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}
					if ((byte*)(pnormbone + normindex) > (byte*)buf + bufSize)
					{
						snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptricmds[1] (%d).", ptricmds[1]);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}
				}
			}
			else
			{
				//GL_TRIANGLE_STRIP;
				for (; t > 0; t--, ptricmds += 4)
				{
					if ((byte*)(ptricmds + 4) > (byte*)buf + bufSize)
					{
						snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pmesh->triindex (%d).", pmesh->triindex);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}

					auto vertindex = ptricmds[0];
					auto normindex = ptricmds[1];

					if (vertindex < 0)
					{
						snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptricmds[0] (%d).", ptricmds[0]);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}

					if (normindex < 0)
					{
						snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptricmds[1] (%d).", ptricmds[1]);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}

					if ((byte*)(pstudioverts + vertindex) > (byte*)buf + bufSize)
					{
						snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptricmds[0] (%d).", ptricmds[0]);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}
					if ((byte*)(pvertbone + vertindex) > (byte*)buf + bufSize)
					{
						snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptricmds[0] (%d).", ptricmds[0]);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}
					if ((byte*)(pstudionorms + normindex) > (byte*)buf + bufSize)
					{
						snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptricmds[1] (%d).", ptricmds[1]);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}
					if ((byte*)(pnormbone + normindex) > (byte*)buf + bufSize)
					{
						snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptricmds[1] (%d).", ptricmds[1]);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}
				}
			}
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel_Submodel(void* buf, size_t bufSize, studiohdr_t* studiohdr, int i, int j, mstudiomodel_t *psubmodel, UtilAssetsIntegrityCheckResult* checkResult)
	{
		if (psubmodel->vertindex < 0)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid psubmodel->vertindex (%d).", psubmodel->vertindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (psubmodel->normindex < 0)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid psubmodel->normindex (%d).", psubmodel->normindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (psubmodel->vertinfoindex < 0)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid psubmodel->vertinfoindex (%d).", psubmodel->vertinfoindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (psubmodel->norminfoindex < 0)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid psubmodel->norminfoindex (%d).", psubmodel->norminfoindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (psubmodel->meshindex < 0)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid psubmodel->meshindex (%d).", psubmodel->meshindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (psubmodel->nummesh < 0)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid psubmodel->nummesh (%d).", psubmodel->nummesh);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto pmesh_base = (mstudiomesh_t*)((byte*)studiohdr + psubmodel->meshindex);

		if ((byte*)pmesh_base > (byte*)buf + bufSize)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid psubmodel->meshindex (%d) .", psubmodel->meshindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto pmesh_end = pmesh_base + psubmodel->nummesh;

		if ((byte*)pmesh_end > (byte*)buf + bufSize)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid psubmodel->meshindex (%d) or psubmodel->nummesh (%d).", psubmodel->meshindex, psubmodel->nummesh);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		for (int k = 0; k < psubmodel->nummesh; ++k)
		{
			auto pmesh = pmesh_base + k;

			auto r = CheckStudioModel_Mesh(buf, bufSize, studiohdr, i, j, k, psubmodel, pmesh, checkResult);

			if (r != UtilAssetsIntegrityCheckReason::OK)
				return r;
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel_BodyPart(void* buf, size_t bufSize, studiohdr_t* studiohdr, int i, mstudiobodyparts_t *pbodypart, UtilAssetsIntegrityCheckResult* checkResult)
	{
		if (safe_strlen(pbodypart->name, sizeof(pbodypart->name)) >= sizeof(pbodypart->name))
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pbodypart->name.");
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (pbodypart->modelindex < 0)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pbodypart->modelindex (%d).", pbodypart->modelindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (pbodypart->nummodels < 0)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pbodypart->nummodels (%d).", pbodypart->nummodels);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto psubmodel_base = (mstudiomodel_t*)((byte*)studiohdr + pbodypart->modelindex);
		auto psubmodel_end = psubmodel_base + pbodypart->nummodels;

		if ((byte*)psubmodel_end > (byte*)buf + bufSize)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pbodypart->modelindex (%d) or pbodypart->nummodels (%d).", pbodypart->modelindex, pbodypart->nummodels);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		for (int j = 0; j < pbodypart->nummodels; ++j)
		{
			auto psubmodel = psubmodel_base + j;

			auto r = CheckStudioModel_Submodel(buf, bufSize, studiohdr, i, j, psubmodel, checkResult);

			if (r != UtilAssetsIntegrityCheckReason::OK)
				return r;
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel_BodyParts(void* buf, size_t bufSize, studiohdr_t* studiohdr, UtilAssetsIntegrityCheckResult* checkResult)
	{
		if (studiohdr->bodypartindex < 0)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->bodypartindex (%d).", studiohdr->bodypartindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto pbodypart_base = (mstudiobodyparts_t*)((byte*)studiohdr + studiohdr->bodypartindex);

		if ((byte*)pbodypart_base < (byte*)buf)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->bodypartindex (%d).", studiohdr->bodypartindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto pbodypart_end = pbodypart_base + studiohdr->numbodyparts;

		if ((byte*)pbodypart_end > (byte*)buf + bufSize)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->bodypartindex (%d) or studiohdr->numbodyparts (%d).", studiohdr->bodypartindex, studiohdr->numbodyparts);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		for (int i = 0; i < studiohdr->numbodyparts; ++i)
		{
			auto pbodypart = pbodypart_base + i;

			auto r = CheckStudioModel_BodyPart(buf, bufSize, studiohdr, i, pbodypart, checkResult);
			if (r != UtilAssetsIntegrityCheckReason::OK)
				return r;
			
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel_IDST(void* buf, size_t bufSize, UtilAssetsIntegrityCheckResult* checkResult)
	{
		auto studiohdr = (studiohdr_t*)buf;

		if (studiohdr->version != 10)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid version number, expect %d, got %d.", 10, studiohdr->version);
			return UtilAssetsIntegrityCheckReason::VersionMismatch;
		}

		if (studiohdr->texturedataindex < 0)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->texturedataindex (%d).", studiohdr->texturedataindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		int system_memory_length = 0;

		if (studiohdr->textureindex)
			system_memory_length = studiohdr->texturedataindex;
		else
			system_memory_length = studiohdr->length;

		if (system_memory_length > bufSize)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->length (%d).", studiohdr->length);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (studiohdr->texturedataindex && studiohdr->numtextures)
		{
			auto r = CheckStudioModel_TextureData(buf, bufSize, studiohdr, checkResult);
			if (r != UtilAssetsIntegrityCheckReason::OK)
				return r;
		}

		if (studiohdr->textureindex && studiohdr->numtextures)
		{
			auto r = CheckStudioModel_Textures(buf, system_memory_length, studiohdr, checkResult);
			if (r != UtilAssetsIntegrityCheckReason::OK)
				return r;
		}

		if (studiohdr->numskinfamilies && studiohdr->numskinref)
		{
			auto r = CheckStudioModel_Skins(buf, system_memory_length, studiohdr, checkResult);
			if (r != UtilAssetsIntegrityCheckReason::OK)
				return r;
		}

		if (studiohdr->bodypartindex && studiohdr->numbodyparts)
		{
			auto r = CheckStudioModel_BodyParts(buf, system_memory_length, studiohdr, checkResult);
			if (r != UtilAssetsIntegrityCheckReason::OK)
				return r;
		}

		if (studiohdr->seqindex && studiohdr->numseq)
		{
			auto r = CheckStudioModel_Sequences(buf, system_memory_length, studiohdr, checkResult);
			if (r != UtilAssetsIntegrityCheckReason::OK)
				return r;
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel_IDSQ(void* buf, size_t bufSize, UtilAssetsIntegrityCheckResult* checkResult)
	{
		auto studiohdr = (studiohdr_t*)buf;

		if (studiohdr->version != 10)
		{
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid version number, expect %d, got %d.", 10, studiohdr->version);
			return UtilAssetsIntegrityCheckReason::VersionMismatch;
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel(void* buf, size_t bufSize, UtilAssetsIntegrityCheckResult* checkResult) override
	{
		if (bufSize < sizeof(studiohdr_t))
		{
			const char* pbuf = (const char*)buf;
			snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel file too small, file size %d < %d.", bufSize, sizeof(studiohdr_t));

			return UtilAssetsIntegrityCheckReason::SizeTooSmall;
		}

		if (0 == memcmp(buf, "IDSQ", 4))
		{
			return CheckStudioModel_IDSQ(buf, bufSize, checkResult);
		}

		if (0 == memcmp(buf, "IDST", 4))
		{
			return CheckStudioModel_IDST(buf, bufSize, checkResult);
		}

		const char* pbuf = (const char*)buf;
		snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has bogus header, expect IDST/IDSQ, got %c%c%c%c.", pbuf[0], pbuf[1], pbuf[2], pbuf[3]);

		return UtilAssetsIntegrityCheckReason::BogusHeader;
	}
};

EXPOSE_SINGLE_INTERFACE(CUtilAssetsIntegrity, IUtilAssetsIntegrity, UTIL_ASSETS_INTEGRITY_INTERFACE_VERSION);