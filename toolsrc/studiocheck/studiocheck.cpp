#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char byte;
typedef float vec_t;
typedef float vec2_t[2];
typedef float vec3_t[3];

#include <studio.h>

int main(int argc, const char **argv)
{
	if (argc < 2)
		return 0;

	FILE *fp = fopen(argv[1], "rb");
	if (!fp)
	{
		printf("Error: failed to read file %s\n", argv[1]);
		return 0;
	}

	fseek(fp, 0, SEEK_END);
	auto size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	auto buffer = (byte *)malloc(size);
	if (!buffer)
	{
		printf("Error: failed to allocate 0x%X bytes for %s\n", size, argv[1]);

		fclose(fp);
		return 0;
	}

	fread(buffer, size, 1, fp);
	fclose(fp);

	if (size < sizeof(studiohdr_t))
	{
		printf("Error: invalid file size (%d < %d)\n", size, sizeof(studiohdr_t));
		free(buffer);
		return 0;
	}

	auto studiohdr = (studiohdr_t *)buffer;

	printf("Checking %s\n", studiohdr->name);

	if (0 == memcmp((const byte *)&studiohdr->id, "IDSQ", 4))
	{
		printf("Sequence file %s is ok\n", studiohdr->name);
		free(buffer);
		return 0;
	}

	if (0 != memcmp((const byte *)&studiohdr->id, "IDST", 4))
	{
		printf("Error: invalid studiohdr->id\n");
		free(buffer);
		return 0;
	}

	if(studiohdr->version != 10)
	{
		printf("Error: invalid studiohdr->version, (%d != %d)\n", studiohdr->version, 10);
		free(buffer);
		return 0;
	}

	int total;

	if (studiohdr->textureindex)
		total = studiohdr->texturedataindex;
	else
		total = studiohdr->length;

	if (studiohdr->textureindex)
	{
		auto ptexture = (mstudiotexture_t *)((byte *)buffer + studiohdr->textureindex);

		if ((byte *)(ptexture + studiohdr->numtextures) < (byte *)buffer)
		{
			printf("Error: invalid numtextures (%p < %p)\n", (byte *)(ptexture + studiohdr->numtextures), (byte *)buffer);
			free(buffer);
			return 0;
		}

		if ((byte *)(ptexture + studiohdr->numtextures) > (byte *)buffer + total)
		{
			printf("Error: invalid numtextures (%p > %p)\n", (byte *)(ptexture + studiohdr->numtextures), (byte *)buffer + total);
			free(buffer);
			return 0;
		}

		auto ptexturedata = (byte *)buffer + studiohdr->texturedataindex;

		for (int i = 0; i < studiohdr->numtextures; i++)
		{
			auto pal = (byte *)buffer + ptexture[i].index;
			auto palsize = (ptexture[i].height * ptexture[i].width);
			if (pal + palsize < (byte *)buffer)
			{
				printf("Error: invalid texturedata at texture[%d], (%p < %p)\n", i, pal + palsize, (byte *)buffer);
				free(buffer);
				return 0;
			}
			
			if (pal + palsize > (byte *)buffer + studiohdr->length)
			{
				printf("Error: invalid texturedata at texture[%d], (%p > %p)\n", i, pal + palsize, (byte *)buffer + studiohdr->length);
				free(buffer);
				return 0;
			}
		}
	}

#define CHECK_VAR(a) 	if ((byte *)(a) < (byte *)buffer)\
	{\
		printf("Error: invalid "#a##" (%p < %p)\n", (byte *)(a), (byte *)buffer);\
		free(buffer);\
		return 0;\
	}\
	if ((byte *)(pbodypart + studiohdr->numbodyparts) > (byte *)buffer + total)\
	{\
		printf("Error: invalid "#a##" (%p > %p)\n", (byte *)(a), (byte *)buffer + total);\
		free(buffer);\
		return 0;\
	}

	auto pbodypart = (mstudiobodyparts_t *)((byte *)studiohdr + studiohdr->bodypartindex);

	CHECK_VAR(pbodypart + studiohdr->numbodyparts);

	for (int i = 0; i < studiohdr->numbodyparts; ++i)
	{
		auto psubmodel = (mstudiomodel_t *)((byte *)studiohdr + pbodypart[i].modelindex);

		CHECK_VAR(psubmodel + pbodypart[i].nummodels);

		for (int j = 0; j < pbodypart[i].nummodels; ++j)
		{
			auto pmesh = (mstudiomesh_t *)((byte *)studiohdr + psubmodel[j].meshindex);

			if(psubmodel[j].nummesh)
				CHECK_VAR(pmesh + psubmodel[j].nummesh);

			auto pstudioverts = (vec3_t *)((byte *)studiohdr + psubmodel[j].vertindex);
			if(psubmodel[j].numverts)
				CHECK_VAR(pstudioverts + psubmodel[j].numverts);

			auto pstudionorms = (vec3_t *)((byte *)studiohdr + psubmodel[j].normindex);
			if(psubmodel[j].numnorms)
				CHECK_VAR(pstudionorms + psubmodel[j].numnorms);

			auto pvertbone = ((byte *)studiohdr + psubmodel[j].vertinfoindex);
			if(psubmodel[j].numverts)
				CHECK_VAR(pvertbone + psubmodel[j].numverts);

			auto pnormbone = ((byte *)studiohdr + psubmodel[j].norminfoindex);
			if(psubmodel[j].numverts)
				CHECK_VAR(pnormbone + psubmodel[j].numverts);

			if (psubmodel[j].numverts > MAXSTUDIOVERTS)
			{
				printf("Error: psubmodel[j].numverts (%d) > MAXSTUDIOVERTS (%d)\n", psubmodel[j].numverts, MAXSTUDIOVERTS);
				free(buffer);
				return 0;
			}

			for (int k = 0; k < psubmodel[j].nummesh; ++k)
			{
				auto ptricmds = (short *)((byte *)studiohdr + pmesh[k].triindex);
				
				/*while (*ptricmds)
				{
					if ((byte *)(ptricmds + 1) > (byte *)buffer + total)
					{
						printf("Error: (byte *)(ptricmds + 1) (%p) > (byte *)buffer + total (%p)\n", (byte *)(ptricmds + 1), (byte *)buffer + total);
						free(buffer);
						return 0;
					}
					ptricmds++;
				}*/

				int iii;

				while (iii = *(ptricmds++))
				{
					if (iii < 0)
					{
						iii = -iii;
					}
					else
					{
						
					}


					for (; iii > 0; iii--, ptricmds += 4)
					{
						if (ptricmds[0] >= 0 && ptricmds[0] < MAXSTUDIOVERTS)
						{

						}
						else
						{
							printf("Error: invalid ptricmds[0] %d\n", ptricmds[0]);
							free(buffer);
							return 0;
						}
						if (ptricmds[1] >= 0 && ptricmds[1] < MAXSTUDIOVERTS)
						{

						}
						else
						{
							printf("Error: invalid ptricmds[1] %d\n", ptricmds[1]);
							free(buffer);
							return 0;
						}
					}
				}
			}
		}
	}

	auto pseqdesc = (mstudioseqdesc_t *)((byte *)studiohdr + studiohdr->seqindex);

	if(studiohdr->numseq)
		CHECK_VAR(pseqdesc + studiohdr->numseq);

	for (int i = 0; i < studiohdr->numseq; ++i)
	{
		auto pevent = (mstudioevent_t *)((byte *)studiohdr + pseqdesc[i].eventindex);
		if(pseqdesc[i].numevents)
			CHECK_VAR(pevent + pseqdesc[i].numevents);

		auto panim = (mstudioanim_t *)((byte *)studiohdr + pseqdesc[i].animindex);
		if(pseqdesc[i].animindex)
			CHECK_VAR(panim + 1);
	}

	auto pseqgroups = (mstudioseqgroup_t *)((byte *)studiohdr + studiohdr->seqgroupindex);

	if (studiohdr->numseqgroups)
		CHECK_VAR(pseqgroups + studiohdr->numseqgroups);

	auto pbbox = (mstudiobbox_t *)((byte *)studiohdr + studiohdr->hitboxindex);

	if (studiohdr->numhitboxes)
		CHECK_VAR(pbbox + studiohdr->numhitboxes);

	for (int i = 0; i < studiohdr->numhitboxes; ++i)
	{
		int boneindex = pbbox[i].bone;
		if (boneindex < -1 || boneindex >= studiohdr->numbones)
		{
			printf("Error: invalid bone request at pbbox[%d].bone (%d)\n", i, pbbox[i].bone);
			free(buffer);
			return 0;
		}
	}

	auto pbones = (mstudiobone_t *)((byte *)studiohdr + studiohdr->boneindex);

	if (studiohdr->numbones > MAXSTUDIOBONES)
	{
		printf("Error: studiohdr->numbones (%d) > MAXSTUDIOBONES (%d)\n", studiohdr->numbones, MAXSTUDIOBONES);
		free(buffer);
		return 0;
	}

	if (studiohdr->numbones)
		CHECK_VAR(pbones + studiohdr->numbones);

	for (int i = 0; i < studiohdr->numbones; ++i)
	{
		if (pbones[i].parent < -1 || pbones[i].parent >= studiohdr->numbones)
		{
			printf("Error: invalid bone request at pbones[%d].parent (%d)\n", i, pbones[i].parent);
			free(buffer);
			return 0;
		}
	}

	auto pbonecontroller = (mstudiobonecontroller_t *)((byte *)studiohdr + studiohdr->bonecontrollerindex);

	if (studiohdr->numbonecontrollers)
		CHECK_VAR(pbonecontroller + studiohdr->numbonecontrollers);

	for (int i = 0; i < studiohdr->numbonecontrollers; i++)
	{
		int index = pbonecontroller[i].index;
		if (index < 0 || index > 4)
		{
			printf("Error: invalid bonecontroller request at pbonecontroller[%d].index (%d)\n", i, index);
			free(buffer);
			return 0;
		}

		int bone = pbonecontroller[i].bone;
		if (bone < -1 || bone >= studiohdr->numbones)
		{
			printf("Error: invalid bone request at pbonecontroller[%d].bone (%d)\n", i, bone);
			free(buffer);
			return 0;
		}
	}

	printf("Model %s is ok\n", studiohdr->name);
	free(buffer);
	return 0;
}