#include <IUtilAssetsIntegrity.h>

#include <metahook.h>
#include <memory>
#include <stdint.h>

#include <studio.h>

#include <FreeImage.h>
#include <ScopeExit/ScopeExit.h>

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

	UtilAssetsIntegrityCheckReason CheckStudioModel_Textures(const void* buf, size_t bufSize, studiohdr_t* studiohdr, UtilAssetsIntegrityCheckResult_StudioModel* checkResult)
	{
		if (studiohdr->numtextures < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->numtextures (%d).", studiohdr->numtextures);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (studiohdr->numtextures > MAXSTUDIOSKINS)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->numtextures (%d).", studiohdr->numtextures);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (studiohdr->textureindex < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->textureindex (%d).", studiohdr->textureindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (studiohdr->textureindex > (int)bufSize)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->textureindex (%d).", studiohdr->textureindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto ptexture_base = (mstudiotexture_t*)((byte*)studiohdr + studiohdr->textureindex);

		if ((byte*)ptexture_base < (byte*)buf)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptexture_base.");
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto ptexture_end = ptexture_base + studiohdr->numtextures;

		if ((byte*)ptexture_end > (byte*)buf + bufSize)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptexture_end.");
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel_TextureData(const void* buf, size_t bufSize, studiohdr_t* studiohdr, UtilAssetsIntegrityCheckResult_StudioModel* checkResult)
	{
		auto ptexture = (mstudiotexture_t*)((byte*)studiohdr + studiohdr->textureindex);

		for (int i = 0; i < studiohdr->numtextures; i++)
		{
			if (ptexture[i].index < 0)
			{
				if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptexture[%d].index (%d).", i, ptexture[i].index);
				return UtilAssetsIntegrityCheckReason::OutOfBound;
			}

			if (ptexture[i].index > (int)bufSize)
			{
				if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptexture[%d].index (%d).", i, ptexture[i].index);
				return UtilAssetsIntegrityCheckReason::OutOfBound;
			}

			auto pal = (byte*)buf + ptexture[i].index;
			auto palsize = ptexture[i].width * ptexture[i].height;

			if (ptexture[i].width < 0)
			{
				if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptexture[%d].width (%d).", i, ptexture[i].width);
				return UtilAssetsIntegrityCheckReason::OutOfBound;
			}

			if (ptexture[i].height < 0)
			{
				if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptexture[%d].height (%d).", i, ptexture[i].height);
				return UtilAssetsIntegrityCheckReason::OutOfBound;
			}

			if (pal < (byte*)buf)
			{
				if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptexture[%d].", i);
				return UtilAssetsIntegrityCheckReason::OutOfBound;
			}

			if (pal + palsize > (byte*)buf + bufSize)
			{
				if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptexture[%d].", i);
				return UtilAssetsIntegrityCheckReason::OutOfBound;
			}

			if (safe_strlen(ptexture[i].name, sizeof(ptexture[i].name)) >= sizeof(ptexture[i].name))
			{
				if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptexture[%d].name.", i);
				return UtilAssetsIntegrityCheckReason::OutOfBound;
			}
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel_Skins(const void* buf, size_t bufSize, studiohdr_t* studiohdr, UtilAssetsIntegrityCheckResult_StudioModel* checkResult)
	{
		if (studiohdr->numskinref < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->numskinref (%d).", studiohdr->numskinref);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (studiohdr->numskinfamilies < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->numskinfamilies (%d).", studiohdr->numskinfamilies);
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
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pskinref_base.");
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto pskinref_end = pskinref_base + (studiohdr->numskinfamilies * studiohdr->numskinref);

		if ((byte*)pskinref_end > (byte*)buf + bufSize)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pskinref_end.");
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
					if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pskinref[%d][%d]=%d.", i, j, ref);
					return UtilAssetsIntegrityCheckReason::OutOfBound;
				}
			}
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel_Event(const void* buf, size_t bufSize, studiohdr_t* studiohdr, int i, int j, mstudioseqdesc_t* pseqdesc, mstudioevent_t *pevent, UtilAssetsIntegrityCheckResult_StudioModel* checkResult)
	{
		if (safe_strlen(pevent->options, sizeof(pevent->options)) >= sizeof(pevent->options))
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pevent->options.");
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel_SeqDescEvents(const void* buf, size_t bufSize, studiohdr_t* studiohdr, int i, mstudioseqdesc_t* pseqdesc, UtilAssetsIntegrityCheckResult_StudioModel* checkResult)
	{
		if (pseqdesc->numevents < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pseqdesc->numevents (%d).", pseqdesc->numevents);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (pseqdesc->numevents > MAXSTUDIOEVENTS)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pseqdesc->numevents (%d).", pseqdesc->numevents);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (pseqdesc->eventindex < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pseqdesc->eventindex (%d).", pseqdesc->eventindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (pseqdesc->eventindex > (int)bufSize)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pseqdesc->eventindex (%d).", pseqdesc->eventindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto pevent_base = (mstudioevent_t*)((byte*)studiohdr + pseqdesc->eventindex);

		if ((byte*)pevent_base < (byte*)buf)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pevent_base.");
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto pevent_end = pevent_base + pseqdesc->numevents;

		if ((byte*)pevent_end > (byte*)buf + bufSize)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pevent_end.");
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

	UtilAssetsIntegrityCheckReason CheckStudioModel_SeqDescAnim(const void* buf, size_t bufSize, studiohdr_t* studiohdr, int i, mstudioseqdesc_t* pseqdesc, UtilAssetsIntegrityCheckResult_StudioModel* checkResult)
	{
		if (pseqdesc->animindex < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pseqdesc->animindex (%d).", pseqdesc->animindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (pseqdesc->animindex > (int)bufSize)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pseqdesc->animindex (%d).", pseqdesc->animindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto panim_base = (mstudioanim_t*)((byte*)studiohdr + pseqdesc->animindex);

		if ((byte*)panim_base < (byte*)buf)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid panim_base.");
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto panim_end = panim_base + studiohdr->numbones;

		if ((byte*)panim_end > (byte*)buf + bufSize)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid panim_end.");
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		for (int i = 0; i < studiohdr->numbones; ++i)
		{
			auto panim = panim_base + i;

			for (int j = 0; j < 3; j++)
			{
				if (panim->offset[j + 3])
				{
					mstudioanimvalue_t* panimvalue = (mstudioanimvalue_t*)((byte*)panim + panim->offset[i + 3]);

					if ((byte*)(panimvalue + 255)> (byte*)buf + bufSize)
					{
						if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid panimvalue.");
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}
				}
			}
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel_SeqDesc(const void* buf, size_t bufSize, studiohdr_t* studiohdr, int i, mstudioseqdesc_t *pseqdesc, UtilAssetsIntegrityCheckResult_StudioModel* checkResult)
	{
		if (pseqdesc->numevents)
		{
			auto r = CheckStudioModel_SeqDescEvents(buf, bufSize, studiohdr, i, pseqdesc, checkResult);

			if (r != UtilAssetsIntegrityCheckReason::OK)
				return r;
		}

		if (pseqdesc->seqgroup == 0)
		{
			auto r = CheckStudioModel_SeqDescAnim(buf, bufSize, studiohdr, i, pseqdesc, checkResult);

			if (r != UtilAssetsIntegrityCheckReason::OK)
				return r;
		}

		if (pseqdesc->motionbone < 0 || pseqdesc->motionbone > MAXSTUDIOBONES)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pseqdesc->motionbone (%d).", pseqdesc->motionbone);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel_Sequences(const void* buf, size_t bufSize, studiohdr_t* studiohdr, UtilAssetsIntegrityCheckResult_StudioModel* checkResult)
	{
		if (studiohdr->numseq < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->numseq (%d).", studiohdr->numseq);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (studiohdr->numseq > MAXSTUDIOSEQUENCES_SVENGINE)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->numseq (%d).", studiohdr->numseq);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (studiohdr->seqindex < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->seqindex (%d).", studiohdr->seqindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (studiohdr->seqindex > (int)bufSize)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->seqindex (%d).", studiohdr->seqindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto pseqdesc_base = (mstudioseqdesc_t*)((byte*)studiohdr + studiohdr->seqindex);

		if ((byte*)pseqdesc_base < (byte*)buf)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pseqdesc_base.");
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto pseqdesc_end = pseqdesc_base + studiohdr->numseq;

		if ((byte*)pseqdesc_end > (byte*)buf + bufSize)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pseqdesc_base.");
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		for (int i = 0; i < studiohdr->numseq; ++i)
		{
			auto pseqdesc = pseqdesc_base + i;

			auto r = CheckStudioModel_SeqDesc(buf, bufSize, studiohdr, i, pseqdesc, checkResult);

			if (r != UtilAssetsIntegrityCheckReason::OK)
				return r;
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel_Hitbox(const void* buf, size_t bufSize, studiohdr_t* studiohdr, int i, mstudiobbox_t * pbbox, UtilAssetsIntegrityCheckResult_StudioModel* checkResult)
	{
		if (pbbox->bone < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pbbox->bone (%d).", pbbox->bone);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (pbbox->bone >= studiohdr->numbones)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pbbox->bone (%d).", pbbox->bone);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel_Hitboxes(const void* buf, size_t bufSize, studiohdr_t* studiohdr, UtilAssetsIntegrityCheckResult_StudioModel* checkResult)
	{
		if (studiohdr->numhitboxes < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->numhitboxes (%d).", studiohdr->numhitboxes);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (studiohdr->hitboxindex < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->hitboxindex (%d).", studiohdr->hitboxindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (studiohdr->hitboxindex > (int)bufSize)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->hitboxindex (%d).", studiohdr->hitboxindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto pbbox_base = (mstudiobbox_t*)((byte*)studiohdr + studiohdr->hitboxindex);

		if ((byte*)pbbox_base < (byte*)buf)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pbbox_base.");
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto pbbox_end = pbbox_base + studiohdr->numhitboxes;

		for (int i = 0; i < studiohdr->numhitboxes; ++i)
		{
			auto pbbox = pbbox_base + i;

			auto r = CheckStudioModel_Hitbox(buf, bufSize, studiohdr, i, pbbox, checkResult);

			if (r != UtilAssetsIntegrityCheckReason::OK)
				return r;
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel_Mesh(const void* buf, size_t bufSize, studiohdr_t* studiohdr, int i, int j, int k, mstudiomodel_t* psubmodel, mstudiomesh_t*pmesh, UtilAssetsIntegrityCheckResult_StudioModel* checkResult)
	{
		auto pstudioverts = (vec3_t*)((byte*)studiohdr + psubmodel->vertindex);
		auto pstudionorms = (vec3_t*)((byte*)studiohdr + psubmodel->normindex);
		auto pvertbone = ((byte*)studiohdr + psubmodel->vertinfoindex);
		auto pnormbone = ((byte*)studiohdr + psubmodel->norminfoindex);

		if (pmesh->triindex < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pmesh->triindex (%d).", pmesh->triindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto ptricmds = (short*)((byte*)studiohdr + pmesh->triindex);

		if ((byte*)(ptricmds + 1) > (byte*)buf + bufSize)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pmesh->triindex (%d).", pmesh->triindex);
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
						if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pmesh->triindex (%d).", pmesh->triindex);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}

					auto vertindex = ptricmds[0];
					auto normindex = ptricmds[1];

					if (vertindex < 0)
					{
						if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptricmds[0] (%d).", ptricmds[0]);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}

					if (normindex < 0)
					{
						if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptricmds[1] (%d).", ptricmds[1]);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}

					if ((byte*)(pstudioverts + vertindex) > (byte*)buf + bufSize)
					{
						if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptricmds[0] (%d).", ptricmds[0]);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}
					if ((byte*)(pvertbone + vertindex) > (byte*)buf + bufSize)
					{
						if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptricmds[0] (%d).", ptricmds[0]);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}
					if ((byte*)(pstudionorms + normindex) > (byte*)buf + bufSize)
					{
						if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptricmds[1] (%d).", ptricmds[1]);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}
					if ((byte*)(pnormbone + normindex) > (byte*)buf + bufSize)
					{
						if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptricmds[1] (%d).", ptricmds[1]);
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
						if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pmesh->triindex (%d).", pmesh->triindex);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}

					auto vertindex = ptricmds[0];
					auto normindex = ptricmds[1];

					if (vertindex < 0)
					{
						if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptricmds[0] (%d).", ptricmds[0]);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}

					if (normindex < 0)
					{
						if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptricmds[1] (%d).", ptricmds[1]);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}

					if ((byte*)(pstudioverts + vertindex) > (byte*)buf + bufSize)
					{
						if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptricmds[0] (%d).", ptricmds[0]);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}
					if ((byte*)(pvertbone + vertindex) > (byte*)buf + bufSize)
					{
						if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptricmds[0] (%d).", ptricmds[0]);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}
					if ((byte*)(pstudionorms + normindex) > (byte*)buf + bufSize)
					{
						if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptricmds[1] (%d).", ptricmds[1]);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}
					if ((byte*)(pnormbone + normindex) > (byte*)buf + bufSize)
					{
						if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid ptricmds[1] (%d).", ptricmds[1]);
						return UtilAssetsIntegrityCheckReason::OutOfBound;
					}
				}
			}
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel_Submodel(const void* buf, size_t bufSize, studiohdr_t* studiohdr, int i, int j, mstudiomodel_t *psubmodel, UtilAssetsIntegrityCheckResult_StudioModel* checkResult)
	{
		if (psubmodel->vertindex < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid psubmodel->vertindex (%d).", psubmodel->vertindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (psubmodel->vertindex > (int)bufSize)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid psubmodel->vertindex (%d).", psubmodel->vertindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (psubmodel->normindex < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid psubmodel->normindex (%d).", psubmodel->normindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (psubmodel->normindex > (int)bufSize)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid psubmodel->normindex (%d).", psubmodel->normindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (psubmodel->vertinfoindex < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid psubmodel->vertinfoindex (%d).", psubmodel->vertinfoindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (psubmodel->vertinfoindex > (int)bufSize)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid psubmodel->vertinfoindex (%d).", psubmodel->vertinfoindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (psubmodel->norminfoindex < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid psubmodel->norminfoindex (%d).", psubmodel->norminfoindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (psubmodel->norminfoindex > (int)bufSize)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid psubmodel->norminfoindex (%d).", psubmodel->norminfoindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (psubmodel->meshindex < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid psubmodel->meshindex (%d).", psubmodel->meshindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (psubmodel->meshindex > (int)bufSize)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid psubmodel->meshindex (%d).", psubmodel->meshindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (psubmodel->nummesh < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid psubmodel->nummesh (%d).", psubmodel->nummesh);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (psubmodel->nummesh > MAXSTUDIOMESHES)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid psubmodel->nummesh (%d).", psubmodel->nummesh);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto pmesh_base = (mstudiomesh_t*)((byte*)studiohdr + psubmodel->meshindex);

		if ((byte*)pmesh_base > (byte*)buf + bufSize)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid psubmodel->meshindex (%d) .", psubmodel->meshindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto pmesh_end = pmesh_base + psubmodel->nummesh;

		if ((byte*)pmesh_end > (byte*)buf + bufSize)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid psubmodel->meshindex (%d) or psubmodel->nummesh (%d).", psubmodel->meshindex, psubmodel->nummesh);
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

	UtilAssetsIntegrityCheckReason CheckStudioModel_BodyPart(const void* buf, size_t bufSize, studiohdr_t* studiohdr, int i, mstudiobodyparts_t *pbodypart, UtilAssetsIntegrityCheckResult_StudioModel* checkResult)
	{
		if (safe_strlen(pbodypart->name, sizeof(pbodypart->name)) >= sizeof(pbodypart->name))
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pbodypart->name.");
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (pbodypart->nummodels < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pbodypart->nummodels (%d).", pbodypart->nummodels);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (pbodypart->nummodels > MAXSTUDIOMODELS_SVENGINE)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pbodypart->nummodels (%d).", pbodypart->nummodels);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (pbodypart->modelindex < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pbodypart->modelindex (%d).", pbodypart->modelindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (pbodypart->modelindex > (int)bufSize)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pbodypart->modelindex (%d).", pbodypart->modelindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto psubmodel_base = (mstudiomodel_t*)((byte*)studiohdr + pbodypart->modelindex);

		if ((byte*)psubmodel_base < (byte*)buf)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pbodypart->modelindex (%d) or pbodypart->nummodels (%d).", pbodypart->modelindex, pbodypart->nummodels);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto psubmodel_end = psubmodel_base + pbodypart->nummodels;

		if ((byte*)psubmodel_end > (byte*)buf + bufSize)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pbodypart->modelindex (%d) or pbodypart->nummodels (%d).", pbodypart->modelindex, pbodypart->nummodels);
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

	UtilAssetsIntegrityCheckReason CheckStudioModel_BodyParts(const void* buf, size_t bufSize, studiohdr_t* studiohdr, UtilAssetsIntegrityCheckResult_StudioModel* checkResult)
	{
		if (studiohdr->numbodyparts < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->numbodyparts (%d).", studiohdr->numbodyparts);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (studiohdr->numbodyparts > MAXSTUDIOBODYPARTS)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->numbodyparts (%d).", studiohdr->numbodyparts);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (studiohdr->bodypartindex < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->bodypartindex (%d).", studiohdr->bodypartindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (studiohdr->bodypartindex > (int)bufSize)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->bodypartindex (%d).", studiohdr->bodypartindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto pbodypart_base = (mstudiobodyparts_t*)((byte*)studiohdr + studiohdr->bodypartindex);

		if ((byte*)pbodypart_base < (byte*)buf)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->bodypartindex (%d).", studiohdr->bodypartindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto pbodypart_end = pbodypart_base + studiohdr->numbodyparts;

		if ((byte*)pbodypart_end > (byte*)buf + bufSize)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->bodypartindex (%d) or studiohdr->numbodyparts (%d).", studiohdr->bodypartindex, studiohdr->numbodyparts);
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

	UtilAssetsIntegrityCheckReason CheckStudioModel_Bone(const void* buf, size_t bufSize, studiohdr_t* studiohdr, int i, mstudiobone_t *pbone,  UtilAssetsIntegrityCheckResult_StudioModel* checkResult)
	{
		if (safe_strlen(pbone->name, sizeof(pbone->name)) >= sizeof(pbone->name))
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pbone->name.");
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}
		
		if (pbone->parent < 0 && pbone->parent != -1)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pbone->parent (%d).", pbone->parent);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (pbone->parent >= studiohdr->numbones)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pbone->parent (%d).", pbone->parent);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		for (int j = 0; j < 6; ++j)
		{
			if (pbone->bonecontroller[j] < 0 && pbone->bonecontroller[j] != -1)
			{
				if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pbone->bonecontroller[%d] (%d).", j, pbone->bonecontroller[j]);
				return UtilAssetsIntegrityCheckReason::OutOfBound;
			}

			if (pbone->bonecontroller[j] >= studiohdr->numbones)
			{
				if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pbone->bonecontroller[%d] (%d).", j, pbone->bonecontroller[j]);
				return UtilAssetsIntegrityCheckReason::OutOfBound;
			}
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel_Bones(const void* buf, size_t bufSize, studiohdr_t* studiohdr, UtilAssetsIntegrityCheckResult_StudioModel* checkResult)
	{
		if (studiohdr->numbones < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->numbones (%d).", studiohdr->numbones);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (studiohdr->numbones > MAXSTUDIOBONES)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->numbones (%d).", studiohdr->numbones);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (studiohdr->boneindex < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->boneindex (%d).", studiohdr->boneindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (studiohdr->boneindex > (int)bufSize)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->boneindex (%d).", studiohdr->boneindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto pbone_base = (mstudiobone_t *)((byte*)studiohdr + studiohdr->boneindex);

		if ((byte*)pbone_base < (byte*)buf)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->boneindex (%d).", studiohdr->boneindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto pbone_end = pbone_base + studiohdr->numbones;

		if ((byte*)pbone_end > (byte*)buf + bufSize)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->boneindex (%d) or studiohdr->numbones (%d).", studiohdr->boneindex, studiohdr->numbones);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		for (int i = 0; i < studiohdr->numbones; ++i)
		{
			auto pbone = pbone_base + i;

			auto r = CheckStudioModel_Bone(buf, bufSize, studiohdr, i, pbone, checkResult);
			if (r != UtilAssetsIntegrityCheckReason::OK)
				return r;
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel_BoneController(const void* buf, size_t bufSize, studiohdr_t* studiohdr, int i, mstudiobonecontroller_t *pbonecontroller, UtilAssetsIntegrityCheckResult_StudioModel* checkResult)
	{
		if (pbonecontroller->bone < 0 && pbonecontroller->bone != -1)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pbonecontroller->bone (%d).", pbonecontroller->bone);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (pbonecontroller->bone > studiohdr->numbones)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid pbonecontroller->bone (%d).", pbonecontroller->bone);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel_BoneControllers(const void* buf, size_t bufSize, studiohdr_t* studiohdr, UtilAssetsIntegrityCheckResult_StudioModel* checkResult)
	{
		if (studiohdr->numbonecontrollers < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->numbones (%d).", studiohdr->numbones);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (studiohdr->numbonecontrollers > MAXSTUDIOCONTROLLERS)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->numbones (%d).", studiohdr->numbonecontrollers);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (studiohdr->bonecontrollerindex < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->bonecontrollerindex (%d).", studiohdr->bonecontrollerindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		if (studiohdr->bonecontrollerindex > (int)bufSize)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->bonecontrollerindex (%d).", studiohdr->bonecontrollerindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto pbonecontroller_base = (mstudiobonecontroller_t*)((byte*)studiohdr + studiohdr->bonecontrollerindex);

		if ((byte*)pbonecontroller_base < (byte*)buf)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->bonecontrollerindex (%d).", studiohdr->bonecontrollerindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		auto pbonecontroller_end = pbonecontroller_base + studiohdr->numbones;

		if ((byte*)pbonecontroller_end > (byte*)buf + bufSize)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->bonecontrollerindex (%d) or studiohdr->numbonecontrollers (%d).", studiohdr->bonecontrollerindex, studiohdr->numbonecontrollers);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		for (int i = 0; i < studiohdr->numbonecontrollers; ++i)
		{
			auto pbonecontroller = pbonecontroller_base + i;

			auto r = CheckStudioModel_BoneController(buf, bufSize, studiohdr, i, pbonecontroller, checkResult);
			if (r != UtilAssetsIntegrityCheckReason::OK)
				return r;

		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel_IDST(const void* buf, size_t bufSize, UtilAssetsIntegrityCheckResult_StudioModel* checkResult)
	{
		auto studiohdr = (studiohdr_t*)buf;

		if (studiohdr->version != 10)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid version number, expect %d, got %d.", 10, studiohdr->version);
			return UtilAssetsIntegrityCheckReason::VersionMismatch;
		}

		if (studiohdr->texturedataindex < 0)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->texturedataindex (%d).", studiohdr->texturedataindex);
			return UtilAssetsIntegrityCheckReason::OutOfBound;
		}

		size_t system_memory_length = 0;

		if (studiohdr->textureindex)
		{
			if (studiohdr->texturedataindex < 0 || studiohdr->texturedataindex > (int)bufSize)
			{
				if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->texturedataindex (%d).", studiohdr->texturedataindex);
				return UtilAssetsIntegrityCheckReason::OutOfBound;
			}

			system_memory_length = (size_t)studiohdr->texturedataindex;
		}
		else
		{
			if (studiohdr->length < 0 || studiohdr->length >(int)bufSize)
			{
				if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid studiohdr->length (%d).", studiohdr->length);
				return UtilAssetsIntegrityCheckReason::OutOfBound;
			}

			system_memory_length = (size_t)studiohdr->length;
		}

		if (studiohdr->numtextures)
		{
			auto r = CheckStudioModel_Textures(buf, system_memory_length, studiohdr, checkResult);
			if (r != UtilAssetsIntegrityCheckReason::OK)
				return r;
		}

		if (studiohdr->numtextures)
		{
			auto r = CheckStudioModel_TextureData(buf, bufSize, studiohdr, checkResult);
			if (r != UtilAssetsIntegrityCheckReason::OK)
				return r;
		}

		if (studiohdr->numskinfamilies && studiohdr->numskinref)
		{
			auto r = CheckStudioModel_Skins(buf, system_memory_length, studiohdr, checkResult);
			if (r != UtilAssetsIntegrityCheckReason::OK)
				return r;
		}

		if (studiohdr->numbodyparts)
		{
			auto r = CheckStudioModel_BodyParts(buf, system_memory_length, studiohdr, checkResult);
			if (r != UtilAssetsIntegrityCheckReason::OK)
				return r;
		}

		if (studiohdr->numbones)
		{
			auto r = CheckStudioModel_Bones(buf, system_memory_length, studiohdr, checkResult);
			if (r != UtilAssetsIntegrityCheckReason::OK)
				return r;
		}

		if (studiohdr->numseq)
		{
			auto r = CheckStudioModel_Sequences(buf, system_memory_length, studiohdr, checkResult);
			if (r != UtilAssetsIntegrityCheckReason::OK)
				return r;
		}

		if (studiohdr->numhitboxes)
		{
			auto r = CheckStudioModel_Hitboxes(buf, system_memory_length, studiohdr, checkResult);
			if (r != UtilAssetsIntegrityCheckReason::OK)
				return r;
		}

		if (studiohdr->numbonecontrollers)
		{
			auto r = CheckStudioModel_BoneControllers(buf, system_memory_length, studiohdr, checkResult);
			if (r != UtilAssetsIntegrityCheckReason::OK)
				return r;
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel_IDSQ(const void* buf, size_t bufSize, UtilAssetsIntegrityCheckResult_StudioModel* checkResult)
	{
		auto studiohdr = (studiohdr_t*)buf;

		if (studiohdr->version != 10)
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has invalid version number, expect %d, got %d.", 10, studiohdr->version);
			return UtilAssetsIntegrityCheckReason::VersionMismatch;
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}

	UtilAssetsIntegrityCheckReason CheckStudioModel(const void* buf, size_t bufSize, UtilAssetsIntegrityCheckResult_StudioModel* checkResult) override
	{
		if (bufSize < sizeof(studiohdr_t))
		{
			if(checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel file too small, file size %d < %d.", bufSize, sizeof(studiohdr_t));
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

		auto pbuf = (const char*)buf;
		if (checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "StudioModel has bogus header, expect IDST/IDSQ, got %c%c%c%c.", pbuf[0], pbuf[1], pbuf[2], pbuf[3]);
		return UtilAssetsIntegrityCheckReason::BogusHeader;
	}

	UtilAssetsIntegrityCheckReason Check8bitBMP(const void* buf, size_t bufSize, UtilAssetsIntegrityCheckResult_BMP* checkResult) override
	{
		auto fim = FreeImage_OpenMemory((BYTE*)buf, bufSize);

		if (!fim)
		{
			if (checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "Failed to open BMP file with FreeImage_OpenMemory!");
			return UtilAssetsIntegrityCheckReason::Unknown;
		}

		SCOPE_EXIT{
			FreeImage_CloseMemory(fim);
		};

		auto fib = FreeImage_LoadFromMemory(FREE_IMAGE_FORMAT::FIF_BMP, fim);

		if (!fib)
		{
			if (checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "Failed to load BMP file with FreeImage_LoadFromMemory!");
			return UtilAssetsIntegrityCheckReason::BogusHeader;
		}

		SCOPE_EXIT{
			FreeImage_Unload(fib);
		};

		auto colorType = FreeImage_GetColorType(fib);

		if (colorType != FIC_PALETTE)
		{
			if (checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "The BMP is not indexed-color one!");
			return UtilAssetsIntegrityCheckReason::InvalidFormat;
		}

		auto width = FreeImage_GetWidth(fib);

		if (checkResult && width > checkResult->MaxWidth)
		{
			if (checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "The width of BMP is too large! (%d > %d)", width, checkResult->MaxWidth);
			return UtilAssetsIntegrityCheckReason::SizeTooLarge;
		}

		auto height = FreeImage_GetHeight(fib);

		if (checkResult && height > checkResult->MaxHeight)
		{
			if (checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "The height of BMP is too large! (%d > %d)", height, checkResult->MaxHeight);
			return UtilAssetsIntegrityCheckReason::SizeTooLarge;
		}

		auto size = width * height;

		if (checkResult && size > checkResult->MaxSize)
		{
			if (checkResult) snprintf(checkResult->ReasonStr, sizeof(checkResult->ReasonStr), "The size of BMP is too large! (%d > %d)", size, checkResult->MaxSize);
			return UtilAssetsIntegrityCheckReason::SizeTooLarge;
		}

		return UtilAssetsIntegrityCheckReason::OK;
	}
};

EXPOSE_SINGLE_INTERFACE(CUtilAssetsIntegrity, IUtilAssetsIntegrity, UTIL_ASSETS_INTEGRITY_INTERFACE_VERSION);